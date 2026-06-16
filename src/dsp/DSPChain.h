#pragma once

#include <QObject>
#include <QVector>
#include <windows.h>
#include "bass.h"

namespace WaveRider {

class IDSPEffect;

/// Ordered chain of DSP effects applied to an audio stream.
///
/// Owns the effect objects.  When the stream changes (new track),
/// call reapplyAll() to remove effects from the old stream and
/// apply to the new one.
///
/// Usage:
///   DSPChain* chain = new DSPChain(this);
///   chain->addEffect(new EqualizerEffect(chain));
///   chain->setStream(audioEngine->streamHandle());
///   // ... later, on track change:
///   chain->setStream(audioEngine->streamHandle());
///   chain->reapplyAll();
class DSPChain : public QObject {
    Q_OBJECT
public:
    explicit DSPChain(QObject* parent = nullptr);
    ~DSPChain() override;

    // ── Stream binding ────────────────────────────────
    void setStream(HSTREAM stream);
    HSTREAM stream() const { return m_stream; }

    // ── Effect management ─────────────────────────────
    void addEffect(IDSPEffect* effect);
    void removeEffect(IDSPEffect* effect);
    void clear();
    QVector<IDSPEffect*> effects() const { return m_effects; }

    /// Find an effect by its effectId().  Returns nullptr if not found.
    IDSPEffect* findEffect(const QString& effectId) const;

    // ── Operations ─────────────────────────────────────
    /// Remove all effects from old stream and re-apply to current stream.
    /// Call this when a new track starts (stream handle changes).
    void reapplyAll();

    /// Master bypass — when true, all effects are bypassed.
    void setMasterBypass(bool bypass);
    bool isMasterBypass() const { return m_masterBypass; }

signals:
    void chainChanged();

private:
    HSTREAM              m_stream        = 0;
    QVector<IDSPEffect*> m_effects;
    bool                 m_masterBypass  = false;
    bool                 m_applied       = false;
};

} // namespace WaveRider
