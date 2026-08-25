#include "DroneApp.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "mobility/BaGaussMarkovMobility.h"
#include "metrics/AlertMetricEvent.h"
#include "sensing/AbstractObstacleSensor.h"

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(DroneApp);

namespace {
/// Valida uma condição nomeando o parâmetro que a violou. A verificação
/// anterior era uma única expressão com quase cinquenta condições encadeadas,
/// e a mensagem de erro não dizia qual delas falhou.
void require(bool satisfied, const char *requirement)
{
    if (!satisfied)
        throw cRuntimeError("Invalid parameter: %s", requirement);
}
}

void DroneApp::validateParameters() const
{
    const auto& f = fitnessParameters;
    const auto& b = batParameters;

    require(!droneId.empty(), "droneId must not be empty");
    require(appPort > 0 && appPort <= 65535, "appPort must be a valid UDP port");
    require(victimAlertPayloadBytes > 0, "victimAlertPayloadBytes must be positive");
    require(applicationIpTtl > 0 && applicationIpTtl <= 255, "applicationIpTtl must be 1..255");

    require(retryInterval > 0, "retryInterval must be positive");
    require(ackTimeout > 0, "ackTimeout must be positive");
    require(ackTimeout <= retryInterval, "ackTimeout must not exceed retryInterval");
    require(maxAttempts > 0, "maxAttempts must be positive");
    require(alertTtl >= retryInterval, "alertTtl must be at least one retryInterval");

    require(teamEntryLifetime > 0, "teamEntryLifetime must be positive");
    require(maintenanceInterval > 0, "maintenanceInterval must be positive");
    require(repositionAfterUnackedAttempts > 0,
            "repositionAfterUnackedAttempts must be positive");
    require(repositionAfterUnackedAttempts < maxAttempts,
            "repositionAfterUnackedAttempts must leave one post-movement attempt");

    require(f.maximumRepositionDistance > 0, "maximumRepositionDistance must be positive");
    require(f.minimumAltitude <= f.maximumAltitude,
            "minimumAltitude must not exceed maximumAltitude");
    require(f.areaMinX < f.areaMaxX, "areaMinX must be below areaMaxX");
    require(f.areaMinY < f.areaMaxY, "areaMinY must be below areaMaxY");
    require(f.horizontalSpeed > 0, "horizontalSpeed must be positive");
    require(f.climbSpeed > 0, "climbSpeed must be positive");
    require(f.descentSpeed > 0, "descentSpeed must be positive");
    require(f.flightTimeLimit > 0, "flightTimeLimit must be positive");

    require(f.obstacleSigma > 0, "obstacleSigma must be positive");
    require(f.obstacleSafetyMargin >= 0, "obstacleSafetyMargin must not be negative");
    require(f.linkNormalizationDistance > 0, "linkNormalizationDistance must be positive");
    require(f.wLink >= 0 && f.wObstacle >= 0 && f.wMove >= 0,
            "fitness weights must not be negative");
    require(std::abs(f.wLink + f.wObstacle + f.wMove - 1) <= 1e-9,
            "fitness weights must sum to 1");

    require(b.populationSize > 0, "batPopulation must be positive");
    require(b.iterations > 0, "batIterations must be positive");
    require(b.initializationAttempts > 0, "batInitializationAttempts must be positive");
    require(b.frequencyMin >= 0, "batFrequencyMin must not be negative");
    require(b.frequencyMax >= b.frequencyMin, "batFrequencyMax must not be below batFrequencyMin");
    require(b.initialAmplitude > 0 && b.initialAmplitude <= 1, "batInitialAmplitude must be 0..1");
    require(b.initialPulseRate >= 0 && b.initialPulseRate <= 1, "batInitialPulseRate must be 0..1");
    require(b.amplitudeDecay > 0 && b.amplitudeDecay <= 1, "batAmplitudeDecay must be 0..1");
    require(b.pulseGrowth > 0, "batPulseGrowth must be positive");
    require(b.localSearchScale > 0 && b.localSearchScale <= 1, "batLocalSearchScale must be 0..1");
}

void DroneApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        // Lê e valida toda a configuração antes de criar sockets e timers.
        droneId = par("droneId").stdstringValue();
        if (droneId.empty())
            droneId = getParentModule()->getFullName();
        appPort = par("appPort");
        victimAlertPayloadBytes = par("victimAlertPayloadBytes");
        retryInterval = par("retryInterval");
        ackTimeout = par("ackTimeout");
        alertTtl = par("alertTtl");
        maxAttempts = par("maxAttempts");
        repositionAfterUnackedAttempts = par("repositionAfterUnackedAttempts");
        teamEntryLifetime = par("teamEntryLifetime");
        maintenanceInterval = par("maintenanceInterval");
        baEnabled = par("baEnabled");
        fitnessParameters.maximumRepositionDistance =
            par("maximumRepositionDistance").doubleValueInUnit("m");
        fitnessParameters.minimumAltitude = par("minimumAltitude").doubleValueInUnit("m");
        fitnessParameters.maximumAltitude = par("maximumAltitude").doubleValueInUnit("m");
        fitnessParameters.areaMinX = par("areaMinX").doubleValueInUnit("m");
        fitnessParameters.areaMaxX = par("areaMaxX").doubleValueInUnit("m");
        fitnessParameters.areaMinY = par("areaMinY").doubleValueInUnit("m");
        fitnessParameters.areaMaxY = par("areaMaxY").doubleValueInUnit("m");
        fitnessParameters.horizontalSpeed = par("horizontalSpeed").doubleValueInUnit("mps");
        fitnessParameters.climbSpeed = par("climbSpeed").doubleValueInUnit("mps");
        fitnessParameters.descentSpeed = par("descentSpeed").doubleValueInUnit("mps");
        fitnessParameters.flightTimeLimit = par("flightTimeLimit");
        applicationIpTtl = par("applicationIpTtl");
        fitnessParameters.wLink = par("wLink");
        fitnessParameters.wObstacle = par("wObstacle");
        fitnessParameters.wMove = par("wMove");
        fitnessParameters.obstacleSigma = par("obstacleSigma").doubleValueInUnit("m");
        fitnessParameters.obstacleSafetyMargin = par("obstacleSafetyMargin").doubleValueInUnit("m");
        fitnessParameters.linkNormalizationDistance =
            par("linkNormalizationDistance").doubleValueInUnit("m");
        batParameters.populationSize = par("batPopulation");
        batParameters.iterations = par("batIterations");
        batParameters.initializationAttempts = par("batInitializationAttempts");
        batParameters.frequencyMin = par("batFrequencyMin");
        batParameters.frequencyMax = par("batFrequencyMax");
        batParameters.initialAmplitude = par("batInitialAmplitude");
        batParameters.initialPulseRate = par("batInitialPulseRate");
        batParameters.amplitudeDecay = par("batAmplitudeDecay");
        batParameters.pulseGrowth = par("batPulseGrowth");
        batParameters.localSearchScale = par("batLocalSearchScale");
        validateParameters();
        alertGeneratedSignal = registerSignal("victimAlertGenerated");
        alertAttemptSentSignal = registerSignal("victimAlertAttemptSent");
        alertConfirmedSignal = registerSignal("victimAlertConfirmed");
        alertExpiredSignal = registerSignal("victimAlertExpired");
        repositionTriggerSignal = registerSignal("victimRepositionTriggered");
        sensorEvaluationSignal = registerSignal("victimSensorEvaluated");
        baActivationSignal = registerSignal("victimBaActivated");
        repositionEventSignal = registerSignal("victimRepositionEvent");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // A tabela de equipes começa vazia. IP, posição e presença são
        // aprendidos exclusivamente de PositionUpdates recebidos por broadcast.
        socket.setOutputGate(gate("socketOut"));
        socket.setCallback(this);
        socket.setBroadcast(true);
        socket.bind(appPort);
        socket.setTimeToLive(applicationIpTtl);
        maintenanceTimer = new cMessage("alertMaintenance");
        movementCompleteTimer = new cMessage("movementComplete");
        scheduleAt(simTime() + maintenanceInterval, maintenanceTimer);
    }
}

