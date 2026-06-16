#include "dsp/builtin/CompressorEffect.h"
#include <QDebug>

namespace WaveRider {

CompressorEffect::CompressorEffect(QObject* parent)
    : IDSPEffect(parent)
{
    // Sensible defaults
    m_params.fGain      = 0.0f;
    m_params.fAttack    = 10.0f;
    m_params.fRelease   = 200.0f;
    m_params.fThreshold = -20.0f;
    m_params.fRatio     = 4.0f;
    m_params.fPredelay  = 4.0f;
}

CompressorEffect::~CompressorEffect()
{
    if (m_applied)
        removeFromStream(m_currentStream);
}

// ── Parameter setters ──────────────────────────────────
void CompressorEffect::setGain(float dB)
{
    dB = qBound(0.0f, dB, 60.0f);
    if (qFuzzyCompare(m_params.fGain, dB)) return;
    m_params.fGain = dB;
    syncParams();
    emit parametersChanged();
}

void CompressorEffect::setAttack(float ms)
{
    ms = qBound(0.01f, ms, 500.0f);
    if (qFuzzyCompare(m_params.fAttack, ms)) return;
    m_params.fAttack = ms;
    syncParams();
    emit parametersChanged();
}

void CompressorEffect::setRelease(float ms)
{
    ms = qBound(50.0f, ms, 3000.0f);
    if (qFuzzyCompare(m_params.fRelease, ms)) return;
    m_params.fRelease = ms;
    syncParams();
    emit parametersChanged();
}

void CompressorEffect::setThreshold(float dB)
{
    dB = qBound(-60.0f, dB, 0.0f);
    if (qFuzzyCompare(m_params.fThreshold, dB)) return;
    m_params.fThreshold = dB;
    syncParams();
    emit parametersChanged();
}

void CompressorEffect::setRatio(float r)
{
    r = qBound(1.0f, r, 100.0f);
    if (qFuzzyCompare(m_params.fRatio, r)) return;
    m_params.fRatio = r;
    syncParams();
    emit parametersChanged();
}

void CompressorEffect::setPredelay(float ms)
{
    ms = qBound(0.0f, ms, 4.0f);
    if (qFuzzyCompare(m_params.fPredelay, ms)) return;
    m_params.fPredelay = ms;
    syncParams();
    emit parametersChanged();
}

void CompressorEffect::syncParams()
{
    if (m_fxHandle) {
        BASS_FXSetParameters(m_fxHandle, &m_params);
    }
}

// ── IDSPEffect interface ────────────────────────────────
bool CompressorEffect::applyToStream(HSTREAM stream)
{
    if (m_applied)
        removeFromStream(m_currentStream);

    if (!stream || m_bypass) return false;

    m_currentStream = stream;
    m_fxHandle = BASS_ChannelSetFX(stream, BASS_FX_DX8_COMPRESSOR, 0);

    if (!m_fxHandle) {
        qWarning() << "CompressorEffect: BASS_ChannelSetFX failed, error:"
                   << BASS_ErrorGetCode();
        return false;
    }

    syncParams();
    m_applied = true;
    return true;
}

void CompressorEffect::removeFromStream(HSTREAM stream)
{
    if (!m_applied) return;

    if (m_fxHandle) {
        BASS_ChannelRemoveFX(stream, m_fxHandle);
        m_fxHandle = 0;
    }
    m_currentStream = 0;
    m_applied = false;
}

void CompressorEffect::setBypass(bool bypass)
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
