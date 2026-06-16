#include "dsp/DSPChain.h"
#include "dsp/IDSPEffect.h"
#include <QDebug>

namespace WaveRider {

DSPChain::DSPChain(QObject* parent)
    : QObject(parent)
{
}

DSPChain::~DSPChain()
{
    // Remove all effects from the current stream before destruction
    for (auto* fx : m_effects) {
        if (fx->isApplied()) {
            fx->removeFromStream(m_stream);
        }
    }
    qDeleteAll(m_effects);
    m_effects.clear();
}

void DSPChain::setStream(HSTREAM stream)
{
    // If the stream changed and effects are applied, remove from old first
    if (m_stream && m_stream != stream && m_applied) {
        for (auto* fx : m_effects) {
            if (fx->isApplied()) {
                fx->removeFromStream(m_stream);
            }
        }
        m_applied = false;
    }
    m_stream = stream;
}

void DSPChain::addEffect(IDSPEffect* effect)
{
    if (!effect || m_effects.contains(effect)) return;

    effect->setParent(this);
    m_effects.append(effect);

    // Apply immediately if we have an active stream
    if (m_stream && !m_masterBypass) {
        effect->applyToStream(m_stream);
    }

    connect(effect, &IDSPEffect::parametersChanged, this, &DSPChain::chainChanged);
    connect(effect, &IDSPEffect::bypassChanged, this, &DSPChain::chainChanged);

    emit chainChanged();
}

void DSPChain::removeEffect(IDSPEffect* effect)
{
    if (!effect) return;

    int idx = m_effects.indexOf(effect);
    if (idx < 0) return;

    if (effect->isApplied()) {
        effect->removeFromStream(m_stream);
    }

    disconnect(effect, nullptr, this, nullptr);
    m_effects.removeAt(idx);
    effect->deleteLater();

    emit chainChanged();
}

void DSPChain::clear()
{
    for (auto* fx : m_effects) {
        if (fx->isApplied()) {
            fx->removeFromStream(m_stream);
        }
        fx->deleteLater();
    }
    m_effects.clear();
    m_applied = false;
    emit chainChanged();
}

IDSPEffect* DSPChain::findEffect(const QString& effectId) const
{
    for (auto* fx : m_effects) {
        if (fx->effectId() == effectId)
            return fx;
    }
    return nullptr;
}

void DSPChain::reapplyAll()
{
    if (!m_stream) return;

    // Remove existing FX handles
    for (auto* fx : m_effects) {
        if (fx->isApplied()) {
            fx->removeFromStream(m_stream);
        }
    }
    m_applied = false;

    // Re-apply if not globally bypassed
    if (m_masterBypass) return;

    for (auto* fx : m_effects) {
        if (!fx->isBypassed()) {
            fx->applyToStream(m_stream);
        }
    }
    m_applied = true;
}

void DSPChain::setMasterBypass(bool bypass)
{
    if (m_masterBypass == bypass) return;
    m_masterBypass = bypass;

    if (!m_stream) return;

    if (bypass) {
        // Remove all effects
        for (auto* fx : m_effects) {
            if (fx->isApplied()) {
                fx->removeFromStream(m_stream);
            }
        }
        m_applied = false;
    } else {
        // Re-apply non-bypassed effects
        for (auto* fx : m_effects) {
            if (!fx->isBypassed()) {
                fx->applyToStream(m_stream);
            }
        }
        m_applied = true;
    }

    emit chainChanged();
}

} // namespace WaveRider
