#include "ui/EqualizerWidget.h"
#include "dsp/builtin/EqualizerEffect.h"
#include "skin/ThemeConfig.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

namespace WaveRider {

EqualizerWidget::EqualizerWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setMinimumSize(360, 180);
}

// ============================================================
// Effect binding
// ============================================================
void EqualizerWidget::setEffect(EqualizerEffect* effect)
{
    if (m_effect) {
        disconnect(m_effect, nullptr, this, nullptr);
    }
    m_effect = effect;
    if (m_effect) {
        connect(m_effect, &EqualizerEffect::parametersChanged, this,
                QOverload<>::of(&QWidget::update));
    }
    update();
}

void EqualizerWidget::loadPreset(int presetIndex)
{
    if (m_effect) {
        m_effect->loadPreset(static_cast<EqualizerEffect::Preset>(presetIndex));
    }
}

QSize EqualizerWidget::sizeHint() const
{
    return QSize(440, 200);
}

// ============================================================
// Layout helpers
// ============================================================
QRect EqualizerWidget::bandRect(int band) const
{
    int usableW = width() - kLeftMargin - kRightMargin;
    int bandW   = usableW / bandCount();
    int x       = kLeftMargin + band * bandW;
    return QRect(x, kTopMargin, bandW, height() - kTopMargin - kBottomMargin);
}

QRect EqualizerWidget::trackRect(int band) const
{
    QRect br = bandRect(band);
    int cx   = br.center().x();
    int tw   = kTrackWidth;
    return QRect(cx - tw / 2, br.top(), tw, br.height());
}

int EqualizerWidget::handleY(int band) const
{
    float gain = 0.0f;
    if (m_effect) {
        gain = m_effect->bandGain(band);
    }
    return gainToY(gain);
}

int EqualizerWidget::bandFromPos(const QPoint& pos) const
{
    for (int i = 0; i < bandCount(); ++i) {
        QRect br = bandRect(i);
        // Expand hit area slightly for easier targeting
        br.adjust(-4, 0, 4, 0);
        if (br.contains(pos)) return i;
    }
    return -1;
}

// ============================================================
// Coordinate transforms
// ============================================================
float EqualizerWidget::yToGain(int y) const
{
    QRect br = bandRect(0);
    int trackTop    = br.top()    + kHandleRadius;
    int trackBottom = br.bottom() - kHandleRadius;

    float t = 1.0f - qBound(0.0f, float(y - trackTop) / (trackBottom - trackTop), 1.0f);
    return kGainMin + t * (kGainMax - kGainMin);
}

int EqualizerWidget::gainToY(float dB) const
{
    QRect br = bandRect(0);
    int trackTop    = br.top()    + kHandleRadius;
    int trackBottom = br.bottom() - kHandleRadius;

    float t = (dB - kGainMin) / (kGainMax - kGainMin);
    return trackBottom - qRound(t * (trackBottom - trackTop));
}

void EqualizerWidget::applyGain(int band, float dB)
{
    dB = qBound(kGainMin, dB, kGainMax);
    if (m_effect) {
        m_effect->setBandGain(band, dB);
    }
    emit bandChanged(band, dB);
    update();
}

