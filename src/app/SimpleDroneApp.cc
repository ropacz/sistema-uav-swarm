#include "SimpleDroneApp.h"

#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/contract/IInterfaceTable.h"

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(SimpleDroneApp);

void SimpleDroneApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        myDroneId     = par("myDroneId").stdstringValue();
        victimInterval = par("victimInterval");
        teamTimeout    = par("teamTimeout");
        retryInterval  = par("retryInterval");
        maxRetries     = par("maxRetries");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        if (myDroneId.empty())
            myDroneId = getParentModule()->getFullName();

        // Descobre próprio IP (necessário para preencher originIp no VictimAlert)
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

        // Socket: recebe TeamUpdate em broadcast da equipe
        teamSocket.setOutputGate(gate("socketOut"));
        teamSocket.setCallback(this);
        teamSocket.setBroadcast(true);
        teamSocket.bind(TEAM_UPDATE_PORT);

        // Socket: envia DroneStatus (ACK) unicast para equipe
        ackSocket.setOutputGate(gate("socketOut"));

        // Socket: envia VictimAlert unicast para equipe
        alertSocket.setOutputGate(gate("socketOut"));

        // Socket: recebe VictimAlert relay de outros drones (passo 8)
        relaySocket.setOutputGate(gate("socketOut"));
        relaySocket.setCallback(this);
        relaySocket.setBroadcast(true);
        relaySocket.bind(RELAY_PORT);

        // Socket: envia VictimAlert relay em broadcast para drones vizinhos (passo 9)
        fwdSocket.setOutputGate(gate("socketOut"));
        fwdSocket.setBroadcast(true);

        // Socket: recebe VictimAck da equipe (passo 12)
        ackRxSocket.setOutputGate(gate("socketOut"));
        ackRxSocket.setCallback(this);
        ackRxSocket.bind(ACK_PORT);

        detectTimer  = new cMessage("detect");
        timeoutTimer = new cMessage("timeout");
        retryTimer   = new cMessage("retry");

        scheduleAt(simTime() + exponential(victimInterval), detectTimer);
        scheduleAt(simTime() + teamTimeout,                 timeoutTimer);
        scheduleAt(simTime() + retryInterval,               retryTimer);
    }
}

void SimpleDroneApp::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (msg == detectTimer) {
            detectVictim();
            scheduleAt(simTime() + exponential(victimInterval), detectTimer);
        } else if (msg == timeoutTimer) {
            checkTimeouts();
            scheduleAt(simTime() + teamTimeout, timeoutTimer);
        } else if (msg == retryTimer) {
            retryPending();
            scheduleAt(simTime() + retryInterval, retryTimer);
        }
    } else if (teamSocket.belongsToSocket(msg)) {
        teamSocket.processMessage(msg);
    } else if (relaySocket.belongsToSocket(msg)) {
        relaySocket.processMessage(msg);
    } else if (ackRxSocket.belongsToSocket(msg)) {
        ackRxSocket.processMessage(msg);
    } else {
        delete msg;
    }
}

void SimpleDroneApp::socketDataArrived(UdpSocket *socket, Packet *pkt)
{
    if (socket == &teamSocket && pkt->hasAtFront<TeamUpdateChunk>())
        handleTeamUpdate(pkt);
    else if (socket == &relaySocket && pkt->hasAtFront<VictimAlertChunk>())
        handleVictimAlertRelay(pkt);
    else if (socket == &ackRxSocket && pkt->hasAtFront<VictimAckChunk>())
        handleVictimAck(pkt);
    else
        delete pkt;
}

// ── Passo 3/4: recebe TeamUpdate, atualiza tabela, envia DroneStatus ─────────

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

    // Passo 5: envia DroneStatus (ACK) unicast para a equipe
    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();

    auto resp = makeShared<DroneStatusChunk>();
    resp->setChunkLength(B(512));
    resp->setDroneId(myDroneId.c_str());
    resp->setSentAt(simTime());
    resp->setPosX(pos.x);
    resp->setPosY(pos.y);
    resp->setPosZ(pos.z);

    ackSocket.sendTo(new Packet("DroneStatus", resp),
                     srcAddr.toIpv4(), DRONE_STATUS_PORT);
    delete pkt;
}

// ── Passo 7: drone detecta vítima e inicia o fluxo de alerta ─────────────────

void SimpleDroneApp::detectVictim()
{
    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();

    std::string msgId = myDroneId + "_" + std::to_string(++victimCounter);
    seenAlerts.insert(msgId);  // marca como originado (não retransmitir relay de volta)

    EV_INFO << "[DRONE " << myDroneId << "] vitima detectada → " << msgId << "\n";

    forwardAlertOnce(msgId, myDroneId, myIp, pos.x, pos.y, simTime());

    // Passo 15: guarda para retry até receber VictimAck da equipe
    pendingAlerts.push_back({msgId, myDroneId, myIp, pos.x, pos.y, simTime(), 0});
}

