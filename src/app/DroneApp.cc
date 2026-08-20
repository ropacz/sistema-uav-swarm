#include "DroneApp.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <sstream>

#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "mobility/BaGaussMarkovMobility.h"
#include "sensing/AbstractObstacleSensor.h"

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(DroneApp);

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
        linkWindow = par("linkWindow");
        teamSilenceTimeout = par("teamSilenceTimeout");
        maintenanceInterval = par("maintenanceInterval");
        pdrThreshold = par("pdrThreshold");
        rssiThresholdDbm = par("rssiThreshold").doubleValueInUnit("dBm");
        baEnabled = par("baEnabled");
        maxBaCycles = par("maxBaCycles");
        maximumRepositionDistance = par("maximumRepositionDistance").doubleValueInUnit("m");
        minimumAltitude = par("minimumAltitude").doubleValueInUnit("m");
        maximumAltitude = par("maximumAltitude").doubleValueInUnit("m");
        areaMinX = par("areaMinX").doubleValueInUnit("m");
        areaMaxX = par("areaMaxX").doubleValueInUnit("m");
        areaMinY = par("areaMinY").doubleValueInUnit("m");
        areaMaxY = par("areaMaxY").doubleValueInUnit("m");
        horizontalSpeed = par("horizontalSpeed").doubleValueInUnit("mps");
        climbSpeed = par("climbSpeed").doubleValueInUnit("mps");
        descentSpeed = par("descentSpeed").doubleValueInUnit("mps");
        flightTimeLimit = par("flightTimeLimit");
        applicationIpTtl = par("applicationIpTtl");
        wLink = par("wLink"); wObstacle = par("wObstacle"); wMove = par("wMove");
        obstacleSigma = par("obstacleSigma").doubleValueInUnit("m");
        obstacleSafetyMargin = par("obstacleSafetyMargin").doubleValueInUnit("m");
        linkNormalizationDistance = par("linkNormalizationDistance").doubleValueInUnit("m");
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
        if (droneId.empty() || retryInterval <= 0 || ackTimeout <= 0 || ackTimeout > retryInterval ||
            appPort <= 0 || appPort > 65535 || victimAlertPayloadBytes <= 0 ||
            maxAttempts <= 0 || alertTtl < retryInterval || linkWindow <= 0 || teamSilenceTimeout <= 0 ||
            maintenanceInterval <= 0 || pdrThreshold < 0 || pdrThreshold > 1 || maxBaCycles < 0 ||
            maximumRepositionDistance <= 0 || minimumAltitude > maximumAltitude ||
            areaMinX >= areaMaxX || areaMinY >= areaMaxY || horizontalSpeed <= 0 || climbSpeed <= 0 ||
            descentSpeed <= 0 || flightTimeLimit <= 0 || applicationIpTtl <= 0 || applicationIpTtl > 255 ||
            obstacleSigma <= 0 || obstacleSafetyMargin < 0 || linkNormalizationDistance <= 0 ||
            batParameters.populationSize <= 0 || batParameters.iterations <= 0 ||
            batParameters.initializationAttempts <= 0 || batParameters.frequencyMin < 0 ||
            batParameters.frequencyMax < batParameters.frequencyMin ||
            batParameters.initialAmplitude <= 0 || batParameters.initialAmplitude > 1 ||
            batParameters.initialPulseRate < 0 || batParameters.initialPulseRate > 1 ||
            batParameters.amplitudeDecay <= 0 || batParameters.amplitudeDecay > 1 ||
            batParameters.pulseGrowth <= 0 || batParameters.localSearchScale <= 0 ||
            batParameters.localSearchScale > 1 ||
            wLink < 0 || wObstacle < 0 || wMove < 0 || std::abs(wLink + wObstacle + wMove - 1) > 1e-9)
            throw cRuntimeError("Invalid alert, link-quality, or fitness parameters");
        rssiSignal = registerSignal("positionUpdateRssi");
        pdrSignal = registerSignal("linkWindowPdr");
        repositionDistanceSignal = registerSignal("repositionDistance");
        recoveryTimeSignal = registerSignal("recoveryTime");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        // O endereço só está disponível após a configuração da camada de rede.
        auto ift = L3AddressResolver().findInterfaceTableOf(getParentModule());
        for (int i = 0; i < ift->getNumInterfaces(); ++i) {
            auto interface = ift->getInterface(i);
            auto data = interface->findProtocolData<Ipv4InterfaceData>();
            if (!interface->isLoopback() && data && !data->getIPAddress().isUnspecified()) {
                ipAddress = data->getIPAddress().str();
                break;
            }
        }
        if (ipAddress.empty())
            throw cRuntimeError("Drone '%s' has no configured IPv4 address", droneId.c_str());
        auto network = getParentModule()->getParentModule();
        int teamCount = network->getSubmoduleVectorSize("team");
        for (int i = 0; i < teamCount; ++i) {
            auto team = network->getSubmodule("team", i);
            auto app = team->getSubmodule("app", 0);
            std::string id = app->par("teamId").stdstringValue();
            if (id.empty()) id = team->getFullName();
            TeamLinkState state;
            state.ipAddress = L3AddressResolver().resolve(team->getFullPath().c_str()).str();
            // A posição só se torna conhecida após um PositionUpdate recebido.
            teams[id] = state;
        }
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
    // Um único timer coordena expiração, degradação e novas tentativas.
    if (message == maintenanceTimer) {
        performMaintenance();
        scheduleAt(simTime() + maintenanceInterval, maintenanceTimer);
    }
    else if (message == movementCompleteTimer) {
        // A próxima tentativa passa a validar a posição escolhida pelo BA.
        repositionState = RepositionState::AWAITING_VALIDATION;
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
    uniqueAlertsGenerated++;
    delete assignment;
    sendAttempt(pendingAlerts.at(alertId));
}

