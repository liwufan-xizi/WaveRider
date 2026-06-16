#include "ui/DSPPanel.h"
#include "ui/EqualizerWidget.h"
#include "dsp/DSPChain.h"
#include "dsp/IDSPEffect.h"
#include "dsp/builtin/EqualizerEffect.h"
#include "dsp/builtin/CompressorEffect.h"
#include "dsp/builtin/ReverbEffect.h"
#include "skin/ThemeConfig.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QVector>

namespace WaveRider {

// ── Preset list ──────────────────────────────────────────
static const EqualizerEffect::Preset kVisiblePresets[] = {
    EqualizerEffect::Flat,
    EqualizerEffect::Rock,
    EqualizerEffect::Pop,
    EqualizerEffect::Jazz,
    EqualizerEffect::Classical,
    EqualizerEffect::Vocal,
};

DSPPanel::DSPPanel(DSPChain* chain, QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFixedSize(kWidth, kHeaderH + kPresetRowH + kPadH + kEqHeight + kPadH);

    setDSPChain(chain);
}

void DSPPanel::setDSPChain(DSPChain* chain)
{
    m_chain = chain;

    // Look up the equalizer effect from the chain, or create one
    if (m_chain) {
        auto* existing = m_chain->findEffect("eq");
        if (existing) {
            m_eqEffect = qobject_cast<EqualizerEffect*>(existing);
        }
        if (!m_eqEffect) {
            m_eqEffect = new EqualizerEffect(this);
            // Don't add to chain automatically — user toggles it
        }
    }

    // Create the EQ widget if not already present
    if (!m_eqWidget) {
        m_eqWidget = new EqualizerWidget(this);
        m_eqWidget->setGeometry(8, kHeaderH + kPresetRowH + kPadH,
                                kWidth - 16, kEqHeight);
        m_eqWidget->setEffect(m_eqEffect);
        m_eqWidget->show();
    } else if (m_eqEffect) {
        m_eqWidget->setEffect(m_eqEffect);
    }

    update();
}

QSize DSPPanel::sizeHint() const
{
    return QSize(kWidth, kHeaderH + kPresetRowH + kPadH + kEqHeight + kPadH);
}

// ============================================================
// Paint
// ============================================================
void DSPPanel::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto* tc = ThemeConfig::instance();
    QColor accent    = tc->accent();
    QColor accentDim = tc->accentDim();
    QColor textPri   = tc->textPrimary();
    QColor textSec   = tc->textSecondary();
    QColor surface   = tc->surface();
    QColor border    = tc->surfaceBorder();

    int w = width(), h = height();

    // ── Panel background ───────────────────────────────
    p.setPen(QPen(border, 1.0));
    p.setBrush(surface);
    p.drawRoundedRect(QRect(0, 0, w, h), kRadius, kRadius);

