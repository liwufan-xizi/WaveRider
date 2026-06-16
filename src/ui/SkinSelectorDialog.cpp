#include "ui/SkinSelectorDialog.h"
#include "skin/SkinManager.h"
#include "skin/ThemeConfig.h"
#include "core/SignalBus.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QFile>
#include <QRegularExpression>
#include <QApplication>

namespace WaveRider {

// ═══════════════════════════════════════════════════════════
//  @vars parser — extract color variables from a theme.qss
// ═══════════════════════════════════════════════════════════

SkinPreviewColors SkinSelectorDialog::parseSkinPreviewColors(const QString& qssPath) const
{
    SkinPreviewColors pc;
    QFile file(qssPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return pc;

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // Match /* @vars ... */ block (supports both comment styles)
    QRegularExpression varBlock(R"(\/\*\s*@vars\s(.*?)\*\/)",
                                QRegularExpression::DotMatchesEverythingOption);
    auto match = varBlock.match(content);
    if (!match.hasMatch())
        return pc;

    QString block = match.captured(1);
    QRegularExpression varLine(R"(\$([a-zA-Z_]+)\s*:\s*([^;]+);)");
    auto it = varLine.globalMatch(block);

    QMap<QString, QString> vars;
    while (it.hasNext()) {
        auto m = it.next();
        vars[m.captured(1)] = m.captured(2).trimmed();
    }

    // Map known keys to our preview struct
    auto v = [&](const QString& key) -> QString {
        return vars.value(key, "#333333");
    };
    pc.primary     = v("primary");
    pc.bgMain      = v("bg_main");
    pc.bgPanel     = v("bg_panel");
    pc.textPrimary = v("text_primary");
    pc.accent      = v("accent");
    pc.surface     = v("bg_surface");
    pc.valid       = !vars.isEmpty();
    return pc;
}

// ═══════════════════════════════════════════════════════════
//  Construction
// ═══════════════════════════════════════════════════════════

SkinSelectorDialog::SkinSelectorDialog(QWidget* parent)
    : QWidget(parent)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setMouseTracking(true);
    setFixedSize(kWidth, kHeight);
    hide();
}

// ═══════════════════════════════════════════════════════════
//  Show / populate
// ═══════════════════════════════════════════════════════════

void SkinSelectorDialog::showDialog()
{
    auto* skinMgr = SkinManager::instance();

    // Refresh skin list and parse previews
    m_skins     = skinMgr->availableSkins();
    m_previews.clear();
    m_cardRects.clear();
    m_hoverIndex   = -1;
    m_pressedIndex = -1;

    QString curName = skinMgr->currentSkin().name;
    m_currentIndex = -1;

    // Compute card layout
    int totalCardW = kCardW * 2 + kCardGap;
    int startX     = (kWidth - totalCardW) / 2;
    int startY     = kHeaderH + kPadH;

    for (int i = 0; i < m_skins.size(); ++i) {
        m_previews.append(parseSkinPreviewColors(m_skins[i].qssFilePath));

        int col = i % 2;
        int row = i / 2;
        int cx  = startX + col * (kCardW + kCardGap);
        int cy  = startY + row * (kCardH + kCardGap);
        m_cardRects.append(QRect(cx, cy, kCardW, kCardH));

        if (m_skins[i].name == curName)
            m_currentIndex = i;
    }

    // Title & close rects
    m_titleRect = QRect(kTitlePadX, 0, kWidth - 2 * kTitlePadX - 24, kHeaderH);
    m_closeRect = QRect(kWidth - kHeaderH, 0, kHeaderH, kHeaderH);

    // Center on parent
    if (parentWidget()) {
        QPoint center = parentWidget()->geometry().center();
        move(center.x() - kWidth / 2, center.y() - kHeight / 2);
    }

    raise();
    show();
}

// ═══════════════════════════════════════════════════════════
//  Apply & close
// ═══════════════════════════════════════════════════════════

void SkinSelectorDialog::applySkin(int index)
{
    if (index < 0 || index >= m_skins.size())
        return;

    SkinManager::instance()->loadSkin(m_skins[index].name);
    m_currentIndex = index;
    hide();
    emit dialogClosed();
}

// ═══════════════════════════════════════════════════════════
//  Painting
// ═══════════════════════════════════════════════════════════

void SkinSelectorDialog::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const auto& tc = ThemeConfig::instance()->colors();

    // ── Backdrop dim ─────────────────────────────────
    // No backdrop — this is a floating tool window

    // ── Panel background ─────────────────────────────
    QPainterPath panelPath;
    panelPath.addRoundedRect(rect(), kRadius, kRadius);
    p.fillPath(panelPath, QColor(14, 14, 26, 242));  // deep dark bg

    // border
    QPen borderPen(tc.accentDim);
    borderPen.setWidthF(1.0);
    p.setPen(borderPen);
    p.drawPath(panelPath);

