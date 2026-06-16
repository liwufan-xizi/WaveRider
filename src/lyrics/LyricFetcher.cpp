#include "lyrics/LyricFetcher.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QUrlQuery>

namespace WaveRider {

// ────────────────────────────────────────────────────────────
// Constants
// ────────────────────────────────────────────────────────────
static const int    kRequestTimeoutMs = 10000;  // 10 seconds
static const char*  kApiBaseUrl       = "https://lrclib.net/api";

// ────────────────────────────────────────────────────────────
// Construction / Destruction
// ────────────────────────────────────────────────────────────
LyricFetcher::LyricFetcher(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_timeout(new QTimer(this))
{
    m_timeout->setSingleShot(true);
}

LyricFetcher::~LyricFetcher()
{
    cancel();
}

// ────────────────────────────────────────────────────────────
// Public API
// ────────────────────────────────────────────────────────────
void LyricFetcher::fetchLyrics(const QString& artist, const QString& title)
{
    // Cancel any in-flight request
    cancel();

    m_artist = artist.trimmed();
    m_title  = title.trimmed();

    if (m_artist.isEmpty() || m_title.isEmpty()) {
        emit fetchError("Artist or title is empty");
        return;
    }

    // ── 1. Check cache ────────────────────────────────
    QString key  = makeCacheKey(m_artist, m_title);
    QString cached = loadFromCache(key);
    if (!cached.isEmpty()) {
        qDebug() << "LyricFetcher: cache hit for" << key;
        emit lyricsFetched(m_artist, m_title, cached);
        return;
    }

    // ── 2. Build API request ──────────────────────────
    QUrl url(QString::fromLatin1("%1/search").arg(kApiBaseUrl));
    QUrlQuery query;
    query.addQueryItem("artist_name", m_artist);
    query.addQueryItem("track_name",  m_title);
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setTransferTimeout(kRequestTimeoutMs);

    qDebug() << "LyricFetcher: searching" << url.toString();

    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, &LyricFetcher::onSearchReply);

    // Timeout guard
    connect(m_timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
    m_timeout->start(kRequestTimeoutMs);
}

void LyricFetcher::cancel()
{
    m_timeout->stop();
    m_nam->deleteLater();
    m_nam = new QNetworkAccessManager(this);
}

// ────────────────────────────────────────────────────────────
// Cache management (static)
// ────────────────────────────────────────────────────────────
QString LyricFetcher::cacheDir()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/lyrics";
    QDir().mkpath(dir);
    return dir;
}

QString LyricFetcher::cachedLyrics(const QString& artist, const QString& title)
{
    LyricFetcher dummy;
    QString key = dummy.makeCacheKey(artist.trimmed(), title.trimmed());
    return dummy.loadFromCache(key);
}

// ────────────────────────────────────────────────────────────
// Private helpers
// ────────────────────────────────────────────────────────────
QString LyricFetcher::makeCacheKey(const QString& artist, const QString& title) const
{
    // Create a filesystem-safe key: lowercase, replace illegal chars
    QString raw = artist + " - " + title;
    raw = raw.toLower().trimmed();
    // Replace characters not allowed in Windows filenames
    static const QString illegalChars = "<>:\"/\\|?*";
    for (const QChar& ch : illegalChars) {
        raw.replace(ch, '_');
    }
    // Collapse multiple underscores/spaces
    while (raw.contains("__")) raw.replace("__", "_");
    return raw;
}

QString LyricFetcher::loadFromCache(const QString& key) const
{
    QFile file(cacheDir() + "/" + key + ".lrc");
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    QByteArray raw = file.readAll();
    file.close();

    // Try UTF-8, then local 8-bit
    QString content = QString::fromUtf8(raw);
    if (content.contains(QChar(0xFFFD))) {
        content = QString::fromLocal8Bit(raw);
    }
    return content;
}

void LyricFetcher::saveToCache(const QString& key, const QString& lrcContent)
{
    QFile file(cacheDir() + "/" + key + ".lrc");
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(lrcContent.toUtf8());
        file.close();
        qDebug() << "LyricFetcher: saved to cache" << key;
    }
}

// ────────────────────────────────────────────────────────────
// Network reply handler
// ────────────────────────────────────────────────────────────
void LyricFetcher::onSearchReply()
{
    m_timeout->stop();

    auto* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        // Shouldn't happen, but guard against stale signals
        return;
    }

    // Ensure we clean up the reply
    reply->deleteLater();

    // ── Check for network errors ──────────────────────
    if (reply->error() != QNetworkReply::NoError) {
        QString errMsg = reply->errorString();
        qWarning() << "LyricFetcher: network error —" << errMsg;
        emit fetchError("Network error: " + errMsg);
        return;
    }

    // ── Parse JSON response ───────────────────────────
    QByteArray body = reply->readAll();
    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(body, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        qWarning() << "LyricFetcher: JSON parse error —" << parseErr.errorString();
        emit fetchError("Failed to parse server response");
        return;
    }

    // Response is a JSON array of search results
    QJsonArray results = doc.array();
    if (results.isEmpty()) {
        qDebug() << "LyricFetcher: no results for" << m_artist << "-" << m_title;
        emit lyricsNotFound(m_artist, m_title);
        return;
    }

    // ── Select the best match ─────────────────────────
    // Priority: exact artist+title match with syncedLyrics, then any syncedLyrics,
    // then the first result
    QJsonObject bestMatch;
    bool foundSynced = false;

    // First pass: look for exact match with synced lyrics
    for (const QJsonValue& val : results) {
        QJsonObject obj = val.toObject();
        QString name   = obj["artistName"].toString().trimmed();
        QString track  = obj["trackName"].toString().trimmed();
        QString synced = obj["syncedLyrics"].toString();

        bool artistOk = (name.compare(m_artist, Qt::CaseInsensitive) == 0);
        bool titleOk  = (track.compare(m_title, Qt::CaseInsensitive) == 0);

        if (artistOk && titleOk && !synced.isEmpty()) {
            bestMatch   = obj;
            foundSynced = true;
            break;  // Perfect match found
        }
    }

    // Second pass: if no exact match, take the first entry with synced lyrics
    if (!foundSynced) {
        for (const QJsonValue& val : results) {
            QJsonObject obj = val.toObject();
            QString synced = obj["syncedLyrics"].toString();
            if (!synced.isEmpty()) {
                bestMatch   = obj;
                foundSynced = true;
                break;
            }
        }
    }

    // Last resort: use the first result even without synced lyrics
    if (!foundSynced) {
        bestMatch = results.first().toObject();
    }

    // ── Extract lyrics ────────────────────────────────
    QString lrcContent = bestMatch["syncedLyrics"].toString();
    if (lrcContent.isEmpty()) {
        // Check for plain text lyrics as fallback
        lrcContent = bestMatch["plainLyrics"].toString();
        if (lrcContent.isEmpty()) {
            emit lyricsNotFound(m_artist, m_title);
            return;
        }
        // Plain lyrics have no timestamps but can still be displayed
    }

    // ── Cache and emit ────────────────────────────────
    QString key = makeCacheKey(m_artist, m_title);
    saveToCache(key, lrcContent);
    emit lyricsFetched(m_artist, m_title, lrcContent);
}

} // namespace WaveRider
