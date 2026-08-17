#pragma once

#include <deque>
#include <map>
#include <set>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/PositionUpdate_m.h"
#include "messages/VictimAck_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAssignment_m.h"
#include "optimization/BatAlgorithm.h"
#include "ports.h"

namespace echosar {

struct LinkSample {
    int64_t sequence = 0;
    omnetpp::simtime_t receptionTime;
    double rssiDbm = NAN;
};

struct TeamLinkState {
    std::string id;
    std::string ipAddress;
    inet::Coord position;
    omnetpp::simtime_t lastSeen = -1;
    std::deque<LinkSample> samples;
};

struct PendingVictimAlert {
    std::string alertId;
    std::string victimId;
    inet::Coord victimPosition;
    omnetpp::simtime_t generationTime;
    omnetpp::simtime_t ackDeadline = -1;
    omnetpp::simtime_t nextAttempt = -1;
    int attempts = 0;
    int64_t sequence = 0;
    std::string lastMessageId;
    std::string targetTeamId;
    std::map<std::string, omnetpp::simtime_t> attemptSentTimes;
    bool degradationEvaluated = false;
    int baCycles = 0;
    std::set<std::string> testedPositions;
    std::string validationMessageId;
    omnetpp::simtime_t repositionStart = -1;
    double preRepositionPdr = NAN;
    double preRepositionRssi = NAN;
};

class DroneApp : public inet::ApplicationBase, public inet::UdpSocket::ICallback
{
  protected:
    enum class RepositionState { IDLE, MOVING, AWAITING_VALIDATION };

    inet::UdpSocket socket;
    omnetpp::cMessage *maintenanceTimer = nullptr;
    omnetpp::cMessage *movementCompleteTimer = nullptr;
    std::string droneId;
    std::string ipAddress;
    std::map<std::string, TeamLinkState> teams;
    std::map<std::string, PendingVictimAlert> pendingAlerts;
    std::string activeRepositionAlertId;
    RepositionState repositionState = RepositionState::IDLE;

    omnetpp::simtime_t retryInterval;
    omnetpp::simtime_t ackTimeout;
    omnetpp::simtime_t alertTtl;
    omnetpp::simtime_t linkWindow;
    omnetpp::simtime_t teamSilenceTimeout;
    omnetpp::simtime_t maintenanceInterval;
    int maxAttempts = 5;
    int maxBaCycles = 2;
    int applicationIpTtl = 32;
    double pdrThreshold = 0.8;
    double rssiThresholdDbm = -80;
    bool baEnabled = true;
    double maximumRepositionDistance = 25;
    double minimumAltitude = 6;
    double maximumAltitude = 20;
    double areaMinX = 0, areaMaxX = 1000, areaMinY = 0, areaMaxY = 1000;
    double horizontalSpeed = 13, climbSpeed = 5, descentSpeed = 3;
    omnetpp::simtime_t flightTimeLimit;
    double wLink = 0.6, wObstacle = 0.25, wMove = 0.15;
    double obstacleSigma = 10;
    double obstacleSafetyMargin = 1;
    double linkNormalizationDistance = 1000;
    BatParameters batParameters;

    int uniqueAlertsGenerated = 0;
    int alertAttemptsSent = 0;
    int uniqueAlertsAcked = 0;
    int alertsExpired = 0;
    int degradationIndications = 0;
    int sensorConfirmations = 0;
    int sensorRejections = 0;
    int baActivations = 0;
    int successfulRepositions = 0;
    int failedRepositions = 0;
    double baDistance = 0;
    omnetpp::simtime_t totalRtt = 0;
    omnetpp::simtime_t totalRecoveryTime = 0;
    int recoverySamples = 0;
    int repositionValidationSamples = 0;
    double preRepositionPdrSum = 0;
    double postRepositionPdrSum = 0;
    double preRepositionRssiSum = 0;
    double postRepositionRssiSum = 0;
    int preRepositionRssiSamples = 0;
    int postRepositionRssiSamples = 0;

    omnetpp::simsignal_t rssiSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t pdrSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t repositionDistanceSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t recoveryTimeSignal = SIMSIGNAL_NULL;

    virtual ~DroneApp();
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

    void handleAssignment(VictimAssignment *assignment);
    void handlePositionUpdate(inet::Packet *packet);
    void handleVictimAck(inet::Packet *packet);
    void performMaintenance();
    void sendAttempt(PendingVictimAlert& alert);
    std::string selectTargetTeam() const;
    bool detectDegradation(const PendingVictimAlert& alert, double& pdr, double& rssi) const;
    void tryReposition(PendingVictimAlert& alert, double prePdr, double preRssi);
    double computeFitness(const inet::Coord& candidate, const inet::Coord& current,
                          const TeamLinkState& team, const inet::Coord& obstaclePoint) const;
    bool isFeasible(const inet::Coord& candidate, const inet::Coord& current,
                    const inet::Coord& obstaclePoint) const;
    void resumeMobility();
};

} // namespace echosar
