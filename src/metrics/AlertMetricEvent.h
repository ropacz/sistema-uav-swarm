#pragma once

#include <string>

#include "omnetpp/cobject.h"
#include "omnetpp/simtime.h"

namespace echosar {

/** Payload efêmero dos sinais consumidos por ExperimentMetrics. */
class AlertMetricEvent : public omnetpp::cObject
{
  public:
    std::string alertId;
    std::string messageId;
    std::string category;
    omnetpp::simtime_t referenceTime;
    double value = 0;
    /// Identidade do alerta, preenchida apenas onde é conhecida: a geração sabe
    /// vítima e drone, a entrega e a confirmação sabem a equipe. Serve ao
    /// registro por alerta, não à agregação.
    std::string victimId;
    std::string droneId;
    std::string teamId;

    AlertMetricEvent(const std::string& alertId = "",
                     omnetpp::simtime_t referenceTime = omnetpp::SimTime::ZERO,
                     const std::string& category = "",
                     double value = 0,
                     const std::string& messageId = "")
        : alertId(alertId), messageId(messageId), category(category),
          referenceTime(referenceTime), value(value)
    {
    }
};

} // namespace echosar
