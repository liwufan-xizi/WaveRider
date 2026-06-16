#include "dsp/builtin/ReverbEffect.h"
#include <QDebug>

namespace WaveRider {

ReverbEffect::ReverbEffect(QObject* parent)
    : IDSPEffect(parent)
{
    // Default: subtle room
    m_params.fInGain          = 0.0f;
    m_params.fReverbMix       = -12.0f;
    m_params.fReverbTime      = 800.0f;
    m_params.fHighFreqRTRatio = 0.5f;
}

ReverbEffect::~ReverbEffect()
{
    if (m_applied)
        removeFromStream(m_currentStream);
}

// ── Parameter setters ──────────────────────────────────
void ReverbEffect::setInGain(float dB)
{
    dB = qBound(-96.0f, dB, 0.0f);
    if (qFuzzyCompare(m_params.fInGain, dB)) return;
    m_params.fInGain = dB;
    syncParams();
    emit parametersChanged();
}

void ReverbEffect::setReverbMix(float dB)
{
    dB = qBound(-96.0f, dB, 0.0f);
    if (qFuzzyCompare(m_params.fReverbMix, dB)) return;
    m_params.fReverbMix = dB;
    syncParams();
    emit parametersChanged();
}

void ReverbEffect::setReverbTime(float ms)
{
    ms = qBound(0.001f, ms, 3000.0f);
    if (qFuzzyCompare(m_params.fReverbTime, ms)) return;
    m_params.fReverbTime = ms;
    syncParams();
    emit parametersChanged();
}

void ReverbEffect::setHighFreqRT(float ratio)
{
    ratio = qBound(0.001f, ratio, 0.999f);
    if (qFuzzyCompare(m_params.fHighFreqRTRatio, ratio)) return;
    m_params.fHighFreqRTRatio = ratio;
    syncParams();
    emit parametersChanged();
}

void ReverbEffect::syncParams()
{
    if (m_fxHandle) {
        BASS_FXSetParameters(m_fxHandle, &m_params);
    }
}

// ── Presets ─────────────────────────────────────────────
void ReverbEffect::loadRoom()
{
    m_params.fInGain          = -2.0f;
    m_params.fReverbMix       = -10.0f;
    m_params.fReverbTime      = 400.0f;
    m_params.fHighFreqRTRatio = 0.5f;
    syncParams();
    emit parametersChanged();
}

void ReverbEffect::loadHall()
{
    m_params.fInGain          = 0.0f;
    m_params.fReverbMix       = -8.0f;
    m_params.fReverbTime      = 2000.0f;
    m_params.fHighFreqRTRatio = 0.3f;
    syncParams();
    emit parametersChanged();
}

void ReverbEffect::loadPlate()
{
    m_params.fInGain          = 0.0f;
    m_params.fReverbMix       = -6.0f;
    m_params.fReverbTime      = 1200.0f;
    m_params.fHighFreqRTRatio = 0.7f;
    syncParams();
    emit parametersChanged();
}

void ReverbEffect::loadSpring()
{
    m_params.fInGain          = 0.0f;
    m_params.fReverbMix       = -8.0f;
    m_params.fReverbTime      = 600.0f;
    m_params.fHighFreqRTRatio = 0.1f;
    syncParams();
    emit parametersChanged();
}

// ── IDSPEffect interface ────────────────────────────────
bool ReverbEffect::applyToStream(HSTREAM stream)
{
    if (m_applied)
        removeFromStream(m_currentStream);

    if (!stream || m_bypass) return false;

    m_currentStream = stream;
    m_fxHandle = BASS_ChannelSetFX(stream, BASS_FX_DX8_REVERB, 0);

    if (!m_fxHandle) {
        qWarning() << "ReverbEffect: BASS_ChannelSetFX failed, error:"
                   << BASS_ErrorGetCode();
        return false;
    }

    syncParams();
    m_applied = true;
    return true;
}

void ReverbEffect::removeFromStream(HSTREAM stream)
{
    if (!m_applied) return;

    if (m_fxHandle) {
        BASS_ChannelRemoveFX(stream, m_fxHandle);
        m_fxHandle = 0;
    }
    m_currentStream = 0;
    m_applied = false;
}

void ReverbEffect::setBypass(bool bypass)
{
    if (m_bypass == bypass) return;
    m_bypass = bypass;

    if (!m_currentStream) {
        emit bypassChanged(m_bypass);
        return;
    }

    if (bypass && m_applied) {
        removeFromStream(m_currentStream);
    } else if (!bypass && !m_applied) {
        applyToStream(m_currentStream);
    }

    emit bypassChanged(m_bypass);
}

} // namespace WaveRider
