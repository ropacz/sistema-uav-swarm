#pragma once

#include <set>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/TeamUpdate_m.h"
#include "messages/DroneStatus_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAck_m.h"
#include "ports.h"

namespace echosar {

class SimpleTeamApp : public inet::ApplicationBase,
                      public inet::UdpSocket::ICallback
{
  protected:
    // ── Sockets ──────────────────────────────────────────────────────────────
    inet::UdpSocket sendSocket;    // envia TeamUpdate broadcast   (sem bind)
    inet::UdpSocket statusSocket;  // recebe DroneStatus           (DRONE_STATUS_PORT)
    inet::UdpSocket alertSocket;   // recebe VictimAlert           (ALERT_PORT)
    inet::UdpSocket ackTxSocket;   // envia VictimAck unicast      (sem bind)  passo 12

    // ── Estado ───────────────────────────────────────────────────────────────
    omnetpp::cMessage *sendTimer   = nullptr;
    std::string myTeamId;
    std::string myIp;
    double sendInterval  = 5.0;
    double beaconJitter  = 5.0;

    std::set<std::string> seenAlerts;  // deduplicação de VictimAlerts recebidos

    // ── Contadores de métricas ────────────────────────────────────────────
    int alertsReceived          = 0;
    int teamUpdatesSent         = 0;
    int droneStatusReceived     = 0;
    omnetpp::simtime_t totalDeliveryDelay = 0;
    omnetpp::cOutVector deliveryDelayVec;

    // ── Ciclo de vida INET ───────────────────────────────────────────────────
    virtual ~SimpleTeamApp();
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
                                    inet::Indication *ind) override { delete ind; }
    virtual void socketClosed(inet::UdpSocket *) override {}

    void sendUpdate();
};

} // namespace echosar