void DroneApp::handleMessageWhenUp(cMessage *message)
{
    // Um único timer coordena expiração, timeouts de ACK e novas tentativas.
    if (message == maintenanceTimer) {
        performMaintenance();
        scheduleAt(simTime() + maintenanceInterval, maintenanceTimer);
    }
    else if (message == movementCompleteTimer) {
        PendingVictimAlert *completedAlert = nullptr;
        for (auto& [id, alert] : pendingAlerts) {
            if (reposition.owns(id)) {
                recordActualRepositionDistance(alert);
                AlertMetricEvent completedEvent(alert.alertId, simTime(), "completed");
                emit(repositionEventSignal, &completedEvent);
                completedAlert = &alert;
                break;
            }
        }
        if (!completedAlert)
            throw cRuntimeError("Movement completed without an active alert");

        // Envia ainda na posição escolhida e só depois retoma o Gauss-Markov.
        reposition.release();
        if (simTime() - completedAlert->generationTime < alertTtl &&
            completedAlert->attempts < maxAttempts)
            sendAttempt(*completedAlert);
        resumeMobility();
    }
    else if (message->arrivedOn("assignmentIn"))
        handleAssignment(check_and_cast<VictimAssignment *>(message));
    else if (socket.belongsToSocket(message))
        socket.processMessage(message);
    else
        delete message;
}

void DroneApp::socketDataArrived(UdpSocket *, Packet *packet)
{
    if (!strcmp(packet->getName(), "PositionUpdate"))
        handlePositionUpdate(packet);
    else if (!strcmp(packet->getName(), "VictimAck"))
        handleVictimAck(packet);
    else
        delete packet;
}

void DroneApp::handleAssignment(VictimAssignment *assignment)
{
    std::string alertId = assignment->getAlertId();
    // O alertId identifica o evento único, independentemente das tentativas.
    if (pendingAlerts.count(alertId)) {
        delete assignment;
        return;
    }
    PendingVictimAlert alert;
    alert.alertId = alertId;
    alert.victimId = assignment->getVictimId();
    alert.victimPosition = Coord(assignment->getVictimPositionX(),
                                 assignment->getVictimPositionY(),
                                 assignment->getVictimPositionZ());
    alert.generationTime = assignment->getDetectionTimestamp();
    alert.nextAttempt = simTime();
    pendingAlerts[alertId] = alert;
    AlertMetricEvent generatedEvent(alertId, alert.generationTime);
    emit(alertGeneratedSignal, &generatedEvent);
    delete assignment;
    sendAttempt(pendingAlerts.at(alertId));
}

void DroneApp::handlePositionUpdate(Packet *packet)
{
    auto update = packet->peekAtFront<PositionUpdateChunk>();
    std::string senderId = update->getSenderId();
    if (senderId.empty()) {
        delete packet;
        return;
    }
    auto sourceAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    auto [teamIt, inserted] = discoveredTeams.try_emplace(senderId);
    auto& team = teamIt->second;
    (void)inserted;
    team.ipAddress = sourceAddress.str();
    Coord receivedPosition(update->getPositionX(), update->getPositionY(), update->getPositionZ());
    if (update->getSequenceNumber() > team.lastSequence) {
        team.position = receivedPosition;
        team.lastSequence = update->getSequenceNumber();
        // Duplicatas e pacotes reordenados não renovam a entrada descoberta.
        team.lastSeen = simTime();
    }
    delete packet;
}

std::string DroneApp::selectTargetTeam() const
{
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord current = mobility->getCurrentPosition();
    std::string selected;
    double best = INFINITY;
    for (const auto& [id, team] : discoveredTeams) {
        if (team.ipAddress.empty()) continue;
        if (team.lastSeen < SIMTIME_ZERO) continue;
        double distance = current.distance(team.position);
        // O identificador resolve empates de forma reprodutível.
        if (selected.empty() || distance < best || (distance == best && id < selected)) {
            selected = id;
            best = distance;
        }
    }
    return selected;
}

