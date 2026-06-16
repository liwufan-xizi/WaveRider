#pragma once

#include <QObject>
#include <QStringList>
#include "core/Constants.h"

namespace WaveRider {

class PlaylistModel;
class PlayModeEngine;
class AudioEngine;

/// Manages the playlist lifecycle: adding tracks, loading/saving M3U files,
/// coordinating between PlaylistModel and PlayModeEngine.
class PlaylistManager : public QObject {
    Q_OBJECT
public:
    explicit PlaylistManager(PlaylistModel* model, AudioEngine* engine, QObject* parent = nullptr);

    // ── File operations ──────────────────────────────
    void openFiles();                       // shows QFileDialog
    void openFolder();                      // shows QFileDialog folder
    void addFiles(const QStringList& paths);
    void removeSelected(const QList<int>& indices);
    void clearPlaylist();

    // ── M3U I/O ─────────────────────────────────────
    bool loadM3u(const QString& filePath);
    bool saveM3u(const QString& filePath);
    QString lastM3uPath() const { return m_lastM3uPath; }

    // ── Play mode ───────────────────────────────────
    PlayMode playMode() const;
    void     setPlayMode(PlayMode mode);
    void     cyclePlayMode();               // Cycle through modes

    // ── Navigation ─────────────────────────────────
    int  nextTrackIndex() const;
    int  prevTrackIndex() const;
    void playTrackAt(int index);
    void playNext();
    void playPrev();

    // ── Accessors ──────────────────────────────────
    PlaylistModel* model() const { return m_model; }
    PlayModeEngine* modeEngine() const { return m_modeEngine; }

signals:
    void playModeChanged(PlayMode mode);

private:
    PlaylistModel*  m_model;
    AudioEngine*    m_audioEngine;
    PlayModeEngine* m_modeEngine;
    QString         m_lastM3uPath;
};

} // namespace WaveRider