    // ── Title ────────────────────────────────────────
    p.setPen(tc.textPrimary);
    QFont titleFont("Segoe UI", 13);
    titleFont.setWeight(QFont::DemiBold);
    p.setFont(titleFont);
    p.drawText(m_titleRect, Qt::AlignVCenter | Qt::AlignLeft, "Select Skin");

    // ── Close button ─────────────────────────────────
    {
        bool hoverClose = m_closeRect.contains(mapFromGlobal(QCursor::pos()));
        QColor closeCol = hoverClose ? tc.accent : tc.textSecondary;
        p.setPen(QPen(closeCol, 1.5));
        QFont closeFont("Segoe UI", 14);
        p.setFont(closeFont);
        p.drawText(m_closeRect, Qt::AlignCenter, QString::fromUtf8("×"));
    }

    // ── Skin cards ───────────────────────────────────
    for (int i = 0; i < m_skins.size(); ++i) {
        const QRect& cr = m_cardRects[i];
        bool isCurrent  = (i == m_currentIndex);
        bool isHovered  = (i == m_hoverIndex);
        bool isPressed  = (i == m_pressedIndex);

        // Card background
        QPainterPath cardPath;
        cardPath.addRoundedRect(QRectF(cr), 4, 4);

        QColor cardBg = isPressed ? QColor(30, 30, 48) :
                        isHovered ? QColor(22, 22, 38) :
                                    QColor(16, 16, 28);
        p.fillPath(cardPath, cardBg);

        // Card border
        QPen cardPen(isCurrent ? tc.accent : QColor(40, 40, 60));
        cardPen.setWidthF(isCurrent ? 1.5 : 0.8);
        p.setPen(cardPen);
        p.drawPath(cardPath);

        // ── Color swatches ───────────────────────────
        const SkinPreviewColors& pc = m_previews[i];
        if (pc.valid) {
            // Show 2 rows × 4 cols of swatches
            QString colors[6] = { pc.primary, pc.bgMain, pc.bgPanel,
                                  pc.textPrimary, pc.accent, pc.surface };
            int swW = (kCardW - 28) / 4;
            int swH = (kSwatchH - 6) / 2;
            int swStartX = cr.x() + 14;
            int swStartY = cr.y() + 8;

            for (int r = 0; r < 2; ++r) {
                for (int c = 0; c < 4; ++c) {
                    int idx = r * 4 + c;
                    QRect sw(swStartX + c * swW, swStartY + r * (swH + 4),
                             swW, swH);
                    QColor col(colors[idx]);
                    p.fillRect(sw, col);
                    // subtle border
                    p.setPen(QPen(QColor(255, 255, 255, 20), 0.5));
                    p.drawRect(sw);
                }
            }
        } else {
            // Fallback: plain swatch
            p.setPen(tc.textSecondary);
            QFont fallFont("Segoe UI", 11);
            p.setFont(fallFont);
            p.drawText(QRect(cr.x(), cr.y(), cr.width(), kSwatchH),
                       Qt::AlignCenter, "?");
        }

        // ── Skin name ────────────────────────────────
        int labelY = cr.y() + kSwatchH + 4;
        QRect labelRect(cr.x() + 8, labelY, cr.width() - 16, kLabelH);

        QColor nameCol = isCurrent ? tc.accent : tc.textSecondary;
        p.setPen(nameCol);
        QFont nameFont("Segoe UI", 10);
        if (isCurrent) nameFont.setWeight(QFont::DemiBold);
        p.setFont(nameFont);
        p.drawText(labelRect, Qt::AlignCenter, m_skins[i].displayName);

        // ── Current checkmark ────────────────────────
        if (isCurrent) {
            QFont ckFont("Segoe UI", 9);
            p.setFont(ckFont);
            p.setPen(tc.accent);
            p.drawText(QRect(cr.right() - 22, cr.y() + 2, 20, 16),
                       Qt::AlignCenter, QString::fromUtf8("✓"));
        }
    }
}

// ═══════════════════════════════════════════════════════════
//  Mouse interaction
// ═══════════════════════════════════════════════════════════

void SkinSelectorDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    QPoint pos = event->pos();

    // Close button
    if (m_closeRect.contains(pos)) {
        hide();
        emit dialogClosed();
        return;
    }

    // Check cards
    for (int i = 0; i < m_cardRects.size(); ++i) {
        if (m_cardRects[i].contains(pos)) {
            m_pressedIndex = i;
            update();
            return;
        }
    }

    // Click outside cards → close
    hide();
    emit dialogClosed();
}

void SkinSelectorDialog::mouseMoveEvent(QMouseEvent* event)
{
    int oldHover = m_hoverIndex;
    m_hoverIndex = -1;

    for (int i = 0; i < m_cardRects.size(); ++i) {
        if (m_cardRects[i].contains(event->pos())) {
            m_hoverIndex = i;
            break;
        }
    }

    if (m_hoverIndex != oldHover)
        update();
}

void SkinSelectorDialog::leaveEvent(QEvent*)
{
    if (m_hoverIndex != -1) {
        m_hoverIndex = -1;
        update();
    }
}

} // namespace WaveRider
