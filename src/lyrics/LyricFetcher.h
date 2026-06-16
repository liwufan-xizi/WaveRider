#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QTimer>

namespace WaveRider {

/// Fetches synced lyrics (LRC) from online APIs and caches them locally.
///
/// Data source: lrclib.net free REST API
///   - Search: GET https://lrclib.net/api/search?artist_name=...&track_name=...
///
/// Workflow:
///   1. Check local cache (AppData/lyrics/) → hit: return immediately
///   2. HTTP GET lrclib.net → parse JSON → extract syncedLyrics field
///   3. Save to cache → emit lyricsFetched()
///
/// All network operations are asynchronous. Connect to the signals
/// to receive results.
class LyricFetcher : public QObject {
    Q_OBJECT
public:
    explicit LyricFetcher(QObject* parent = nullptr);
    ~LyricFetcher() override;

    /// Start searching lyrics for the given track.
    /// If cached, emits lyricsFetched() synchronously before returning.
    /// Otherwise, emits lyricsFetched/lyricsNotFound/fetchError asynchronously.
    void fetchLyrics(const QString& artist, const QString& title);

    /// Cancel any in-flight network request.
    void cancel();

    /// Returns the cache directory, creating it if needed.
    static QString cacheDir();

    /// Look up cached LRC content for a given artist/title.
    /// Returns empty string if not cached.
    static QString cachedLyrics(const QString& artist, const QString& title);

signals:
    /// Emitted when lyrics are successfully obtained (from cache or network).
    void lyricsFetched(const QString& artist, const QString& title,
                       const QString& lrcContent);

    /// Emitted when search returns no results.
    void lyricsNotFound(const QString& artist, const QString& title);

    /// Emitted on network or parse error.
    void fetchError(const QString& message);

private slots:
    void onSearchReply();

private:
    QString makeCacheKey(const QString& artist, const QString& title) const;
    void saveToCache(const QString& key, const QString& lrcContent);
    QString loadFromCache(const QString& key) const;

    QNetworkAccessManager* m_nam      = nullptr;
    QTimer*                m_timeout  = nullptr;  // 10s request timeout
    QString                m_artist;
    QString                m_title;
};

} // namespace WaveRider
