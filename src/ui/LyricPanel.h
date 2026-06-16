#pragma once

#include <QWidget>
#include "lyrics/LyricParser.h"

namespace WaveRider {

class LyricModel;

/// Status states for the lyric display.
enum class LyricStatus {
    Idle,       // No track loaded / initial state
    Searching,  // Fetching lyrics from network
    NotFound,   // No lyrics available for this track
    Loaded,     // Lyrics loaded and ready to display
};

/// Displays synced lyrics with smooth vertical scrolling and current-line highlighting.
/// Connects to LyricModel for data and emits lyricLineChanged via SignalBus.
class LyricPanel : public QWidget {
    Q_OBJECT
public:
    explicit LyricPanel(QWidget* parent = nullptr);

    /// Set the lyric data model.
    void setModel(LyricModel* model);

    /// Update the display to the given playback position.
    void scrollToTime(qint64 positionMs);

    /// Load lyrics from a local LRC file path.
    void loadLyricFile(const QString& lrcFilePath);

    /// Set the display status (e.g., Searching, NotFound).
    void setStatus(LyricStatus status);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void drawEmptyState(QPainter& p, const QString& message);
    void drawLyrics(QPainter& p);
    int  computeCenterY() const;

    LyricModel*  m_model           = nullptr;
    int          m_currentLineIndex = -1;
    qint64       m_lastPositionMs   = 0;
    LyricStatus  m_status           = LyricStatus::Idle;

    // Layout constants
    static constexpr int kLineSpacing    = 44;   // vertical pixels between lines
    static constexpr int kFontSizeNormal = 14;
    static constexpr int kFontSizeActive = 18;
    static constexpr int kSidePadding    = 20;
};

} // namespace WaveRider
