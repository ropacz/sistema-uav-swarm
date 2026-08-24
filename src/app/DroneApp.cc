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

    require(linkWindow > 0, "linkWindow must be positive");
    require(positionUpdateInterval > 0, "expectedPositionUpdateInterval must be positive");
    require(teamSilenceTimeout > 0, "teamSilenceTimeout must be positive");
    require(teamEntryLifetime > teamSilenceTimeout,
            "teamEntryLifetime must exceed teamSilenceTimeout");
    require(teamPredictionHorizon >= 0, "teamPredictionHorizon must not be negative");
    require(maximumTeamPredictionSpeed > 0, "maximumTeamPredictionSpeed must be positive");
    require(maintenanceInterval > 0, "maintenanceInterval must be positive");
    require(pdrThreshold >= 0 && pdrThreshold <= 1, "pdrThreshold must be 0..1");

    require(maxBaCycles >= 0, "maxBaCycles must not be negative");
    require(maximumRepositionDistance > 0, "maximumRepositionDistance must be positive");
    require(minimumAltitude <= maximumAltitude, "minimumAltitude must not exceed maximumAltitude");
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
        linkWindow = par("linkWindow");
        positionUpdateInterval = par("expectedPositionUpdateInterval");
        teamSilenceTimeout = par("teamSilenceTimeout");
        teamEntryLifetime = par("teamEntryLifetime");
        teamPredictionHorizon = par("teamPredictionHorizon");
        maximumTeamPredictionSpeed = par("maximumTeamPredictionSpeed").doubleValueInUnit("mps");
        maintenanceInterval = par("maintenanceInterval");
        pdrThreshold = par("pdrThreshold");
        rssiThresholdDbm = par("rssiThreshold").doubleValueInUnit("dBm");
        baEnabled = par("baEnabled");
        requireObstacleConfirmation = par("requireObstacleConfirmation");
        maxBaCycles = par("maxBaCycles");
        maximumRepositionDistance = par("maximumRepositionDistance").doubleValueInUnit("m");
        fitnessParameters.maximumRepositionDistance = maximumRepositionDistance;
        minimumAltitude = par("minimumAltitude").doubleValueInUnit("m");
        maximumAltitude = par("maximumAltitude").doubleValueInUnit("m");
        fitnessParameters.minimumAltitude = minimumAltitude;
        fitnessParameters.maximumAltitude = maximumAltitude;
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
        rssiSignal = registerSignal("positionUpdateRssi");
        receivedPowerSignal = registerSignal("positionUpdatePowerMilliwatt");
        pdrSignal = registerSignal("linkWindowPdr");
        repositionDistanceSignal = registerSignal("repositionDistance");
        recoveryTimeSignal = registerSignal("recoveryTime");
        alertGeneratedSignal = registerSignal("victimAlertGenerated");
        alertAttemptSentSignal = registerSignal("victimAlertAttemptSent");
        alertConfirmedSignal = registerSignal("victimAlertConfirmed");
        alertExpiredSignal = registerSignal("victimAlertExpired");
        degradationSignal = registerSignal("victimDegradationIndicated");
        sensorEvaluationSignal = registerSignal("victimSensorEvaluated");
        baActivationSignal = registerSignal("victimBaActivated");
        repositionEventSignal = registerSignal("victimRepositionEvent");
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
    // Um único timer coordena expiração, degradação e novas tentativas.
    if (message == maintenanceTimer) {
        performMaintenance();
        scheduleAt(simTime() + maintenanceInterval, maintenanceTimer);
    }
    else if (message == movementCompleteTimer) {
        for (auto& [id, alert] : pendingAlerts)
            if (reposition.owns(id)) {
                recordActualRepositionDistance(alert);
                AlertMetricEvent completedEvent(alert.alertId,
                                                alert.activeRepositionCycleId, simTime(),
                                                SimTime::ZERO, "completed");
                emit(repositionEventSignal, &completedEvent);
                break;
            }
        // A próxima tentativa passa a validar a posição escolhida pelo BA.
        reposition.arrived();
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
    AlertMetricEvent generatedEvent(alertId, "", alert.generationTime);
    emit(alertGeneratedSignal, &generatedEvent);
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
    std::string senderId = update->getSenderId();
    if (senderId.empty()) {
        delete packet;
        return;
    }
    auto sourceAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    auto [teamIt, discovered] = discoveredTeams.try_emplace(senderId);
    auto& team = teamIt->second;
    if (discovered) {
        teamEntriesDiscovered++;
        team.observationStart = simTime();
    }
    team.ipAddress = sourceAddress.str();
    Coord receivedPosition(update->getPositionX(), update->getPositionY(), update->getPositionZ());
    simtime_t sourceTime = update->getTimestamp();
    if (sourceTime < SIMTIME_ZERO || sourceTime > simTime())
        sourceTime = simTime();
    if (update->getSequenceNumber() > team.lastSequence) {
        if (team.positionTime >= SIMTIME_ZERO && sourceTime > team.positionTime) {
            double elapsed = (sourceTime - team.positionTime).dbl();
            Coord measuredVelocity((receivedPosition.x - team.position.x) / elapsed,
                                   (receivedPosition.y - team.position.y) / elapsed,
                                   (receivedPosition.z - team.position.z) / elapsed);
            double speed = measuredVelocity.length();
            if (speed > maximumTeamPredictionSpeed)
                measuredVelocity *= maximumTeamPredictionSpeed / speed;
            team.velocity = measuredVelocity;
            team.velocityValid = true;
        }
        team.position = receivedPosition;
        team.positionTime = sourceTime;
        team.lastSequence = update->getSequenceNumber();
        // Somente informação nova comprova que a equipe ainda está visível.
        // Duplicatas ou pacotes reordenados não podem renovar o silêncio.
        team.lastSeen = simTime();
    }
    double rssi = NAN;
    if (auto power = packet->findTag<SignalPowerInd>()) {
        const double powerMilliwatt = power->getPower().get() / 0.001;
        rssi = 10 * std::log10(powerMilliwatt);
        emit(rssiSignal, rssi);
        // Potências devem ser agregadas em escala linear. A conversão da média
        // global para dBm é feita apenas no pós-processamento.
        emit(receivedPowerSignal, powerMilliwatt);
    }
    if (team.samples.empty() || update->getSequenceNumber() > team.samples.back().sequence) {
        team.samples.push_back({update->getSequenceNumber(), simTime(), rssi});
        if (std::isnan(rssi))
            rssiSamplesMissing++;
        else
            rssiSamplesAvailable++;
    }
    // Mantém somente amostras pertencentes à janela deslizante configurada.
    // A janela é (agora-linkWindow, agora]. Remover também a fronteira esquerda
    // faz uma janela de 10 s com período de 1 s conter no máximo 10 beacons.
    while (!team.samples.empty() && simTime() - team.samples.front().receptionTime >= linkWindow)
        team.samples.pop_front();
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
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord position = mobility->getCurrentPosition();
    alert.attempts++;
    alert.sequence++;
    std::string messageId = alert.alertId + "-attempt-" + std::to_string(alert.attempts);
    if (reposition.awaitingValidationOf(alert.alertId)) {
        alert.validationMessageId = messageId;
        alert.validationCycleId = alert.activeRepositionCycleId;
    }
    alert.attemptSentTimes[messageId] = simTime();
    alert.attemptTeamIds[messageId] = alert.targetTeamId;
    alert.attemptTeamAddresses[messageId] = team.ipAddress;
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
    AlertMetricEvent attemptEvent(alert.alertId, messageId, simTime());
    emit(alertAttemptSentSignal, &attemptEvent);
}

