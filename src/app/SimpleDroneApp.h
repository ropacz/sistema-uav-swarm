#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/TeamUpdate_m.h"
#include "messages/DroneStatus_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAck_m.h"
#include "ports.h"

namespace echosar {

struct TeamEntry {
    std::string ip;
    double posX = 0, posY = 0;
    bool available = false;
    omnetpp::simtime_t lastSeen;
};

struct PendingAlert {
    std::string msgId;
    std::string droneId;
    std::string originIp;
    double posX, posY;
    omnetpp::simtime_t sentAt;
    int retries = 0;
};

class SimpleDroneApp : public inet::ApplicationBase,
                       public inet::UdpSocket::ICallback
{
  protected:
    // ── Sockets ──────────────────────────────────────────────────────────────
    inet::UdpSocket teamSocket;   // recebe TeamUpdate         (TEAM_UPDATE_PORT)
    inet::UdpSocket ackSocket;    // envia DroneStatus unicast (sem bind)
    inet::UdpSocket alertSocket;  // envia VictimAlert unicast → equipe (sem bind)
    inet::UdpSocket relaySocket;  // recebe VictimAlert relay de drones  (RELAY_PORT)
    inet::UdpSocket fwdSocket;    // envia VictimAlert relay broadcast   (sem bind)
    inet::UdpSocket ackRxSocket;  // recebe VictimAck da equipe          (ACK_PORT)

    // ── Estado ───────────────────────────────────────────────────────────────
    std::string myDroneId;
    std::string myIp;
    std::map<std::string, TeamEntry> teamTable;
    int victimCounter = 0;

    std::set<std::string>    seenAlerts;    // deduplicação de msgIds recebidos
    std::vector<PendingAlert> pendingAlerts; // store-and-forward de alertas originados

    // ── Timers ───────────────────────────────────────────────────────────────
    omnetpp::cMessage *detectTimer  = nullptr;
    omnetpp::cMessage *timeoutTimer = nullptr;
    omnetpp::cMessage *retryTimer   = nullptr;

    // ── Parâmetros NED ───────────────────────────────────────────────────────
    double victimInterval = 20.0;
    double teamTimeout    = 30.0;
    double retryInterval  = 10.0;
    int    maxRetries     = 5;

    // ── Contadores de métricas ────────────────────────────────────────────
    int alertsGenerated  = 0;  // vítimas detectadas → pendingAlerts criados
    int alertsSentDirect = 0;  // VictimAlert unicast → equipe
    int alertsSentRelay  = 0;  // VictimAlert broadcast relay → drones
    int alertsRelayed    = 0;  // alertas de outro drone recebidos e repassados
    int alertsAcked      = 0;  // VictimAck recebidos (entregas confirmadas)
    int alertsExpired    = 0;  // descartados após maxRetries
    int totalRetries     = 0;  // total de tentativas store-forward
    omnetpp::simtime_t totalE2EDelay = 0;  // soma dos atrasos E2E confirmados

    // ── Ciclo de vida INET ───────────────────────────────────────────────────
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

    // ── Handlers de mensagem ─────────────────────────────────────────────────
    void handleTeamUpdate(inet::Packet *pkt);
    void handleVictimAlertRelay(inet::Packet *pkt);   // passo 8/9
    void handleVictimAck(inet::Packet *pkt);           // passo 12

    // ── Lógica de encaminhamento e retry ─────────────────────────────────────
    void detectVictim();                                // passo 7
    void forwardAlertOnce(const std::string &msgId,
                          const std::string &droneId,
                          const std::string &originIp,
                          double posX, double posY,
                          omnetpp::simtime_t sentAt);   // passos 10/11/9
    void retryPending();                                // passo 15
    void checkTimeouts();                               // passo 13
};

} // namespace echosar
