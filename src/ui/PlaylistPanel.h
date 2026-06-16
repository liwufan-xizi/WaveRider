#pragma once

#include <QWidget>
#include <QListView>
#include <QLineEdit>
#include <QPushButton>
#include <QPropertyAnimation>

namespace WaveRider {

class PlaylistModel;

/// Playlist overlay panel with slide-in/out animation and backdrop dismiss.
///
/// Visual: fills the entire parent (MainWindow).  The left portion is a
/// semi-transparent backdrop (click to dismiss); the right 320 px is the
/// content area with search bar and track QListView in Phigros style.
///
/// Animation: QPropertyAnimation on `slideOffset` (0 = fully shown,
/// 1 = hidden).  250 ms OutCubic easing.
class PlaylistPanel : public QWidget {
    Q_OBJECT

    /// 0.0 = panel fully visible (slid in),  1.0 = panel hidden (slid out)
    Q_PROPERTY(float slideOffset READ slideOffset WRITE setSlideOffset)

public:
    static constexpr int kPanelWidth  = 320;
    static constexpr int kAnimDurationMs = 250;

    explicit PlaylistPanel(PlaylistModel* model, QWidget* parent = nullptr);

    QListView* listView() const { return m_listView; }

    /// Whether the panel is currently shown or showing.
    bool isOpen() const { return m_open; }

public slots:
    /// Slide the panel in from the right edge.
    void slideIn();

    /// Slide the panel out to the right edge.
    void slideOut();

signals:
    void trackDoubleClicked(int index);
    void removeRequested(const QList<int>& indices);
    void clearRequested();
    void slideOutFinished();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onSlideOutFinished();

private:
    void setupUi();

    // ── Custom property for animation ───────────────
    float slideOffset() const { return m_slideOffset; }
    void  setSlideOffset(float offset);

    PlaylistModel*       m_model        = nullptr;
    QListView*           m_listView     = nullptr;
    QLineEdit*           m_searchEdit   = nullptr;
    QPushButton*         m_clearBtn     = nullptr;

    QPropertyAnimation*  m_anim         = nullptr;
    float                m_slideOffset  = 1.0f;   // start hidden
    bool                 m_open         = false;
};

} // namespace WaveRider
