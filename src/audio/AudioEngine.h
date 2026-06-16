#pragma once

#include <QObject>
#include <QTimer>
#include <memory>
#include <windows.h>
#include "bass.h"
#include "core/Constants.h"

namespace WaveRider {

class BASSInitGuard;

/// Core audio engine wrapping the BASS library.
/// Manages: init, load, play, pause, stop, seek, volume, position-polling.
///
/// Thread-safety: all public methods must be called from the same thread
/// (Qt main / GUI thread), except where noted.
class AudioEngine : public QObject {
    Q_OBJECT
public:
    explicit AudioEngine(QObject* parent = nullptr);
    ~AudioEngine() override;

    // ── Lifecycle ────────────────────────────────────
    /// Initialize BASS with the given window handle. Must be called once.
    bool initialize(HWND winHandle);
    void shutdown();

    // ── File loading ─────────────────────────────────
    /// Load an audio file and prepare for playback.
    /// Frees the previous stream if any.
    /// @return true on success
    bool loadFile(const QString& filePath);

    /// Unload the current stream without stopping.
    void unload();

    // ── Playback control ─────────────────────────────
    bool play();
    bool pause();
    bool stop();

    /// Seek to absolute position in seconds.
    bool seek(double seconds);

    /// Toggle between play and pause.
    void togglePlayPause();

    // ── State queries ────────────────────────────────
    PlaybackState state() const { return m_state; }
    bool isPlaying() const { return m_state == PlaybackState::Playing; }
    bool isPaused()  const { return m_state == PlaybackState::Paused; }

    qint64  positionMs() const;
    qint64  durationMs() const;

    float   volume() const { return m_volume; }
    void    setVolume(float level);   // 0.0 – 1.0

    QString currentFilePath() const { return m_currentFile; }

    /// Expose the BASS stream handle for DSP chain integration.
    HSTREAM streamHandle() const { return m_stream; }

signals:
    void stateChanged(PlaybackState newState);
    void positionUpdated(qint64 posMs, qint64 durMs);
    void trackStarted(const QString& filePath);
    void trackFinished(const QString& filePath);
    void errorOccurred(const QString& message);

private slots:
    void onPositionTimer();

private:
    void setState(PlaybackState s);
    void emitTrackFinished();  // called from BASS_SYNC_END callback

    // BASS sync callback — must be static or free function
    static void CALLBACK endSyncCallback(HSYNC handle, DWORD channel, DWORD data, void* user);

    std::unique_ptr<BASSInitGuard> m_guard;
    HSTREAM  m_stream = 0;
    QString  m_currentFile;
    PlaybackState m_state = PlaybackState::Stopped;
    float    m_volume = 0.8f;
    QTimer*  m_positionTimer = nullptr;
    bool     m_initialized = false;
};

} // namespace WaveRider
