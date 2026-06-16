#pragma once

#include "dsp/IDSPEffect.h"
#include <windows.h>
#include "bass.h"

namespace WaveRider {

/// Dynamics compressor using BASS_DX8_COMPRESSOR.
///
/// Parameters:
///   gain       — output gain after compression  (0 … 60 dB,    default 0)
///   attack     — attack time                     (0.01 … 500 ms, default 10)
///   release    — release time                    (50 … 3000 ms,  default 200)
///   threshold  — level where compression starts  (-60 … 0 dB,   default -20)
///   ratio      — compression ratio               (1 … 100,      default 4)
///   predelay   — look-ahead delay                (0 … 4 ms,     default 4)
class CompressorEffect : public IDSPEffect {
    Q_OBJECT

    Q_PROPERTY(float gain      READ gain      WRITE setGain      NOTIFY parametersChanged)
    Q_PROPERTY(float attack    READ attack    WRITE setAttack    NOTIFY parametersChanged)
    Q_PROPERTY(float release   READ release   WRITE setRelease   NOTIFY parametersChanged)
    Q_PROPERTY(float threshold READ threshold WRITE setThreshold NOTIFY parametersChanged)
    Q_PROPERTY(float ratio     READ ratio     WRITE setRatio     NOTIFY parametersChanged)
    Q_PROPERTY(float predelay  READ predelay  WRITE setPredelay  NOTIFY parametersChanged)

public:
    explicit CompressorEffect(QObject* parent = nullptr);
    ~CompressorEffect() override;

    // ── IDSPEffect interface ──────────────────────────
    QString effectId()   const override { return "comp"; }
    QString displayName() const override { return "Compressor"; }

    bool applyToStream(HSTREAM stream) override;
    void removeFromStream(HSTREAM stream) override;
    bool isApplied() const override { return m_applied; }

    void setBypass(bool bypass) override;
    bool isBypassed() const override { return m_bypass; }

    // ── Parameter accessors ────────────────────────────
    float gain()      const { return m_params.fGain;      }
    float attack()    const { return m_params.fAttack;    }
    float release()   const { return m_params.fRelease;   }
    float threshold() const { return m_params.fThreshold; }
    float ratio()     const { return m_params.fRatio;     }
    float predelay()  const { return m_params.fPredelay;  }

    void setGain(float dB);
    void setAttack(float ms);
    void setRelease(float ms);
    void setThreshold(float dB);
    void setRatio(float r);
    void setPredelay(float ms);

private:
    void syncParams();

    BASS_DX8_COMPRESSOR m_params;
    HFX     m_fxHandle       = 0;
    HSTREAM m_currentStream  = 0;
    bool    m_applied        = false;
    bool    m_bypass         = false;
};

} // namespace WaveRider
