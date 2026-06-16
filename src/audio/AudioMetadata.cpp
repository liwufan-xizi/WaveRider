#include "audio/AudioMetadata.h"
#include "bass.h"
#include <QFileInfo>

namespace WaveRider {

// Helper: extract a BASS tag as QString
static QString bassTagToString(DWORD handle, const char* tagName)
{
    const char* tag = BASS_ChannelGetTags(handle, BASS_TAG_ID3V2);
    // For ID3v2, BASS returns the raw ID3v2 frame data.
    // For simpler tags, use BASS_TAG_OGG, BASS_TAG_APE, or BASS_TAG_ID3.
    // We try a few strategies.

    // Strategy 1: ID3v1 (simplest — BASS_TAG_ID3 returns array of char[30] strings)
    const char* id3v1 = static_cast<const char*>(BASS_ChannelGetTags(handle, BASS_TAG_ID3));
    if (id3v1) {
        // BASS_TAG_ID3 layout (all fields are 30 bytes, except comment which is 30):
        // offset  0: title  (30 chars)
        // offset 30: artist (30 chars)
        // offset 60: album  (30 chars)
        // offset 90: year   (4 chars)
        // offset 94: comment(30 chars)
        // offset 124: genre  (1 byte, index)
        if (strcmp(tagName, "title") == 0)
            return QString::fromLatin1(id3v1, 30).trimmed();
        if (strcmp(tagName, "artist") == 0)
            return QString::fromLatin1(id3v1 + 30, 30).trimmed();
        if (strcmp(tagName, "album") == 0)
            return QString::fromLatin1(id3v1 + 60, 30).trimmed();
        if (strcmp(tagName, "year") == 0) {
            QString y = QString::fromLatin1(id3v1 + 90, 4).trimmed();
            return y;
        }
        if (strcmp(tagName, "comment") == 0)
            return QString::fromLatin1(id3v1 + 94, 30).trimmed();
    }

    return {};
}

AudioMetadata AudioMetadata::fromStream(DWORD handle)
{
    AudioMetadata m;

    // ── Tags ───────────────────────────────────────
    m.title   = bassTagToString(handle, "title");
    m.artist  = bassTagToString(handle, "artist");
    m.album   = bassTagToString(handle, "album");
    m.year    = bassTagToString(handle, "year").toInt();
    m.comment = bassTagToString(handle, "comment");

    // If ID3v1 returned nothing for title, try the filename
    if (m.title.isEmpty()) {
        // Could try ID3v2 parsing here in a more complete implementation
    }

    // ── Duration & format ──────────────────────────
    QWORD lenBytes = BASS_ChannelGetLength(handle, BASS_POS_BYTE);
    double lenSecs = BASS_ChannelBytes2Seconds(handle, lenBytes);
    m.durationMs = static_cast<qint64>(lenSecs * 1000.0);

    BASS_CHANNELINFO info;
    if (BASS_ChannelGetInfo(handle, &info)) {
        m.sampleRateHz = info.freq;
        m.channels = info.chans;
        // Approximate bitrate
        if (m.durationMs > 0) {
            m.bitrateKbps = static_cast<int>(info.origres);  // BASS uses origres for bitrate on some formats
            if (m.bitrateKbps == 0) {
                // Fallback: estimate from file size
                QWORD fileLen = BASS_StreamGetFilePosition(handle, BASS_FILEPOS_END);
                m.bitrateKbps = static_cast<int>((fileLen * 8.0 / lenSecs) / 1000.0);
            }
        }
    }

    return m;
}

AudioMetadata AudioMetadata::fromFile(const QString& filePath)
{
    // Create a temporary stream just for tag reading
    DWORD stream = BASS_StreamCreateFile(
        FALSE,
        reinterpret_cast<const void*>(filePath.utf16()),
        0, 0,
        BASS_UNICODE
    );

    if (!stream) return {};

    AudioMetadata m = fromStream(stream);

    // If title is still empty, use the filename without extension
    if (m.title.isEmpty()) {
        m.title = QFileInfo(filePath).completeBaseName();
    }

    BASS_StreamFree(stream);
    return m;
}

QString AudioMetadata::formatDuration(qint64 ms)
{
    if (ms <= 0) return "--:--";

    qint64 totalSecs = ms / 1000;
    qint64 hours = totalSecs / 3600;
    qint64 mins  = (totalSecs % 3600) / 60;
    qint64 secs  = totalSecs % 60;

    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    }
    return QString("%1:%2")
        .arg(mins)
        .arg(secs, 2, 10, QChar('0'));
}

} // namespace WaveRider
