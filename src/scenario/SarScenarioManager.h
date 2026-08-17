#pragma once

#include <map>
#include <string>

#include "omnetpp.h"

namespace echosar {

class SarScenarioManager : public omnetpp::cSimpleModule
{
  protected:
    std::map<omnetpp::cMessage *, omnetpp::cModule *> detections;
    virtual ~SarScenarioManager();
    virtual int numInitStages() const override;
    virtual void initialize(int stage) override;
    virtual void handleMessage(omnetpp::cMessage *message) override;
};

} // namespace echosar
