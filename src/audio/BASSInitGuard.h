#pragma once

#include <Windows.h>
#include "bass.h"

namespace WaveRider {

/// RAII guard for BASS library initialization.
/// Calls BASS_Init() on construction and BASS_Free() on destruction.
///
/// Usage:
///   BASSInitGuard guard(hwnd);
///   if (!guard.isOk()) { /* handle error */ }
///   // ... use BASS ...
///   // BASS_Free() called automatically when guard goes out of scope
class BASSInitGuard {
public:
    /// Initialize BASS with the given window handle.
    /// @param winHandle  HWND from any QWidget::winId(), or 0 for console
    explicit BASSInitGuard(HWND winHandle = nullptr);

    ~BASSInitGuard();

    /// Returns true if BASS_Init() succeeded.
    bool isOk() const { return m_ok; }

    /// Returns the last BASS error code (BASS_ErrorGetCode()).
    int lastError() const { return m_errorCode; }

    // Non-copyable, non-movable
    BASSInitGuard(const BASSInitGuard&) = delete;
    BASSInitGuard& operator=(const BASSInitGuard&) = delete;

private:
    bool m_ok = false;
    int  m_errorCode = 0;
};

} // namespace WaveRider
