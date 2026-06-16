#include "ui/LyricPanel.h"
#include "lyrics/LyricModel.h"
#include "core/SignalBus.h"
#include "skin/ThemeConfig.h"

#include <QPainter>
#include <QPaintEvent>
#include <QFont>
#include <QDebug>
#include <algorithm>

namespace WaveRider {

// ────────────────────────────────────────────────────────────
// Construction
// ────────────────────────────────────────────────────────────
LyricPanel::LyricPanel(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumHeight(150);
    setAutoFillBackground(false);

    connect(ThemeConfig::instance(), &ThemeConfig::themeColorsChanged,
            this, [this]() { update(); });
}

// ────────────────────────────────────────────────────────────
// Public API
// ────────────────────────────────────────────────────────────
void LyricPanel::setModel(LyricModel* model)
{
    if (m_model) {
        disconnect(m_model, nullptr, this, nullptr);
    }
    m_model = model;
    if (m_model) {
        connect(m_model, &LyricModel::modelLoaded,  this, [this]() { update(); });
        connect(m_model, &LyricModel::modelCleared, this, [this]() { update(); });
    }
    m_currentLineIndex = -1;
    m_lastPositionMs = 0;
    m_status = LyricStatus::Idle;
    update();
}

void LyricPanel::loadLyricFile(const QString& lrcFilePath)
{
    if (m_model) {
        if (m_model->loadFromFile(lrcFilePath)) {
            m_status = LyricStatus::Loaded;
            m_currentLineIndex = -1;
            update();
        }
    }
}

void LyricPanel::setStatus(LyricStatus status)
{
    m_status = status;
    update();
}

// ────────────────────────────────────────────────────────────
// Scroll to time
// ────────────────────────────────────────────────────────────
void LyricPanel::scrollToTime(qint64 positionMs)
{
    if (!m_model || m_model->isEmpty()) return;

    m_lastPositionMs = positionMs;
    int newIdx = m_model->lineIndexAtTime(positionMs);

    if (newIdx != m_currentLineIndex) {
        m_currentLineIndex = newIdx;
        QString text = (newIdx >= 0) ? m_model->lineAt(newIdx).text : QString();
        SignalBus::instance()->lyricLineChanged(newIdx, text);
    }
    update();
}

// ────────────────────────────────────────────────────────────
// Paint
// ────────────────────────────────────────────────────────────
void LyricPanel::paintEvent(QPaintEvent* /*event*/)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    // Fill with transparent — parent background shows through
    p.setCompositionMode(QPainter::CompositionMode_Source);
    p.fillRect(rect(), QColor(0, 0, 0, 0));

    switch (m_status) {
    case LyricStatus::Idle:
        drawEmptyState(p, "No track loaded");
        break;
    case LyricStatus::Searching:
        drawEmptyState(p, "Searching for lyrics online...");
        break;
    case LyricStatus::NotFound:
        drawEmptyState(p, "No lyrics available for this track");
        break;
    case LyricStatus::Loaded:
        if (!m_model || m_model->isEmpty()) {
            drawEmptyState(p, "No lyrics available for this track");
        } else {
            drawLyrics(p);
        }
        break;
    }
}

// ────────────────────────────────────────────────────────────
// Empty state rendering
// ────────────────────────────────────────────────────────────
void LyricPanel::drawEmptyState(QPainter& p, const QString& message)
{
    QFont font("Segoe UI Light", 14);
    p.setFont(font);
    p.setPen(ThemeConfig::instance()->textSecondary());

    QRect textRect = rect().adjusted(kSidePadding, 0, -kSidePadding, 0);
    p.drawText(textRect, Qt::AlignCenter, message);
}

// ────────────────────────────────────────────────────────────
// Lyric rendering with smooth scroll
// ────────────────────────────────────────────────────────────
void LyricPanel::drawLyrics(QPainter& p)
{
    if (!m_model || m_model->isEmpty()) return;

    const int centerY      = computeCenterY();
    const int panelWidth   = width();
    const int textMaxWidth = panelWidth - kSidePadding * 2;

    // ── Compute scroll offset (with interpolation) ────
    double scrollOffset;

    if (m_currentLineIndex < 0) {
        // Before the first line — show first line at center
        scrollOffset = 0.0;
    } else if (m_currentLineIndex >= m_model->lineCount() - 1) {
        // At or past the last line
        scrollOffset = static_cast<double>(m_currentLineIndex * kLineSpacing);
    } else {
        // Between two lines — interpolate
        qint64 currentTime = m_model->lineTimeMs(m_currentLineIndex);
        qint64 nextTime    = m_model->lineTimeMs(m_currentLineIndex + 1);
        double progress;
        if (nextTime > currentTime) {
            progress = static_cast<double>(m_lastPositionMs - currentTime)
                     / static_cast<double>(nextTime - currentTime);
        } else {
            progress = 0.0;
        }
        progress = qBound(0.0, progress, 1.0);
        scrollOffset = (m_currentLineIndex + progress) * kLineSpacing;
    }

    // ── Determine visible line range ───────────────────
    const int halfHeight = height() / 2;
    int firstLine = qMax(0, static_cast<int>((scrollOffset - halfHeight) / kLineSpacing) - 2);
    int lastLine  = qMin(m_model->lineCount() - 1,
                         static_cast<int>((scrollOffset + halfHeight) / kLineSpacing) + 2);

    // ── Draw each visible line ─────────────────────────
    auto* tc = ThemeConfig::instance();
    for (int i = firstLine; i <= lastLine; ++i) {
        int y = centerY + i * kLineSpacing - static_cast<int>(scrollOffset);
        LyricLine line = m_model->lineAt(i);

        int distance = qAbs(i - m_currentLineIndex);

        QColor color;
        int fontSize;
        if (distance == 0) {
            color    = tc->accent();
            fontSize = kFontSizeActive;
        } else if (distance == 1) {
            color    = tc->textSecondary();
            fontSize = kFontSizeNormal;
        } else {
            color    = tc->textSecondary();
            color.setAlphaF(qMax(0.25f, 1.0f - (distance - 1) * 0.22f));
            fontSize = kFontSizeNormal - 1;
        }

        QFont font("Segoe UI Light", fontSize);
        if (distance == 0) {
            font.setWeight(QFont::DemiBold);
        }
        p.setFont(font);
        p.setPen(color);

        QRectF textRect(kSidePadding, y - kLineSpacing / 2,
                        textMaxWidth, kLineSpacing);
        p.drawText(textRect, Qt::AlignVCenter | Qt::AlignHCenter, line.text);

        if (distance == 0 && !line.text.isEmpty()) {
            float barX = kSidePadding;
            float barY = y - 14.0f;
            p.setPen(Qt::NoPen);
            p.setBrush(tc->accent());
            p.drawRoundedRect(QRectF(barX, barY, 3.0f, 28.0f), 1.5f, 1.5f);
        }
    }
}

// ────────────────────────────────────────────────────────────
// Helpers
// ────────────────────────────────────────────────────────────
int LyricPanel::computeCenterY() const
{
    // Anchor the current line at ~40% from the top
    return static_cast<int>(height() * 0.40);
}

} // namespace WaveRider
