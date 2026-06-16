#pragma once

#include <QObject>
#include <QVector>
#include "core/Constants.h"

namespace WaveRider {

/// Determines which track to play next based on the current play mode.
class PlayModeEngine : public QObject {
    Q_OBJECT
public:
    explicit PlayModeEngine(QObject* parent = nullptr);

    PlayMode mode() const { return m_mode; }
    void     setMode(PlayMode mode);

    /// Compute the next track index given the current index and playlist size.
    /// @param current   currently playing track index (-1 if none)
    /// @param count     total number of tracks in the playlist
    /// @return next index, or -1 if playback should stop
    int nextTrack(int current, int count);

signals:
    void modeChanged(PlayMode mode);

private:
    void regenerateShuffleOrder(int count);

    PlayMode      m_mode = PlayMode::LoopAll;
    QVector<int>  m_shuffleOrder;  // pre-generated random order
    int           m_shufflePos = 0;
};

} // namespace WaveRider
