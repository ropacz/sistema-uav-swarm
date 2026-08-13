#include "SimpleTeamApp.h"

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
        beaconJitter = par("beaconJitter");
        deliveryDelaySignal = registerSignal("deliveryDelay");
        if (beaconJitter == 0)
            throw cRuntimeError("%s: beaconJitter=0 causa colisões MAC com múltiplas equipes "
                                "(todas transmitem em t=sendInterval simultaneamente → PDR ~3%%). "
                                "Use beaconJitter = sendInterval.", getFullPath().c_str());
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

        // Faixa 9100–9115 (até 8 equipes × 2 sockets): sem conflito com outros ports.
        int idx = getParentModule()->getIndex();

        // Socket: envia TeamUpdate em broadcast
        sendSocket.setOutputGate(gate("socketOut"));
        sendSocket.setCallback(this);
        sendSocket.setBroadcast(true);
        sendSocket.bind(9100 + idx * 2);

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
        ackTxSocket.setCallback(this);
        ackTxSocket.bind(9101 + idx * 2);

        sendTimer = new cMessage("sendTimer");
        scheduleAt(simTime() + uniform(0, beaconJitter), sendTimer);
    }
}

void SimpleTeamApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == sendTimer) {
            sendUpdate();
            scheduleAt(simTime() + sendInterval, sendTimer);
        }
    } else if (statusSocket.belongsToSocket(msg)) {
        statusSocket.processMessage(msg);
    } else if (alertSocket.belongsToSocket(msg)) {
        alertSocket.processMessage(msg);
    } else if (sendSocket.belongsToSocket(msg)) {
        sendSocket.processMessage(msg);
    } else if (ackTxSocket.belongsToSocket(msg)) {
        ackTxSocket.processMessage(msg);
    } else {
        delete msg;
    }
}

void SimpleTeamApp::sendUpdate()
{
    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();

    auto chunk = makeShared<TeamUpdateChunk>();
    chunk->setChunkLength(B(128));
    chunk->setTeamId(myTeamId.c_str());
    chunk->setIpAddress(myIp.c_str());
    chunk->setPosX(pos.x);
    chunk->setPosY(pos.y);

    sendSocket.sendTo(new Packet("TeamUpdate", chunk),
                      Ipv4Address::ALLONES_ADDRESS, TEAM_UPDATE_PORT);
    teamUpdatesSent++;
    EV_INFO << "[TEAM " << myTeamId << "] TeamUpdate broadcast (ip=" << myIp << ")\n";
}

void SimpleTeamApp::socketDataArrived(UdpSocket *socket, Packet *pkt)
{
    if (socket == &statusSocket && pkt->hasAtFront<DroneStatusChunk>()) {
        // Passo 5: recebe DroneStatus (ACK de posição)
        auto chunk = pkt->peekAtFront<DroneStatusChunk>();
        droneStatusReceived++;
        EV_INFO << "[TEAM " << myTeamId << "] DroneStatus de " << chunk->getDroneId()
                << " pos=(" << chunk->getPosX() << ","
                << chunk->getPosY() << "," << chunk->getPosZ()
                << ") RTT=" << (simTime() - chunk->getSentAt()) << "s\n";

    } else if (socket == &alertSocket && pkt->hasAtFront<VictimAlertChunk>()) {
        // Passo 11: recebe VictimAlert de drone (direto ou via relay)
        auto chunk   = pkt->peekAtFront<VictimAlertChunk>();
        std::string msgId = chunk->getMsgId();

        if (seenAlerts.count(msgId)) {
            // Duplicata — reenviar ACK sem reprocessar métricas nem atendimento.
            // Necessário porque o primeiro ACK pode ter sido perdido em trânsito;
            // sem o reenvio, o drone retransmite até expirar mesmo já entregue.
            EV_INFO << "[TEAM " << myTeamId << "] dedup: " << msgId
                    << " — reenviando ACK idempotente\n";
            std::string originIp = chunk->getOriginIp();
            if (!originIp.empty()) {
                auto ack = makeShared<VictimAckChunk>();
                ack->setChunkLength(B(64));
                ack->setMsgId(msgId.c_str());
                ack->setTeamId(myTeamId.c_str());
                ack->setSentAt(simTime());
                ackTxSocket.sendTo(new Packet("VictimAck", ack),
                                   Ipv4Address(originIp.c_str()), ACK_PORT);
            }
            delete pkt;
            return;
        }
        seenAlerts.insert(msgId);
        alertsReceived++;
        simtime_t delay = simTime() - chunk->getSentAt();
        totalDeliveryDelay += delay;
        emit(deliveryDelaySignal, delay);

        EV_INFO << "[TEAM " << myTeamId << "] *** ALERTA de " << chunk->getDroneId()
                << " msgId=" << msgId
                << " vitima em (" << chunk->getPosX() << "," << chunk->getPosY()
                << ") delay=" << delay << "s\n";

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

SimpleTeamApp::~SimpleTeamApp()
{
    cancelAndDelete(sendTimer);
}

void SimpleTeamApp::finish()
{
    recordScalar("alertsReceived",          alertsReceived);
    recordScalar("teamUpdatesSent",         teamUpdatesSent);
    recordScalar("droneStatusReceived",     droneStatusReceived);
    // Soma bruta dos atrasos de entrega (1 via) — permite média global ponderada
    recordScalar("totalDeliveryDelay",      totalDeliveryDelay.dbl());
    recordScalar("meanDeliveryDelay",
        alertsReceived > 0 ? totalDeliveryDelay.dbl() / alertsReceived : -1.0);
}

} // namespace echosar
