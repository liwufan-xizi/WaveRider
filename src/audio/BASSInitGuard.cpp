#include "audio/BASSInitGuard.h"

namespace WaveRider {

BASSInitGuard::BASSInitGuard(HWND winHandle)
{
    // BASS_Init: device=-1 (default), freq=44100, flags=0
    m_ok = BASS_Init(-1, 44100, 0, winHandle, nullptr);
    if (!m_ok) {
        m_errorCode = BASS_ErrorGetCode();
    }
}

BASSInitGuard::~BASSInitGuard()
{
    if (m_ok) {
        BASS_Free();
    }
}

} // namespace WaveRider
