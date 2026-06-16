#pragma once

#include <QObject>
#include <QString>
#include <windows.h>
#include "bass.h"

namespace WaveRider {

/// Abstract base for all DSP effects (built-in or plugin).
///
/// Each effect wraps one or more BASS FX handles (HFX) applied to an audio
/// stream via BASS_ChannelSetFX / BASS_ChannelRemoveFX.
///
/// Subclasses override applyToStream / removeFromStream to manage their
/// native FX handles.  DSPChain calls these when the stream changes.
class IDSPEffect : public QObject {
    Q_OBJECT
public:
    explicit IDSPEffect(QObject* parent = nullptr) : QObject(parent) {}
    ~IDSPEffect() override = default;

    /// Unique machine-readable ID (e.g. "eq", "comp", "reverb").
    virtual QString effectId() const = 0;

    /// Human-readable name for UI display (e.g. "Equalizer").
    virtual QString displayName() const = 0;

    /// Apply this effect to a BASS stream.  Called when a track starts
    /// or when the effect is added to the chain mid-playback.
    /// @return true on success.
    virtual bool applyToStream(HSTREAM stream) = 0;

    /// Remove this effect from a BASS stream.  Called when the effect is
    /// removed from the chain or before re-applying to a new stream.
    virtual void removeFromStream(HSTREAM stream) = 0;

    /// Whether this effect is currently applied to the given stream.
    virtual bool isApplied() const = 0;

    // ── Bypass ────────────────────────────────────────
    virtual void setBypass(bool bypass) = 0;
    virtual bool isBypassed() const = 0;

signals:
    void bypassChanged(bool bypassed);
    void parametersChanged();
};

} // namespace WaveRider
