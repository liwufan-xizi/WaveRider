#include "favorites/FavoritesManager.h"
#include "core/SignalBus.h"
#include "core/ConfigManager.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QStandardPaths>
#include <QDir>

namespace WaveRider {

FavoritesManager::FavoritesManager(QObject* parent)
    : QObject(parent)
{
    load();
}

FavoritesManager* FavoritesManager::instance()
{
    static FavoritesManager mgr;
    return &mgr;
}

QString FavoritesManager::storagePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/favorites.json";
}

// ── Query ───────────────────────────────────────────

bool FavoritesManager::isFavorite(const QString& filePath) const
{
    return m_favorites.contains(filePath);
}

QStringList FavoritesManager::allFavorites() const
{
    return m_favorites.values();
}

// ── Mutation ────────────────────────────────────────

void FavoritesManager::addFavorite(const QString& filePath)
{
    if (m_favorites.contains(filePath)) return;
    m_favorites.insert(filePath);
    save();
    emit favoriteAdded(filePath);
    emit favoritesChanged();
    SignalBus::instance()->favoriteAdded(filePath);
}

void FavoritesManager::removeFavorite(const QString& filePath)
{
    if (!m_favorites.contains(filePath)) return;
    m_favorites.remove(filePath);
    save();
    emit favoriteRemoved(filePath);
    emit favoritesChanged();
    SignalBus::instance()->favoriteRemoved(filePath);
}

void FavoritesManager::toggleFavorite(const QString& filePath)
{
    if (isFavorite(filePath)) {
        removeFavorite(filePath);
    } else {
        addFavorite(filePath);
    }
}

// ── Persistence ─────────────────────────────────────

void FavoritesManager::load()
{
    QFile file(storagePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isArray()) return;

    m_favorites.clear();
    for (const auto& val : doc.array()) {
        if (val.isString()) {
            m_favorites.insert(val.toString());
        }
    }
}

void FavoritesManager::save()
{
    QJsonArray arr;
    for (const auto& path : m_favorites) {
        arr.append(path);
    }

    QFile file(storagePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(arr).toJson());
        file.close();
    }
}

} // namespace WaveRider
