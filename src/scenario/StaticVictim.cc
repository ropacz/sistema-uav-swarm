#include "omnetpp.h"

#include <string>

namespace echosar {

class StaticVictim : public omnetpp::cSimpleModule
{
  protected:
    void initialize() override
    {
        auto& display = getDisplayString();
        display.setTagArg("p", 0, std::to_string(par("positionX").doubleValueInUnit("m")).c_str());
        display.setTagArg("p", 1, std::to_string(par("positionY").doubleValueInUnit("m")).c_str());
    }

    void handleMessage(omnetpp::cMessage *message) override { delete message; }
};

Define_Module(StaticVictim);

} // namespace echosar
