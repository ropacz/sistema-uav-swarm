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

/// Distingue o repasse de TeamUpdate agendado pelo próprio drone dos pacotes
/// que chegam do socket.
static constexpr short TEAM_UPDATE_RELAY_KIND = 0x5401;

namespace {
/// Valida uma condição nomeando o parâmetro que a violou.
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
    require(droneStatusPayloadBytes > 0, "droneStatusPayloadBytes must be positive");
    require(applicationIpTtl > 0 && applicationIpTtl <= 255, "applicationIpTtl must be 1..255");

    require(retryInterval > 0, "retryInterval must be positive");
    require(ackTimeout > 0, "ackTimeout must be positive");
    require(ackTimeout <= retryInterval, "ackTimeout must not exceed retryInterval");
    require(maxAttempts > 0, "maxAttempts must be positive");
    require(alertTtl >= retryInterval, "alertTtl must be at least one retryInterval");
    require(alertInterval > alertTtl,
            "alertInterval must exceed alertTtl to prevent overlapping alerts");

    require(teamEntryLifetime > 0, "teamEntryLifetime must be positive");
    require(lastKnownTeamRetention >= teamEntryLifetime,
            "lastKnownTeamRetention must cover at least teamEntryLifetime");
    require(teamUpdateMaxHops >= 0, "teamUpdateMaxHops must not be negative");
    require(teamUpdateForwardJitter >= 0,
            "teamUpdateForwardJitter must not be negative");
    require(droneStatusInterval > 0, "droneStatusInterval must be positive");
    require(droneStatusInitialOffset >= 0 &&
            droneStatusInitialOffset < droneStatusInterval,
            "droneStatusInitialOffset must be within one status interval");
    require(droneEntryLifetime >= droneStatusInterval,
            "droneEntryLifetime must cover at least one droneStatusInterval");
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
    require(f.communicationRange > 0, "communicationRange must be positive");
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
            droneId = getParentModule()->getFullPath();
        appPort = par("appPort");
        victimAlertPayloadBytes = par("victimAlertPayloadBytes");
        droneStatusPayloadBytes = par("droneStatusPayloadBytes");
        droneStatusInterval = par("droneStatusInterval");
        droneStatusInitialOffset = par("droneStatusInitialOffset");
        droneEntryLifetime = par("droneEntryLifetime");
        retryInterval = par("retryInterval");
        ackTimeout = par("ackTimeout");
        alertTtl = par("alertTtl");
        alertInterval = par("alertInterval");
        maxAttempts = par("maxAttempts");
        repositionAfterUnackedAttempts = par("repositionAfterUnackedAttempts");
        teamEntryLifetime = par("teamEntryLifetime");
        lastKnownTeamRetention = par("lastKnownTeamRetention");
        teamUpdateMaxHops = par("teamUpdateMaxHops");
        teamUpdateForwardJitter = par("teamUpdateForwardJitter");
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
        fitnessParameters.communicationRange =
            par("communicationRange").doubleValueInUnit("m");
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
        operationalFailureSignal = registerSignal("victimAlertOperationalFailure");
        repositionTriggerSignal = registerSignal("victimRepositionTriggered");
        sensorEvaluationSignal = registerSignal("victimSensorEvaluated");
        baActivationSignal = registerSignal("victimBaActivated");
        repositionEventSignal = registerSignal("victimRepositionEvent");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // A tabela de equipes começa vazia. IP, posição e presença são
        // aprendidos exclusivamente de TeamUpdates recebidos por broadcast.
        socket.setOutputGate(gate("socketOut"));
        socket.setCallback(this);
        socket.setBroadcast(true);
        socket.bind(appPort);
        socket.setTimeToLive(applicationIpTtl);
        maintenanceTimer = new cMessage("alertMaintenance");
        movementCompleteTimer = new cMessage("movementComplete");
        droneStatusTimer = new cMessage("droneStatusTimer");
        scheduleAt(simTime() + maintenanceInterval, maintenanceTimer);
        scheduleAt(simTime() + droneStatusInitialOffset, droneStatusTimer);
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
                recordCompletedRepositionDistance(alert);
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
        if (simTime() - completedAlert->creationTime < alertTtl &&
            completedAlert->attempts < maxAttempts)
            sendAttempt(*completedAlert);
        resumeMobility();
    }
    else if (message == droneStatusTimer) {
        sendDroneStatus();
        scheduleAt(simTime() + droneStatusInterval, droneStatusTimer);
    }
    else if (message->isSelfMessage() && message->getKind() == TEAM_UPDATE_RELAY_KIND) {
        auto forwarded = check_and_cast<Packet *>(message);
        pendingTeamUpdateRelays.erase(forwarded);
        socket.sendTo(forwarded, Ipv4Address::ALLONES_ADDRESS, appPort);
    }
    else if (message->arrivedOn("assignmentIn"))
        handleAssignment(check_and_cast<VictimAssignment *>(message));
    else if (socket.belongsToSocket(message))
        socket.processMessage(message);
    else
        delete message;
}

