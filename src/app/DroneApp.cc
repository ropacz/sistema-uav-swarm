#include "DroneApp.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
#include <vector>

#include "inet/mobility/contract/IMobility.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "mobility/BaGaussMarkovMobility.h"
#include "metrics/AlertMetricEvent.h"
#include "camera/AbstractObstacleSensor.h"

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

/// Constrói o conteúdo sintético de uma miniatura. O corpo depende apenas do
/// photoId, portanto é idêntico em todas as tentativas do mesmo alerta e
/// reprodutível entre execuções. Nenhum fluxo aleatório do simulador é
/// consultado: usar rand() ou um RNG do módulo deslocaria os sorteios de
/// mobilidade e do BA e quebraria a comparabilidade entre cenários.
std::vector<uint8_t> synthesizePhoto(const std::string& photoId, size_t byteCount)
{
    // Marcadores JFIF de início e fim. Não há codificador por trás deles: são
    // o que torna o anexo reconhecível como imagem em uma inspeção de pacote.
    static const uint8_t marker[] = {0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10,
                                     'J', 'F', 'I', 'F', 0x00, 0x01, 0x01,
                                     0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00};
    static const uint8_t trailer[] = {0xFF, 0xD9};

    // FNV-1a sobre o identificador semeia um xorshift64: ambos têm resultado
    // definido pelo padrão, sem depender de std::hash, cujo valor varia entre
    // implementações e tornaria o conteúdo dependente do compilador.
    uint64_t state = 1469598103934665603ULL;
    for (unsigned char c : photoId) {
        state ^= c;
        state *= 1099511628211ULL;
    }
    if (state == 0)
        state = 0x9E3779B97F4A7C15ULL;

    std::vector<uint8_t> photo(byteCount);
    for (size_t i = 0; i < byteCount; ++i) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        photo[i] = static_cast<uint8_t>(state >> 24);
    }
    size_t markerSize = std::min(sizeof(marker), byteCount);
    std::copy(marker, marker + markerSize, photo.begin());
    if (byteCount >= markerSize + sizeof(trailer))
        std::copy(trailer, trailer + sizeof(trailer), photo.end() - sizeof(trailer));
    return photo;
}
}

