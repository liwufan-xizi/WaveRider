#include "audio/AudioEngine.h"
#include "audio/BASSInitGuard.h"
#include <QDebug>

namespace WaveRider {

// ============================================================
// BASS end-of-stream sync callback (C-linkage)
// ============================================================
void CALLBACK AudioEngine::endSyncCallback(HSYNC /*handle*/, DWORD /*channel*/,
                                            DWORD /*data*/, void* user)
{
    auto* engine = static_cast<AudioEngine*>(user);
    if (engine) {
        // Defer to the main thread via QMetaObject to avoid threading issues.
        // BASS sync callbacks come from a BASS internal thread.
        QMetaObject::invokeMethod(engine, "emitTrackFinished", Qt::QueuedConnection);
    }
}

// ============================================================
// Construction / Destruction
// ============================================================
AudioEngine::AudioEngine(QObject* parent)
    : QObject(parent)
    , m_positionTimer(new QTimer(this))
{
    connect(m_positionTimer, &QTimer::timeout, this, &AudioEngine::onPositionTimer);
}

AudioEngine::~AudioEngine()
{
    shutdown();
}

// ============================================================
// Lifecycle
// ============================================================
bool AudioEngine::initialize(HWND winHandle)
{
    m_guard = std::make_unique<BASSInitGuard>(winHandle);
    if (!m_guard->isOk()) {
        qWarning() << "BASS_Init failed, error code:" << m_guard->lastError();
        emit errorOccurred(QString("BASS initialization failed (error %1)").arg(m_guard->lastError()));
        return false;
    }
    m_initialized = true;
    return true;
}

void AudioEngine::shutdown()
{
    m_positionTimer->stop();
    if (m_stream) {
        BASS_StreamFree(m_stream);
        m_stream = 0;
    }
    m_guard.reset();
    m_initialized = false;
    m_currentFile.clear();
}

// ============================================================
// File loading
// ============================================================
bool AudioEngine::loadFile(const QString& filePath)
{
    if (!m_initialized) {
        qWarning() << "AudioEngine::loadFile called before initialize()";
        return false;
    }

    // Free the previous stream
    if (m_stream) {
        BASS_StreamFree(m_stream);
        m_stream = 0;
    }

    // BASS_StreamCreateFile with Unicode path
    // BASS_UNICODE flag tells BASS the path is UTF-16
    m_stream = BASS_StreamCreateFile(
        FALSE,                              // not from memory
        reinterpret_cast<const void*>(filePath.utf16()),  // UTF-16 path
        0, 0,                               // offset, length (whole file)
        BASS_UNICODE                         // path is Unicode
    );

    if (!m_stream) {
        int err = BASS_ErrorGetCode();
        qWarning() << "BASS_StreamCreateFile failed for" << filePath << "error:" << err;
        emit errorOccurred(QString("Cannot open file (error %1): %2").arg(err).arg(filePath));
        return false;
    }

    // Register end-of-stream sync
    BASS_ChannelSetSync(
        m_stream,
        BASS_SYNC_END,
        0,
        endSyncCallback,
        this
    );

    m_currentFile = filePath;
    emit trackStarted(filePath);
    return true;
}

void AudioEngine::unload()
{
    if (m_stream) {
        BASS_StreamFree(m_stream);
        m_stream = 0;
    }
    m_currentFile.clear();
    setState(PlaybackState::Stopped);
}

// ============================================================
// Playback control
// ============================================================
bool AudioEngine::play()
{
    if (!m_stream) return false;
    if (!BASS_ChannelPlay(m_stream, FALSE)) {
        emit errorOccurred("Playback failed");
        return false;
    }
    m_positionTimer->start(Config::PosTimerMs);
    setState(PlaybackState::Playing);
    return true;
}

bool AudioEngine::pause()
{
    if (!m_stream) return false;
    BASS_ChannelPause(m_stream);
    m_positionTimer->stop();
    setState(PlaybackState::Paused);
    return true;
}

bool AudioEngine::stop()
{
    if (!m_stream) return false;
    BASS_ChannelStop(m_stream);
    m_positionTimer->stop();
    setState(PlaybackState::Stopped);
    return true;
}

bool AudioEngine::seek(double seconds)
{
    if (!m_stream) return false;
    QWORD bytePos = BASS_ChannelSeconds2Bytes(m_stream, seconds);
    return BASS_ChannelSetPosition(m_stream, bytePos, BASS_POS_BYTE);
}

void AudioEngine::togglePlayPause()
{
    if (m_state == PlaybackState::Playing) {
        pause();
    } else if (m_state == PlaybackState::Paused) {
        play();
    }
}

// ============================================================
// Position & duration
// ============================================================
qint64 AudioEngine::positionMs() const
{
    if (!m_stream) return 0;
    QWORD bytes = BASS_ChannelGetPosition(m_stream, BASS_POS_BYTE);
    double secs = BASS_ChannelBytes2Seconds(m_stream, bytes);
    return static_cast<qint64>(secs * 1000.0);
}

qint64 AudioEngine::durationMs() const
{
    if (!m_stream) return 0;
    QWORD bytes = BASS_ChannelGetLength(m_stream, BASS_POS_BYTE);
    double secs = BASS_ChannelBytes2Seconds(m_stream, bytes);
    return static_cast<qint64>(secs * 1000.0);
}

// ============================================================
// Volume
// ============================================================
void AudioEngine::setVolume(float level)
{
    m_volume = qBound(0.0f, level, 1.0f);
    if (m_stream) {
        BASS_ChannelSetAttribute(m_stream, BASS_ATTRIB_VOL, m_volume);
    }
}

// ============================================================
// Internal
// ============================================================
void AudioEngine::setState(PlaybackState s)
{
    if (m_state != s) {
        m_state = s;
        emit stateChanged(m_state);
    }
}

void AudioEngine::emitTrackFinished()
{
    emit trackFinished(m_currentFile);
}

void AudioEngine::onPositionTimer()
{
    if (m_state == PlaybackState::Playing && m_stream) {
        emit positionUpdated(positionMs(), durationMs());
    }
}

} // namespace WaveRider
