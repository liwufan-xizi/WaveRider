#pragma once

#include <QStyledItemDelegate>

class QListView;

namespace WaveRider {

/// Custom Phigros-style delegate for playlist rows.
///
/// Each row is 48 px tall with a two-line layout:
///   - Title (11 px DemiBold, textPrimary) on the first line
///   - Artist · Duration (9 px Light, textSecondary) on the second line
///
/// Visual indicators:
///   - 3 px accent vertical bar on the left when the track is playing
///   - Small accent-filled heart on the right when the track is favorited
///   - Subtle alternating-row background, hover highlight, selection highlight
///
/// All colors are read from ThemeConfig at paint time so rows respond
/// automatically to background-image changes.
class PlaylistDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit PlaylistDelegate(QListView* view, QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;

    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;

private:
    QListView* m_view;
    static constexpr int kRowHeight    = 48;
    static constexpr int kBarWidth     = 3;
    static constexpr int kHeartSize    = 12;
    static constexpr int kLeftPadding  = 12;
    static constexpr int kRightPadding = 10;
};

} // namespace WaveRider
