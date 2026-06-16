#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QPoint>
#include <QSize>
#include "core/Constants.h"

namespace WaveRider {

/// Persistent application settings backed by QSettings (registry on Windows).
class ConfigManager : public QObject {
    Q_OBJECT
public:
    static ConfigManager* instance();

    // ── Window geometry ──────────────────────────────
    QPoint windowPosition() const;
    void   setWindowPosition(const QPoint& pos);
    QSize  windowSize() const;
    void   setWindowSize(const QSize& size);

    // ── Playback ──────────────────────────────────────
    float  volume() const;                   // 0.0 – 1.0
    void   setVolume(float level);
    PlayMode playMode() const;
    void     setPlayMode(PlayMode mode);

    // ── Last session ─────────────────────────────────
    QString lastPlaylistPath() const;
    void    setLastPlaylistPath(const QString& path);
    QString lastTrackPath() const;
    void    setLastTrackPath(const QString& path);
    qint64  lastTrackPosition() const;
    void    setLastTrackPosition(qint64 ms);

    // ── Skin ─────────────────────────────────────────
    QString currentSkin() const;
    void    setCurrentSkin(const QString& name);

    // ── Background ───────────────────────────────────
    QString backgroundImagePath() const;
    void    setBackgroundImagePath(const QString& path);
    int     backgroundDisplayMode() const;
    void    setBackgroundDisplayMode(int mode);
    int     backgroundBlurRadius() const;
    void    setBackgroundBlurRadius(int radius);
    float   backgroundOverlayOpacity() const;
    void    setBackgroundOverlayOpacity(float opacity);

    // ── DSP / EQ ─────────────────────────────────────
    QByteArray eqPreset() const;
    void       setEqPreset(const QByteArray& data);

    // ── Generic helpers ──────────────────────────────
    QString  value(const QString& key, const QString& defaultValue = {}) const;
    void     setValue(const QString& key, const QString& value);

signals:
    void configChanged(const QString& key);

private:
    explicit ConfigManager(QObject* parent = nullptr);
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    QString groupKey(const char* group, const char* key) const;

    QSettings m_settings;
};

} // namespace WaveRider
