#include "SimpleDroneApp.h"

#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(SimpleDroneApp);

void SimpleDroneApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        myDroneId      = par("myDroneId").stdstringValue();
        victimInterval = par("victimInterval");
        teamTimeout    = par("teamTimeout");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        if (myDroneId.empty())
            myDroneId = getParentModule()->getFullName();

        teamSocket.setOutputGate(gate("socketOut"));
        teamSocket.setCallback(this);
        teamSocket.setBroadcast(true);
        teamSocket.bind(TEAM_UPDATE_PORT);

        ackSocket.setOutputGate(gate("socketOut"));

        alertSocket.setOutputGate(gate("socketOut"));

        detectTimer  = new cMessage("detect");
        timeoutTimer = new cMessage("timeout");
        scheduleAt(simTime() + exponential(victimInterval), detectTimer);
        scheduleAt(simTime() + teamTimeout, timeoutTimer);
    }
}

void SimpleDroneApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == detectTimer) {
            detectVictim();
            scheduleAt(simTime() + exponential(victimInterval), detectTimer);
        } else {
            checkTimeouts();
            scheduleAt(simTime() + teamTimeout, timeoutTimer);
        }
    } else if (teamSocket.belongsToSocket(msg)) {
        teamSocket.processMessage(msg);
    } else {
        delete msg;
    }
}

void SimpleDroneApp::socketDataArrived(UdpSocket *socket, Packet *pkt)
{
    if (socket == &teamSocket && pkt->hasAtFront<TeamUpdateChunk>())
        handleTeamUpdate(pkt);
    else
        delete pkt;
}

void SimpleDroneApp::handleTeamUpdate(Packet *pkt)
{
    auto chunk   = pkt->peekAtFront<TeamUpdateChunk>();
    auto srcAddr = pkt->getTag<L3AddressInd>()->getSrcAddress();

    auto& e    = teamTable[chunk->getTeamId()];
    e.ip        = chunk->getIpAddress();
    e.lat       = chunk->getLat();
    e.lon       = chunk->getLon();
    e.available = chunk->getAvailable();
    e.lastSeen  = simTime();

    EV_INFO << "[DRONE " << myDroneId << "] tabela: " << chunk->getTeamId()
            << " ip=" << e.ip << " disponivel=" << e.available << "\n";

    // ACK com posição do drone
    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();

    auto resp = makeShared<DroneStatusChunk>();
    resp->setChunkLength(B(512));   // telemetria de drone: 512 B [FEA-2024]
    resp->setDroneId(myDroneId.c_str());
    resp->setSentAt(simTime());
    resp->setPosX(pos.x);
    resp->setPosY(pos.y);
    resp->setPosZ(pos.z);

    ackSocket.sendTo(new Packet("DroneStatus", resp),
                     srcAddr.toIpv4(), DRONE_STATUS_PORT);
    delete pkt;
}

void SimpleDroneApp::detectVictim()
{
    std::string teamIp;
    for (auto& [id, e] : teamTable)
        if (e.available && !e.ip.empty()) { teamIp = e.ip; break; }

    if (teamIp.empty()) {
        EV_INFO << "[DRONE " << myDroneId << "] vitima detectada, sem equipe disponivel na tabela\n";
        return;
    }

    std::string msgId = myDroneId + "_" + std::to_string(++victimCounter);

    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();

    auto chunk = makeShared<VictimAlertChunk>();
    chunk->setChunkLength(B(1024));  // alerta de vítima: 1024 B [FEA-2024]
    chunk->setDroneId(myDroneId.c_str());
    chunk->setMsgId(msgId.c_str());
    chunk->setLat(pos.x);
    chunk->setLon(pos.y);
    chunk->setSentAt(simTime());

    alertSocket.sendTo(new Packet("VictimAlert", chunk),
                       Ipv4Address(teamIp.c_str()), ALERT_PORT);

    EV_INFO << "[DRONE " << myDroneId << "] VictimAlert " << msgId
            << " → " << teamIp << "\n";
}

void SimpleDroneApp::checkTimeouts()
{
    simtime_t now = simTime();
    for (auto it = teamTable.begin(); it != teamTable.end(); ) {
        if (now - it->second.lastSeen > teamTimeout) {
            EV_INFO << "[DRONE " << myDroneId << "] timeout: removendo " << it->first << "\n";
            it = teamTable.erase(it);
        } else {
            ++it;
        }
    }
}

void SimpleDroneApp::finish()
{
    cancelAndDelete(detectTimer);
    cancelAndDelete(timeoutTimer);
}

} // namespace echosar