void DroneApp::handlePositionUpdate(Packet *packet)
{
    auto update = packet->peekAtFront<PositionUpdateChunk>();
    if (std::string(update->getSenderType()) != "team") {
        delete packet;
        return;
    }
    auto teamIt = teams.find(update->getSenderId());
    if (teamIt == teams.end()) {
        delete packet;
        return;
    }
    auto& team = teamIt->second;
    auto sourceAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    team.ipAddress = sourceAddress.str();
    team.position = Coord(update->getPositionX(), update->getPositionY(), update->getPositionZ());
    team.lastSeen = simTime();
    double rssi = NAN;
    if (auto power = packet->findTag<SignalPowerInd>()) {
        rssi = 10 * std::log10(power->getPower().get() / 0.001);
        emit(rssiSignal, rssi);
    }
    if (team.samples.empty() || update->getSequenceNumber() > team.samples.back().sequence)
        team.samples.push_back({update->getSequenceNumber(), simTime(), rssi});
    // Mantém somente amostras pertencentes à janela deslizante configurada.
    while (!team.samples.empty() && simTime() - team.samples.front().receptionTime > linkWindow)
        team.samples.pop_front();
    delete packet;
}

std::string DroneApp::selectTargetTeam() const
{
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord current = mobility->getCurrentPosition();
    std::string selected;
    std::string fallback;
    double best = INFINITY;
    for (const auto& [id, team] : teams) {
        if (team.ipAddress.empty()) continue;
        // Broadcasts de posição não atravessam AODV. O menor ID fornece um
        // destino determinístico quando nenhuma equipe foi ouvida diretamente.
        if (fallback.empty() || id < fallback)
            fallback = id;
        if (team.lastSeen < SIMTIME_ZERO) continue;
        double distance = current.distance(team.position);
        // O identificador resolve empates de forma reprodutível.
        if (selected.empty() || distance < best || (distance == best && id < selected)) {
            selected = id;
            best = distance;
        }
    }
    return selected.empty() ? fallback : selected;
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
    auto& team = teams.at(alert.targetTeamId);
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord position = mobility->getCurrentPosition();
    alert.attempts++;
    alert.sequence++;
    std::string messageId = alert.alertId + "-attempt-" + std::to_string(alert.attempts);
    if (activeRepositionAlertId == alert.alertId &&
        repositionState == RepositionState::AWAITING_VALIDATION)
        alert.validationMessageId = messageId;
    alert.attemptSentTimes[messageId] = simTime();
    auto message = makeShared<VictimAlertChunk>();
    message->setChunkLength(B(victimAlertPayloadBytes));
    message->setAlertId(alert.alertId.c_str());
    message->setMessageId(messageId.c_str());
    message->setVictimId(alert.victimId.c_str());
    message->setOriginDroneId(droneId.c_str());
    message->setOriginDroneAddress(ipAddress.c_str());
    message->setVictimPositionX(alert.victimPosition.x);
    message->setVictimPositionY(alert.victimPosition.y);
    message->setVictimPositionZ(alert.victimPosition.z);
    message->setDronePositionX(position.x);
    message->setDronePositionY(position.y);
    message->setDronePositionZ(position.z);
    auto controlledMobility = dynamic_cast<BaGaussMarkovMobility *>(mobility);
    message->setWaypointId(controlledMobility ? controlledMobility->getWaypointId() : -1);
    message->setSequenceNumber(alert.sequence);
    message->setAttemptNumber(alert.attempts);
    message->setGenerationTimestamp(alert.generationTime);
    message->setTransmissionTimestamp(simTime());
    message->setTimeToLive(alertTtl);
    socket.sendTo(new Packet("VictimAlert", message),
                  Ipv4Address(team.ipAddress.c_str()), appPort);
    alert.ackDeadline = simTime() + ackTimeout;
    alert.nextAttempt = simTime() + retryInterval;
    alert.degradationEvaluated = false;
    alertAttemptsSent++;
}

