#pragma once

#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/TeamUpdate_m.h"
#include "messages/DroneStatus_m.h"
#include "messages/VictimAlert_m.h"
#include "ports.h"

namespace echosar {

class SimpleTeamApp : public inet::ApplicationBase,
                      public inet::UdpSocket::ICallback
{
  protected:
    inet::UdpSocket sendSocket;   // envia TeamUpdate   (broadcast, sem bind)
    inet::UdpSocket statusSocket; // recebe DroneStatus (port DRONE_STATUS_PORT)
    inet::UdpSocket alertSocket;  // recebe VictimAlert (port ALERT_PORT)

    omnetpp::cMessage *sendTimer = nullptr;
    std::string myTeamId;
    std::string myIp;
    double sendInterval = 5.0;

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

    void sendUpdate();
};

} // namespace echosar