namespace {
/// O nome do pacote carrega "<Tipo>:<alertId>" para VictimAlert/VictimAck, de
/// modo que o event log do OMNeT++ (.elog) mostre a identidade do alerta sem
/// exigir um dissector de payload. É metadado do simulador — TeamApp.cc tem a
/// mesma função porque cada arquivo despacha seus próprios tipos.
bool hasTypePrefix(const char *name, const char *type)
{
    size_t length = strlen(type);
    return strncmp(name, type, length) == 0 && name[length] == ':';
}
}

void DroneApp::socketDataArrived(UdpSocket *, Packet *packet)
{
    if (!strcmp(packet->getName(), "TeamUpdate"))
        handleTeamUpdate(packet);
    else if (!strcmp(packet->getName(), "DroneStatus"))
        handleDroneStatus(packet);
    else if (hasTypePrefix(packet->getName(), "VictimAck"))
        handleVictimAck(packet);
    else
        delete packet;
}

void DroneApp::handleAssignment(VictimAssignment *assignment)
{
    std::string victimId = assignment->getVictimId();
    if (victimId.empty() || assignment->getDetectionTimestamp() > simTime()) {
        delete assignment;
        return;
    }
    auto [it, inserted] = activeVictims.try_emplace(victimId);
    if (!inserted) {
        delete assignment;
        return;
    }
    auto& victim = it->second;
    victim.victimId = victimId;
    victim.position = Coord(assignment->getVictimPositionX(),
                            assignment->getVictimPositionY(),
                            assignment->getVictimPositionZ());
    delete assignment;
    startAlertCycle(victim);
}

void DroneApp::startAlertCycle(ActiveVictim& victim)
{
    if (!victim.pendingAlertId.empty())
        throw cRuntimeError("Victim '%s' already has pending alert '%s'",
                            victim.victimId.c_str(), victim.pendingAlertId.c_str());
    PendingVictimAlert alert;
    alert.victimId = victim.victimId;
    alert.victimPosition = victim.position;
    alert.creationTime = simTime();
    alert.nextAttempt = simTime();
    alert.alertId = droneId + "-" + victim.victimId + "-alert-" +
        std::to_string(++victim.alertSequence);
    victim.pendingAlertId = alert.alertId;
    pendingAlerts.emplace(alert.alertId, alert);
    AlertMetricEvent generatedEvent(alert.alertId, alert.creationTime);
    generatedEvent.victimId = alert.victimId;
    generatedEvent.droneId = droneId;
    emit(alertGeneratedSignal, &generatedEvent);
    sendAttempt(pendingAlerts.at(alert.alertId));
}

void DroneApp::completeAlertCycle(const PendingVictimAlert& alert)
{
    auto victimIt = activeVictims.find(alert.victimId);
    if (victimIt == activeVictims.end() ||
        victimIt->second.pendingAlertId != alert.alertId)
        throw cRuntimeError("Alert '%s' is not owned by its active victim",
                            alert.alertId.c_str());
    victimIt->second.pendingAlertId.clear();
    victimIt->second.nextAlertTime = alert.creationTime + alertInterval;
}

