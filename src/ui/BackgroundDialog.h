#pragma once

#include <QWidget>
#include <QVector>

namespace WaveRider {

enum class BackgroundDisplayMode;

/// Phigros-style background settings dialog.
///
/// Layout (400×320 fixed):
///   ┌──────────────────────────────────────┐
///   │  Background Settings              ×  │  title bar (36px)
///   ├──────────────────────────────────────┤
///   │  ┌─────────┐    Current: ...         │
///   │  │         │    Size: 1920×1080      │  preview (144×96)
///   │  │ preview │                         │  + file info
///   │  └─────────┘                         │
///   │                                      │
///   │  Mode: [Fill] [Fit] [Stretch] [T] [C]│  5 mode buttons
///   │  Blur  ───●─────────────────────     │  slider
///   │  Dim   ────●────────────────────     │  slider
///   │                                      │
///   │  [Browse...]  [Clear]  [None]        │  action buttons
///   └──────────────────────────────────────┘
class BackgroundDialog : public QWidget {
    Q_OBJECT
public:
    explicit BackgroundDialog(QWidget* parent = nullptr);

    /// Refresh state and show at center of parent.
    void showDialog();

signals:
    void dialogClosed();
    void backgroundChanged();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    void refreshState();
    void applyMode(int index);
    void browseImage();
    void clearImage();

    // ── Hit-test zones ───────────────────────────
    enum HitZone { None, CloseBtn, ModeBtn, BlurTrack, BlurHandle, OverlayTrack, OverlayHandle, BrowseBtn, ClearBtn, NoneBtn };
    HitZone hitTest(const QPoint& pos) const;
    int     modeIndexAtPos(const QPoint& pos) const;

    // ── State ─────────────────────────────────────
    QString               m_imagePath;
    QPixmap               m_previewPix;
    BackgroundDisplayMode m_mode;
    int                   m_blurRadius     = 0;
    float                 m_overlayOpacity = 0.3f;
    bool                  m_hasBackground  = false;

    // ── Interaction ───────────────────────────────
    HitZone  m_hoverZone  = None;
    HitZone  m_activeZone = None;
    int      m_hoverModeIdx = -1;

    // ── Layout rects (computed in paintEvent) ─────
    QRect m_titleRect;
    QRect m_closeRect;
    QRect m_previewRect;
    QRect m_infoRect;
    QVector<QRect> m_modeRects;
    QRect m_blurTrackRect;
    QRect m_blurHandleRect;
    QRect m_overlayTrackRect;
    QRect m_overlayHandleRect;
    QRect m_browseRect;
    QRect m_clearRect;
    QRect m_noneRect;

    // ── Layout constants ──────────────────────────
    static constexpr int kWidth      = 400;
    static constexpr int kHeight     = 320;
    static constexpr int kHeaderH    = 36;
    static constexpr int kPreviewW   = 144;
    static constexpr int kPreviewH   = 96;
    static constexpr int kPadH       = 10;
    static constexpr int kPadV       = 8;
    static constexpr int kBtnW       = 56;
    static constexpr int kBtnH       = 24;
    static constexpr int kSliderH    = 18;
    static constexpr int kTrackH     = 4;
    static constexpr int kHandleR    = 6;
    static constexpr int kActionW    = 90;
    static constexpr int kActionH    = 28;
    static constexpr int kRadius     = 6;
};

} // namespace WaveRider
