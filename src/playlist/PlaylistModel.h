#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QString>
#include "audio/AudioMetadata.h"

namespace WaveRider {

/// QAbstractListModel for the playlist.
/// Each row represents one track. Data is exposed via roles.
class PlaylistModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        FilePathRole     = Qt::UserRole + 1,
        TitleRole,
        ArtistRole,
        AlbumRole,
        DurationMsRole,
        DurationTextRole,
        IsPlayingRole,
        IsFavoriteRole,
        IndexRole,
        TrackNumberRole,
    };

    explicit PlaylistModel(QObject* parent = nullptr);

    // ── QAbstractListModel interface ─────────────────
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // ── Mutation ─────────────────────────────────────
    void     appendTrack(const QString& filePath);
    void     appendTracks(const QStringList& filePaths);
    void     insertTrack(int pos, const QString& filePath);
    void     removeTrack(int pos);
    void     removeTracks(const QList<int>& positions);
    void     moveTrack(int from, int to);
    void     clear();

    // ── Access ───────────────────────────────────────
    QString  filePathAt(int pos) const;
    AudioMetadata metadataAt(int pos) const;
    int      trackCount() const { return m_tracks.size(); }
    int      currentIndex() const { return m_currentIndex; }
    void     setCurrentIndex(int index);
    int      findTrackByPath(const QString& path) const;

    // ── Favorites integration ────────────────────────
    void     setFavoriteState(int pos, bool isFav);
    void     refreshFavorites();

    // ── Next / Prev helpers ──────────────────────────
    int      nextIndex(int current, bool loop) const;
    int      prevIndex(int current) const;
    int      randomIndex() const;

signals:
    void trackDoubleClicked(int index);

private:
    struct TrackInfo {
        QString       filePath;
        AudioMetadata metadata;
        bool          isFavorite = false;
    };

    void refreshMetadata(int pos);

    QVector<TrackInfo> m_tracks;
    int                m_currentIndex = -1;
};

} // namespace WaveRider
