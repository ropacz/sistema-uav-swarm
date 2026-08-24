#include "TeamApp.h"

#include <cstring>

#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
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
        positionUpdatePayloadBytes = par("positionUpdatePayloadBytes");
        victimAckPayloadBytes = par("victimAckPayloadBytes");
        updateInterval = par("positionUpdateInterval");
        initialJitter = par("initialJitter");
        ackStartTime = par("ackStartTime");
        int applicationIpTtl = par("applicationIpTtl");
        if (teamId.empty())
            teamId = getParentModule()->getFullName();
        if (teamId.empty() || appPort <= 0 || appPort > 65535 ||
            positionUpdatePayloadBytes <= 0 || victimAckPayloadBytes <= 0 ||
            updateInterval <= 0 || initialJitter < 0 || ackStartTime < 0 ||
            applicationIpTtl <= 0 || applicationIpTtl > 255)
            throw cRuntimeError("Invalid team identity, timing, or IP TTL parameter");
        alertDeliveredSignal = registerSignal("victimAlertDelivered");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
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
            throw cRuntimeError("Team '%s' has no configured IPv4 address", teamId.c_str());
        socket.setOutputGate(gate("socketOut"));
        socket.setCallback(this);
        socket.setBroadcast(true);
        socket.bind(appPort);
        socket.setTimeToLive(par("applicationIpTtl"));
        updateTimer = new cMessage("positionUpdateTimer");
        // O jitter evita que todas as equipes iniciem broadcasts no mesmo instante.
        scheduleAt(simTime() + (initialJitter == 0 ? 0 : uniform(0, initialJitter.dbl())), updateTimer);
    }
}

void TeamApp::handleMessageWhenUp(cMessage *message)
{
    if (message == updateTimer) {
        sendPositionUpdate();
        scheduleAt(simTime() + updateInterval, updateTimer);
    }
    else if (socket.belongsToSocket(message))
        socket.processMessage(message);
    else
        delete message;
}

void TeamApp::sendPositionUpdate()
{
    auto mobility = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord position = mobility->getCurrentPosition();
    auto update = makeShared<PositionUpdateChunk>();
    update->setChunkLength(B(positionUpdatePayloadBytes));
    update->setMessageId((teamId + "-pos-" + std::to_string(++updateSequence)).c_str());
    update->setSenderId(teamId.c_str());
    update->setSenderType("team");
    update->setIpAddress(ipAddress.c_str());
    // A trajetória da equipe não expõe o waypoint interno; -1 significa indisponível.
    update->setWaypointId(-1);
    update->setPositionX(position.x);
    update->setPositionY(position.y);
    update->setPositionZ(position.z);
    update->setSequenceNumber(updateSequence);
    update->setTimestamp(simTime());
    update->setOperationalState("mobile");
    socket.sendTo(new Packet("PositionUpdate", update),
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
    std::string alertId = alert->getAlertId();
    std::string messageId = alert->getMessageId();
    bool invalid = alertId.empty() || messageId.empty() || std::string(alert->getVictimId()).empty() ||
        std::string(alert->getOriginDroneId()).empty() || std::string(alert->getOriginDroneAddress()).empty() ||
        alert->getAttemptNumber() <= 0 ||
        alert->getTimeToLive() <= SIMTIME_ZERO || alert->getGenerationTimestamp() > simTime() ||
        alert->getTransmissionTimestamp() < alert->getGenerationTimestamp() ||
        alert->getTransmissionTimestamp() > simTime();
    if (invalid) {
        delete packet;
        return;
    }
    if (simTime() - alert->getGenerationTimestamp() >= alert->getTimeToLive()) {
        delete packet;
        return;
    }

    bool newAttempt = receivedAttempts.insert(messageId).second;
    // messageId deduplica uma tentativa que a pilha possa entregar novamente.
    // A deduplicacao fim a fim por alertId pertence ao coletor central.
    if (newAttempt) {
        AlertMetricEvent deliveredEvent(alertId,
                                        alert->getGenerationTimestamp());
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
    ack->setReceivedMessageId(alert->getMessageId());
    ack->setVictimId(alert->getVictimId());
    ack->setTeamId(teamId.c_str());
    ack->setOriginDroneId(alert->getOriginDroneId());
    ack->setReceptionTimestamp(simTime());
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