void DroneApp::validateParameters() const
{
    const auto& f = fitnessParameters;
    const auto& b = batParameters;

    require(!droneId.empty(), "droneId must not be empty");
    require(appPort > 0 && appPort <= 65535, "appPort must be a valid UDP port");
    require(victimAlertPayloadBytes > 0, "victimAlertPayloadBytes must be positive");
    // Zero desabilita o anexo. Abaixo do cabeçalho JFIF a miniatura não teria
    // sequer os marcadores, e acima de 32 KiB o corpo não caberia no campo de
    // comprimento do wire format.
    require(victimAlertPhotoBytes == 0 ||
            (victimAlertPhotoBytes >= 64 && victimAlertPhotoBytes <= 32768),
            "victimAlertPhotoBytes must be zero or between 64 and 32768");
    require(victimAlertPhotoWidth > 0 && victimAlertPhotoWidth <= 65535,
            "victimAlertPhotoWidth must be 1..65535");
    require(victimAlertPhotoHeight > 0 && victimAlertPhotoHeight <= 65535,
            "victimAlertPhotoHeight must be 1..65535");
    // As exigências abaixo só valem para quem liga a sondagem: um mecanismo
    // desabilitado não pode rejeitar a configuração de um cenário que não o usa.
    if (recoveryProbeEnabled) {
        require(recoveryProbePayloadBytes > 0,
                "recoveryProbePayloadBytes must be positive");
        // Uma sondagem que custasse o mesmo que o alerta não economizaria nada.
        require(recoveryProbePayloadBytes < victimAlertPayloadBytes,
                "recoveryProbePayloadBytes must be smaller than victimAlertPayloadBytes");
        require(recoveryProbeMaxAttempts > 0,
                "recoveryProbeMaxAttempts must be positive");
        require(recoveryProbeTimeout > 0, "recoveryProbeTimeout must be positive");
        // A verificação inteira precisa caber entre duas tentativas do alerta,
        // ou ela atrasaria o retry que deveria apenas anteceder.
        require(recoveryProbeTimeout * recoveryProbeMaxAttempts <= retryInterval,
                "recoveryProbeTimeout times recoveryProbeMaxAttempts must fit in retryInterval");
    }
    require(rssiWindow > 0, "rssiWindow must be positive");
    require(directUpdateTimeout > 0, "directUpdateTimeout must be positive");
    require(rssiReferenceDistance > 0, "rssiReferenceDistance must be positive");
    require(losPathLossExponent > 0, "losPathLossExponent must be positive");
    require(excessLossThresholdDb > 0, "excessLossThresholdDb must be positive");
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
    require(f.droneRadius >= 0, "droneRadius must not be negative");
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
        victimAlertPhotoBytes = par("victimAlertPhotoBytes");
        victimAlertPhotoWidth = par("victimAlertPhotoWidth");
        victimAlertPhotoHeight = par("victimAlertPhotoHeight");
        recoveryProbeEnabled = par("recoveryProbeEnabled");
        recoveryProbePayloadBytes = par("recoveryProbePayloadBytes");
        recoveryProbeTimeout = par("recoveryProbeTimeout");
        recoveryProbeMaxAttempts = par("recoveryProbeMaxAttempts");
        rssiWindow = par("rssiWindow");
        directUpdateTimeout = par("directUpdateTimeout");
        rssiReferenceDbm = par("rssiReferenceDbm");
        rssiReferenceDistance = par("rssiReferenceDistance").doubleValueInUnit("m");
        losPathLossExponent = par("losPathLossExponent");
        excessLossThresholdDb = par("excessLossThresholdDb");
        requireObstructionIndication = par("requireObstructionIndication");
        droneStatusPayloadBytes = par("droneStatusPayloadBytes");
        droneStatusInterval = par("droneStatusInterval");
        droneStatusInitialOffset = par("droneStatusInitialOffset");
        droneEntryLifetime = par("droneEntryLifetime");
        retryInterval = par("retryInterval");
        ackTimeout = par("ackTimeout");
        alertTtl = par("alertTtl");
        alertInterval = par("alertInterval");
        alertGenerationEndTime = par("alertGenerationEndTime");
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
        fitnessParameters.droneRadius = par("droneRadius").doubleValueInUnit("m");
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
        recoveryProbeSignal = registerSignal("victimRecoveryProbe");
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

        // Age ainda na posição escolhida e só depois retoma o Gauss-Markov.
        reposition.release();
        if (simTime() - completedAlert->creationTime < alertTtl &&
            completedAlert->attempts < maxAttempts) {
            // A chegada é exatamente o instante em que não se sabe se o
            // deslocamento resolveu: sondar aqui evita queimar o alerta
            // completo contra um enlace que continua obstruído.
            if (recoveryProbeEnabled)
                startRecoveryProbe(*completedAlert);
            else
                sendAttempt(*completedAlert);
        }
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
    else if (hasTypePrefix(packet->getName(), "RecoveryProbe"))
        handleRecoveryProbe(packet);
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
    if (!canStartAlertCycle()) {
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

bool DroneApp::canStartAlertCycle() const
{
    return alertGenerationEndTime < SIMTIME_ZERO || simTime() <= alertGenerationEndTime;
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
    if (victimAlertPhotoBytes > 0) {
        // A foto é da vítima observada agora, não de cada retransmissão: fixar
        // identidade e instante aqui mantém a mesma imagem em todo o ciclo.
        alert.photoId = alert.alertId + "-photo";
        alert.photoCaptureTime = alert.creationTime;
    }
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
    if (alert.probePending) {
        // O alerta terminou (por ACK ou por expiração) com uma verificação
        // ainda aberta: a resposta, se vier, chegará tarde demais para mudar
        // qualquer coisa. Sem este desfecho a verificação sumiria do funil, e
        // "sondagens emitidas" passaria a não bater com os desfechos contados.
        AlertMetricEvent abandonedEvent(alert.alertId, simTime(), "abandoned");
        emit(recoveryProbeSignal, &abandonedEvent);
    }
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
    // A etiqueta de potência só existe em pacote que cruzou o rádio. O repasse
    // que este drone acabou de emitir volta pelo laço local do broadcast IPv4 e
    // chega sem ela: é o eco do próprio nó, não uma recepção, e contá-lo como
    // atualização encaminhada dobraria o diagnóstico sem nenhuma recepção nova.
    bool receivedOverTheAir = packet->findTag<SignalPowerInd>() != nullptr;
    // hopCount zero é a transmissão da própria equipe: só ela observa o enlace
    // direto. O repasse de outro drone atualiza a posição conhecida, mas o RSSI
    // que ele carregaria seria o do último salto, não o do enlace em questão.
    // A amostragem precede a checagem de sequência: uma recepção duplicada ou
    // reordenada não renova a entrada, mas foi uma recepção real, com potência
    // real, e é evidência legítima sobre o enlace.
    if (update->getHopCount() == 0) {
        team.hasDirectReception = true;
        team.lastDirectUpdateTime = simTime();
        recordDirectRssiSample(packet, team, receivedPosition);
    }
    else if (receivedOverTheAir)
        forwardedTeamUpdatesIgnoredForRssi++;
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

void DroneApp::recordDirectRssiSample(const Packet *packet, TeamLinkState& team,
                                      const Coord& teamPosition)
{
    // A etiqueta é anexada pelo rádio receptor e sobe intacta pela pilha. Sua
    // ausência não é erro: um pacote que não veio do meio sem fio simplesmente
    // não tem potência recebida para informar.
    auto powerTag = packet->findTag<SignalPowerInd>();
    if (!powerTag)
        return;
    double powerWatts = powerTag->getPower().get();
    if (!(powerWatts > 0))
        return;
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    RssiSample sample;
    sample.receptionTime = simTime();
    sample.rssiDbm = 10.0 * std::log10(powerWatts / 1e-3);
    // A posição anunciada no pacote é a da equipe no instante do envio, que é a
    // geometria à qual este RSSI corresponde.
    sample.distanceMeters = mobility->getCurrentPosition().distance(teamPosition);
    team.rssiSamples.push_back(sample);
    directRssiSamples++;
    // Soma da atenuação excedente por amostra, e não por avaliação da janela:
    // dividida por directRssiSamples ao final, dá a média não enviesada de
    // Delta. É o que torna a verificação numérica — o escalar comparável ao
    // valor analítico do cenário, em vez de apenas "indicou ou não indicou".
    directRssiExcessLossSum += expectedRssiDbm(sample.distanceMeters) - sample.rssiDbm;
    discardExpiredRssiSamples(team);
}

double DroneApp::expectedRssiDbm(double distanceMeters) const
{
    // O Log-Distance não vale abaixo da distância de referência: sem o piso,
    // uma equipe a poucos centímetros produziria atenuação esperada negativa e
    // uma degradação aparente onde o enlace está no melhor caso possível.
    double distance = std::max(distanceMeters, rssiReferenceDistance);
    return rssiReferenceDbm -
        10.0 * losPathLossExponent * std::log10(distance / rssiReferenceDistance);
}

void DroneApp::discardExpiredRssiSamples(TeamLinkState& team)
{
    simtime_t limit = simTime() - rssiWindow;
    while (!team.rssiSamples.empty() && team.rssiSamples.front().receptionTime < limit)
        team.rssiSamples.pop_front();
}

std::optional<double> DroneApp::computeExcessAttenuation(TeamLinkState& team)
{
    discardExpiredRssiSamples(team);
    if (team.rssiSamples.empty())
        return std::nullopt;
    double sum = 0;
    for (const auto& sample : team.rssiSamples)
        sum += expectedRssiDbm(sample.distanceMeters) - sample.rssiDbm;
    return sum / team.rssiSamples.size();
}

bool DroneApp::evaluatePossibleObstruction(TeamLinkState& team)
{
    std::optional<double> attenuation = computeExcessAttenuation(team);
    bool weakSignal = attenuation.has_value() && *attenuation > excessLossThresholdDb;
    // Só perde o enlace direto quem chegou a tê-lo. Uma equipe conhecida apenas
    // por repasse nunca forneceu recepção direta, e contar sua ausência como
    // indicação transformaria multi-hop normal em suspeita de obstrução.
    bool missingDirectUpdates = team.hasDirectReception &&
        simTime() - team.lastDirectUpdateTime > directUpdateTimeout;
    // Os contadores registram episódios, não avaliações: a manutenção reavalia
    // a cada maintenanceInterval e inflaria qualquer contagem feita por tick.
    if (weakSignal && !team.rssiDegraded)
        rssiDegradationIndications++;
    team.rssiDegraded = weakSignal;
    if (missingDirectUpdates && !team.directUpdateTimedOut)
        directUpdateTimeoutIndications++;
    team.directUpdateTimedOut = missingDirectUpdates;
    bool indication = weakSignal || missingDirectUpdates;
    if (indication && !team.possibleObstruction)
        possibleObstructionIndications++;
    team.possibleObstruction = indication;
    return indication;
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
    // A miniatura soma ao payload: o alerta com foto ocupa no enlace os bytes
    // do formato textual mais os da imagem.
    message->setChunkLength(B(victimAlertPayloadBytes +
                              (alert.photoId.empty() ? 0 : victimAlertPhotoBytes)));
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
    attachVictimPhoto(message, alert);
    socket.sendTo(new Packet(("VictimAlert:" + alert.alertId).c_str(), message),
                  Ipv4Address(team.ipAddress.c_str()), appPort);
    alert.ackDeadline = simTime() + ackTimeout;
    alert.nextAttempt = simTime() + retryInterval;
    AlertMetricEvent attemptEvent(alert.alertId, simTime(), "", 0, messageId);
    emit(alertAttemptSentSignal, &attemptEvent);
}

void DroneApp::attachVictimPhoto(const inet::Ptr<VictimAlertChunk>& message,
                                 const PendingVictimAlert& alert) const
{
    if (alert.photoId.empty()) {
        // Alerta sem anexo: dimensões zeradas evitam que o receptor leia
        // metadados de uma imagem que não foi enviada.
        message->setPhotoId("");
        message->setPhotoWidth(0);
        message->setPhotoHeight(0);
        message->setPhotoCaptureTime(SIMTIME_ZERO);
        message->setPhotoDataArraySize(0);
        return;
    }
    message->setPhotoId(alert.photoId.c_str());
    message->setPhotoWidth(static_cast<uint16_t>(victimAlertPhotoWidth));
    message->setPhotoHeight(static_cast<uint16_t>(victimAlertPhotoHeight));
    message->setPhotoCaptureTime(alert.photoCaptureTime);
    std::vector<uint8_t> photo =
        synthesizePhoto(alert.photoId, static_cast<size_t>(victimAlertPhotoBytes));
    message->setPhotoDataArraySize(photo.size());
    for (size_t i = 0; i < photo.size(); ++i)
        message->setPhotoData(i, photo[i]);
}

void DroneApp::startRecoveryProbe(PendingVictimAlert& alert)
{
    alert.probePending = true;
    alert.probeAttempts = 0;
    // "started" abre a verificação no funil. Sem ele o coletor não teria como
    // saber quantas verificações existiram: "sent" conta pacotes, e uma
    // verificação pode não chegar a emitir nenhum.
    AlertMetricEvent startedEvent(alert.alertId, simTime(), "started");
    emit(recoveryProbeSignal, &startedEvent);
    if (!sendRecoveryProbe(alert)) {
        // Sem equipe endereçável não há o que sondar. O alerta volta ao retry
        // normal, que já trata a ausência de destino como falha operacional.
        finishRecoveryProbe(alert, "unreachable");
    }
}

bool DroneApp::sendRecoveryProbe(PendingVictimAlert& alert)
{
    std::string targetTeamId = selectTargetTeam();
    if (targetTeamId.empty())
        return false;
    const auto& team = discoveredTeams.at(targetTeamId);
    alert.probeAttempts++;
    // O sequencial global impede que a resposta atrasada de uma sondagem
    // anterior seja aceita como confirmação da sondagem atual. O alertId não
    // entra aqui: ele já viaja em campo próprio, e repeti-lo dentro do
    // identificador só engordaria um pacote que existe para ser barato.
    alert.probeId = droneId + "-probe-" +
        std::to_string(++recoveryProbeSequence);
    alert.probeDeadline = simTime() + recoveryProbeTimeout;

    auto probe = makeShared<RecoveryProbeChunk>();
    probe->setChunkLength(B(recoveryProbePayloadBytes));
    probe->setProbeId(alert.probeId.c_str());
    probe->setAlertId(alert.alertId.c_str());
    probe->setSourceDroneId(droneId.c_str());
    probe->setTargetTeamId(targetTeamId.c_str());
    probe->setSendTime(simTime());
    probe->setReply(false);
    socket.sendTo(new Packet(("RecoveryProbe:" + alert.alertId).c_str(), probe),
                  Ipv4Address(team.ipAddress.c_str()), appPort);
    AlertMetricEvent sentEvent(alert.alertId, simTime(), "sent");
    emit(recoveryProbeSignal, &sentEvent);
    return true;
}

void DroneApp::finishRecoveryProbe(PendingVictimAlert& alert, const char *outcome)
{
    alert.probePending = false;
    alert.probeId.clear();
    alert.probeDeadline = -1;
    AlertMetricEvent outcomeEvent(alert.alertId, simTime(), outcome);
    emit(recoveryProbeSignal, &outcomeEvent);
}

void DroneApp::expireRecoveryProbes()
{
    for (auto& [id, alert] : pendingAlerts) {
        if (!alert.probePending || simTime() < alert.probeDeadline)
            continue;
        if (alert.probeAttempts < recoveryProbeMaxAttempts) {
            if (sendRecoveryProbe(alert))
                continue;
            // A equipe sumiu no meio da verificação. É a mesma situação que
            // "unreachable" na abertura, e não uma sondagem sem resposta:
            // misturar as duas esconderia a diferença entre "o enlace não
            // respondeu" e "não havia para quem perguntar".
            finishRecoveryProbe(alert, "unreachable");
            continue;
        }
        // Esgotada a verificação, o alerta não fica retido: a sondagem existe
        // para poupar transmissões, não para impedir uma entrega que ainda
        // poderia ocorrer. O retry normal reassume no mesmo ciclo.
        finishRecoveryProbe(alert, "failed");
    }
}

void DroneApp::handleRecoveryProbe(Packet *packet)
{
    auto probe = packet->peekAtFront<RecoveryProbeChunk>();
    // O drone só trata a volta; a ida endereçada a ele seria um eco indevido.
    if (!probe->getReply() ||
        std::string(probe->getSourceDroneId()) != droneId) {
        delete packet;
        return;
    }
    auto it = pendingAlerts.find(probe->getAlertId());
    // Uma resposta de sondagem já encerrada, ou de um alerta que expirou
    // enquanto ela viajava, não reabre nada.
    if (it == pendingAlerts.end() || !it->second.probePending ||
        it->second.probeId != probe->getProbeId()) {
        delete packet;
        return;
    }
    auto& alert = it->second;
    finishRecoveryProbe(alert, "confirmed");
    delete packet;
    // O enlace respondeu agora: é o melhor instante para gastar o alerta.
    if (simTime() - alert.creationTime < alertTtl && alert.attempts < maxAttempts)
        sendAttempt(alert);
}

void DroneApp::performMaintenance()
{
    expireDiscoveredEntries();
    // S_ij é avaliado para toda equipe retida, e não apenas no instante da
    // decisão de reposicionamento: a expiração da janela e o vencimento do
    // prazo sem recepção direta são eventos de tempo, que passariam
    // despercebidos se só fossem observados quando um alerta falha.
    for (auto& entry : discoveredTeams)
        evaluatePossibleObstruction(entry.second);
    expireRecoveryProbes();

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
                if (!alert.repositionTriggerRecorded) {
                    alert.repositionTriggerRecorded = true;
                    AlertMetricEvent triggerEvent(alert.alertId);
                    emit(repositionTriggerSignal, &triggerEvent);
                }
                // Só encerra a decisão quando ela é definitiva. Uma recusa
                // temporária mantém o alerta elegível para a próxima
                // oportunidade, em vez de perder o reposicionamento por causa
                // de uma condição que já passou.
                if (tryReposition(alert))
                    alert.repositionDecisionMade = true;
            }
        }
        // O alerta que comanda o movimento espera a tentativa imediata da
        // chegada, e a sondagem pendente retém o pacote pesado até saber se o
        // enlace voltou.
        if (simTime() >= alert.nextAttempt && alert.attempts < maxAttempts &&
            !alert.probePending &&
            !(reposition.moving() && reposition.owns(alert.alertId)))
            sendAttempt(alert);
        ++it;
    }

    for (auto& entry : activeVictims) {
        auto& victim = entry.second;
        if (victim.pendingAlertId.empty() && simTime() >= victim.nextAlertTime &&
            canStartAlertCycle())
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

bool DroneApp::tryReposition(PendingVictimAlert& alert)
{
    // O BA nunca é acionado por uma equipe que jamais foi conhecida. Uma equipe
    // que era conhecida e cuja entrada expirou continua elegível enquanto sua
    // última posição estiver retida: é exatamente o caso que a recuperação trata.
    // Ambas as recusas abaixo são temporárias: a retenção da equipe e a
    // ocupação do drone mudam com o tempo, então o alerta continua elegível.
    auto teamIt = discoveredTeams.find(alert.targetTeamId);
    if (teamIt == discoveredTeams.end() || teamIt->second.lastUpdateTime < SIMTIME_ZERO) {
        EV_DEBUG << "Reposition skipped: target team is no longer retained\n";
        return false;
    }
    if (!reposition.idle()) {
        EV_DEBUG << "Reposition skipped: drone is already moving for another alert\n";
        return false;
    }
    // S_ij é evidência adicional sobre o enlace, nunca gatilho: quem decide
    // *quando* avaliar continua sendo a ausência de ACK acima do limiar.
    // Habilitado, requireObstructionIndication exige que a evidência acompanhe
    // a falha antes de gastar uma consulta ao sensor. A recusa é temporária —
    // a indicação pode surgir na próxima janela —, então o alerta continua
    // elegível em vez de perder o reposicionamento.
    bool obstructionIndicated = evaluatePossibleObstruction(teamIt->second);
    if (requireObstructionIndication && !obstructionIndicated) {
        obstructionGateSuppressions++;
        EV_DEBUG << "Reposition skipped: no obstruction indication for the target team\n";
        return false;
    }
    auto sensor = check_and_cast<AbstractObstacleSensor *>(getParentModule()->getSubmodule("obstacleSensor"));
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord current = mobility->getCurrentPosition();
    Coord teamPosition = teamIt->second.position;
    // A observação visual é informativa, não é condição de acionamento. Não
    // detectar obstáculo significa apenas que nenhum obstáculo foi observado
    // dentro do alcance da câmera — não que o enlace esteja íntegro. A
    // degradação pode vir de bloqueador além dos 30 m, afastamento da equipe,
    // interferência, desvanecimento ou posição desatualizada. Quem decide
    // *quando* reposicionar é a rede (ausência de ACK acima do limiar); a
    // câmera apenas informa *o que* foi visto. Ver docs/desvios_e_extensoes.md,
    // D4/E4.
    ObstacleObservation observation = sensor->inspect(current, teamPosition);
    AlertMetricEvent sensorEvent(alert.alertId, simTime(),
                                 observation.detected ? "detected" : "notDetected");
    emit(sensorEvaluationSignal, &sensorEvent);
    // A partir daqui a decisão é definitiva: o sensor foi consultado uma vez
    // para este alerta, como o gatilho prevê.
    if (!baEnabled)
        return true;

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
    // Modelo híbrido (D4/E4, §14): o ponto de obstáculo entra na aptidão só se
    // a câmera realmente o observou dentro de 30 m — sem observação, o termo
    // de obstáculo é zero e a aptidão fica com enlace e deslocamento. A
    // viabilidade das candidatas é testada pelo avaliador geométrico
    // idealizado do simulador, que é o que continua excluindo a posição atual
    // quando ela permanece obstruída.
    // distanceValid, e não detected: uma observação sem distância utilizável
    // não tem ponto de superfície para dar à aptidão. Usar o ponto mesmo assim
    // seria contrabandear a geometria exata do simulador para dentro do termo
    // de obstáculo.
    std::optional<Coord> obstaclePoint;
    if (observation.distanceValid)
        obstaclePoint = observation.nearestSurfacePoint;
    RepositionFitness fitness(fitnessParameters, sensor, current, teamPosition,
                              obstaclePoint,
                              neighborPositions, preserveConnectivity, simTime());
    BatResult result = BatAlgorithm::optimize(
        current, fitnessParameters.maximumRepositionDistance,
        // O RNG 1 é exclusivo do tratamento. O RNG 0 continua reservado ao
        // jitter operacional de TeamUpdate, preservando o pareamento dos braços.
        batParameters, getRNG(1),
        [&](const Coord& candidate) { return fitness.cost(candidate); },
        [&](const Coord& candidate) { return fitness.feasible(candidate); },
        [&](const Coord& candidate) { return fitness.inDomain(candidate); });
    EV_INFO << "BA result: current=" << current << " target=" << teamPosition
            << " candidate=" << result.position << " fitness=" << result.fitness
            << " valid=" << result.valid << "\n";
    if (!result.valid) {
        EV_DEBUG << "Reposition skipped: BA found no feasible solution\n";
        return true;
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
        return true;
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
        return true;
    }
    reposition.begin(alert.alertId);
    AlertMetricEvent startedEvent(alert.alertId, simTime(), "started", distance);
    emit(repositionEventSignal, &startedEvent);
    scheduleAt(simTime() + travelTime, movementCompleteTimer);
    return true;
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
    recordScalar("directRssiSamples", directRssiSamples);
    recordScalar("directRssiExcessLossSum", directRssiExcessLossSum);
    recordScalar("forwardedTeamUpdatesIgnoredForRssi", forwardedTeamUpdatesIgnoredForRssi);
    recordScalar("rssiDegradationIndications", rssiDegradationIndications);
    recordScalar("directUpdateTimeoutIndications", directUpdateTimeoutIndications);
    recordScalar("possibleObstructionIndications", possibleObstructionIndications);
    recordScalar("obstructionGateSuppressions", obstructionGateSuppressions);
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
