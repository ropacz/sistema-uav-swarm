#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/environment/contract/IPhysicalEnvironment.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/TeamUpdate_m.h"
#include "messages/DroneStatus_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAck_m.h"
#include "messages/ProbeAck_m.h"
#include "ports.h"
#include "control/LinkMonitor.h"
#include "control/BatOptimizer.h"

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
    std::set<std::string> triedTeams;
};

class SimpleDroneApp : public inet::ApplicationBase,
                       public inet::UdpSocket::ICallback
{
  protected:
    // ── Sockets ──────────────────────────────────────────────────────────────
    inet::UdpSocket teamSocket;     // recebe TeamUpdate         (TEAM_UPDATE_PORT)
    inet::UdpSocket ackSocket;      // envia DroneStatus unicast
    inet::UdpSocket alertSocket;    // envia VictimAlert unicast → equipe
    inet::UdpSocket relaySocket;    // recebe VictimAlert relay de drones  (RELAY_PORT)
    inet::UdpSocket fwdSocket;      // envia VictimAlert relay broadcast
    inet::UdpSocket ackRxSocket;    // recebe VictimAck da equipe  (ACK_PORT)
    inet::UdpSocket probeRxSocket;  // recebe ProbeAck da equipe  (PROBE_ACK_PORT) [BA]

    // ── Estado ───────────────────────────────────────────────────────────────
    std::string myDroneId;
    std::string myIp;
    std::map<std::string, TeamEntry> teamTable;
    int victimCounter = 0;

    std::set<std::string>    seenAlerts;
    std::vector<PendingAlert> pendingAlerts;

    // ── Timers ───────────────────────────────────────────────────────────────
    omnetpp::cMessage *detectTimer      = nullptr;
    omnetpp::cMessage *timeoutTimer     = nullptr;
    omnetpp::cMessage *retryTimer       = nullptr;
    omnetpp::cMessage *repositionTimer  = nullptr;  // [BA]
    omnetpp::cMessage *probeTimer       = nullptr;  // [BA]

    // ── Parâmetros NED (missão) ───────────────────────────────────────────────
    double victimInterval = 40.0;
    double teamTimeout    = 30.0;
    double retryInterval  = 10.0;
    int    maxRetries     = 5;

    // ── Parâmetros NED (BA) ───────────────────────────────────────────────────
    bool   enableBatReposition  = false;
    int    windowSize           = 20;
    int    maxLosses            = 2;
    double rssiRef              = -30.0;    // RSSI(d0) em dBm — calibrar
    double refDistance          = 1.0;      // d0 em m
    double pathLossExp          = 2.0;      // η
    double rssiMargin_dB        = 6.0;      // Δ em dB
    double probeTimeout_s       = 5.0;
    int    maxRepositionTries   = 3;
    BatParams batParams_;                   // ver NED para defaults

    // ── Estado BA ────────────────────────────────────────────────────────────
    enum class DroneState { CRUISE, REPOSITIONING, VALIDATING };
    DroneState droneState = DroneState::CRUISE;

    std::map<std::string, LinkMonitor>  linkMonitors;
    std::string  activeTeamId;
    inet::Coord  activeTeamPos;
    inet::Coord  activeObsPos;
    inet::Coord  activeTargetPos;
    std::vector<inet::Coord> tabuPoints;
    int          repositionTries = 0;
    omnetpp::simtime_t gammaTriggeredAt;

    inet::physicalenvironment::IPhysicalEnvironment *physEnv   = nullptr;
    BatOptimizer *batOptimizer = nullptr;

    // ── Contadores de métricas (missão) ──────────────────────────────────────
    int alertsGenerated  = 0;
    int alertsSentDirect = 0;
    int alertsSentRelay  = 0;
    int alertsRelayed    = 0;
    int alertsAcked      = 0;
    int alertsExpired    = 0;
    int totalRetries     = 0;
    omnetpp::simtime_t totalE2EDelay = 0;

    // ── Contadores de métricas (BA) ───────────────────────────────────────────
    int    gammaTriggers          = 0;
    int    repositionsAttempted   = 0;
    int    repositionsSucceeded   = 0;
    int    sensorAbortsNoObstacle = 0;
    int    tabuActivations        = 0;
    double totalRepositionDistance = 0.0;
    omnetpp::simtime_t totalRecoveryTime = 0;

    // ── Ciclo de vida INET ───────────────────────────────────────────────────
    virtual ~SimpleDroneApp();
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

    // ── Handlers de mensagem (missão) ────────────────────────────────────────
    void handleTeamUpdate(inet::Packet *pkt);
    void handleVictimAlertRelay(inet::Packet *pkt);
    void handleVictimAck(inet::Packet *pkt);

    // ── Lógica de encaminhamento ──────────────────────────────────────────────
    void detectVictim();
    std::string forwardAlertOnce(const std::string &msgId,
                                 const std::string &droneId,
                                 const std::string &originIp,
                                 double posX, double posY,
                                 omnetpp::simtime_t sentAt,
                                 const std::set<std::string> &exclude = {});
    void retryPending();
    void checkTimeouts();

    // ── Lógica BA ────────────────────────────────────────────────────────────
    void handleProbeAck(inet::Packet *pkt);
    void checkGamma(const std::string& teamId, double teamX, double teamY);
    void startOptimize();
    void onRepositionTimer();
    void onProbeTimer();
    void sendProbeToActiveTeam();
};

} // namespace echosar
