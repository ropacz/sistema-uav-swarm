#include "SimpleTeamApp.h"

#include <cmath>

#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(SimpleTeamApp);

void SimpleTeamApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        myTeamId     = par("myTeamId").stdstringValue();
        sendInterval = par("sendInterval");
        teamSpeed    = par("teamSpeed");
        serviceTime  = par("serviceTime");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        if (myTeamId.empty())
            myTeamId = getParentModule()->getFullName();

        // Descobre IP próprio
        auto *ift = L3AddressResolver().findInterfaceTableOf(getParentModule());
        for (int i = 0; i < ift->getNumInterfaces(); i++) {
            auto *iface = ift->getInterface(i);
            if (iface->isLoopback()) continue;
            auto *ipd = iface->findProtocolData<Ipv4InterfaceData>();
            if (ipd && !ipd->getIPAddress().isUnspecified()) {
                myIp = ipd->getIPAddress().str();
                break;
            }
        }

        // Socket: envia TeamUpdate em broadcast
        sendSocket.setOutputGate(gate("socketOut"));
        sendSocket.setBroadcast(true);

        // Socket: recebe DroneStatus (ACK de posição do drone)
        statusSocket.setOutputGate(gate("socketOut"));
        statusSocket.setCallback(this);
        statusSocket.bind(DRONE_STATUS_PORT);

        // Socket: recebe VictimAlert do drone
        alertSocket.setOutputGate(gate("socketOut"));
        alertSocket.setCallback(this);
        alertSocket.bind(ALERT_PORT);

        // Socket: envia VictimAck unicast de volta ao drone origem (passo 12)
        ackTxSocket.setOutputGate(gate("socketOut"));

        sendTimer   = new cMessage("sendTimer");
        attendTimer = new cMessage("attendTimer");
        scheduleAt(simTime() + sendInterval, sendTimer);
    }
}

void SimpleTeamApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == sendTimer) {
            sendUpdate();
            scheduleAt(simTime() + sendInterval, sendTimer);
        } else if (msg == attendTimer) {
            available = true;
            EV_INFO << "[TEAM " << myTeamId << "] atendimento concluído → disponível\n";
        }
    } else if (statusSocket.belongsToSocket(msg)) {
        statusSocket.processMessage(msg);
    } else if (alertSocket.belongsToSocket(msg)) {
        alertSocket.processMessage(msg);
    } else {
        delete msg;
    }
}

void SimpleTeamApp::sendUpdate()
{
    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();

    auto chunk = makeShared<TeamUpdateChunk>();
    chunk->setChunkLength(B(512));
    chunk->setTeamId(myTeamId.c_str());
    chunk->setIpAddress(myIp.c_str());
    chunk->setAvailable(available);
    chunk->setLat(pos.x);
    chunk->setLon(pos.y);

    sendSocket.sendTo(new Packet("TeamUpdate", chunk),
                      Ipv4Address::ALLONES_ADDRESS, TEAM_UPDATE_PORT);
    EV_INFO << "[TEAM " << myTeamId << "] TeamUpdate broadcast (ip=" << myIp << ")\n";
}

void SimpleTeamApp::socketDataArrived(UdpSocket *socket, Packet *pkt)
{
    if (socket == &statusSocket && pkt->hasAtFront<DroneStatusChunk>()) {
        // Passo 5: recebe DroneStatus (ACK de posição)
        auto chunk = pkt->peekAtFront<DroneStatusChunk>();
        EV_INFO << "[TEAM " << myTeamId << "] DroneStatus de " << chunk->getDroneId()
                << " pos=(" << chunk->getPosX() << ","
                << chunk->getPosY() << "," << chunk->getPosZ()
                << ") RTT=" << (simTime() - chunk->getSentAt()) << "s\n";

    } else if (socket == &alertSocket && pkt->hasAtFront<VictimAlertChunk>()) {
        // Passo 11: recebe VictimAlert de drone (direto ou via relay)
        auto chunk   = pkt->peekAtFront<VictimAlertChunk>();
        std::string msgId = chunk->getMsgId();

        if (seenAlerts.count(msgId)) {
            // Duplicata — descarta silenciosamente (já processado)
            EV_INFO << "[TEAM " << myTeamId << "] dedup: VictimAlert duplicado "
                    << msgId << " descartado\n";
            delete pkt;
            return;
        }
        seenAlerts.insert(msgId);

        // Calcula distância e tempo de deslocamento até a vítima
        auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
        Coord myPos = mob->getCurrentPosition();
        double dx       = chunk->getLat() - myPos.x;
        double dy       = chunk->getLon() - myPos.y;
        double distance = std::sqrt(dx*dx + dy*dy);
        double travelTime   = distance / teamSpeed;
        double busyDuration = travelTime + serviceTime;

        EV_INFO << "[TEAM " << myTeamId << "] *** ALERTA de " << chunk->getDroneId()
                << " msgId=" << msgId
                << " vitima em (" << chunk->getLat() << "," << chunk->getLon()
                << ") dist=" << distance << "m"
                << " travel=" << travelTime << "s"
                << " busy=" << busyDuration << "s"
                << " delay=" << (simTime() - chunk->getSentAt()) << "s\n";

        // Muda status para ocupada apenas se estava disponível
        if (available) {
            available = false;
            scheduleAt(simTime() + busyDuration, attendTimer);
            EV_INFO << "[TEAM " << myTeamId << "] → OCUPADA por " << busyDuration
                    << "s (retorna livre em t=" << (simTime() + busyDuration) << "s)\n";
        } else {
            EV_INFO << "[TEAM " << myTeamId << "] → já OCUPADA, alerta registrado\n";
        }

        // Passo 12: envia VictimAck para o drone ORIGEM do alerta (via originIp)
        std::string originIp = chunk->getOriginIp();
        if (!originIp.empty()) {
            auto ack = makeShared<VictimAckChunk>();
            ack->setChunkLength(B(64));
            ack->setMsgId(msgId.c_str());
            ack->setTeamId(myTeamId.c_str());
            ack->setSentAt(simTime());

            ackTxSocket.sendTo(new Packet("VictimAck", ack),
                               Ipv4Address(originIp.c_str()), ACK_PORT);
            EV_INFO << "[TEAM " << myTeamId << "] VictimAck " << msgId
                    << " → drone origem " << originIp << "\n";
        }

    } else {
        EV_WARN << "[TEAM " << myTeamId << "] pacote inesperado descartado\n";
    }
    delete pkt;
}

void SimpleTeamApp::finish()
{
    cancelAndDelete(sendTimer);
    cancelAndDelete(attendTimer);
}

} // namespace echosar
