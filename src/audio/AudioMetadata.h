#pragma once

#include <QString>
#include <QImage>
#include <QStringList>

// DWORD type for BASS interop (avoid including <windows.h> in header)
using DWORD = unsigned long;

namespace WaveRider {

/// Audio file metadata — tags, duration, codec info.
/// Uses BASS channel tags + BASS_ChannelGetInfo for duration and format.
struct AudioMetadata {
    QString title;
    QString artist;
    QString album;
    QString genre;
    int     year = 0;
    int     trackNumber = 0;
    QString comment;

    qint64  durationMs = 0;      // from BASS_ChannelGetLength
    int     bitrateKbps = 0;
    int     sampleRateHz = 0;
    int     channels = 0;

    bool    hasCoverArt = false;

    bool isEmpty() const { return title.isEmpty() && artist.isEmpty(); }

    /// Read tags from a BASS stream handle (must be a valid HSTREAM).
    /// Returns true if any metadata was read.
    static AudioMetadata fromStream(DWORD streamHandle);

    /// Read tags from a file path using BASS_StreamCreateFile internally.
    /// This creates a temporary stream, reads tags, then frees it.
    static AudioMetadata fromFile(const QString& filePath);

    /// Format duration as "m:ss" or "h:mm:ss"
    static QString formatDuration(qint64 ms);
};

} // namespace WaveRider