bool DroneApp::detectDegradation(const PendingVictimAlert& alert, double& pdr, double& rssi) const
{
    pdr = 0;
    rssi = NAN;
    auto teamIt = teams.find(alert.targetTeamId);
    if (teamIt == teams.end())
        return true;
    const auto& team = teamIt->second;
    if (!team.samples.empty()) {
        // Lacunas de sequência estimam perdas sem gerar tráfego de sondagem.
        int64_t expected = team.samples.back().sequence - team.samples.front().sequence + 1;
        pdr = expected > 0 ? std::clamp(static_cast<double>(team.samples.size()) / expected, 0.0, 1.0) : 0;
        double sum = 0;
        int count = 0;
        for (const auto& sample : team.samples)
            if (!std::isnan(sample.rssiDbm)) { sum += sample.rssiDbm; count++; }
        if (count) rssi = sum / count;
    }
    bool silence = team.lastSeen < SIMTIME_ZERO || simTime() - team.lastSeen >= teamSilenceTimeout;
    return silence || pdr < pdrThreshold || (!std::isnan(rssi) && rssi < rssiThresholdDbm);
}

void DroneApp::performMaintenance()
{
    for (auto& [id, team] : teams)
        while (!team.samples.empty() && simTime() - team.samples.front().receptionTime > linkWindow)
            team.samples.pop_front();

    for (auto it = pendingAlerts.begin(); it != pendingAlerts.end(); ) {
        auto& alert = it->second;
        // TTL e limite de tentativas encerram alertas sem confirmação.
        if (simTime() - alert.generationTime >= alertTtl ||
            (alert.attempts >= maxAttempts && simTime() >= alert.nextAttempt)) {
            alertsExpired++;
            if (activeRepositionAlertId == alert.alertId && repositionState != RepositionState::IDLE) {
                failedRepositions++;
                repositionExpiredBeforeAck++;
            }
            if (activeRepositionAlertId == alert.alertId) {
                activeRepositionAlertId.clear();
                repositionState = RepositionState::IDLE;
                if (movementCompleteTimer->isScheduled()) cancelEvent(movementCompleteTimer);
                resumeMobility();
            }
            it = pendingAlerts.erase(it);
            continue;
        }
        if (!alert.degradationEvaluated && alert.ackDeadline >= SIMTIME_ZERO && simTime() >= alert.ackDeadline) {
            // Ausência de ACK apenas dispara a avaliação; não confirma obstáculo.
            double pdr, rssi;
            bool degraded = detectDegradation(alert, pdr, rssi);
            emit(pdrSignal, pdr);
            alert.degradationEvaluated = true;
            if (degraded) {
                degradationIndications++;
                tryReposition(alert, pdr, rssi);
            }
        }
        if (simTime() >= alert.nextAttempt && alert.attempts < maxAttempts)
            sendAttempt(alert);
        ++it;
    }
}

