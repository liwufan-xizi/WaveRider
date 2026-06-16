#include "dsp/builtin/EqualizerEffect.h"
#include <QDebug>
#include <cstring>

namespace WaveRider {

// ── Standard 10-band ISO frequencies ─────────────────────
static const float kCenterFreqs[] = {
    32.0f, 64.0f, 125.0f, 250.0f, 500.0f,
    1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
};

// ── Preset gain arrays (dB per band) ─────────────────────
//       32   64  125  250  500   1K   2K   4K   8K  16K
static const float kPresetFlat[]       = {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 };
static const float kPresetRock[]       = {  4,  3,  0, -2, -3, -2,  1,  3,  4,  4 };
static const float kPresetPop[]        = { -1,  2,  3,  1, -1, -2, -1,  1,  2,  2 };
static const float kPresetJazz[]       = {  3,  2,  1,  0, -1, -2, -1,  0,  1,  2 };
static const float kPresetClassical[]  = {  2,  1,  0,  0, -2, -3, -2,  0,  1,  2 };
static const float kPresetVocal[]      = { -2, -2, -1,  2,  4,  3,  2,  1,  0,  0 };
static const float kPresetBassBoost[]  = {  6,  5,  3,  0,  0,  0,  0,  0,  0,  0 };
static const float kPresetTrebleBoost[]= {  0,  0,  0,  0,  0,  1,  3,  5,  6,  6 };

static const float* const kPresetTables[] = {
    kPresetFlat, kPresetRock, kPresetPop, kPresetJazz,
    kPresetClassical, kPresetVocal, kPresetBassBoost, kPresetTrebleBoost
};

static const char* const kPresetNames[] = {
    "Flat", "Rock", "Pop", "Jazz", "Classical", "Vocal", "Bass Boost", "Treble Boost"
};

// ============================================================
EqualizerEffect::EqualizerEffect(QObject* parent)
    : IDSPEffect(parent)
{
    resetToFlat();
}

EqualizerEffect::~EqualizerEffect()
{
    if (m_applied) {
        removeFromStream(m_currentStream);
    }
}

// ── Static helpers ──────────────────────────────────────
float EqualizerEffect::centerFreq(int band)
{
    if (band < 0 || band >= kNumBands) return 1000.0f;
    return kCenterFreqs[band];
}

const float* EqualizerEffect::centerFreqs()
{
    return kCenterFreqs;
}

// ── Per-band control ────────────────────────────────────
void EqualizerEffect::setBandGain(int band, float dB)
{
    if (band < 0 || band >= kNumBands) return;

    dB = qBound(kMinGain, dB, kMaxGain);
    if (qFuzzyCompare(m_gains[band], dB)) return;

    m_gains[band] = dB;

    // Update the FX parameter live if applied
    if (m_applied && m_fxHandles[band]) {
        updateBandFx(band);
    }

    emit parametersChanged();
}

float EqualizerEffect::bandGain(int band) const
{
    if (band < 0 || band >= kNumBands) return 0.0f;
    return m_gains[band];
}

void EqualizerEffect::resetToFlat()
{
    for (int i = 0; i < kNumBands; ++i) {
        m_gains[i] = 0.0f;
    }
    // Update live FX if applied
    if (m_applied) {
        for (int i = 0; i < kNumBands; ++i) {
            if (m_fxHandles[i]) updateBandFx(i);
        }
    }
    emit parametersChanged();
}

// ── Presets ─────────────────────────────────────────────
void EqualizerEffect::loadPreset(Preset preset)
{
    if (preset < 0 || preset > TrebleBoost) return;
    const float* table = kPresetTables[preset];
    for (int i = 0; i < kNumBands; ++i) {
        m_gains[i] = table[i];
    }
    if (m_applied) {
        for (int i = 0; i < kNumBands; ++i) {
            if (m_fxHandles[i]) updateBandFx(i);
        }
    }
    emit parametersChanged();
}

const char* EqualizerEffect::presetName(Preset preset)
{
    if (preset < 0 || preset > TrebleBoost) return "Unknown";
    return kPresetNames[preset];
}

// ── FX handle management ────────────────────────────────
void EqualizerEffect::createFxHandles(HSTREAM stream)
{
    for (int i = 0; i < kNumBands; ++i) {
        m_fxHandles[i] = BASS_ChannelSetFX(stream, BASS_FX_DX8_PARAMEQ, i);

        if (m_fxHandles[i]) {
            updateBandFx(i);
        } else {
            int err = BASS_ErrorGetCode();
            qWarning() << "EqualizerEffect: BASS_ChannelSetFX failed for band"
                       << i << "(" << kCenterFreqs[i] << "Hz), error:" << err;
        }
    }
}

void EqualizerEffect::destroyFxHandles(HSTREAM stream)
{
    for (int i = 0; i < kNumBands; ++i) {
        if (m_fxHandles[i]) {
            BASS_ChannelRemoveFX(stream, m_fxHandles[i]);
            m_fxHandles[i] = 0;
        }
    }
}

void EqualizerEffect::updateBandFx(int band)
{
    BASS_DX8_PARAMEQ params;
    params.fCenter    = kCenterFreqs[band];
    params.fBandwidth = kBandwidth;
    params.fGain      = m_gains[band];

    BASS_FXSetParameters(m_fxHandles[band], &params);
}

// ── IDSPEffect interface ────────────────────────────────
bool EqualizerEffect::applyToStream(HSTREAM stream)
{
    if (m_applied) {
        // Already applied — remove first to be safe
        destroyFxHandles(m_currentStream);
        m_applied = false;
    }

    if (!stream || m_bypass) return false;

    m_currentStream = stream;
    createFxHandles(stream);
    m_applied = true;
    return true;
}

void EqualizerEffect::removeFromStream(HSTREAM stream)
{
    if (!m_applied) return;

    destroyFxHandles(stream);
    m_currentStream = 0;
    m_applied = false;
}

void EqualizerEffect::setBypass(bool bypass)
{
    if (m_bypass == bypass) return;
    m_bypass = bypass;

    if (!m_currentStream) {
        emit bypassChanged(m_bypass);
        return;
    }

    if (bypass && m_applied) {
        destroyFxHandles(m_currentStream);
        m_applied = false;
    } else if (!bypass && !m_applied) {
        createFxHandles(m_currentStream);
        m_applied = true;
    }

    emit bypassChanged(m_bypass);
}

} // namespace WaveRider