bool DroneApp::detectDegradation(const PendingVictimAlert& alert, double& pdr, double& rssi) const
{
    pdr = 0;
    rssi = NAN;
    auto teamIt = discoveredTeams.find(alert.targetTeamId);
    if (teamIt == discoveredTeams.end())
        return true;
    const auto& team = teamIt->second;
    if (!team.samples.empty()) {
        pdr = calculatePositionUpdatePdr(team);
        double sum = 0;
        int count = 0;
        for (const auto& sample : team.samples)
            if (!std::isnan(sample.rssiDbm)) { sum += sample.rssiDbm; count++; }
        if (count) rssi = sum / count;
    }
    bool silence = team.lastSeen < SIMTIME_ZERO || simTime() - team.lastSeen >= teamSilenceTimeout;
    return silence || pdr < pdrThreshold || (!std::isnan(rssi) && rssi < rssiThresholdDbm);
}

double DroneApp::calculatePositionUpdatePdr(const TeamLinkState& team) const
{
    if (team.samples.empty())
        return 0;

    // O denominador temporal cresce desde a primeira observação até preencher a
    // janela. Assim, uma equipe recém-descoberta começa em 1/1, não em 1/N.
    const double observedAge = std::min(
        linkWindow.dbl(),
        std::max(0.0, (simTime() - team.observationStart).dbl()));
    const int windowCapacity = std::max(
        1, static_cast<int>(std::ceil(linkWindow.dbl() /
                                     positionUpdateInterval.dbl() - 1e-12)));
    const int expectedByTime = std::min(
        windowCapacity,
        1 + static_cast<int>(std::floor(observedAge /
                                       positionUpdateInterval.dbl() + 1e-12)));

    // A sequência preserva perdas internas mesmo quando os pacotes nas bordas da
    // janela já expiraram. O maior dos dois estimadores é o denominador honesto:
    // tempo detecta silêncio final; sequência detecta lacunas entre recepções.
    const int64_t sequenceSpan = team.samples.back().sequence -
        team.samples.front().sequence + 1;
    const int expected = std::max(expectedByTime,
                                  static_cast<int>(std::max<int64_t>(1, sequenceSpan)));
    return std::clamp(static_cast<double>(team.samples.size()) / expected, 0.0, 1.0);
}