void DroneApp::tryReposition(PendingVictimAlert& alert, double prePdr, double preRssi)
{
    auto teamIt = teams.find(alert.targetTeamId);
    if (teamIt == teams.end() || teamIt->second.lastSeen < SIMTIME_ZERO) {
        // Sem posição conhecida da equipe não há linha de visada a consultar.
        // Isto não é uma rejeição do sensor: a consulta nem chegou a ocorrer.
        teamUnknownForReposition++;
        return;
    }
    auto sensor = check_and_cast<AbstractObstacleSensor *>(getParentModule()->getSubmodule("obstacleSensor"));
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord current = mobility->getCurrentPosition();
    auto observation = sensor->inspect(current, teamIt->second.position);
    // O BA depende de confirmação geométrica independente da camada de rede.
    if (!observation.confirmed) {
        sensorRejections++;
        return;
    }
    sensorConfirmations++;
    if (!baEnabled || alert.baCycles >= maxBaCycles || repositionState == RepositionState::MOVING ||
        (!activeRepositionAlertId.empty() && activeRepositionAlertId != alert.alertId))
        return;

    baActivations++;
    alert.baCycles++;
    alert.repositionStart = simTime();
    alert.preRepositionPdr = prePdr;
    alert.preRepositionRssi = preRssi;
    BatResult result = BatAlgorithm::optimize(current, maximumRepositionDistance,
        batParameters, getRNG(0),
        [&](const Coord& candidate) {
            return computeFitness(candidate, current, teamIt->second, observation.nearestSurfacePoint);
        },
        [&](const Coord& candidate) {
            return isFeasible(candidate, current, observation.nearestSurfacePoint);
        });
    if (!result.valid) {
        failedRepositions++;
        baNoFeasibleSolution++;
        return;
    }
    std::ostringstream key;
    key << std::round(result.position.x * 10) / 10 << ':'
        << std::round(result.position.y * 10) / 10 << ':'
        << std::round(result.position.z * 10) / 10;
    // A quantização evita repetir candidatos praticamente idênticos.
    if (!alert.testedPositions.insert(key.str()).second) {
        failedRepositions++;
        baRedundantCandidate++;
        return;
    }
    auto controlled = dynamic_cast<BaGaussMarkovMobility *>(mobility);
    if (!controlled) {
        failedRepositions++;
        baNoFeasibleSolution++;
        return;
    }
    double distance = current.distance(result.position);
    if (distance <= 1e-6) {
        failedRepositions++;
        baRedundantCandidate++;
        return;
    }
    baDistance += distance;
    emit(repositionDistanceSignal, distance);
    double travelTime = std::max(std::hypot(result.position.x - current.x, result.position.y - current.y) / horizontalSpeed,
        std::abs(result.position.z - current.z) / (result.position.z >= current.z ? climbSpeed : descentSpeed));
    // A mobilidade executa o trajeto no tempo calculado; não há teletransporte.
    controlled->moveTo(result.position, horizontalSpeed, climbSpeed, descentSpeed);
    activeRepositionAlertId = alert.alertId;
    repositionState = RepositionState::MOVING;
    scheduleAt(simTime() + travelTime, movementCompleteTimer);
}

double DroneApp::computeFitness(const Coord& candidate, const Coord& current,
                                      const TeamLinkState& team, const Coord& obstaclePoint) const
{
    // Usa apenas qualidade estimada; RSSI futuro não é conhecido pelo BA.
    auto sensor = check_and_cast<AbstractObstacleSensor *>(getParentModule()->getSubmodule("obstacleSensor"));
    double linkCost = std::clamp(candidate.distance(team.position) / linkNormalizationDistance, 0.0, 1.0);
    double proximity = std::exp(-candidate.distance(obstaclePoint) / obstacleSigma);
    double obstacleCost = std::max(proximity, sensor->intersectsAnyObstacle(candidate, team.position) ? 1.0 : 0.0);
    double movementCost = std::clamp(candidate.distance(current) / maximumRepositionDistance, 0.0, 1.0);
    return wLink * linkCost + wObstacle * obstacleCost + wMove * movementCost;
}

bool DroneApp::isFeasible(const Coord& candidate, const Coord& current, const Coord& obstaclePoint) const
{
    if (candidate.x < areaMinX || candidate.x > areaMaxX || candidate.y < areaMinY || candidate.y > areaMaxY ||
        candidate.z < minimumAltitude || candidate.z > maximumAltitude ||
        candidate.distance(current) > maximumRepositionDistance ||
        candidate.distance(obstaclePoint) < obstacleSafetyMargin)
        return false;
    double travelTime = std::max(std::hypot(candidate.x - current.x, candidate.y - current.y) / horizontalSpeed,
        std::abs(candidate.z - current.z) / (candidate.z >= current.z ? climbSpeed : descentSpeed));
    if (simTime() + travelTime > flightTimeLimit)
        return false;
    auto sensor = check_and_cast<AbstractObstacleSensor *>(getParentModule()->getSubmodule("obstacleSensor"));
    return !sensor->intersectsAnyObstacle(current, candidate);
}

