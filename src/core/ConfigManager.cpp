#include "core/ConfigManager.h"

namespace WaveRider {

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
    , m_settings(Config::OrgName, Config::AppName)
{
}

ConfigManager* ConfigManager::instance()
{
    static ConfigManager mgr;
    return &mgr;
}

QString ConfigManager::groupKey(const char* group, const char* key) const
{
    return QStringLiteral("%1/%2").arg(QLatin1String(group), QLatin1String(key));
}

// ── Window ──────────────────────────────────────────

QPoint ConfigManager::windowPosition() const
{
    return m_settings.value("window/position", QPoint(100, 100)).toPoint();
}

void ConfigManager::setWindowPosition(const QPoint& pos)
{
    m_settings.setValue("window/position", pos);
}

QSize ConfigManager::windowSize() const
{
    return m_settings.value("window/size", QSize(1100, 700)).toSize();
}

void ConfigManager::setWindowSize(const QSize& size)
{
    m_settings.setValue("window/size", size);
}

// ── Playback ─────────────────────────────────────────

float ConfigManager::volume() const
{
    return m_settings.value("playback/volume", 0.8f).toFloat();
}

void ConfigManager::setVolume(float level)
{
    m_settings.setValue("playback/volume", level);
}

PlayMode ConfigManager::playMode() const
{
    return static_cast<PlayMode>(m_settings.value("playback/playMode", static_cast<int>(PlayMode::LoopAll)).toInt());
}

void ConfigManager::setPlayMode(PlayMode mode)
{
    m_settings.setValue("playback/playMode", static_cast<int>(mode));
}

// ── Last session ─────────────────────────────────────

QString ConfigManager::lastPlaylistPath() const
{
    return m_settings.value("session/lastPlaylist").toString();
}

void ConfigManager::setLastPlaylistPath(const QString& path)
{
    m_settings.setValue("session/lastPlaylist", path);
}

QString ConfigManager::lastTrackPath() const
{
    return m_settings.value("session/lastTrack").toString();
}

void ConfigManager::setLastTrackPath(const QString& path)
{
    m_settings.setValue("session/lastTrack", path);
}

qint64 ConfigManager::lastTrackPosition() const
{
    return m_settings.value("session/lastPosition", 0).toLongLong();
}

void ConfigManager::setLastTrackPosition(qint64 ms)
{
    m_settings.setValue("session/lastPosition", ms);
}

// ── Skin ────────────────────────────────────────────

QString ConfigManager::currentSkin() const
{
    return m_settings.value("skin/current", "DarkModern").toString();
}

void ConfigManager::setCurrentSkin(const QString& name)
{
    m_settings.setValue("skin/current", name);
}

// ── Background ──────────────────────────────────────

QString ConfigManager::backgroundImagePath() const
{
    return m_settings.value("background/path").toString();
}

void ConfigManager::setBackgroundImagePath(const QString& path)
{
    m_settings.setValue("background/path", path);
}

int ConfigManager::backgroundDisplayMode() const
{
    return m_settings.value("background/mode", 0).toInt();
}

void ConfigManager::setBackgroundDisplayMode(int mode)
{
    m_settings.setValue("background/mode", mode);
}

int ConfigManager::backgroundBlurRadius() const
{
    return m_settings.value("background/blur", 0).toInt();
}

void ConfigManager::setBackgroundBlurRadius(int radius)
{
    m_settings.setValue("background/blur", radius);
}

float ConfigManager::backgroundOverlayOpacity() const
{
    return m_settings.value("background/opacity", 0.3f).toFloat();
}

void ConfigManager::setBackgroundOverlayOpacity(float opacity)
{
    m_settings.setValue("background/opacity", opacity);
}

// ── DSP / EQ ────────────────────────────────────────

QByteArray ConfigManager::eqPreset() const
{
    return m_settings.value("dsp/eqPreset").toByteArray();
}

void ConfigManager::setEqPreset(const QByteArray& data)
{
    m_settings.setValue("dsp/eqPreset", data);
}

// ── Generic ─────────────────────────────────────────

QString ConfigManager::value(const QString& key, const QString& defaultValue) const
{
    return m_settings.value(key, defaultValue).toString();
}

void ConfigManager::setValue(const QString& key, const QString& value)
{
    m_settings.setValue(key, value);
    emit configChanged(key);
}

} // namespace WaveRider
