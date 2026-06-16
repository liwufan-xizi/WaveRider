#include "ui/PlaylistDelegate.h"
#include "playlist/PlaylistModel.h"
#include "skin/ThemeConfig.h"

#include <QPainter>
#include <QPainterPath>
#include <QListView>

namespace WaveRider {

PlaylistDelegate::PlaylistDelegate(QListView* view, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_view(view)
{
    // Repaint rows when the theme changes
    connect(ThemeConfig::instance(), &ThemeConfig::themeColorsChanged, this, [this]() {
        if (m_view) m_view->viewport()->update();
    });
}

QSize PlaylistDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    int w = m_view ? m_view->viewport()->width() : 320;
    return QSize(w, kRowHeight);
}

void PlaylistDelegate::paint(QPainter* p, const QStyleOptionViewItem& option,
                              const QModelIndex& index) const
{
    p->save();
    p->setRenderHint(QPainter::Antialiasing);

    auto* tc = ThemeConfig::instance();
    const QRect r = option.rect;
    const bool isPlaying  = index.data(PlaylistModel::IsPlayingRole).toBool();
    const bool isFavorite = index.data(PlaylistModel::IsFavoriteRole).toBool();
    const bool isHovered  = (option.state & QStyle::State_MouseOver);
    const bool isSelected = (option.state & QStyle::State_Selected);

    // ── Background ──────────────────────────────────────
    QColor bg = Qt::transparent;
    if (isSelected) {
        bg = tc->accent();
        bg.setAlpha(22);
    } else if (isHovered) {
        bg = tc->surface();
    } else if (index.row() % 2 == 0) {
        bg = QColor(255, 255, 255, 5);  // subtle zebra
    }

    if (bg.alpha() > 0) {
        p->fillRect(r, bg);
    }

    // ── Playing indicator bar ──────────────────────────
    if (isPlaying) {
        QColor barColor = tc->accent();
        QRect bar(r.left(), r.top() + 6, kBarWidth, r.height() - 12);
        p->setPen(Qt::NoPen);
        p->setBrush(barColor);
        p->drawRoundedRect(bar, 1.5, 1.5);
    }

    // ── Content area ────────────────────────────────────
    int contentX = r.left() + kLeftPadding;
    int contentW = r.width() - kLeftPadding - kRightPadding;
    int titleY   = r.top() + 7;
    int subY     = r.top() + 24;

    // ── Favorite heart ──────────────────────────────────
    int heartRight = r.right() - kRightPadding;
    if (isFavorite) {
        int cx = heartRight - kHeartSize / 2;
        int cy = r.top() + kRowHeight / 2;
        float hs = kHeartSize * 0.45f;

        QPainterPath heart;
        heart.moveTo(cx, cy + hs * 0.85f);
        heart.cubicTo(cx - hs * 1.1f, cy + hs * 0.15f,
                      cx - hs * 0.15f, cy - hs,
                      cx, cy - hs * 0.35f);
        heart.cubicTo(cx + hs * 0.15f, cy - hs,
                      cx + hs * 1.1f, cy + hs * 0.15f,
                      cx, cy + hs * 0.85f);
        heart.closeSubpath();

        p->setPen(Qt::NoPen);
        p->setBrush(tc->accent());
        p->drawPath(heart);

        contentW -= (kHeartSize + 6);
    }

    // ── Title ───────────────────────────────────────────
    QFont titleFont("Segoe UI", 11, QFont::DemiBold);
    p->setFont(titleFont);
    QString title = index.data(PlaylistModel::TitleRole).toString();
    QString elidedTitle = p->fontMetrics().elidedText(title, Qt::ElideRight, contentW);

    p->setPen(isSelected ? tc->accent() : tc->textPrimary());
    p->drawText(contentX, titleY, contentW, 15, Qt::AlignLeft | Qt::AlignTop, elidedTitle);

    // ── Subtitle: Artist · Duration ─────────────────────
    QFont subFont("Segoe UI Light", 9);
    p->setFont(subFont);
    QString artist   = index.data(PlaylistModel::ArtistRole).toString();
    QString duration = index.data(PlaylistModel::DurationTextRole).toString();

    QString subLine;
    if (!artist.isEmpty() && artist != "Unknown Artist") {
        subLine = artist;
        if (!duration.isEmpty()) subLine += " · " + duration;
    } else {
        subLine = duration;
    }
    QString elidedSub = p->fontMetrics().elidedText(subLine, Qt::ElideRight, contentW);

    p->setPen(tc->textSecondary());
    p->drawText(contentX, subY, contentW, 14, Qt::AlignLeft | Qt::AlignTop, elidedSub);

    // ── Bottom divider ──────────────────────────────────
    QColor divider = tc->surfaceBorder();
    divider.setAlpha(40);
    p->setPen(QPen(divider, 0.5));
    p->drawLine(r.left() + kLeftPadding, r.bottom(), r.right() - kRightPadding, r.bottom());

    p->restore();
}

} // namespace WaveRider
