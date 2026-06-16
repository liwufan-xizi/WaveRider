#pragma once

#include <QWidget>
#include "core/Constants.h"

namespace WaveRider {

/// Fully custom-painted transport control bar.
/// Phigros-style: thin seek bar, diamond play button, geometric icons.
/// All colors from ThemeConfig singleton.
class PlayerControlBar : public QWidget {
    Q_OBJECT
public:
    explicit PlayerControlBar(QWidget* parent = nullptr);

    void setTrackInfo(const QString& title, const QString& artist, qint64 durationMs);
    void setPosition(qint64 posMs, qint64 durMs);
    void setPlaying(bool playing);
    void setPlayMode(PlayMode mode);
    void setCurrentFavorited(bool favorited);

    QSize sizeHint() const override { return QSize(0, qRound(80.0 * m_scale)); }

    /// Set dynamic scale factor (1.0 = 800×450 baseline).
    void setScale(qreal scale);

signals:
    void playClicked();
    void pauseClicked();
    void stopClicked();
    void nextClicked();
    void prevClicked();
    void seekRequested(qint64 positionMs);
    void volumeChanged(float level);
    void playModeCycleClicked();
    void playlistToggleClicked();
    void favoriteToggleClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    enum class HitZone {
        None, SeekBar, SeekHandle,
        Prev, Play, Next, Mode, VolumeIcon, VolumeBar, Favorite, Playlist
    };
    HitZone hitTest(const QPoint& pos) const;
    void    recomputeLayout();

    // Hit regions (in widget coords)
    QRectF m_seekTrack;
    QPointF m_seekHandlePos;
    QRectF m_playBtn;       // diamond bounding rect
    QRectF m_prevBtn;
    QRectF m_nextBtn;
    QRectF m_modeBtn;
    QRectF m_playlistBtn;
    QRectF m_favBtn;
    QRectF m_volIcon;
    QRectF m_volTrack;

    // State
    QString m_title, m_artist;
    qint64  m_posMs = 0, m_durMs = 0;
    bool    m_playing = false;
    bool    m_currentFavorited = false;
    PlayMode m_playMode = PlayMode::LoopAll;
    float   m_volume = 0.8f;
    bool    m_muted = false;
    float   m_mutedVolume = 0.8f;

    // Interaction
    HitZone m_hoverZone  = HitZone::None;
    HitZone m_activeZone = HitZone::None;
    bool    m_seekDragging = false;
    float   m_playScale = 1.0f;

    // Layout constants (baseline, multiplied by m_scale at runtime)
    static constexpr float kSeekHandleRadius = 5.0f;
    static constexpr float kPlayBtnSize      = 28.0f;
    static constexpr float kSideBtnSize      = 18.0f;

    qreal m_scale = 1.0;
};

} // namespace WaveRider