void DroneApp::sendAttempt(PendingVictimAlert& alert)
{
    if (alert.attempts >= maxAttempts)
        return;
    alert.targetTeamId = selectTargetTeam();
    if (alert.targetTeamId.empty()) {
        alert.nextAttempt = simTime() + retryInterval;
        return;
    }
    auto& team = discoveredTeams.at(alert.targetTeamId);
    alert.attempts++;
    std::string messageId = alert.alertId + "-attempt-" + std::to_string(alert.attempts);
    alert.attemptTeamIds[messageId] = alert.targetTeamId;
    alert.attemptTeamAddresses[messageId] = team.ipAddress;
    auto message = makeShared<VictimAlertChunk>();
    message->setChunkLength(B(victimAlertPayloadBytes));
    message->setAlertId(alert.alertId.c_str());
    message->setMessageId(messageId.c_str());
    message->setVictimId(alert.victimId.c_str());
    message->setOriginDroneId(droneId.c_str());
    message->setVictimPositionX(alert.victimPosition.x);
    message->setVictimPositionY(alert.victimPosition.y);
    message->setVictimPositionZ(alert.victimPosition.z);
    message->setAttemptNumber(alert.attempts);
    message->setGenerationTimestamp(alert.generationTime);
    message->setTimeToLive(alertTtl);
    socket.sendTo(new Packet("VictimAlert", message),
                  Ipv4Address(team.ipAddress.c_str()), appPort);
    alert.ackDeadline = simTime() + ackTimeout;
    alert.nextAttempt = simTime() + retryInterval;
    AlertMetricEvent attemptEvent(alert.alertId);
    emit(alertAttemptSentSignal, &attemptEvent);
}

void DroneApp::performMaintenance()
{
    for (auto it = discoveredTeams.begin(); it != discoveredTeams.end(); ) {
        auto& team = it->second;
        if (team.lastSeen >= SIMTIME_ZERO &&
            simTime() - team.lastSeen >= teamEntryLifetime)
            it = discoveredTeams.erase(it);
        else
            ++it;
    }

    for (auto it = pendingAlerts.begin(); it != pendingAlerts.end(); ) {
        auto& alert = it->second;
        // TTL e limite de tentativas encerram alertas sem confirmação.
        if (simTime() - alert.generationTime >= alertTtl ||
            (alert.attempts >= maxAttempts && simTime() >= alert.nextAttempt)) {
            AlertMetricEvent expiredEvent(alert.alertId);
            emit(alertExpiredSignal, &expiredEvent);
            if (reposition.owns(alert.alertId)) {
                recordActualRepositionDistance(alert);
                reposition.release();
                if (movementCompleteTimer->isScheduled())
                    cancelEvent(movementCompleteTimer);
                resumeMobility();
            }
            it = pendingAlerts.erase(it);
            continue;
        }
        if (alert.ackDeadline >= SIMTIME_ZERO && simTime() >= alert.ackDeadline) {
            // O alerta continua pendente, portanto todas as tentativas feitas
            // até aqui são consecutivas sem ACK válido.
            alert.ackDeadline = -1;
            if (alert.attempts >= repositionAfterUnackedAttempts &&
                !alert.repositionDecisionMade) {
                alert.repositionDecisionMade = true;
                AlertMetricEvent triggerEvent(alert.alertId);
                emit(repositionTriggerSignal, &triggerEvent);
                tryReposition(alert);
            }
        }
        // O alerta que comanda o movimento espera a tentativa imediata da chegada.
        if (simTime() >= alert.nextAttempt && alert.attempts < maxAttempts &&
            !(reposition.moving() && reposition.owns(alert.alertId)))
            sendAttempt(alert);
        ++it;
    }
}

