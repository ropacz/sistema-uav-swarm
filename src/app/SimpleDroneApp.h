#pragma once

#include <map>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/TeamUpdate_m.h"
#include "messages/DroneStatus_m.h"
#include "messages/VictimAlert_m.h"
#include "ports.h"

namespace echosar {

struct TeamEntry {
    std::string ip;
    double lat = 0, lon = 0;
    bool available = false;
    omnetpp::simtime_t lastSeen;
};

class SimpleDroneApp : public inet::ApplicationBase,
                       public inet::UdpSocket::ICallback
{
  protected:
    inet::UdpSocket teamSocket;   // recebe TeamUpdate  (port TEAM_UPDATE_PORT)
    inet::UdpSocket ackSocket;    // envia DroneStatus  (sem bind)
    inet::UdpSocket alertSocket;  // envia VictimAlert  (sem bind, unicast)

    std::string myDroneId;
    std::map<std::string, TeamEntry> teamTable;
    int victimCounter = 0;

    omnetpp::cMessage *detectTimer  = nullptr;
    omnetpp::cMessage *timeoutTimer = nullptr;

    double victimInterval = 20.0;
    double teamTimeout    = 15.0;

    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(omnetpp::cMessage *msg) override;
    virtual void finish() override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    virtual void handleStartOperation(inet::LifecycleOperation *) override {}
    virtual void handleStopOperation(inet::LifecycleOperation *) override {}
    virtual void handleCrashOperation(inet::LifecycleOperation *) override {}

    virtual void socketDataArrived(inet::UdpSocket *socket,
                                   inet::Packet *pkt) override;
    virtual void socketErrorArrived(inet::UdpSocket *,
                                    inet::Indication *) override {}
    virtual void socketClosed(inet::UdpSocket *) override {}

    void handleTeamUpdate(inet::Packet *pkt);
    void detectVictim();
    void checkTimeouts();
};

} // namespace echosar