void DroneApp::handleTeamUpdate(Packet *packet)
{
    auto update = packet->peekAtFront<TeamUpdateChunk>();
    std::string senderId = update->getTeamId();
    std::string messageId = update->getMessageId();
    if (senderId.empty() || messageId.empty() || update->getSequenceNumber() <= 0 ||
        update->getTimestamp() > simTime() || update->getHopCount() < 0) {
        delete packet;
        return;
    }
    // No primeiro salto o endereço vem do pacote recebido, como exige a
    // descoberta pela rede. Os repasses carregam esse mesmo endereço, para que
    // um drone distante possa endereçar o alerta em unicast e deixar o AODV
    // encontrar a rota.
    std::string teamAddress = update->getTeamAddress();
    if (teamAddress.empty()) {
        auto source = packet->findTag<L3AddressInd>();
        if (!source) {
            delete packet;
            return;
        }
        teamAddress = source->getSrcAddress().str();
    }
    auto [teamIt, inserted] = discoveredTeams.try_emplace(senderId);
    auto& team = teamIt->second;
    (void)inserted;
    Coord receivedPosition(update->getPositionX(), update->getPositionY(), update->getPositionZ());
    // Duplicatas e pacotes reordenados não renovam a entrada nem são repassados.
    // Como o repasse acontece só nesta condição, cada drone encaminha no máximo
    // uma vez por (teamId, sequenceNumber).
    if (update->getSequenceNumber() > team.lastSequence) {
        team.ipAddress = teamAddress;
        team.position = receivedPosition;
        team.lastSequence = update->getSequenceNumber();
        team.lastUpdateTime = update->getTimestamp();
        team.stale = false;
        everKnewTeam = true;
        if (update->getHopCount() < teamUpdateMaxHops)
            scheduleTeamUpdateRelay(*update, teamAddress);
    }
    delete packet;
}

void DroneApp::scheduleTeamUpdateRelay(const TeamUpdateChunk& original,
                                       const std::string& teamAddress)
{
    auto relay = makeShared<TeamUpdateChunk>();
    relay->setChunkLength(original.getChunkLength());
    relay->setTeamId(original.getTeamId());
    relay->setMessageId(original.getMessageId());
    relay->setSequenceNumber(original.getSequenceNumber());
    relay->setPositionX(original.getPositionX());
    relay->setPositionY(original.getPositionY());
    relay->setPositionZ(original.getPositionZ());
    relay->setTimestamp(original.getTimestamp());
    relay->setTeamAddress(teamAddress.c_str());
    relay->setHopCount(original.getHopCount() + 1);
    auto forwarded = new Packet("TeamUpdate", relay);
    forwarded->setKind(TEAM_UPDATE_RELAY_KIND);
    pendingTeamUpdateRelays.insert(forwarded);
    // O jitter dispersa os repasses simultâneos dos drones que ouviram o mesmo
    // broadcast e reduz colisão no acesso ao meio.
    simtime_t delay = teamUpdateForwardJitter > SIMTIME_ZERO
        ? uniform(0, teamUpdateForwardJitter.dbl()) : SIMTIME_ZERO;
    scheduleAt(simTime() + delay, forwarded);
}

void DroneApp::handleDroneStatus(Packet *packet)
{
    auto status = packet->peekAtFront<DroneStatusChunk>();
    std::string senderId = status->getDroneId();
    std::string messageId = status->getMessageId();
    if (senderId.empty() || senderId == droneId || messageId.empty() ||
        status->getSequenceNumber() <= 0 || status->getTimestamp() > simTime()) {
        delete packet;
        return;
    }
    auto [droneIt, inserted] = discoveredDrones.try_emplace(senderId);
    auto& drone = droneIt->second;
    (void)inserted;
    if (status->getSequenceNumber() > drone.lastSequence) {
        drone.position = Coord(status->getPositionX(), status->getPositionY(),
                               status->getPositionZ());
        drone.lastSequence = status->getSequenceNumber();
        drone.lastUpdateTime = status->getTimestamp();
        droneStatusUpdatesAccepted++;
    }
    delete packet;
}

void DroneApp::sendDroneStatus()
{
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord position = mobility->getCurrentPosition();
    auto status = makeShared<DroneStatusChunk>();
    status->setChunkLength(B(droneStatusPayloadBytes));
    status->setDroneId(droneId.c_str());
    status->setMessageId((droneId + "-status-" +
                          std::to_string(droneStatusSequence + 1)).c_str());
    status->setSequenceNumber(++droneStatusSequence);
    status->setPositionX(position.x);
    status->setPositionY(position.y);
    status->setPositionZ(position.z);
    status->setTimestamp(simTime());
    socket.sendTo(new Packet("DroneStatus", status),
                  Ipv4Address::ALLONES_ADDRESS, appPort);
}

