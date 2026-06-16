#pragma once

#include <QWidget>
#include <QListView>
#include <QLineEdit>
#include <QStandardItemModel>
#include <QPropertyAnimation>

namespace WaveRider {

/// Favorites overlay panel with slide-in/out animation and backdrop dismiss.
///
/// Mirrors PlaylistPanel's pattern: fills entire parent, QPropertyAnimation
/// on `slideOffset` (0=visible, 1=hidden), 320px right-side content panel,
/// semi-transparent backdrop (click to dismiss).
///
/// Data: QStandardItemModel rebuilt on each refreshFavorites() call.
/// Auto-updates via SignalBus::favoriteAdded / favoriteRemoved.
class FavoritesPanel : public QWidget {
    Q_OBJECT

    /// 0.0 = panel fully visible (slid in), 1.0 = panel hidden (slid out)
    Q_PROPERTY(float slideOffset READ slideOffset WRITE setSlideOffset)

public:
    static constexpr int kPanelWidth      = 320;
    static constexpr int kAnimDurationMs  = 250;

    explicit FavoritesPanel(QWidget* parent = nullptr);

    bool isOpen() const { return m_open; }

public slots:
    void slideIn();
    void slideOut();
    void refreshFavorites();

signals:
    void trackDoubleClicked(const QString& filePath);
    void slideOutFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onSlideOutFinished();
    void onSearchTextChanged(const QString& text);
    void onItemDoubleClicked(const QModelIndex& index);

private:
    void setupUi();
    void setupConnections();

    float slideOffset() const { return m_slideOffset; }
    void  setSlideOffset(float offset);

    QListView*          m_listView   = nullptr;
    QLineEdit*          m_searchEdit = nullptr;
    QStandardItemModel* m_model      = nullptr;

    QPropertyAnimation* m_anim        = nullptr;
    float               m_slideOffset = 1.0f;
    bool                m_open        = false;
};

} // namespace WaveRider