void DroneApp::handleVictimAck(Packet *packet)
{
    auto ack = packet->peekAtFront<VictimAckChunk>();
    auto it = pendingAlerts.find(ack->getAlertId());
    if (it != pendingAlerts.end()) {
        auto& alert = it->second;
        auto sentIt = alert.attemptSentTimes.find(ack->getReceivedMessageId());
        bool valid = ack->getOriginDroneId() == droneId && ack->getVictimId() == alert.victimId &&
            teams.count(ack->getTeamId()) &&
            sentIt != alert.attemptSentTimes.end();
        // Aceita somente ACK de equipe conhecida para uma tentativa realmente enviada.
        if (!valid) {
            delete packet;
            return;
        }
        uniqueAlertsAcked++;
        totalRtt += simTime() - sentIt->second;
        bool ownsReposition = activeRepositionAlertId == it->first;
        bool validatedReposition = ownsReposition && !alert.validationMessageId.empty() &&
            alert.validationMessageId == ack->getReceivedMessageId();
        if (ownsReposition) {
            // Separa a entrega do alerta da validação específica do reposicionamento.
            repositionValidationSamples++;
            double postPdr, postRssi;
            detectDegradation(alert, postPdr, postRssi);
            preRepositionPdrSum += alert.preRepositionPdr;
            postRepositionPdrSum += postPdr;
            if (!std::isnan(alert.preRepositionRssi)) {
                preRepositionRssiSum += alert.preRepositionRssi;
                preRepositionRssiSamples++;
            }
            if (!std::isnan(postRssi)) {
                postRepositionRssiSum += postRssi;
                postRepositionRssiSamples++;
            }
            if (validatedReposition) {
                successfulRepositions++;
                simtime_t recovery = simTime() - alert.repositionStart;
                totalRecoveryTime += recovery;
                recoverySamples++;
                emit(recoveryTimeSignal, recovery);
            }
            else {
                // O alerta foi entregue, mas por uma tentativa anterior à
                // chegada: a posição escolhida pelo BA não chegou a ser testada.
                failedRepositions++;
                repositionAckedBeforeValidation++;
            }
        }
        if (ownsReposition) {
            activeRepositionAlertId.clear();
            repositionState = RepositionState::IDLE;
            if (movementCompleteTimer->isScheduled()) cancelEvent(movementCompleteTimer);
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

void DroneApp::finish()
{
    recordScalar("uniqueAlertsGenerated", uniqueAlertsGenerated);
    recordScalar("alertAttemptsSent", alertAttemptsSent);
    recordScalar("uniqueAlertsAcked", uniqueAlertsAcked);
    recordScalar("alertsExpired", alertsExpired);
    recordScalar("degradationIndications", degradationIndications);
    recordScalar("sensorConfirmations", sensorConfirmations);
    recordScalar("sensorRejections", sensorRejections);
    recordScalar("teamUnknownForReposition", teamUnknownForReposition);
    recordScalar("baActivations", baActivations);
    recordScalar("successfulRepositions", successfulRepositions);
    recordScalar("failedRepositions", failedRepositions);
    recordScalar("baNoFeasibleSolution", baNoFeasibleSolution);
    recordScalar("baRedundantCandidate", baRedundantCandidate);
    recordScalar("repositionExpiredBeforeAck", repositionExpiredBeforeAck);
    recordScalar("repositionAckedBeforeValidation", repositionAckedBeforeValidation);
    recordScalar("baDistance", baDistance);
    recordScalar("totalRTT", totalRtt.dbl());
    recordScalar("totalRecoveryTime", totalRecoveryTime.dbl());
    recordScalar("recoverySamples", recoverySamples);
    recordScalar("repositionValidationSamples", repositionValidationSamples);
    recordScalar("preRepositionPdrSum", preRepositionPdrSum);
    recordScalar("postRepositionPdrSum", postRepositionPdrSum);
    recordScalar("preRepositionRssiSum", preRepositionRssiSum);
    recordScalar("postRepositionRssiSum", postRepositionRssiSum);
    recordScalar("preRepositionRssiSamples", preRepositionRssiSamples);
    recordScalar("postRepositionRssiSamples", postRepositionRssiSamples);
}

} // namespace echosar