std::string DroneApp::selectTargetTeam() const
{
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord current = mobility->getCurrentPosition();
    std::string selected;
    double best = INFINITY;
    for (const auto& [id, team] : discoveredTeams) {
        if (team.ipAddress.empty()) continue;
        if (team.lastUpdateTime < SIMTIME_ZERO) continue;
        // Uma posição retida está desatualizada: não endereça nova tentativa.
        if (team.stale) continue;
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
        // Nunca ter conhecido uma equipe e ter perdido a que se conhecia são
        // falhas operacionais distintas: só a segunda pode acionar recuperação.
        const char *category = everKnewTeam ? "expiredKnownTeam" : "neverKnownTeam";
        AlertMetricEvent failureEvent(alert.alertId, simTime(), category);
        emit(operationalFailureSignal, &failureEvent);
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
    message->setSourceDroneId(droneId.c_str());
    message->setTargetTeamId(alert.targetTeamId.c_str());
    message->setVictimPositionX(alert.victimPosition.x);
    message->setVictimPositionY(alert.victimPosition.y);
    message->setVictimPositionZ(alert.victimPosition.z);
    message->setCreationTime(alert.creationTime);
    message->setAttemptNumber(alert.attempts);
    message->setTimeToLive(alertTtl);
    socket.sendTo(new Packet(("VictimAlert:" + alert.alertId).c_str(), message),
                  Ipv4Address(team.ipAddress.c_str()), appPort);
    alert.ackDeadline = simTime() + ackTimeout;
    alert.nextAttempt = simTime() + retryInterval;
    AlertMetricEvent attemptEvent(alert.alertId, simTime(), "", 0, messageId);
    emit(alertAttemptSentSignal, &attemptEvent);
}

void DroneApp::performMaintenance()
{
    expireDiscoveredEntries();

    for (auto it = pendingAlerts.begin(); it != pendingAlerts.end(); ) {
        auto& alert = it->second;
        // TTL e limite de tentativas encerram alertas sem confirmação.
        if (simTime() - alert.creationTime >= alertTtl ||
            (alert.attempts >= maxAttempts && simTime() >= alert.nextAttempt)) {
            AlertMetricEvent expiredEvent(alert.alertId);
            emit(alertExpiredSignal, &expiredEvent);
            // Houve equipe conhecida e o alerta chegou a ser transmitido, mas
            // nenhuma tentativa foi confirmada. É a falha que o mecanismo de
            // reposicionamento existe para atacar.
            if (alert.attempts > 0 && !alert.targetTeamId.empty()) {
                AlertMetricEvent noAckEvent(alert.alertId, simTime(), "knownTeamNoAck");
                emit(operationalFailureSignal, &noAckEvent);
            }
            if (reposition.owns(alert.alertId)) {
                reposition.release();
                if (movementCompleteTimer->isScheduled())
                    cancelEvent(movementCompleteTimer);
                resumeMobility();
            }
            completeAlertCycle(alert);
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

    for (auto& entry : activeVictims) {
        auto& victim = entry.second;
        if (victim.pendingAlertId.empty() && simTime() >= victim.nextAlertTime)
            startAlertCycle(victim);
    }
}

void DroneApp::expireDiscoveredEntries()
{
    for (auto it = discoveredTeams.begin(); it != discoveredTeams.end(); ) {
        auto& team = it->second;
        if (team.lastUpdateTime < SIMTIME_ZERO) {
            ++it;
            continue;
        }
        simtime_t age = simTime() - team.lastUpdateTime;
        // A entrada operacional expira em teamEntryLifetime, mas a última
        // posição continua retida até lastKnownTeamRetention. Enquanto está
        // retida serve apenas ao mecanismo de recuperação, nunca à seleção de
        // destino de uma nova tentativa.
        if (age >= lastKnownTeamRetention)
            it = discoveredTeams.erase(it);
        else {
            team.stale = age >= teamEntryLifetime;
            ++it;
        }
    }

    for (auto it = discoveredDrones.begin(); it != discoveredDrones.end(); ) {
        if (it->second.lastUpdateTime >= SIMTIME_ZERO &&
            simTime() - it->second.lastUpdateTime >= droneEntryLifetime)
            it = discoveredDrones.erase(it);
        else
            ++it;
    }
}

void DroneApp::tryReposition(PendingVictimAlert& alert)
{
    // O BA nunca é acionado por uma equipe que jamais foi conhecida. Uma equipe
    // que era conhecida e cuja entrada expirou continua elegível enquanto sua
    // última posição estiver retida: é exatamente o caso que a recuperação trata.
    auto teamIt = discoveredTeams.find(alert.targetTeamId);
    if (teamIt == discoveredTeams.end() || teamIt->second.lastUpdateTime < SIMTIME_ZERO) {
        EV_DEBUG << "Reposition skipped: target team is no longer retained\n";
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
    std::vector<Coord> neighborPositions;
    neighborPositions.reserve(discoveredDrones.size());
    for (const auto& entry : discoveredDrones)
        neighborPositions.push_back(entry.second.position);
    bool preserveConnectivity = std::any_of(
        neighborPositions.begin(), neighborPositions.end(),
        [&](const Coord& position) {
            return current.distance(position) <= fitnessParameters.communicationRange;
        });
    if (preserveConnectivity)
        connectivityConstraintsApplied++;
    RepositionFitness fitness(fitnessParameters, sensor, current,
                              teamPosition, observation.nearestSurfacePoint,
                              neighborPositions, preserveConnectivity, simTime());
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
    if (preserveConnectivity && std::any_of(
            neighborPositions.begin(), neighborPositions.end(),
            [&](const Coord& position) {
                return result.position.distance(position) <=
                    fitnessParameters.communicationRange;
            }))
        connectivityPreservedSelections++;
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
    // O trajeto é executado gradualmente pela mobilidade, que devolve o instante
    // real de chegada. Agendar a retomada com esse valor evita que a aplicação
    // recalcule o tempo por conta própria e divirja do movimento em curso.
    double travelTime = controlled->moveTo(result.position, fitnessParameters.horizontalSpeed,
                                           fitnessParameters.climbSpeed,
                                           fitnessParameters.descentSpeed);
    if (travelTime <= 0) {
        EV_DEBUG << "Reposition skipped: mobility reported no movement\n";
        return;
    }
    reposition.begin(alert.alertId);
    AlertMetricEvent startedEvent(alert.alertId, simTime(), "started", distance);
    emit(repositionEventSignal, &startedEvent);
    scheduleAt(simTime() + travelTime, movementCompleteTimer);
}

void DroneApp::recordCompletedRepositionDistance(const PendingVictimAlert& alert)
{
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    double distance = alert.repositionOrigin.distance(mobility->getCurrentPosition());
    AlertMetricEvent distanceEvent(alert.alertId, simTime(), "distance", distance);
    emit(repositionEventSignal, &distanceEvent);
}

void DroneApp::handleVictimAck(Packet *packet)
{
    auto ack = packet->peekAtFront<VictimAckChunk>();
    auto it = pendingAlerts.find(ack->getAlertId());
    if (it != pendingAlerts.end()) {
        auto& alert = it->second;
        auto teamIt = alert.attemptTeamIds.find(ack->getMessageId());
        auto addressIt = alert.attemptTeamAddresses.find(ack->getMessageId());
        auto source = packet->findTag<L3AddressInd>();
        bool valid = ack->getSourceDroneId() == droneId &&
            ack->getVictimId() == alert.victimId &&
            ack->getAckTimestamp() <= simTime() &&
            ack->getAckTimestamp() >= alert.creationTime &&
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
        confirmedEvent.teamId = ack->getTeamId();
        emit(alertConfirmedSignal, &confirmedEvent);
        bool ownsReposition = reposition.owns(it->first);
        if (ownsReposition) {
            reposition.release();
            if (movementCompleteTimer->isScheduled())
                cancelEvent(movementCompleteTimer);
        }
        completeAlertCycle(alert);
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

void DroneApp::finish()
{
    recordScalar("droneStatusUpdatesAccepted", droneStatusUpdatesAccepted);
    recordScalar("connectivityConstraintsApplied", connectivityConstraintsApplied);
    recordScalar("connectivityPreservedSelections", connectivityPreservedSelections);
}

DroneApp::~DroneApp()
{
    cancelAndDelete(maintenanceTimer);
    cancelAndDelete(movementCompleteTimer);
    cancelAndDelete(droneStatusTimer);
    for (auto *relay : pendingTeamUpdateRelays)
        cancelAndDelete(relay);
}

} // namespace echosar
