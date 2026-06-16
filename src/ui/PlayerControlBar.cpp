#include "ui/PlayerControlBar.h"
#include "skin/ThemeConfig.h"
#include "core/ConfigManager.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QFont>
#include <QtMath>

namespace WaveRider {

// ────────────────────────────────────────────────────────────
// Construction
// ────────────────────────────────────────────────────────────
PlayerControlBar::PlayerControlBar(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

// ────────────────────────────────────────────────────────────
// Public setters
// ────────────────────────────────────────────────────────────
void PlayerControlBar::setTrackInfo(const QString& title, const QString& artist, qint64 durationMs)
{
    m_title  = title;
    m_artist = artist;
    m_durMs  = durationMs;
    m_posMs  = 0;
    recomputeLayout();
    update();
}

void PlayerControlBar::setPosition(qint64 posMs, qint64 durMs)
{
    m_durMs = durMs;
    if (!m_seekDragging) {
        m_posMs = posMs;
    }
    recomputeLayout();
    update();
}

void PlayerControlBar::setPlaying(bool playing)
{
    m_playing = playing;
    update();
}

void PlayerControlBar::setPlayMode(PlayMode mode)
{
    m_playMode = mode;
    update();
}

void PlayerControlBar::setScale(qreal scale)
{
    if (qFuzzyCompare(scale, m_scale)) return;
    m_scale = scale;
    recomputeLayout();
    update();
}

// ────────────────────────────────────────────────────────────
// Layout computation
// ────────────────────────────────────────────────────────────
void PlayerControlBar::recomputeLayout()
{
    const float W = static_cast<float>(width());
    const float H = static_cast<float>(height());
    const qreal s  = m_scale;
    const float pad = static_cast<float>(16.0 * s);

    // ── Seek bar (top zone) ──────────────────────────
    float seekY = static_cast<float>(14.0 * s);
    float trackH = qMax(1.5f, static_cast<float>(2.0 * s));
    m_seekTrack = QRectF(pad, seekY, W - pad * 2, trackH);

    float seekRatio = (m_durMs > 0) ? static_cast<float>(m_posMs) / m_durMs : 0.0f;
    seekRatio = qBound(0.0f, seekRatio, 1.0f);
    m_seekHandlePos = QPointF(pad + seekRatio * (W - pad * 2), seekY + trackH * 0.5f);

    // ── Control buttons (bottom zone) ────────────────
    float ctrlY = static_cast<float>(32.0 * s);
    float ctrlH = H - ctrlY - static_cast<float>(4.0 * s);
    float centerX = W * 0.5f;
    float centerY = ctrlY + ctrlH * 0.5f;

    // Play (center, diamond)
    float pbSize = static_cast<float>(kPlayBtnSize * s);
    float pbHalf = pbSize * 0.5f;
    m_playBtn = QRectF(centerX - pbHalf, centerY - pbHalf, pbSize, pbSize);

    // Prev / Next
    float sideSize  = static_cast<float>(kSideBtnSize * s);
    float sideDist  = static_cast<float>(62.0 * s);
    m_prevBtn = QRectF(centerX - sideDist, centerY - sideSize * 0.5f, sideSize, sideSize);
    m_nextBtn = QRectF(centerX + sideDist - sideSize, centerY - sideSize * 0.5f, sideSize, sideSize);

    // Mode (left)
    float modeW = static_cast<float>(32.0 * s);
    float modeH = static_cast<float>(20.0 * s);
    m_modeBtn = QRectF(pad, centerY - modeH * 0.5f, modeW, modeH);

    // Favorite heart (left of playlist button)
    float favW = static_cast<float>(24.0 * s);
    float favH = static_cast<float>(20.0 * s);
    m_favBtn = QRectF(W - pad - favW - static_cast<float>(30.0 * s), centerY - favH * 0.5f, favW, favH);

    // Playlist hamburger (rightmost)
    float plW = static_cast<float>(24.0 * s);
    float plH = static_cast<float>(20.0 * s);
    m_playlistBtn = QRectF(W - pad - plW, centerY - plH * 0.5f, plW, plH);

    // Volume
    float volIconW = static_cast<float>(18.0 * s);
    float volIconH = static_cast<float>(16.0 * s);
    m_volIcon  = QRectF(pad + static_cast<float>(42.0 * s), centerY - volIconH * 0.5f, volIconW, volIconH);
    float volTrackW = static_cast<float>(72.0 * s);
    float volTrackH = qMax(2.0f, static_cast<float>(3.0 * s));
    m_volTrack = QRectF(pad + static_cast<float>(64.0 * s), centerY - volTrackH * 0.5f, volTrackW, volTrackH);
}

void PlayerControlBar::resizeEvent(QResizeEvent* event)
{
    recomputeLayout();
    QWidget::resizeEvent(event);
}

// ────────────────────────────────────────────────────────────
// Hit testing
// ────────────────────────────────────────────────────────────
PlayerControlBar::HitZone PlayerControlBar::hitTest(const QPoint& pos) const
{
    QPointF pf(pos);

    // Handle (bigger hit area)
    QRectF hh(m_seekHandlePos.x() - 8, m_seekHandlePos.y() - 8, 16, 16);
    if (hh.contains(pf)) return HitZone::SeekHandle;

    // Track (extended vertically for easier clicking)
    QRectF th(m_seekTrack.x(), m_seekTrack.y() - 4,
              m_seekTrack.width(), m_seekTrack.height() + 8);
    if (th.contains(pf)) return HitZone::SeekBar;

    if (m_playBtn.contains(pf))    return HitZone::Play;
    if (m_prevBtn.contains(pf))    return HitZone::Prev;
    if (m_nextBtn.contains(pf))    return HitZone::Next;
    if (m_modeBtn.contains(pf))    return HitZone::Mode;
    if (m_volIcon.contains(pf))    return HitZone::VolumeIcon;
    if (m_volTrack.contains(pf))   return HitZone::VolumeBar;
    if (m_favBtn.contains(pf))     return HitZone::Favorite;
    if (m_playlistBtn.contains(pf)) return HitZone::Playlist;

    return HitZone::None;
}

// ────────────────────────────────────────────────────────────
// Paint
// ────────────────────────────────────────────────────────────
void PlayerControlBar::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    auto* tc = ThemeConfig::instance();
    const int W = width();

    // Scale font sizes relative to baseline (1.0 = 800×450)
    auto sf = [this](int basePt) -> int {
        return qMax(6, qRound(basePt * m_scale));
    };

    // ── Top border ────────────────────────────────────
    p.setPen(QPen(tc->surfaceBorder(), 1.0f));
    p.drawLine(0, 0, W, 0);

    // ── Seek bar track ───────────────────────────────
    {
        QRectF tr = m_seekTrack;
        p.setPen(Qt::NoPen);
        p.setBrush(tc->surfaceBorder());
        p.drawRoundedRect(tr, 1.0f, 1.0f);

        // Filled
        if (m_durMs > 0) {
            float ratio = (m_durMs > 0) ? static_cast<float>(m_posMs) / static_cast<float>(m_durMs) : 0.0f; ratio = qBound(0.0f, ratio, 1.0f);
            float fillW = tr.width() * ratio;
            if (fillW > 1.0f) {
                p.setBrush(tc->accent());
                p.drawRoundedRect(QRectF(tr.x(), tr.y(), fillW, tr.height()), 1.0f, 1.0f);
            }
        }

        // Handle
        bool hoverH = (m_hoverZone == HitZone::SeekHandle || m_seekDragging);
        float hR = hoverH ? kSeekHandleRadius + 1.5f : kSeekHandleRadius;

        // Glow
        if (hoverH) {
            QRadialGradient glow(m_seekHandlePos, hR * 2.5f);
            QColor ga = tc->accent();
            glow.setColorAt(0.0, ga);
            glow.setColorAt(0.3, QColor(ga.red(), ga.green(), ga.blue(), 40));
            glow.setColorAt(1.0, Qt::transparent);
            p.setPen(Qt::NoPen);
            p.setBrush(glow);
            p.drawEllipse(m_seekHandlePos, hR * 2.5f, hR * 2.5f);
        }

        p.setPen(QPen(Qt::white, 1.5f));
        p.setBrush(tc->accent());
        p.drawEllipse(m_seekHandlePos, hR, hR);

        // Time label
        auto fmt = [](qint64 ms) {
            int s = static_cast<int>(ms / 1000);
            return QString("%1:%2").arg(s / 60).arg(s % 60, 2, 10, QChar('0'));
        };
        p.setPen(tc->textSecondary());
        p.setFont(QFont("Segoe UI", sf(9)));
        p.drawText(QRectF(W - 120.0f * m_scale, 0, 110.0f * m_scale, 28.0f * m_scale),
                   Qt::AlignVCenter | Qt::AlignRight, fmt(m_posMs) + " / " + fmt(m_durMs));
    }

    // ── Diamond play button ──────────────────────────
    {
        bool hover = (m_hoverZone == HitZone::Play);
        bool active = (m_activeZone == HitZone::Play);
        float scale = active ? 0.92f : (hover ? 1.06f : 1.0f);

        QRectF r = m_playBtn;
        float cx = r.center().x(), cy = r.center().y();
        float half = r.width() * 0.5f * scale;

        QPainterPath diamond;
        diamond.moveTo(cx, cy - half);
        diamond.lineTo(cx + half, cy);
        diamond.lineTo(cx, cy + half);
        diamond.lineTo(cx - half, cy);
        diamond.closeSubpath();

        QColor fill = tc->accent();
        fill.setAlpha(hover ? 60 : 30);
        p.setPen(QPen(tc->accent(), 1.5f));
        p.setBrush(fill);
        p.drawPath(diamond);

        // Play / Pause icon
        p.setPen(Qt::NoPen);
        p.setBrush(tc->accent());
        if (m_playing) {
            float bw = half * 0.35f, bh = half * 0.7f, gap = half * 0.15f;
            p.drawRoundedRect(QRectF(cx - gap - bw, cy - bh * 0.5f, bw, bh), 1.0, 1.0);
            p.drawRoundedRect(QRectF(cx + gap, cy - bh * 0.5f, bw, bh), 1.0, 1.0);
        } else {
            float th = half * 0.65f, tw = half * 0.55f;
            QPainterPath tri;
            tri.moveTo(cx - tw * 0.3f, cy - th * 0.5f);
            tri.lineTo(cx + tw * 0.7f, cy);
            tri.lineTo(cx - tw * 0.3f, cy + th * 0.5f);
            tri.closeSubpath();
            p.drawPath(tri);
        }
    }

    // ── Prev / Next ──────────────────────────────────
    auto drawChevrons = [&](const QRectF& r, bool right) {
        bool h = (right ? m_hoverZone == HitZone::Next : m_hoverZone == HitZone::Prev);
        QColor col = h ? tc->accent() : tc->textSecondary();
        p.setPen(QPen(col, 2.0f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        float cx = r.center().x(), cy = r.center().y();
        float s = r.width() * 0.28f, dir = right ? 1.0f : -1.0f;
        for (int k = 0; k < 2; ++k) {
            float off = dir * s * (0.8f + k * 0.5f);
            QPainterPath ch;
            ch.moveTo(cx - dir * s * 0.3f + off, cy - s);
            ch.lineTo(cx + dir * s * 0.6f + off, cy);
            ch.lineTo(cx - dir * s * 0.3f + off, cy + s);
            p.drawPath(ch);
        }
    };
    drawChevrons(m_prevBtn, false);
    drawChevrons(m_nextBtn, true);

    // ── Mode label ───────────────────────────────────
    {
        auto ms = [](PlayMode m) {
            switch (m) {
            case PlayMode::Sequential: return QStringLiteral("→");  // →
            case PlayMode::LoopAll:    return QStringLiteral("↻");  // ↻
            case PlayMode::LoopOne:    return QStringLiteral("↻") + QStringLiteral("1");
            case PlayMode::Shuffle:    return QStringLiteral("⇄");  // ⇄
            default: return QStringLiteral("?");
            }
        };
        bool h = (m_hoverZone == HitZone::Mode);
        p.setPen(h ? tc->accent() : tc->textSecondary());
        p.setFont(QFont("Segoe UI", sf(13)));
        p.drawText(m_modeBtn, Qt::AlignCenter, ms(m_playMode));
    }

    // ── Volume icon ──────────────────────────────────
    {
        bool h = (m_hoverZone == HitZone::VolumeIcon);
        QColor col = h ? tc->accent() : tc->textSecondary();
        if (m_muted) col.setAlpha(80);
        p.setPen(QPen(col, 1.5f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        QRectF vr = m_volIcon;
        float bx = vr.x(), by = vr.center().y();

        QPainterPath spk;
        spk.moveTo(bx + 2, by - 5);
        spk.lineTo(bx + 7, by - 5);
        spk.lineTo(bx + 12, by - 9);
        spk.lineTo(bx + 12, by + 9);
        spk.lineTo(bx + 7, by + 5);
        spk.lineTo(bx + 2, by + 5);
        spk.closeSubpath();
        p.drawPath(spk);

        if (!m_muted && m_volume > 0.01f) {
            for (int w = 0; w < 2; ++w) {
                float ax = bx + 14 + w * 4, ar = 4 + w * 2;
                QColor wc = col;
                wc.setAlphaF(m_volume * (1.0f - w * 0.3f));
                p.setPen(QPen(wc, 1.2f));
                p.drawArc(QRectF(ax - ar, by - ar, ar * 2, ar * 2), -70 * 16, 140 * 16);
            }
        }
        if (m_muted) {
            p.setPen(QPen(QColor(0xe9, 0x45, 0x60), 1.5f));
            p.drawLine(QPointF(bx + 14, by - 6), QPointF(bx + 20, by + 6));
            p.drawLine(QPointF(bx + 14, by + 6), QPointF(bx + 20, by - 6));
        }
    }

    // ── Volume track ─────────────────────────────────
    {
        QRectF vr = m_volTrack;
        p.setPen(Qt::NoPen);
        p.setBrush(tc->surfaceBorder());
        p.drawRoundedRect(vr, 1.5f, 1.5f);

        float vol = m_muted ? 0.0f : m_volume;
        float fillW = vr.width() * vol;
        if (fillW > 1.0f) {
            p.setBrush(tc->accent());
            p.drawRoundedRect(QRectF(vr.x(), vr.y(), fillW, vr.height()), 1.5f, 1.5f);
        }

        float hx = vr.x() + fillW;
        p.setPen(QPen(Qt::white, 1.2f));
        p.setBrush(tc->accent());
        p.drawEllipse(QPointF(hx, vr.center().y()), 4.0f, 4.0f);
    }

    // ── Favorite (heart) ─────────────────────────────
    {
        bool h = (m_hoverZone == HitZone::Favorite);
        QColor col = m_currentFavorited ? tc->accent()
                    : (h ? tc->accent() : tc->textSecondary());

        QRectF fr = m_favBtn;
        float cx = fr.center().x(), cy = fr.center().y();
        float s = 8.0f;

        QPainterPath heart;
        heart.moveTo(cx, cy + s * 0.85f);
        heart.cubicTo(cx - s * 1.1f, cy + s * 0.15f,
                      cx - s * 0.15f, cy - s,
                      cx, cy - s * 0.35f);
        heart.cubicTo(cx + s * 0.15f, cy - s,
                      cx + s * 1.1f, cy + s * 0.15f,
                      cx, cy + s * 0.85f);
        heart.closeSubpath();

        if (m_currentFavorited) {
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawPath(heart);
        } else {
            p.setPen(QPen(col, 1.5f, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.setBrush(Qt::NoBrush);
            p.drawPath(heart);
        }
    }

    // ── Playlist (hamburger) ─────────────────────────
    {
        bool h = (m_hoverZone == HitZone::Playlist);
        QColor col = h ? tc->accent() : tc->textSecondary();
        p.setPen(QPen(col, 2.0f, Qt::SolidLine, Qt::RoundCap));
        QRectF pr = m_playlistBtn;
        float cx = pr.center().x(), cy = pr.center().y(), lw = pr.width() * 0.6f;
        for (int i = 0; i < 3; ++i) {
            float ly = cy - 5.0f + i * 5.0f;
            p.drawLine(QPointF(cx - lw, ly), QPointF(cx + lw, ly));
        }
    }

    // ── Track info (left of prev) ────────────────────
    {
        QRectF ir(16.0f * m_scale, m_playBtn.top() + 4.0f * m_scale,
                  m_prevBtn.left() - 24.0f * m_scale, m_playBtn.height() - 8.0f * m_scale);

        p.setFont(QFont("Segoe UI", sf(11), QFont::DemiBold));
        p.setPen(tc->textPrimary());
        p.drawText(ir, Qt::AlignLeft | Qt::AlignBottom,
                   p.fontMetrics().elidedText(m_title, Qt::ElideRight, static_cast<int>(ir.width())));

        p.setFont(QFont("Segoe UI Light", sf(9)));
        p.setPen(tc->textSecondary());
        p.drawText(ir, Qt::AlignLeft | Qt::AlignTop,
                   p.fontMetrics().elidedText(m_artist, Qt::ElideRight, static_cast<int>(ir.width())));
    }
}

// ────────────────────────────────────────────────────────────
// Mouse events
// ────────────────────────────────────────────────────────────
void PlayerControlBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;

    HitZone zone = hitTest(event->pos());
    m_activeZone = zone;

    switch (zone) {
    case HitZone::SeekBar:
    case HitZone::SeekHandle:
        m_seekDragging = true;
        {
            float r = static_cast<float>(event->pos().x() - m_seekTrack.x()) / m_seekTrack.width(); r = qBound(0.0f, r, 1.0f);
            m_posMs = static_cast<qint64>(r * m_durMs);
            recomputeLayout();
        }
        break;
    case HitZone::Play:
        if (m_playing) emit pauseClicked(); else emit playClicked();
        break;
    case HitZone::Prev: emit prevClicked(); break;
    case HitZone::Next: emit nextClicked(); break;
    case HitZone::Mode: emit playModeCycleClicked(); break;
    case HitZone::VolumeIcon:
        m_muted = !m_muted;
        if (m_muted) { m_mutedVolume = m_volume; emit volumeChanged(0.0f); }
        else         { m_volume = m_mutedVolume;  emit volumeChanged(m_volume); }
        recomputeLayout();
        break;
    case HitZone::VolumeBar:
        {
            float r = static_cast<float>(event->pos().x() - m_volTrack.x()) / m_volTrack.width(); r = qBound(0.0f, r, 1.0f);
            m_volume = r; m_muted = (r < 0.01f);
            emit volumeChanged(m_volume);
            recomputeLayout();
        }
        break;
    case HitZone::Favorite: emit favoriteToggleClicked(); break;
    case HitZone::Playlist: emit playlistToggleClicked(); break;
    default: break;
    }
    update();
}

void PlayerControlBar::mouseMoveEvent(QMouseEvent* event)
{
    QPoint pos = event->pos();

    if (m_seekDragging) {
        float r = static_cast<float>(pos.x() - m_seekTrack.x()) / m_seekTrack.width(); r = qBound(0.0f, r, 1.0f);
        m_posMs = static_cast<qint64>(r * m_durMs);
        recomputeLayout();
        update();
        return;
    }
    if (m_activeZone == HitZone::VolumeBar) {
        float r = static_cast<float>(pos.x() - m_volTrack.x()) / m_volTrack.width(); r = qBound(0.0f, r, 1.0f);
        m_volume = r; m_muted = (r < 0.01f);
        emit volumeChanged(m_volume);
        recomputeLayout();
        update();
        return;
    }

    HitZone nh = hitTest(pos);
    if (nh != m_hoverZone) {
        m_hoverZone = nh;
        setCursor(nh == HitZone::None ? Qt::ArrowCursor : Qt::PointingHandCursor);
        update();
    }
}

void PlayerControlBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) return;
    if (m_seekDragging) {
        m_seekDragging = false;
        emit seekRequested(m_posMs);
    }
    m_activeZone = HitZone::None;
    update();
}

void PlayerControlBar::wheelEvent(QWheelEvent* event)
{
    float d = event->angleDelta().y() / 1200.0f;
    m_volume = qBound(0.0f, m_volume + d, 1.0f);
    m_muted = (m_volume < 0.01f);
    emit volumeChanged(m_volume);
    recomputeLayout();
    update();
}

void PlayerControlBar::enterEvent(QEvent*)  { update(); }
void PlayerControlBar::leaveEvent(QEvent*)
{
    m_hoverZone = HitZone::None;
    setCursor(Qt::ArrowCursor);
    update();
}

void PlayerControlBar::setCurrentFavorited(bool favorited)
{
    if (m_currentFavorited != favorited) {
        m_currentFavorited = favorited;
        update();
    }
}

} // namespace WaveRider
