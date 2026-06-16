#include "core/SignalBus.h"

namespace WaveRider {

SignalBus::SignalBus(QObject* parent)
    : QObject(parent)
{
}

SignalBus* SignalBus::instance()
{
    static SignalBus bus;
    return &bus;
}

} // namespace WaveRider
