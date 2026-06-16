#include "lyrics/LyricParser.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <algorithm>

namespace WaveRider {

// ────────────────────────────────────────────────────────────
// Construction
// ────────────────────────────────────────────────────────────
LyricParser::LyricParser(QObject* parent)
    : QObject(parent)
{
}

// ────────────────────────────────────────────────────────────
// Core parsing logic
// ────────────────────────────────────────────────────────────
LyricData LyricParser::parse(const QString& content)
{
    LyricData data;

    // Regex for timestamps: [mm:ss], [mm:ss.xx], [mm:ss:xx], [mm:ss.xxx]
    // Captures: group 1 = minutes, group 2 = seconds, group 3 = fractional
    static const QRegularExpression timestampRx(
        QStringLiteral(R"(\[(\d+):(\d{2})[\.:](\d{2,3})\])")
    );

    // Regex for meta tags: [ti:...], [ar:...], etc.
    static const QRegularExpression tagRx(
        QStringLiteral(R"(\[(ti|ar|al|by|offset):(.+?)\])"),
        QRegularExpression::CaseInsensitiveOption
    );

    const QStringList lines = content.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

    for (const QString& rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;

        // ── Collect all timestamps on this line ────────
        QVector<qint64> timestamps;
        QRegularExpressionMatchIterator it = timestampRx.globalMatch(line);
        while (it.hasNext()) {
            QRegularExpressionMatch m = it.next();
            int minutes = m.captured(1).toInt();
            int seconds = m.captured(2).toInt();
            QString frac = m.captured(3);

            // Fractional part: "50" → 500ms, "500" → 500ms
            int centiseconds = 0;
            if (frac.length() == 2) {
                centiseconds = frac.toInt() * 10;    // "50" → 500ms
            } else if (frac.length() == 3) {
                centiseconds = frac.toInt();          // "500" → 500ms
            }

            qint64 ms = static_cast<qint64>(minutes) * 60000
                      + static_cast<qint64>(seconds) * 1000
                      + centiseconds;
            timestamps.append(ms);
        }

        // Remove all timestamp tags to get the lyric text
        line.remove(timestampRx);
        QString text = line.trimmed();

        // ── Extract meta tags from the remaining text ──
        if (!timestamps.isEmpty()) {
            // This is a lyric line with timestamps
            for (qint64 ts : timestamps) {
                data.lines.append({ts, text});
            }
        } else {
            // No timestamps — check for meta tags
            QRegularExpressionMatch m = tagRx.match(line);
            if (m.hasMatch()) {
                QString tag = m.captured(1).toLower();
                QString value = m.captured(2).trimmed();
                if (tag == "ti") {
                    data.title = value;
                } else if (tag == "ar") {
                    data.artist = value;
                }
                // Other tags (al, by, offset) are silently ignored for now
            }
        }
    }

    // ── Sort by timestamp ascending ────────────────────
    std::sort(data.lines.begin(), data.lines.end(),
        [](const LyricLine& a, const LyricLine& b) {
            return a.timeMs < b.timeMs;
        });

    return data;
}

// ────────────────────────────────────────────────────────────
// File parsing with encoding fallback
// ────────────────────────────────────────────────────────────
LyricData LyricParser::parseFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "LyricParser: cannot open file" << filePath;
        return {};
    }

    QByteArray raw = file.readAll();
    file.close();

    if (raw.isEmpty()) return {};

    // Skip UTF-8 BOM if present
    if (raw.startsWith("\xEF\xBB\xBF")) {
        raw.remove(0, 3);
    }

    // Try UTF-8 first
    QString content = QString::fromUtf8(raw);

    // If the result has replacement characters, fall back to local 8-bit
    if (content.contains(QChar(0xFFFD))) {
        content = QString::fromLocal8Bit(raw);
    }

    return parse(content);
}

// ────────────────────────────────────────────────────────────
// Auto-discover LRC file from audio path
// ────────────────────────────────────────────────────────────
QString LyricParser::findLrcFile(const QString& audioFilePath)
{
    QFileInfo audioInfo(audioFilePath);
    QString basePath = audioInfo.absolutePath() + "/" + audioInfo.completeBaseName();

    // Try case variations: .lrc, .LRC
    for (const QString& ext : {QStringLiteral(".lrc"), QStringLiteral(".LRC")}) {
        QString candidate = basePath + ext;
        if (QFile::exists(candidate)) {
            return candidate;
        }
    }
    return {};
}

} // namespace WaveRider