// ── Passos 10/11/9: tenta equipe direta; senão relay broadcast ───────────────

void SimpleDroneApp::forwardAlertOnce(const std::string &msgId,
                                      const std::string &droneId,
                                      const std::string &originIp,
                                      double lat, double lon,
                                      simtime_t sentAt)
{
    // Passo 10: verifica tabela por equipe disponível e alcançável
    std::string teamIp;
    for (auto& [id, e] : teamTable)
        if (e.available && !e.ip.empty()) { teamIp = e.ip; break; }

    auto chunk = makeShared<VictimAlertChunk>();
    chunk->setChunkLength(B(1024));
    chunk->setDroneId(droneId.c_str());
    chunk->setMsgId(msgId.c_str());
    chunk->setOriginIp(originIp.c_str());
    chunk->setLat(lat);
    chunk->setLon(lon);
    chunk->setSentAt(sentAt);

    if (!teamIp.empty()) {
        // Passo 11: unicast direto para equipe
        alertSocket.sendTo(new Packet("VictimAlert", chunk),
                           Ipv4Address(teamIp.c_str()), ALERT_PORT);
        EV_INFO << "[DRONE " << myDroneId << "] VictimAlert " << msgId
                << " → equipe " << teamIp << "\n";
    } else {
        // Passo 9: relay broadcast para drones vizinhos
        fwdSocket.sendTo(new Packet("VictimAlert", chunk),
                         Ipv4Address::ALLONES_ADDRESS, RELAY_PORT);
        EV_INFO << "[DRONE " << myDroneId << "] relay: " << msgId
                << " → broadcast (sem equipe na tabela)\n";
    }
}

// ── Passos 8/9: recebe VictimAlert de outro drone, deduplica e repassa ───────

void SimpleDroneApp::handleVictimAlertRelay(Packet *pkt)
{
    auto chunk = pkt->peekAtFront<VictimAlertChunk>();
    std::string msgId = chunk->getMsgId();

    // Passo 8: deduplicação — descarta se já visto
    if (seenAlerts.count(msgId)) {
        EV_INFO << "[DRONE " << myDroneId << "] dedup: descartando " << msgId << "\n";
        delete pkt;
        return;
    }
    seenAlerts.insert(msgId);

    EV_INFO << "[DRONE " << myDroneId << "] relay recebido: " << msgId
            << " de " << chunk->getDroneId() << "\n";

    // Passo 9: repassa (relay não adiciona a pendingAlerts — quem originou é responsável)
    forwardAlertOnce(msgId,
                     chunk->getDroneId(),
                     chunk->getOriginIp(),
                     chunk->getLat(),
                     chunk->getLon(),
                     chunk->getSentAt());
    delete pkt;
}

// ── Passo 12: recebe VictimAck da equipe, cancela retry do alerta ────────────

void SimpleDroneApp::handleVictimAck(Packet *pkt)
{
    auto chunk = pkt->peekAtFront<VictimAckChunk>();
    std::string msgId = chunk->getMsgId();

    EV_INFO << "[DRONE " << myDroneId << "] VictimAck recebido para " << msgId
            << " de " << chunk->getTeamId() << "\n";

    // Remove de pendingAlerts — interrompe retries
    auto it = std::remove_if(pendingAlerts.begin(), pendingAlerts.end(),
                              [&](const PendingAlert& p){ return p.msgId == msgId; });
    pendingAlerts.erase(it, pendingAlerts.end());

    delete pkt;
}

// ── Passo 15: retry de alertas pendentes ─────────────────────────────────────

void SimpleDroneApp::retryPending()
{
    if (pendingAlerts.empty()) return;

    std::vector<PendingAlert> next;
    for (auto& p : pendingAlerts) {
        p.retries++;
        if (p.retries > maxRetries) {
            EV_WARN << "[DRONE " << myDroneId << "] store-forward: descartando "
                    << p.msgId << " após " << maxRetries << " tentativas\n";
            continue;
        }
        EV_INFO << "[DRONE " << myDroneId << "] store-forward: retry "
                << p.retries << "/" << maxRetries << " para " << p.msgId << "\n";
        forwardAlertOnce(p.msgId, p.droneId, p.originIp, p.lat, p.lon, p.sentAt);
        next.push_back(p);
    }
    pendingAlerts = std::move(next);
}

// ── Passo 13: remove equipes inativas da tabela ───────────────────────────────

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
    cancelAndDelete(retryTimer);
}

} // namespace echosar
