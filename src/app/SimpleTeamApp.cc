#include "SimpleTeamApp.h"

#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/contract/IInterfaceTable.h"

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

        sendSocket.setOutputGate(gate("socketOut"));
        sendSocket.setBroadcast(true);

        statusSocket.setOutputGate(gate("socketOut"));
        statusSocket.setCallback(this);
        statusSocket.bind(DRONE_STATUS_PORT);

        alertSocket.setOutputGate(gate("socketOut"));
        alertSocket.setCallback(this);
        alertSocket.bind(ALERT_PORT);

        sendTimer = new cMessage("sendTimer");
        scheduleAt(simTime() + sendInterval, sendTimer);
    }
}

void SimpleTeamApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        sendUpdate();
        scheduleAt(simTime() + sendInterval, sendTimer);
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
    chunk->setChunkLength(B(512));   // beacon de equipe: 512 B [FEA-2024]
    chunk->setTeamId(myTeamId.c_str());
    chunk->setIpAddress(myIp.c_str());
    chunk->setAvailable(true);
    chunk->setLat(pos.x);
    chunk->setLon(pos.y);

    sendSocket.sendTo(new Packet("TeamUpdate", chunk),
                      Ipv4Address::ALLONES_ADDRESS, TEAM_UPDATE_PORT);
    EV_INFO << "[TEAM " << myTeamId << "] TeamUpdate broadcast (ip=" << myIp << ")\n";
}

void SimpleTeamApp::socketDataArrived(UdpSocket *socket, Packet *pkt)
{
    if (socket == &statusSocket && pkt->hasAtFront<DroneStatusChunk>()) {
        auto chunk = pkt->peekAtFront<DroneStatusChunk>();
        EV_INFO << "[TEAM " << myTeamId << "] ACK de " << chunk->getDroneId()
                << " pos=(" << chunk->getPosX() << ","
                << chunk->getPosY() << "," << chunk->getPosZ()
                << ") RTT=" << (simTime() - chunk->getSentAt()) << "s\n";
    } else if (socket == &alertSocket && pkt->hasAtFront<VictimAlertChunk>()) {
        auto chunk = pkt->peekAtFront<VictimAlertChunk>();
        EV_INFO << "[TEAM " << myTeamId << "] *** ALERTA de " << chunk->getDroneId()
                << " vitima em (" << chunk->getLat() << "," << chunk->getLon()
                << ") delay=" << (simTime() - chunk->getSentAt()) << "s\n";
    } else {
        EV_WARN << "[TEAM " << myTeamId << "] pacote inesperado descartado\n";
    }
    delete pkt;
}

void SimpleTeamApp::finish()
{
    cancelAndDelete(sendTimer);
}

} // namespace echosar
