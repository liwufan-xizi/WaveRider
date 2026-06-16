#pragma once

#include <QObject>
#include "lyrics/LyricParser.h"

namespace WaveRider {

/// Model holding parsed lyric data.
/// Provides indexed access and time-based binary-search lookup.
class LyricModel : public QObject {
    Q_OBJECT
public:
    explicit LyricModel(QObject* parent = nullptr);

    /// Load lyrics from an LRC file on disk. Returns true on success.
    bool loadFromFile(const QString& lrcFilePath);

    /// Load lyrics from a raw LRC text string. Returns true on success.
    bool loadFromText(const QString& lrcContent);

    /// Clear all lyric data.
    void clear();

    bool isEmpty()     const { return m_data.isEmpty(); }
    int  lineCount()   const { return m_data.lines.size(); }

    QString title()    const { return m_data.title; }
    QString artist()   const { return m_data.artist; }

    /// Get a lyric line by index. Returns empty LyricLine if out of range.
    LyricLine lineAt(int index) const;

    /// Get the timestamp (ms) of a line by index, or 0 if out of bounds.
    qint64 lineTimeMs(int index) const;

    /// Find the lyric line index active at the given playback position.
    /// Uses binary search (O(log N)). Returns:
    ///   -1           if position is before the first line
    ///   last index   if position is at or past the last line
    int lineIndexAtTime(qint64 positionMs) const;

    /// Raw access to parsed data.
    const LyricData& data() const { return m_data; }

signals:
    void modelLoaded();
    void modelCleared();

private:
    LyricData m_data;
};

} // namespace WaveRider
