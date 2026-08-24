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
    std::string category;
    omnetpp::simtime_t referenceTime;
    double value = 0;

    AlertMetricEvent(const std::string& alertId = "",
                     omnetpp::simtime_t referenceTime = omnetpp::SimTime::ZERO,
                     const std::string& category = "",
                     double value = 0)
        : alertId(alertId), category(category), referenceTime(referenceTime),
          value(value)
    {
    }
};

} // namespace echosar
