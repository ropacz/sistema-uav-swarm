#include "omnetpp.h"

namespace echosar {

class StaticVictim : public omnetpp::cSimpleModule
{
  protected:
    void handleMessage(omnetpp::cMessage *message) override { delete message; }
};

Define_Module(StaticVictim);

} // namespace echosar
