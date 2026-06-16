#pragma once

#include <QObject>
#include <QString>
#include <QVector>

namespace WaveRider {

/// A single timestamped line of lyrics.
struct LyricLine {
    qint64  timeMs = 0;   // timestamp in milliseconds
    QString text;          // lyric text (may be empty for instrumental breaks)
};

/// Parsed result of an LRC file.
struct LyricData {
    QString title;             // [ti:...] tag, empty if not present
    QString artist;            // [ar:...] tag, empty if not present
    QVector<LyricLine> lines;  // sorted ascending by timeMs

    bool isEmpty() const { return lines.isEmpty(); }
};

/// Static parser for LRC (LyRiCs) format files.
///
/// LRC format example:
///   [ti:Song Title]
///   [ar:Artist Name]
///   [00:12.00]First line of lyrics
///   [00:15.80]Second line
///   [00:18.50][00:22.00]Repeated line (same text at two timestamps)
///
/// Timestamps can be [mm:ss], [mm:ss.xx], [mm:ss.xxx], or [hh:mm:ss].
class LyricParser : public QObject {
    Q_OBJECT
public:
    explicit LyricParser(QObject* parent = nullptr);

    /// Parse LRC content from a raw string.
    /// Returns LyricData with lines sorted ascending by timeMs.
    static LyricData parse(const QString& content);

    /// Parse an LRC file from disk.
    /// Tries UTF-8 first, falls back to local 8-bit encoding.
    /// Returns empty LyricData on failure.
    static LyricData parseFile(const QString& filePath);

    /// Given an audio file path (e.g. "C:/Music/song.mp3"),
    /// search for a matching .lrc file in the same directory.
    /// Looks for:  sameBaseName.lrc, sameBaseName.LRC (case-insensitive).
    /// Returns the LRC path if found, empty string otherwise.
    static QString findLrcFile(const QString& audioFilePath);
};

} // namespace WaveRider
