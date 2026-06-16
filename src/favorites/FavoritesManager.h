#pragma once

#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

namespace WaveRider {

/// Manages favorited tracks with JSON persistence.
/// Singleton — use FavoritesManager::instance().
class FavoritesManager : public QObject {
    Q_OBJECT
public:
    static FavoritesManager* instance();

    // ── Query ────────────────────────────────────────
    bool isFavorite(const QString& filePath) const;
    QStringList allFavorites() const;

    // ── Mutation ─────────────────────────────────────
    void addFavorite(const QString& filePath);
    void removeFavorite(const QString& filePath);
    void toggleFavorite(const QString& filePath);

    // ── Persistence ─────────────────────────────────
    void load();
    void save();

signals:
    void favoriteAdded(const QString& filePath);
    void favoriteRemoved(const QString& filePath);
    void favoritesChanged();

private:
    explicit FavoritesManager(QObject* parent = nullptr);
    FavoritesManager(const FavoritesManager&) = delete;
    FavoritesManager& operator=(const FavoritesManager&) = delete;

    QString storagePath() const;

    QSet<QString> m_favorites;
};

} // namespace WaveRider
