#pragma once

#include <QString>
#include <QMetaType>

namespace WaveRider {

// ============================================================
// Playback
// ============================================================
enum class PlaybackState {
    Stopped,
    Playing,
    Paused,
};

// ============================================================
// Play mode (how to advance to the next track)
// ============================================================
enum class PlayMode {
    Sequential,   // play through the list, stop at end
    LoopOne,      // repeat the current track
    LoopAll,      // loop the entire playlist
    Shuffle,      // random order
};

// ============================================================
// Background display mode
// ============================================================
enum class BackgroundDisplayMode {
    Fill,
    Fit,
    Stretch,
    Tile,
    Center,
};

// ============================================================
// DSP effect category
// ============================================================
namespace DspCategory {
    constexpr const char* Equalizer = "EQ";
    constexpr const char* Dynamics  = "Dynamics";
    constexpr const char* Reverb    = "Reverb";
    constexpr const char* Spatial   = "Spatial";
    constexpr const char* Other     = "Other";
}

// ============================================================
// Application constants
// ============================================================
namespace Config {
    constexpr const char* AppName    = "WaveRider";
    constexpr const char* OrgName    = "WaveRider";
    constexpr int         FftTimerMs = 50;   // spectrum update interval
    constexpr int         PosTimerMs = 100;  // position update interval
}

namespace Theme {
    constexpr const char* DefaultAccent = "#00d4aa";
    constexpr int         ColorGridSize = 12;
}

} // namespace WaveRider

// Register enums with Qt's meta-type system for use in signals/slots
Q_DECLARE_METATYPE(WaveRider::PlaybackState)
Q_DECLARE_METATYPE(WaveRider::PlayMode)
Q_DECLARE_METATYPE(WaveRider::BackgroundDisplayMode)
