#include "ui/TrackInfoWidget.h"
#include "skin/ThemeConfig.h"

#include <QPainter>
#include <QFont>

namespace WaveRider {

TrackInfoWidget::TrackInfoWidget(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void TrackInfoWidget::setMetadata(const AudioMetadata& meta)
{
    m_meta     = meta;
    m_hasTrack = true;
    update();
}

void TrackInfoWidget::clear()
{
    m_meta     = AudioMetadata();
    m_hasTrack = false;
    update();
}

void TrackInfoWidget::setScale(qreal scale)
{
    if (qFuzzyCompare(scale, m_scale)) return;
    m_scale = scale;
    update();
}

void TrackInfoWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto* tc = ThemeConfig::instance();
    const int W = width();
    const int H = height();

    // Scale font sizes and dimensions relative to baseline (1.0 = 800×450)
    auto sf = [this](int basePt) -> int {
        return qMax(6, qRound(basePt * m_scale));
    };

    // ── Background ────────────────────────────────────
    p.fillRect(rect(), Qt::transparent);

    if (!m_hasTrack) {
        p.setPen(tc->textSecondary());
        p.setFont(QFont("Segoe UI Light", sf(13)));
        p.drawText(rect(), Qt::AlignCenter, "No track loaded");
        return;
    }

    // ── Album art placeholder (scaled) ───────────────
    const float artSize = 48.0f * static_cast<float>(m_scale);
    const float artX = 12.0f * static_cast<float>(m_scale);
    const float artY = (H - artSize) * 0.5f;
    QRectF artRect(artX, artY, artSize, artSize);

    // Geometric placeholder: rotated concentric squares
    p.setPen(QPen(tc->accentDim(), qMax(1.0, 1.5 * m_scale)));
    p.setBrush(Qt::NoBrush);
    p.save();
    p.translate(artRect.center());
    for (int i = 0; i < 2; ++i) {
        float sq = (22.0f - i * 7.0f) * static_cast<float>(m_scale);
        p.drawRect(QRectF(-sq, -sq, sq * 2, sq * 2));
        p.rotate(15.0f);
    }
    p.restore();

    // Musical note icon in center
    p.setPen(tc->accent());
    QFont noteFont("Segoe UI", sf(16));
    p.setFont(noteFont);
    p.drawText(artRect, Qt::AlignCenter, QString(QChar(0x266A))); // ♪

    // ── Text info ─────────────────────────────────────
    const float textX = artRect.right() + 14.0f * static_cast<float>(m_scale);
    const float textW = W - textX - 20.0f * static_cast<float>(m_scale);

    // Title
    QFont titleFont("Segoe UI", sf(14), QFont::DemiBold);
    p.setFont(titleFont);
    p.setPen(tc->textPrimary());
    QString title = m_meta.title.isEmpty() ? "Unknown Title" : m_meta.title;
    title = p.fontMetrics().elidedText(title, Qt::ElideRight, static_cast<int>(textW));
    const float titleH = 20.0f * static_cast<float>(m_scale);
    p.drawText(QRectF(textX, artY + 2.0f * m_scale, textW, titleH),
               Qt::AlignLeft | Qt::AlignVCenter, title);

    // Artist + Album + Info one-liner
    QFont infoFont("Segoe UI Light", sf(10));
    p.setFont(infoFont);
    p.setPen(tc->textSecondary());

    QStringList parts;
    if (!m_meta.artist.isEmpty()) parts << m_meta.artist;
    if (!m_meta.album.isEmpty())  parts << m_meta.album;
    parts << AudioMetadata::formatDuration(m_meta.durationMs);
    parts << QString("%1 kHz").arg(m_meta.sampleRateHz / 1000.0, 0, 'f', 1);
    parts << QString("%1 kbps").arg(m_meta.bitrateKbps);

    QString info = parts.join(" · ");
    info = p.fontMetrics().elidedText(info, Qt::ElideRight, static_cast<int>(textW));
    const float infoH = 18.0f * static_cast<float>(m_scale);
    p.drawText(QRectF(textX, artY + 24.0f * m_scale, textW, infoH),
               Qt::AlignLeft | Qt::AlignVCenter, info);

    // ── Bottom separator ──────────────────────────────
    p.setPen(QPen(tc->surfaceBorder(), qMax(0.5, 1.0 * m_scale)));
    p.drawLine(QPointF(12.0 * m_scale, H - 1),
               QPointF(W - 12.0 * m_scale, H - 1));
}

} // namespace WaveRider