// ============================================================
// Paint
// ============================================================
void EqualizerWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto* tc = ThemeConfig::instance();
    QColor accent      = tc->accent();
    QColor accentDim   = tc->accentDim();
    QColor textPri     = tc->textPrimary();
    QColor textSec     = tc->textSecondary();
    QColor surface     = tc->surface();
    QColor border      = tc->surfaceBorder();

    int w = width(), h = height();

    // ── Background ──────────────────────────────────────
    p.fillRect(rect(), Qt::transparent);

    // ── Center line (0 dB) ─────────────────────────────
    int zeroY = gainToY(0.0f);
    p.setPen(QPen(border, 1.0));
    p.drawLine(kLeftMargin, zeroY, w - kRightMargin, zeroY);

    // ── Gain labels (left edge) ────────────────────────
    QFont labelFont("Segoe UI", 8);
    p.setFont(labelFont);
    p.setPen(textSec);
    int topY    = gainToY(kGainMax);
    int bottomY = gainToY(kGainMin);
    p.drawText(0, topY    - 6, kLeftMargin - 4, 12, Qt::AlignRight | Qt::AlignVCenter, "+12");
    p.drawText(0, zeroY   - 6, kLeftMargin - 4, 12, Qt::AlignRight | Qt::AlignVCenter, " 0");
    p.drawText(0, bottomY - 6, kLeftMargin - 4, 12, Qt::AlignRight | Qt::AlignVCenter, "−12");

    // ── Frequency labels (bottom) ──────────────────────
    static const char* kFreqLabels[] = {
        "32", "64", "125", "250", "500", "1K", "2K", "4K", "8K", "16K"
    };
    QFont freqFont("Segoe UI Light", 8);
    p.setFont(freqFont);
    for (int i = 0; i < bandCount(); ++i) {
        QRect br = bandRect(i);
        p.setPen(textSec);
        p.drawText(br.left(), h - kBottomMargin, br.width(), kBottomMargin,
                   Qt::AlignHCenter | Qt::AlignTop, kFreqLabels[i]);
    }

    // ── Tracks and handles ─────────────────────────────
    for (int i = 0; i < bandCount(); ++i) {
        QRect tr  = trackRect(i);
        int   hy  = handleY(i);
        bool  hover = (m_hoverBand == i) || (m_dragBand == i);

        // Track background (inactive)
        p.fillRect(tr, accentDim);

        // Track active fill (from 0 dB to current gain)
        float gain = m_effect ? m_effect->bandGain(i) : 0.0f;
        if (gain > 0.1f || gain < -0.1f) {
            int fillTop, fillBottom;
            if (gain > 0) {
                fillTop    = hy;
                fillBottom = zeroY;
            } else {
                fillTop    = zeroY;
                fillBottom = hy;
            }
            QRect fillRect(tr.left(), fillTop, tr.width(), fillBottom - fillTop);
            QColor fillColor = accent;
            if (!hover) fillColor.setAlpha(180);
            p.fillRect(fillRect, fillColor);
        }

        // Hover glow
        if (hover) {
            QColor glow = accent;
            glow.setAlpha(30);
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawEllipse(QPoint(tr.center().x(), hy), kHandleRadius + 6, kHandleRadius + 6);
        }

        // Handle (small circle)
        QColor handleColor = (hover) ? accent : accentDim;
        p.setPen(QPen(accent, 1.5));
        p.setBrush(handleColor);
        p.drawEllipse(QPoint(tr.center().x(), hy), kHandleRadius, kHandleRadius);
    }

    // ── Top border line ────────────────────────────────
    p.setPen(QPen(border, 1.0));
    p.drawLine(kLeftMargin, kTopMargin - 1, w - kRightMargin, kTopMargin - 1);
}

// ============================================================
// Mouse interaction
// ============================================================
void EqualizerWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int band = bandFromPos(event->pos());
        if (band >= 0) {
            m_dragBand = band;
            m_dragging = true;
            float gain = yToGain(event->pos().y());
            applyGain(band, gain);
            setCursor(Qt::BlankCursor);  // hide cursor during drag for precision
        }
    }
}

void EqualizerWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && m_dragBand >= 0) {
        float gain = yToGain(event->pos().y());
        applyGain(m_dragBand, gain);
    } else {
        int band = bandFromPos(event->pos());
        if (band != m_hoverBand) {
            m_hoverBand = band;
            update();
            setCursor(band >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        }
    }
}

void EqualizerWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        m_dragBand = -1;
        setCursor(m_hoverBand >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

void EqualizerWidget::wheelEvent(QWheelEvent* event)
{
    int band = bandFromPos(event->pos());
    if (band < 0) return;

    float delta = (event->angleDelta().y() > 0) ? 0.5f : -0.5f;
    float current = m_effect ? m_effect->bandGain(band) : 0.0f;
    applyGain(band, current + delta);
}

void EqualizerWidget::leaveEvent(QEvent*)
{
    m_hoverBand = -1;
    update();
}

} // namespace WaveRider
