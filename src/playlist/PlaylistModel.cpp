#include "playlist/PlaylistModel.h"
#include "core/SignalBus.h"
#include "favorites/FavoritesManager.h"
#include <QFileInfo>
#include <algorithm>
#include <random>

namespace WaveRider {

PlaylistModel::PlaylistModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

// ============================================================
// QAbstractListModel interface
// ============================================================
int PlaylistModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_tracks.size();
}

QVariant PlaylistModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tracks.size())
        return {};

    const auto& track = m_tracks.at(index.row());

    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        if (track.metadata.title.isEmpty())
            return QFileInfo(track.filePath).completeBaseName();
        return track.metadata.title;

    case ArtistRole:
        return track.metadata.artist.isEmpty() ? QStringLiteral("Unknown Artist") : track.metadata.artist;

    case AlbumRole:
        return track.metadata.album.isEmpty() ? QString() : track.metadata.album;

    case FilePathRole:
        return track.filePath;

    case DurationMsRole:
        return track.metadata.durationMs;

    case DurationTextRole:
        return AudioMetadata::formatDuration(track.metadata.durationMs);

    case IsPlayingRole:
        return index.row() == m_currentIndex;

    case IsFavoriteRole:
        return track.isFavorite;

    case IndexRole:
        return index.row();

    case TrackNumberRole:
        return track.metadata.trackNumber;

    case Qt::ToolTipRole: {
        QString tip = QString("%1\n%2 — %3\n%4")
            .arg(data(index, TitleRole).toString(),
                 data(index, ArtistRole).toString(),
                 data(index, AlbumRole).toString(),
                 data(index, DurationTextRole).toString());
        return tip;
    }

    default:
        break;
    }
    return {};
}

Qt::ItemFlags PlaylistModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled;
}

// ============================================================
// Mutation
// ============================================================
void PlaylistModel::appendTrack(const QString& filePath)
{
    int pos = m_tracks.size();
    beginInsertRows(QModelIndex(), pos, pos);

    TrackInfo info;
    info.filePath = filePath;
    info.metadata = AudioMetadata::fromFile(filePath);
    // If title still empty, use filename
    if (info.metadata.title.isEmpty()) {
        info.metadata.title = QFileInfo(filePath).completeBaseName();
    }
    info.isFavorite = FavoritesManager::instance()->isFavorite(filePath);

    m_tracks.append(info);
    endInsertRows();

    emit SignalBus::instance()->playlistChanged();
}

void PlaylistModel::appendTracks(const QStringList& filePaths)
{
    if (filePaths.isEmpty()) return;

    int first = m_tracks.size();
    int last  = first + filePaths.size() - 1;
    beginInsertRows(QModelIndex(), first, last);

    for (const auto& path : filePaths) {
        TrackInfo info;
        info.filePath = path;
        info.metadata = AudioMetadata::fromFile(path);
        if (info.metadata.title.isEmpty()) {
            info.metadata.title = QFileInfo(path).completeBaseName();
        }
        info.isFavorite = FavoritesManager::instance()->isFavorite(path);
        m_tracks.append(info);
    }

    endInsertRows();
    emit SignalBus::instance()->playlistChanged();
}

void PlaylistModel::insertTrack(int pos, const QString& filePath)
{
    pos = qBound(0, pos, m_tracks.size());
    beginInsertRows(QModelIndex(), pos, pos);

    TrackInfo info;
    info.filePath = filePath;
    info.metadata = AudioMetadata::fromFile(filePath);
    if (info.metadata.title.isEmpty()) {
        info.metadata.title = QFileInfo(filePath).completeBaseName();
    }
    info.isFavorite = FavoritesManager::instance()->isFavorite(filePath);

    m_tracks.insert(pos, info);

    // Adjust current index if needed
    if (m_currentIndex >= pos) {
        m_currentIndex++;
    }

    endInsertRows();
    emit SignalBus::instance()->playlistChanged();
}

void PlaylistModel::removeTrack(int pos)
{
    if (pos < 0 || pos >= m_tracks.size()) return;

    beginRemoveRows(QModelIndex(), pos, pos);
    m_tracks.removeAt(pos);

    // Adjust current index
    if (m_currentIndex > pos) {
        m_currentIndex--;
    } else if (m_currentIndex == pos) {
        m_currentIndex = (m_tracks.isEmpty()) ? -1 : qMin(m_currentIndex, m_tracks.size() - 1);
    }

    endRemoveRows();
    emit SignalBus::instance()->playlistChanged();
}