Coord DroneApp::estimateTeamPosition(const TeamLinkState& team, double& predictionAge) const
{
    predictionAge = 0;
    if (!team.velocityValid || team.positionTime < SIMTIME_ZERO)
        return team.position;
    predictionAge = std::clamp((simTime() - team.positionTime).dbl(), 0.0,
                               teamPredictionHorizon.dbl());
    Coord estimate = team.position + team.velocity * predictionAge;
    estimate.x = std::clamp(estimate.x, fitnessParameters.areaMinX, fitnessParameters.areaMaxX);
    estimate.y = std::clamp(estimate.y, fitnessParameters.areaMinY, fitnessParameters.areaMaxY);
    return estimate;
}

void DroneApp::performMaintenance()
{
    for (auto it = discoveredTeams.begin(); it != discoveredTeams.end(); ) {
        auto& team = it->second;
        while (!team.samples.empty() && simTime() - team.samples.front().receptionTime >= linkWindow)
            team.samples.pop_front();
        if (team.lastSeen >= SIMTIME_ZERO &&
            simTime() - team.lastSeen >= teamEntryLifetime) {
            teamEntriesExpired++;
            it = discoveredTeams.erase(it);
        }
        else
            ++it;
    }

    for (auto it = pendingAlerts.begin(); it != pendingAlerts.end(); ) {
        auto& alert = it->second;
        // TTL e limite de tentativas encerram alertas sem confirmação.
        if (simTime() - alert.generationTime >= alertTtl ||
            (alert.attempts >= maxAttempts && simTime() >= alert.nextAttempt)) {
            alertsExpired++;
            AlertMetricEvent expiredEvent(alert.alertId);
            emit(alertExpiredSignal, &expiredEvent);
            if (reposition.owns(alert.alertId) && !reposition.idle()) {
                recordActualRepositionDistance(alert);
                failedRepositions++;
                repositionExpiredBeforeAck++;
                AlertMetricEvent repositionExpiredEvent(
                    alert.alertId, alert.activeRepositionCycleId,
                    alert.repositionStart,
                    SimTime::ZERO, "expired");
                emit(repositionEventSignal, &repositionExpiredEvent);
            }
            if (reposition.owns(alert.alertId)) {
                reposition.release();
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
                AlertMetricEvent degradationEvent(alert.alertId);
                emit(degradationSignal, &degradationEvent);
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
    auto teamIt = discoveredTeams.find(alert.targetTeamId);
    if (teamIt == discoveredTeams.end() || teamIt->second.lastSeen < SIMTIME_ZERO) {
        // Sem posição conhecida da equipe não há linha de visada a consultar.
        // Isto não é uma rejeição do sensor: a consulta nem chegou a ocorrer.
        teamUnknownForReposition++;
        AlertMetricEvent sensorEvent(alert.alertId, "", simTime(),
                                     SimTime::ZERO, "teamUnknown");
        emit(sensorEvaluationSignal, &sensorEvent);
        return;
    }
    auto sensor = check_and_cast<AbstractObstacleSensor *>(getParentModule()->getSubmodule("obstacleSensor"));
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord current = mobility->getCurrentPosition();
    double predictionAge = 0;
    Coord teamPosition = estimateTeamPosition(teamIt->second, predictionAge);
    if (predictionAge > 0) {
        predictedTeamPositions++;
        teamPredictionAgeSum += predictionAge;
        teamPredictionAgeMax = std::max(teamPredictionAgeMax, predictionAge);
    }
    ObstacleObservation observation;
    if (requireObstacleConfirmation)
        observation = sensor->inspect(current, teamPosition);
    EV_INFO << "Team position for repositioning: observed=" << teamIt->second.position
            << " estimated=" << teamPosition << " velocity=" << teamIt->second.velocity
            << " predictionAge=" << predictionAge << "s\n";
    // No experimento principal, o BA depende de confirmação geométrica
    // independente. A ablação pode ativá-lo somente pelos indicadores de rede.
    if (requireObstacleConfirmation && !observation.confirmed) {
        sensorRejections++;
        // Separa "não havia obstáculo" de "havia, porém fora do alcance": as
        // duas rejeições apontam para causas opostas da degradação.
        if (observation.reason == "clearLineOfSight")
            sensorClearLineOfSight++;
        else if (observation.reason == "outsideVisualRange")
            sensorOutsideRange++;
        AlertMetricEvent sensorEvent(alert.alertId, "", simTime(),
                                     SimTime::ZERO, observation.reason);
        emit(sensorEvaluationSignal, &sensorEvent);
        return;
    }
    if (observation.confirmed) {
        sensorConfirmations++;
        AlertMetricEvent sensorEvent(alert.alertId, "", simTime(),
                                     SimTime::ZERO, "confirmed");
        emit(sensorEvaluationSignal, &sensorEvent);
    }
    if (!baEnabled || alert.baCycles >= maxBaCycles || reposition.moving() ||
        reposition.busyWithOther(alert.alertId))
        return;

    baActivations++;
    AlertMetricEvent baEvent(alert.alertId);
    emit(baActivationSignal, &baEvent);
    alert.baCycles++;
    RepositionFitness fitness(fitnessParameters, sensor, current,
                              teamPosition, observation.nearestSurfacePoint, simTime(),
                              requireObstacleConfirmation);
    BatResult result = BatAlgorithm::optimize(current, maximumRepositionDistance,
        batParameters, getRNG(0),
        [&](const Coord& candidate) { return fitness.cost(candidate); },
        [&](const Coord& candidate) { return fitness.feasible(candidate); });
    EV_INFO << "BA result: current=" << current << " target=" << teamPosition
            << " candidate=" << result.position << " fitness=" << result.fitness
            << " valid=" << result.valid << "\n";
    if (!result.valid) {
        failedRepositions++;
        baNoFeasibleSolution++;
        AlertMetricEvent failedEvent(alert.alertId, "", simTime(),
                                     SimTime::ZERO, "noFeasibleSolution");
        emit(repositionEventSignal, &failedEvent);
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
        AlertMetricEvent failedEvent(alert.alertId, "", simTime(),
                                     SimTime::ZERO, "redundantCandidate");
        emit(repositionEventSignal, &failedEvent);
        return;
    }
    auto controlled = dynamic_cast<BaGaussMarkovMobility *>(mobility);
    if (!controlled) {
        failedRepositions++;
        baNoFeasibleSolution++;
        AlertMetricEvent failedEvent(alert.alertId, "", simTime(),
                                     SimTime::ZERO, "noFeasibleSolution");
        emit(repositionEventSignal, &failedEvent);
        return;
    }
    double distance = current.distance(result.position);
    if (distance <= 1e-6) {
        failedRepositions++;
        baRedundantCandidate++;
        AlertMetricEvent failedEvent(alert.alertId, "", simTime(),
                                     SimTime::ZERO, "redundantCandidate");
        emit(repositionEventSignal, &failedEvent);
        return;
    }
    // O ciclo só nasce depois que há candidato e mobilidade utilizáveis. Uma
    // execução do otimizador que falha não pode substituir a identidade do
    // movimento anterior que ainda aguarda validação.
    alert.repositionCycle++;
    alert.activeRepositionCycleId = alert.alertId + "-reposition-" +
        std::to_string(alert.repositionCycle);
    alert.validationMessageId.clear();
    alert.validationCycleId.clear();
    alert.repositionStart = simTime();
    alert.repositionOrigin = current;
    alert.repositionDistanceRecorded = false;
    alert.preRepositionPdr = prePdr;
    alert.preRepositionRssi = preRssi;
    commandedBaDistance += distance;
    double travelTime = fitness.travelTime(current, result.position);
    // A mobilidade executa o trajeto no tempo calculado; não há teletransporte.
    controlled->moveTo(result.position, fitnessParameters.horizontalSpeed,
                       fitnessParameters.climbSpeed, fitnessParameters.descentSpeed);
    reposition.begin(alert.alertId);
    AlertMetricEvent startedEvent(alert.alertId, alert.activeRepositionCycleId,
                                  alert.repositionStart,
                                  SimTime::ZERO, "started", distance);
    emit(repositionEventSignal, &startedEvent);
    scheduleAt(simTime() + travelTime, movementCompleteTimer);
}

void DroneApp::recordActualRepositionDistance(PendingVictimAlert& alert)
{
    if (alert.repositionDistanceRecorded)
        return;
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    double distance = alert.repositionOrigin.distance(mobility->getCurrentPosition());
    baDistance += distance;
    emit(repositionDistanceSignal, distance);
    AlertMetricEvent distanceEvent(alert.alertId, alert.activeRepositionCycleId,
                                   simTime(),
                                   SimTime::ZERO, "distance", distance);
    emit(repositionEventSignal, &distanceEvent);
    alert.repositionDistanceRecorded = true;
}

void DroneApp::handleVictimAck(Packet *packet)
{
    auto ack = packet->peekAtFront<VictimAckChunk>();
    auto it = pendingAlerts.find(ack->getAlertId());
    if (it != pendingAlerts.end()) {
        auto& alert = it->second;
        auto sentIt = alert.attemptSentTimes.find(ack->getReceivedMessageId());
        auto teamIt = alert.attemptTeamIds.find(ack->getReceivedMessageId());
        auto addressIt = alert.attemptTeamAddresses.find(ack->getReceivedMessageId());
        auto source = packet->findTag<L3AddressInd>();
        bool valid = ack->getOriginDroneId() == droneId && ack->getVictimId() == alert.victimId &&
            sentIt != alert.attemptSentTimes.end() &&
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
        uniqueAlertsAcked++;
        totalRtt += simTime() - sentIt->second;
        AlertMetricEvent confirmedEvent(alert.alertId,
                                        ack->getReceivedMessageId(),
                                        sentIt->second);
        emit(alertConfirmedSignal, &confirmedEvent);
        bool ownsReposition = reposition.owns(it->first);
        bool validatedReposition = ownsReposition && !alert.validationMessageId.empty() &&
            alert.validationMessageId == ack->getReceivedMessageId() &&
            alert.validationCycleId == alert.activeRepositionCycleId;
        if (ownsReposition) {
            recordActualRepositionDistance(alert);
            if (validatedReposition) {
                // Comparações pré/pós só são causais quando o ACK corresponde à
                // tentativa transmitida na posição final deste mesmo ciclo.
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
                successfulRepositions++;
                simtime_t recovery = simTime() - alert.repositionStart;
                totalRecoveryTime += recovery;
                recoverySamples++;
                emit(recoveryTimeSignal, recovery);
                AlertMetricEvent repositionEvent(alert.alertId,
                                                 alert.activeRepositionCycleId,
                                                 alert.repositionStart,
                                                 SimTime::ZERO, "validated");
                emit(repositionEventSignal, &repositionEvent);
            }
            else {
                // O alerta foi entregue, mas por uma tentativa anterior à
                // validação: a recuperação ocorreu durante o trajeto ou por
                // pacote antigo após a chegada; a posição final não foi testada.
                repositionAckedBeforeValidation++;
                const char *recoveryCategory = reposition.moving()
                    ? "recoveredDuringMovement" : "recoveredAfterArrival";
                AlertMetricEvent repositionEvent(
                    alert.alertId, alert.activeRepositionCycleId,
                    alert.repositionStart,
                    SimTime::ZERO, recoveryCategory);
                emit(repositionEventSignal, &repositionEvent);
            }
        }
        if (ownsReposition) {
            reposition.release();
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
    recordScalar("sensorClearLineOfSight", sensorClearLineOfSight);
    recordScalar("sensorOutsideRange", sensorOutsideRange);
    recordScalar("baActivations", baActivations);
    recordScalar("successfulRepositions", successfulRepositions);
    recordScalar("failedRepositions", failedRepositions);
    recordScalar("baNoFeasibleSolution", baNoFeasibleSolution);
    recordScalar("baRedundantCandidate", baRedundantCandidate);
    recordScalar("repositionExpiredBeforeAck", repositionExpiredBeforeAck);
    recordScalar("repositionAckedBeforeValidation", repositionAckedBeforeValidation);
    recordScalar("baDistance", baDistance);
    recordScalar("commandedBaDistance", commandedBaDistance);
    recordScalar("predictedTeamPositions", predictedTeamPositions);
    recordScalar("teamPredictionAgeSum", teamPredictionAgeSum);
    recordScalar("teamPredictionAgeMax", teamPredictionAgeMax);
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
    recordScalar("rssiSamplesAvailable", rssiSamplesAvailable);
    recordScalar("rssiSamplesMissing", rssiSamplesMissing);
    recordScalar("teamEntriesDiscovered", teamEntriesDiscovered);
    recordScalar("teamEntriesExpired", teamEntriesExpired);
}

} // namespace echosar
