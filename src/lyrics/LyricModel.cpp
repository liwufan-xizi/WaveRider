#include "lyrics/LyricModel.h"
#include <algorithm>
#include <QDebug>

namespace WaveRider {

// ────────────────────────────────────────────────────────────
// Construction
// ────────────────────────────────────────────────────────────
LyricModel::LyricModel(QObject* parent)
    : QObject(parent)
{
}

// ────────────────────────────────────────────────────────────
// Loading / clearing
// ────────────────────────────────────────────────────────────
bool LyricModel::loadFromFile(const QString& lrcFilePath)
{
    LyricData parsed = LyricParser::parseFile(lrcFilePath);
    if (parsed.isEmpty()) {
        qWarning() << "LyricModel: failed to parse" << lrcFilePath;
        return false;
    }
    m_data = std::move(parsed);
    emit modelLoaded();
    return true;
}

bool LyricModel::loadFromText(const QString& lrcContent)
{
    LyricData parsed = LyricParser::parse(lrcContent);
    if (parsed.isEmpty()) {
        qWarning() << "LyricModel: failed to parse LRC text";
        return false;
    }
    m_data = std::move(parsed);
    emit modelLoaded();
    return true;
}

void LyricModel::clear()
{
    m_data = {};
    emit modelCleared();
}

// ────────────────────────────────────────────────────────────
// Accessors
// ────────────────────────────────────────────────────────────
LyricLine LyricModel::lineAt(int index) const
{
    if (index >= 0 && index < m_data.lines.size()) {
        return m_data.lines[index];
    }
    return {};
}

qint64 LyricModel::lineTimeMs(int index) const
{
    if (index >= 0 && index < m_data.lines.size()) {
        return m_data.lines[index].timeMs;
    }
    return 0;
}

// ────────────────────────────────────────────────────────────
// Time-based lookup (binary search)
// ────────────────────────────────────────────────────────────
int LyricModel::lineIndexAtTime(qint64 positionMs) const
{
    if (m_data.lines.isEmpty()) return -1;

    // Before the first line
    if (positionMs < m_data.lines.first().timeMs) return -1;

    // At or past the last line
    if (positionMs >= m_data.lines.last().timeMs) {
        return m_data.lines.size() - 1;
    }

    // Binary search: find the last line with timeMs <= positionMs
    // std::upper_bound returns first element where timeMs > positionMs
    auto it = std::upper_bound(
        m_data.lines.begin(), m_data.lines.end(), positionMs,
        [](qint64 t, const LyricLine& line) {
            return t < line.timeMs;
        }
    );

    // Move back one to get the last line with timeMs <= positionMs
    return static_cast<int>(std::distance(m_data.lines.begin(), it)) - 1;
}

} // namespace WaveRider
