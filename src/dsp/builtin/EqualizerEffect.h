#pragma once

#include "dsp/IDSPEffect.h"
#include <QVector>
#include <windows.h>
#include "bass.h"

namespace WaveRider {

/// 10-band parametric equalizer using BASS_DX8_PARAMEQ.
///
/// Each band is a separate peaking EQ filter applied to the stream.
/// Center frequencies follow the standard ISO 10-band layout:
///   32, 64, 125, 250, 500, 1K, 2K, 4K, 8K, 16K  Hz
///
/// Gain range: -15 dB  …  +15 dB  (default: 0 dB per band)
/// Bandwidth:  1.0 octave (12 semitones), fixed per band.
class EqualizerEffect : public IDSPEffect {
    Q_OBJECT

    // Per-band gain properties so QSS / property bindings work
    Q_PROPERTY(float band0  READ band0  WRITE setBand0  NOTIFY parametersChanged)
    Q_PROPERTY(float band1  READ band1  WRITE setBand1  NOTIFY parametersChanged)
    Q_PROPERTY(float band2  READ band2  WRITE setBand2  NOTIFY parametersChanged)
    Q_PROPERTY(float band3  READ band3  WRITE setBand3  NOTIFY parametersChanged)
    Q_PROPERTY(float band4  READ band4  WRITE setBand4  NOTIFY parametersChanged)
    Q_PROPERTY(float band5  READ band5  WRITE setBand5  NOTIFY parametersChanged)
    Q_PROPERTY(float band6  READ band6  WRITE setBand6  NOTIFY parametersChanged)
    Q_PROPERTY(float band7  READ band7  WRITE setBand7  NOTIFY parametersChanged)
    Q_PROPERTY(float band8  READ band8  WRITE setBand8  NOTIFY parametersChanged)
    Q_PROPERTY(float band9  READ band9  WRITE setBand9  NOTIFY parametersChanged)

public:
    static constexpr int  kNumBands      = 10;
    static constexpr float kMinGain      = -15.0f;
    static constexpr float kMaxGain      =  15.0f;
    static constexpr float kBandwidth    =  12.0f;   // semitones (1 octave)

    explicit EqualizerEffect(QObject* parent = nullptr);
    ~EqualizerEffect() override;

    // ── IDSPEffect interface ──────────────────────────
    QString effectId()   const override { return "eq"; }
    QString displayName() const override { return "Equalizer"; }

    bool applyToStream(HSTREAM stream) override;
    void removeFromStream(HSTREAM stream) override;
    bool isApplied() const override { return m_applied; }

    void setBypass(bool bypass) override;
    bool isBypassed() const override { return m_bypass; }

    // ── Band access ────────────────────────────────────
    /// Get center frequency for a band index (0-9).
    static float centerFreq(int band);
    static const float* centerFreqs();

    /// Set gain for a specific band (dB, clamped to [-15, +15]).
    void setBandGain(int band, float dB);

    /// Get gain for a specific band.
    float bandGain(int band) const;

    /// Reset all bands to 0 dB (flat).
    void resetToFlat();

    // ── Presets ────────────────────────────────────────
    enum Preset { Flat, Rock, Pop, Jazz, Classical, Vocal, BassBoost, TrebleBoost };
    void loadPreset(Preset preset);
    static const char* presetName(Preset preset);

    // ── Convenience per-band getters/setters ───────────
#define BAND_PROP(n) \
    float band##n() const { return bandGain(n); } \
    void setBand##n(float dB) { setBandGain(n, dB); }
    BAND_PROP(0) BAND_PROP(1) BAND_PROP(2) BAND_PROP(3) BAND_PROP(4)
    BAND_PROP(5) BAND_PROP(6) BAND_PROP(7) BAND_PROP(8) BAND_PROP(9)
#undef BAND_PROP

private:
    void createFxHandles(HSTREAM stream);
    void destroyFxHandles(HSTREAM stream);
    void updateBandFx(int band);

    float  m_gains[kNumBands];
    HFX    m_fxHandles[kNumBands];
    HSTREAM m_currentStream = 0;
    bool   m_applied = false;
    bool   m_bypass  = false;
};

} // namespace WaveRider
