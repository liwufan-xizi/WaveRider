#include "playlist/PlayModeEngine.h"
#include <algorithm>
#include <random>

namespace WaveRider {

PlayModeEngine::PlayModeEngine(QObject* parent)
    : QObject(parent)
{
}

void PlayModeEngine::setMode(PlayMode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        m_shufflePos = 0;
        emit modeChanged(m_mode);
    }
}

int PlayModeEngine::nextTrack(int current, int count)
{
    if (count <= 0) return -1;

    switch (m_mode) {
    case PlayMode::Sequential:
        // Play tracks in order, stop at the end
        if (current + 1 < count)
            return current + 1;
        return -1;  // stop

    case PlayMode::LoopOne:
        // Repeat the current track
        return current;

    case PlayMode::LoopAll:
        // Loop back to the beginning
        return (current + 1) % count;

    case PlayMode::Shuffle:
        // Pre-generate a random permutation of all indices
        if (m_shuffleOrder.size() != count || m_shufflePos >= count) {
            regenerateShuffleOrder(count);
            m_shufflePos = 0;
        }
        if (m_shufflePos < count) {
            return m_shuffleOrder.at(m_shufflePos++);
        }
        return -1;
    }

    return -1;
}

void PlayModeEngine::regenerateShuffleOrder(int count)
{
    m_shuffleOrder.resize(count);
    std::iota(m_shuffleOrder.begin(), m_shuffleOrder.end(), 0);

    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::shuffle(m_shuffleOrder.begin(), m_shuffleOrder.end(), gen);
    m_shufflePos = 0;
}

} // namespace WaveRider
