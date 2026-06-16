#pragma once

#include <QObject>
#include "core/Constants.h"

namespace WaveRider {

/// Central signal bus — singleton.
/// All cross-module communication flows through signals emitted here.
/// Modules connect to the signals they care about without #include-ing
/// the emitting module, keeping the architecture decoupled.
class SignalBus : public QObject {
    Q_OBJECT
public:
    static SignalBus* instance();

signals:
    // ── Playback ──────────────────────────────────────
    void trackStarted(const QString& filePath, const QString& title, const QString& artist);
    void trackFinished(const QString& filePath);
    void playbackPaused();
    void playbackResumed();
    void playbackStopped();
    void positionChanged(qint64 positionMs, qint64 durationMs);
    void volumeChanged(float level);          // 0.0 – 1.0
    void playbackStateChanged(PlaybackState state);

    // ── Playlist ─────────────────────────────────────
    void playlistChanged();
    void currentTrackChanged(int index);
    void playModeChanged(PlayMode mode);

    // ── DSP ──────────────────────────────────────────
    void effectAdded(const QString& effectId);
    void effectRemoved(const QString& effectId);
    void effectBypassChanged(const QString& effectId, bool bypassed);
    void dspChainChanged();

    // ── Skin / Theme ─────────────────────────────────
    void skinChanged(const QString& skinName);
    void themeColorsChanged();

    // ── Background ───────────────────────────────────
    void backgroundChanged();

    // ── Favorites ────────────────────────────────────
    void favoriteAdded(const QString& filePath);
    void favoriteRemoved(const QString& filePath);

    // ── Lyrics ───────────────────────────────────────
    void lyricLineChanged(int lineIndex, const QString& text);

private:
    explicit SignalBus(QObject* parent = nullptr);
    SignalBus(const SignalBus&) = delete;
    SignalBus& operator=(const SignalBus&) = delete;
};

} // namespace WaveRider
