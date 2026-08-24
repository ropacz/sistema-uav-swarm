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
    omnetpp::simtime_t secondaryTime;
    double value = 0;

    AlertMetricEvent(const std::string& alertId = "",
                     const std::string& messageId = "",
                     omnetpp::simtime_t referenceTime = omnetpp::SimTime::ZERO,
                     omnetpp::simtime_t secondaryTime = omnetpp::SimTime::ZERO,
                     const std::string& category = "",
                     double value = 0)
        : alertId(alertId), messageId(messageId), category(category),
          referenceTime(referenceTime), secondaryTime(secondaryTime), value(value)
    {
    }
};

} // namespace echosar
