#include "TeamApp.h"

#include <cmath>
#include <cstring>

#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "metrics/AlertMetricEvent.h"

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(TeamApp);

void TeamApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        teamId = par("teamId").stdstringValue();
        appPort = par("appPort");
        teamUpdatePayloadBytes = par("teamUpdatePayloadBytes");
        victimAckPayloadBytes = par("victimAckPayloadBytes");
        updateInterval = par("teamUpdateInterval");
        initialJitter = par("initialJitter");
        ackStartTime = par("ackStartTime");
        applicationIpTtl = par("applicationIpTtl");
        if (teamId.empty())
            teamId = getParentModule()->getFullName();
        if (teamId.empty() || appPort <= 0 || appPort > 65535 ||
            teamUpdatePayloadBytes <= 0 || victimAckPayloadBytes <= 0 ||
            updateInterval <= 0 || initialJitter < 0 || ackStartTime < 0 ||
            applicationIpTtl <= 0 || applicationIpTtl > 255)
            throw cRuntimeError("Invalid team identity, timing, or IP TTL parameter");
        alertDeliveredSignal = registerSignal("victimAlertDelivered");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        socket.setOutputGate(gate("socketOut"));
        socket.setCallback(this);
        socket.setBroadcast(true);
        socket.bind(appPort);
        socket.setTimeToLive(applicationIpTtl);
        updateTimer = new cMessage("teamUpdateTimer");
        // O jitter evita que todas as equipes iniciem broadcasts no mesmo instante.
        scheduleAt(simTime() + (initialJitter == 0 ? 0 : uniform(0, initialJitter.dbl())), updateTimer);
    }
}

void TeamApp::handleMessageWhenUp(cMessage *message)
{
    if (message == updateTimer) {
        sendTeamUpdate();
        scheduleAt(simTime() + updateInterval, updateTimer);
    }
    else if (socket.belongsToSocket(message))
        socket.processMessage(message);
    else
        delete message;
}

void TeamApp::sendTeamUpdate()
{
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord position = mobility->getCurrentPosition();
    auto update = makeShared<TeamUpdateChunk>();
    update->setChunkLength(B(teamUpdatePayloadBytes));
    update->setTeamId(teamId.c_str());
    update->setMessageId((teamId + "-update-" + std::to_string(updateSequence + 1)).c_str());
    update->setSequenceNumber(++updateSequence);
    update->setPositionX(position.x);
    update->setPositionY(position.y);
    update->setPositionZ(position.z);
    update->setTimestamp(simTime());
    socket.sendTo(new Packet("TeamUpdate", update),
                  Ipv4Address::ALLONES_ADDRESS, appPort);
}

void TeamApp::socketDataArrived(UdpSocket *, Packet *packet)
{
    if (!strcmp(packet->getName(), "VictimAlert"))
        handleVictimAlert(packet);
    else
        delete packet;
}

void TeamApp::handleVictimAlert(Packet *packet)
{
    auto alert = packet->peekAtFront<VictimAlertChunk>();
    auto hopLimit = packet->findTag<HopLimitInd>();
    std::string alertId = alert->getAlertId();
    std::string messageId = alert->getMessageId();
    bool invalid = alertId.empty() || messageId.empty() ||
        std::string(alert->getVictimId()).empty() ||
        std::string(alert->getSourceDroneId()).empty() ||
        std::string(alert->getTargetTeamId()) != teamId ||
        !std::isfinite(alert->getVictimPositionX()) ||
        !std::isfinite(alert->getVictimPositionY()) ||
        !std::isfinite(alert->getVictimPositionZ()) ||
        !hopLimit || hopLimit->getHopLimit() <= 0 ||
        hopLimit->getHopLimit() > applicationIpTtl ||
        alert->getAttemptNumber() <= 0 ||
        alert->getTimeToLive() <= SIMTIME_ZERO ||
        alert->getCreationTime() > simTime();
    if (invalid) {
        delete packet;
        return;
    }
    if (simTime() - alert->getCreationTime() >= alert->getTimeToLive()) {
        delete packet;
        return;
    }

    bool newAttempt = receivedAttempts.insert(messageId).second;
    // messageId deduplica uma tentativa que a pilha possa entregar novamente.
    // A deduplicacao fim a fim por alertId pertence ao coletor central.
    if (newAttempt) {
        int hopCount = applicationIpTtl - hopLimit->getHopLimit() + 1;
        AlertMetricEvent deliveredEvent(alertId,
                                        alert->getCreationTime(), "", hopCount,
                                        messageId);
        emit(alertDeliveredSignal, &deliveredEvent);
    }

    // Janela de injeção determinística: antes de ackStartTime a equipe recebe
    // e contabiliza o alerta, mas não confirma.
    if (simTime() < ackStartTime) {
        delete packet;
        return;
    }
    auto ack = makeShared<VictimAckChunk>();
    // Mesmo uma tentativa duplicada recebe novo ACK para encerrar o retry no drone.
    ack->setChunkLength(B(victimAckPayloadBytes));
    ack->setAlertId(alert->getAlertId());
    ack->setMessageId(alert->getMessageId());
    ack->setVictimId(alert->getVictimId());
    ack->setSourceDroneId(alert->getSourceDroneId());
    ack->setTeamId(teamId.c_str());
    ack->setAckTimestamp(simTime());
    auto sourceAddress = packet->getTag<L3AddressInd>()->getSrcAddress();
    socket.sendTo(new Packet("VictimAck", ack), sourceAddress, appPort);
    delete packet;
}

TeamApp::~TeamApp()
{
    cancelAndDelete(updateTimer);
}

} // namespace echosar
