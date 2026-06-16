#pragma once

#include "dsp/IDSPEffect.h"
#include <windows.h>
#include "bass.h"

namespace WaveRider {

/// Reverb effect using BASS_DX8_REVERB (lightweight).
///
/// Parameters:
///   inGain        — input gain              (-96 … 0 dB,     default 0)
///   reverbMix     — wet/dry mix             (-96 … 0 dB,     default -10)
///   reverbTime    — decay time              (0.001 … 3000 ms, default 1000)
///   highFreqRT    — high-frequency RT ratio  (0.001 … 0.999,  default 0.001)
class ReverbEffect : public IDSPEffect {
    Q_OBJECT

    Q_PROPERTY(float inGain      READ inGain      WRITE setInGain      NOTIFY parametersChanged)
    Q_PROPERTY(float reverbMix   READ reverbMix   WRITE setReverbMix   NOTIFY parametersChanged)
    Q_PROPERTY(float reverbTime  READ reverbTime  WRITE setReverbTime  NOTIFY parametersChanged)
    Q_PROPERTY(float highFreqRT  READ highFreqRT  WRITE setHighFreqRT  NOTIFY parametersChanged)

public:
    explicit ReverbEffect(QObject* parent = nullptr);
    ~ReverbEffect() override;

    // ── IDSPEffect interface ──────────────────────────
    QString effectId()   const override { return "reverb"; }
    QString displayName() const override { return "Reverb"; }

    bool applyToStream(HSTREAM stream) override;
    void removeFromStream(HSTREAM stream) override;
    bool isApplied() const override { return m_applied; }

    void setBypass(bool bypass) override;
    bool isBypassed() const override { return m_bypass; }

    // ── Parameter accessors ────────────────────────────
    float inGain()      const { return m_params.fInGain;          }
    float reverbMix()   const { return m_params.fReverbMix;       }
    float reverbTime()  const { return m_params.fReverbTime;      }
    float highFreqRT()  const { return m_params.fHighFreqRTRatio; }

    void setInGain(float dB);
    void setReverbMix(float dB);
    void setReverbTime(float ms);
    void setHighFreqRT(float ratio);

    // ── Presets ────────────────────────────────────────
    void loadRoom();
    void loadHall();
    void loadPlate();
    void loadSpring();

private:
    void syncParams();

    BASS_DX8_REVERB m_params;
    HFX     m_fxHandle       = 0;
    HSTREAM m_currentStream  = 0;
    bool    m_applied        = false;
    bool    m_bypass         = false;
};

} // namespace WaveRider
