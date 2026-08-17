#pragma once

#include <set>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/PositionUpdate_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAck_m.h"
#include "ports.h"

namespace echosar {

class TeamApp : public inet::ApplicationBase, public inet::UdpSocket::ICallback
{
  protected:
    inet::UdpSocket socket;
    omnetpp::cMessage *updateTimer = nullptr;
    std::string teamId;
    std::string ipAddress;
    omnetpp::simtime_t updateInterval;
    omnetpp::simtime_t initialJitter;
    bool ackEnabled = true;
    omnetpp::simtime_t ackStartTime;
    int64_t updateSequence = 0;
    std::set<std::string> attendedAlerts;
    std::set<std::string> receivedAttempts;

    int positionUpdatesSent = 0;
    int uniqueAlertsReceived = 0;
    int attemptsReceived = 0;
    int duplicateAlerts = 0;
    int applicationAcksSent = 0;
    omnetpp::simtime_t totalDeliveryDelay = 0;
    omnetpp::simtime_t totalAlertAgeAtReception = 0;
    omnetpp::simsignal_t deliveryDelaySignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t hopCountSignal = SIMSIGNAL_NULL;

    virtual ~TeamApp();
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(omnetpp::cMessage *message) override;
    virtual void finish() override;
    virtual void handleStartOperation(inet::LifecycleOperation *) override {}
    virtual void handleStopOperation(inet::LifecycleOperation *) override {}
    virtual void handleCrashOperation(inet::LifecycleOperation *) override {}
    virtual void socketDataArrived(inet::UdpSocket *, inet::Packet *packet) override;
    virtual void socketErrorArrived(inet::UdpSocket *, inet::Indication *indication) override { delete indication; }
    virtual void socketClosed(inet::UdpSocket *) override {}

    void sendPositionUpdate();
    void handleVictimAlert(inet::Packet *packet);
};

} // namespace echosar