    // ── Header ─────────────────────────────────────────
    QRect headerRect(0, 0, w, kHeaderH);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(accent.red(), accent.green(), accent.blue(), 20));
    p.drawRoundedRect(headerRect.adjusted(1, 1, -1, 0), kRadius, kRadius);
    // Clip bottom corners of header fill
    p.fillRect(headerRect.adjusted(1, kHeaderH - kRadius, -1, 0),
               surface);

    // Title
    QFont titleFont("Segoe UI Light", 13);
    p.setFont(titleFont);
    p.setPen(textPri);
    p.drawText(QRect(14, 0, 100, kHeaderH), Qt::AlignVCenter, "DSP");

    // ── Master toggle ──────────────────────────────────
    int toggleCX = w - 40;
    int toggleCY = kHeaderH / 2;
    int toggleR  = 8;
    m_masterToggleRect = QRect(toggleCX - toggleR, toggleCY - toggleR,
                               toggleR * 2, toggleR * 2);

    // Outer circle
    p.setPen(QPen(m_masterOn ? accent : textSec, 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPoint(toggleCX, toggleCY), toggleR, toggleR);

    // Inner dot (filled when on)
    if (m_masterOn) {
        p.setPen(Qt::NoPen);
        p.setBrush(accent);
        p.drawEllipse(QPoint(toggleCX, toggleCY), toggleR - 4, toggleR - 4);
    }

    // ── Close button ───────────────────────────────────
    int closeX = w - 18;
    int closeY = 8;
    int closeS = 12;
    m_closeRect = QRect(closeX, closeY, closeS, closeS);

    bool closeHover = (m_hoverZone == CloseButton);
    p.setPen(QPen(closeHover ? accent : textSec, 1.5));
    p.drawLine(closeX, closeY, closeX + closeS, closeY + closeS);
    p.drawLine(closeX + closeS, closeY, closeX, closeY + closeS);

    // ── Header bottom rule ─────────────────────────────
    p.setPen(QPen(border, 1.0));
    p.drawLine(14, kHeaderH, w - 14, kHeaderH);

    // ── Preset buttons ─────────────────────────────────
    m_presetBtns.clear();
    int presetY   = kHeaderH + 4;
    int presetW   = 64;
    int presetH   = kPresetRowH - 4;
    int totalPresetW = 6 * (presetW + 4);  // 6 buttons with 4px gap
    int presetStartX = (w - totalPresetW) / 2;

    QFont presetFont("Segoe UI Light", 9);
    p.setFont(presetFont);

    for (int i = 0; i < 6; ++i) {
        int px = presetStartX + i * (presetW + 4);
        QRect btnRect(px, presetY, presetW, presetH);

        EqualizerEffect::Preset preset = kVisiblePresets[i];
        m_presetBtns.append(PresetBtn(btnRect, preset));

        bool isActive = m_eqEffect &&
            true;  // Could track "current preset", but gains change on drag

        QColor btnBg = (m_hoverZone == PresetButton && presetAtPos(mapFromGlobal(QCursor::pos())))
                           ? QColor(accent.red(), accent.green(), accent.blue(), 20)
                           : Qt::transparent;

        p.setPen(QPen(border, 1.0));
        p.setBrush(btnBg);
        p.drawRoundedRect(btnRect, 3, 3);

        p.setPen((m_hoverZone == PresetButton && presetAtPos(mapFromGlobal(QCursor::pos())))
                     ? accent : textSec);
        p.drawText(btnRect, Qt::AlignCenter, EqualizerEffect::presetName(preset));
    }
}

// ============================================================
// Hit testing
// ============================================================
DSPPanel::HitZone DSPPanel::hitTest(const QPoint& pos) const
{
    if (m_masterToggleRect.contains(pos)) return MasterToggle;
    if (m_closeRect.contains(pos))       return CloseButton;

    for (const auto& btn : m_presetBtns) {
        if (btn.rect.contains(pos)) return PresetButton;
    }
    return None;
}

const DSPPanel::PresetBtn* DSPPanel::presetAtPos(const QPoint& pos) const
{
    for (const auto& btn : m_presetBtns) {
        if (btn.rect.contains(pos)) return &btn;
    }
    return nullptr;
}

// ============================================================
// Mouse interaction
// ============================================================
void DSPPanel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    QPoint pos = event->pos();
    HitZone zone = hitTest(pos);

    switch (zone) {
    case MasterToggle:
        toggleMasterBypass();
        break;
    case CloseButton:
        emit panelClosed();
        break;
    case PresetButton: {
        const auto* btn = presetAtPos(pos);
        if (btn) {
            applyPreset(btn->preset);
        }
        break;
    }
    default:
        break;
    }
}

void DSPPanel::mouseMoveEvent(QMouseEvent* event)
{
    HitZone zone = hitTest(event->pos());
    if (zone != m_hoverZone) {
        m_hoverZone = zone;
        update();
    }
}

// ============================================================
// Actions
// ============================================================
void DSPPanel::applyPreset(EqualizerEffect::Preset preset)
{
    if (m_eqEffect) {
        m_eqEffect->loadPreset(preset);
        if (m_eqWidget) m_eqWidget->update();
    }
}

void DSPPanel::toggleMasterBypass()
{
    m_masterOn = !m_masterOn;

    if (m_chain) {
        m_chain->setMasterBypass(!m_masterOn);
    }

    // Apply/remove EQ from chain
    if (m_eqEffect && m_chain) {
        if (m_masterOn) {
            // Add EQ to chain if not already there
            if (!m_chain->effects().contains(m_eqEffect)) {
                m_chain->addEffect(m_eqEffect);
            } else {
                m_eqEffect->setBypass(false);
            }
        } else {
            m_eqEffect->setBypass(true);
        }
    }

    update();
}

} // namespace WaveRider