void DroneApp::tryReposition(PendingVictimAlert& alert)
{
    auto teamIt = discoveredTeams.find(alert.targetTeamId);
    if (teamIt == discoveredTeams.end() || teamIt->second.lastSeen < SIMTIME_ZERO) {
        EV_DEBUG << "Reposition skipped: target team is no longer known\n";
        return;
    }
    if (!reposition.idle()) {
        EV_DEBUG << "Reposition skipped: drone is already moving for another alert\n";
        return;
    }
    auto sensor = check_and_cast<AbstractObstacleSensor *>(getParentModule()->getSubmodule("obstacleSensor"));
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord current = mobility->getCurrentPosition();
    Coord teamPosition = teamIt->second.position;
    ObstacleObservation observation = sensor->inspect(current, teamPosition);
    AlertMetricEvent sensorEvent(alert.alertId, simTime(),
                                 observation.confirmed ? "detected" : "notDetected");
    emit(sensorEvaluationSignal, &sensorEvent);
    if (!observation.confirmed) {
        EV_DEBUG << "Reposition skipped: obstacle not detected ("
                 << observation.reason << ")\n";
        return;
    }
    if (!baEnabled)
        return;

    AlertMetricEvent baEvent(alert.alertId);
    emit(baActivationSignal, &baEvent);
    RepositionFitness fitness(fitnessParameters, sensor, current,
                              teamPosition, observation.nearestSurfacePoint, simTime());
    BatResult result = BatAlgorithm::optimize(
        current, fitnessParameters.maximumRepositionDistance,
        batParameters, getRNG(0),
        [&](const Coord& candidate) { return fitness.cost(candidate); },
        [&](const Coord& candidate) { return fitness.feasible(candidate); });
    EV_INFO << "BA result: current=" << current << " target=" << teamPosition
            << " candidate=" << result.position << " fitness=" << result.fitness
            << " valid=" << result.valid << "\n";
    if (!result.valid) {
        EV_DEBUG << "Reposition skipped: BA found no feasible solution\n";
        return;
    }
    auto controlled = dynamic_cast<BaGaussMarkovMobility *>(mobility);
    if (!controlled) {
        throw cRuntimeError("BA reposition requires BaGaussMarkovMobility");
    }
    double distance = current.distance(result.position);
    if (distance <= 1e-6) {
        EV_DEBUG << "Reposition skipped: BA candidate equals current position\n";
        return;
    }
    alert.repositionOrigin = current;
    alert.repositionDistanceRecorded = false;
    double travelTime = fitness.travelTime(current, result.position);
    // A mobilidade executa o trajeto no tempo calculado; não há teletransporte.
    controlled->moveTo(result.position, fitnessParameters.horizontalSpeed,
                       fitnessParameters.climbSpeed, fitnessParameters.descentSpeed);
    reposition.begin(alert.alertId);
    AlertMetricEvent startedEvent(alert.alertId, simTime(), "started", distance);
    emit(repositionEventSignal, &startedEvent);
    scheduleAt(simTime() + travelTime, movementCompleteTimer);
}

void DroneApp::recordActualRepositionDistance(PendingVictimAlert& alert)
{
    if (alert.repositionDistanceRecorded)
        return;
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    double distance = alert.repositionOrigin.distance(mobility->getCurrentPosition());
    AlertMetricEvent distanceEvent(alert.alertId, simTime(), "distance", distance);
    emit(repositionEventSignal, &distanceEvent);
    alert.repositionDistanceRecorded = true;
}

void DroneApp::handleVictimAck(Packet *packet)
{
    auto ack = packet->peekAtFront<VictimAckChunk>();
    auto it = pendingAlerts.find(ack->getAlertId());
    if (it != pendingAlerts.end()) {
        auto& alert = it->second;
        auto teamIt = alert.attemptTeamIds.find(ack->getReceivedMessageId());
        auto addressIt = alert.attemptTeamAddresses.find(ack->getReceivedMessageId());
        auto source = packet->findTag<L3AddressInd>();
        bool valid = ack->getOriginDroneId() == droneId && ack->getVictimId() == alert.victimId &&
            teamIt != alert.attemptTeamIds.end() &&
            addressIt != alert.attemptTeamAddresses.end() && source &&
            ack->getTeamId() == teamIt->second &&
            source->getSrcAddress().str() == addressIt->second;
        // A identidade vem do destino histórico da tentativa. Assim, um ACK
        // legítimo continua válido após expirar a descoberta da equipe, enquanto
        // um teamId que não corresponde ao endereço de origem é rejeitado.
        if (!valid) {
            delete packet;
            return;
        }
        AlertMetricEvent confirmedEvent(alert.alertId);
        emit(alertConfirmedSignal, &confirmedEvent);
        bool ownsReposition = reposition.owns(it->first);
        if (ownsReposition)
            recordActualRepositionDistance(alert);
        if (ownsReposition) {
            reposition.release();
            if (movementCompleteTimer->isScheduled())
                cancelEvent(movementCompleteTimer);
        }
        pendingAlerts.erase(it);
        if (ownsReposition)
            resumeMobility();
    }
    delete packet;
}

void DroneApp::resumeMobility()
{
    auto mobility = dynamic_cast<BaGaussMarkovMobility *>(getParentModule()->getSubmodule("mobility"));
    if (mobility) mobility->resumeNormal();
}

DroneApp::~DroneApp()
{
    cancelAndDelete(maintenanceTimer);
    cancelAndDelete(movementCompleteTimer);
}

} // namespace echosar