void PlaylistModel::removeTracks(const QList<int>& positions)
{
    if (positions.isEmpty()) return;

    // Remove from highest to lowest to keep indices valid
    QList<int> sorted = positions;
    std::sort(sorted.begin(), sorted.end(), std::greater<int>());

    for (int pos : sorted) {
        removeTrack(pos);
    }
}

void PlaylistModel::moveTrack(int from, int to)
{
    if (from < 0 || from >= m_tracks.size() || to < 0 || to >= m_tracks.size())
        return;
    if (from == to) return;

    // Qt's model move requires beginMoveRows/endMoveRows
    // but for simplicity, we use a layout change
    beginResetModel();
    m_tracks.move(from, to);
    if (m_currentIndex == from) {
        m_currentIndex = to;
    } else if (from < m_currentIndex && to >= m_currentIndex) {
        m_currentIndex--;
    } else if (from > m_currentIndex && to <= m_currentIndex) {
        m_currentIndex++;
    }
    endResetModel();
    emit SignalBus::instance()->playlistChanged();
}

void PlaylistModel::clear()
{
    if (m_tracks.isEmpty()) return;
    beginResetModel();
    m_tracks.clear();
    m_currentIndex = -1;
    endResetModel();
    emit SignalBus::instance()->playlistChanged();
}

// ============================================================
// Access
// ============================================================
QString PlaylistModel::filePathAt(int pos) const
{
    if (pos < 0 || pos >= m_tracks.size()) return {};
    return m_tracks.at(pos).filePath;
}

AudioMetadata PlaylistModel::metadataAt(int pos) const
{
    if (pos < 0 || pos >= m_tracks.size()) return {};
    return m_tracks.at(pos).metadata;
}

void PlaylistModel::setCurrentIndex(int index)
{
    if (index == m_currentIndex) return;

    int oldIndex = m_currentIndex;
    m_currentIndex = index;

    // Notify views about the change
    if (oldIndex >= 0 && oldIndex < m_tracks.size()) {
        emit dataChanged(this->index(oldIndex), this->index(oldIndex), {IsPlayingRole});
    }
    if (m_currentIndex >= 0 && m_currentIndex < m_tracks.size()) {
        emit dataChanged(this->index(m_currentIndex), this->index(m_currentIndex), {IsPlayingRole});
    }

    emit SignalBus::instance()->currentTrackChanged(m_currentIndex);
}

int PlaylistModel::findTrackByPath(const QString& path) const
{
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks.at(i).filePath == path) return i;
    }
    return -1;
}

// ============================================================
// Favorites integration
// ============================================================
void PlaylistModel::setFavoriteState(int pos, bool isFav)
{
    if (pos < 0 || pos >= m_tracks.size()) return;
    m_tracks[pos].isFavorite = isFav;
    emit dataChanged(index(pos), index(pos), {IsFavoriteRole});
}

void PlaylistModel::refreshFavorites()
{
    auto* favMgr = FavoritesManager::instance();
    for (int i = 0; i < m_tracks.size(); ++i) {
        bool isFav = favMgr->isFavorite(m_tracks[i].filePath);
        if (m_tracks[i].isFavorite != isFav) {
            m_tracks[i].isFavorite = isFav;
            emit dataChanged(index(i), index(i), {IsFavoriteRole});
        }
    }
}

// ============================================================
// Next / Prev / Random helpers
// ============================================================
int PlaylistModel::nextIndex(int current, bool loop) const
{
    if (m_tracks.isEmpty()) return -1;
    if (current + 1 < m_tracks.size()) return current + 1;
    return loop ? 0 : -1;
}

int PlaylistModel::prevIndex(int current) const
{
    if (m_tracks.isEmpty()) return -1;
    if (current - 1 >= 0) return current - 1;
    return m_tracks.size() - 1;  // wrap to last
}

int PlaylistModel::randomIndex() const
{
    if (m_tracks.isEmpty()) return -1;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, m_tracks.size() - 1);
    return dist(gen);
}

} // namespace WaveRider
