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

        // Portas únicas por drone para sockets de envio, necessárias para que o
        // MessageDispatcher do INET consiga rotear ICMPs de volta ao socket correto.
        // Faixa 9000–9059 (até 20 drones × 3 sockets): sem conflito com app ports 5000–5004.
        int idx = getParentModule()->getIndex();

        // Socket: envia DroneStatus unicast para equipe
        ackSocket.setOutputGate(gate("socketOut"));
        ackSocket.setCallback(this);
        ackSocket.bind(9000 + idx * 3);

        // Socket: envia VictimAlert unicast para equipe
        alertSocket.setOutputGate(gate("socketOut"));
        alertSocket.setCallback(this);
        alertSocket.bind(9001 + idx * 3);

        // Socket: recebe VictimAlert relay de outros drones (passo 8)
        relaySocket.setOutputGate(gate("socketOut"));
        relaySocket.setCallback(this);
        relaySocket.setBroadcast(true);
        relaySocket.bind(RELAY_PORT);

        // Socket: envia VictimAlert relay em broadcast para drones vizinhos (passo 9)
        fwdSocket.setOutputGate(gate("socketOut"));
        fwdSocket.setCallback(this);
        fwdSocket.setBroadcast(true);
        fwdSocket.bind(9002 + idx * 3);

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
    } else if (alertSocket.belongsToSocket(msg)) {
        alertSocket.processMessage(msg);  // consome ICMP errors do socket de envio
    } else if (ackSocket.belongsToSocket(msg)) {
        ackSocket.processMessage(msg);
    } else if (fwdSocket.belongsToSocket(msg)) {
        fwdSocket.processMessage(msg);
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
    e.posX      = chunk->getPosX();
    e.posY      = chunk->getPosY();
    e.available = chunk->getAvailable();
    e.lastSeen  = simTime();

    EV_INFO << "[DRONE " << myDroneId << "] tabela: " << chunk->getTeamId()
            << " ip=" << e.ip << " disponivel=" << e.available << "\n";

    // Passo 5: envia DroneStatus (ACK) unicast para a equipe
    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();

    auto resp = makeShared<DroneStatusChunk>();
    resp->setChunkLength(B(128));
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
    seenAlerts.insert(msgId);

    EV_INFO << "[DRONE " << myDroneId << "] alerta sintético gerado → " << msgId << "\n";

    PendingAlert pa;
    pa.msgId    = msgId;
    pa.droneId  = myDroneId;
    pa.originIp = myIp;
    pa.posX     = pos.x;
    pa.posY     = pos.y;
    pa.sentAt   = simTime();

    std::string sel = forwardAlertOnce(msgId, myDroneId, myIp, pos.x, pos.y,
                                       simTime(), pa.triedTeams);
    if (!sel.empty()) pa.triedTeams.insert(sel);

    pendingAlerts.push_back(std::move(pa));
    alertsGenerated++;
}

// ── Passos 10/11/9: seleciona UMA equipe (disponível mais próxima → qualquer
//    mais próxima → relay broadcast). Retorna o teamId escolhido ou "".
//
// Política de seleção única evita fan-out de múltiplos unicasts simultâneos,
// que saturariam o MAC e disparariam múltiplas descobertas de rota no AODV.
// Fallback sequencial: em retries, as equipes já tentadas ficam em 'exclude'.

std::string SimpleDroneApp::forwardAlertOnce(const std::string &msgId,
                                              const std::string &droneId,
                                              const std::string &originIp,
                                              double posX, double posY,
                                              simtime_t sentAt,
                                              const std::set<std::string> &exclude)
{
    auto chunk = makeShared<VictimAlertChunk>();
    chunk->setChunkLength(B(256));
    chunk->setDroneId(droneId.c_str());
    chunk->setMsgId(msgId.c_str());
    chunk->setOriginIp(originIp.c_str());
    chunk->setPosX(posX);
    chunk->setPosY(posY);
    chunk->setSentAt(sentAt);

    // Seleciona a melhor equipe: disponível + mais próxima; senão qualquer + mais próxima.
    // Usa distância ao quadrado para evitar sqrt (mesma ordenação, sem dependência extra).
    const TeamEntry *best  = nullptr;
    std::string      bestId;
    double           bestDist2 = 1e30;
    bool             bestAvail = false;

    for (auto& [id, e] : teamTable) {
        if (e.ip.empty() || exclude.count(id)) continue;
        double dx = e.posX - posX, dy = e.posY - posY;
        double d2 = dx*dx + dy*dy;
        bool win = !best
                || (e.available && !bestAvail)
                || (e.available == bestAvail && d2 < bestDist2);
        if (win) { best = &e; bestId = id; bestDist2 = d2; bestAvail = e.available; }
    }

    if (best) {
        alertsSentDirect++;
        alertSocket.sendTo(new Packet("VictimAlert", chunk),
                           Ipv4Address(best->ip.c_str()), ALERT_PORT);
        EV_INFO << "[DRONE " << myDroneId << "] VictimAlert " << msgId
                << " → " << bestId << " (" << best->ip
                << ") avail=" << bestAvail << "\n";
        return bestId;
    }

    // Nenhuma equipe elegível: relay broadcast para drones vizinhos
    alertsSentRelay++;
    fwdSocket.sendTo(new Packet("VictimAlert", chunk),
                     Ipv4Address::ALLONES_ADDRESS, RELAY_PORT);
    EV_INFO << "[DRONE " << myDroneId << "] relay: " << msgId
            << " → broadcast (sem equipe elegível)\n";
    return "";
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
    alertsRelayed++;

    EV_INFO << "[DRONE " << myDroneId << "] relay recebido: " << msgId
            << " de " << chunk->getDroneId() << "\n";

    // Passo 9: repassa para a melhor equipe conhecida (sem histórico de tentativas)
    static const std::set<std::string> noExclude;
    forwardAlertOnce(msgId,
                     chunk->getDroneId(),
                     chunk->getOriginIp(),
                     chunk->getPosX(),
                     chunk->getPosY(),
                     chunk->getSentAt(),
                     noExclude);
    delete pkt;
}

// ── Passo 12: recebe VictimAck da equipe, cancela retry do alerta ────────────

void SimpleDroneApp::handleVictimAck(Packet *pkt)
{
    auto chunk = pkt->peekAtFront<VictimAckChunk>();
    std::string msgId = chunk->getMsgId();

    EV_INFO << "[DRONE " << myDroneId << "] VictimAck recebido para " << msgId
            << " de " << chunk->getTeamId() << "\n";

    // Captura sentAt e acumula atraso E2E antes de remover
    for (const auto& p : pendingAlerts)
        if (p.msgId == msgId) {
            totalE2EDelay += simTime() - p.sentAt;
            alertsAcked++;
            break;
        }

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
            alertsExpired++;
            EV_WARN << "[DRONE " << myDroneId << "] store-forward: descartando "
                    << p.msgId << " após " << maxRetries << " tentativas\n";
            continue;
        }
        totalRetries++;
        // Se todas as equipes conhecidas já foram tentadas, recomeça o ciclo
        bool allTried = !p.triedTeams.empty();
        for (auto& [id, e] : teamTable)
            if (!p.triedTeams.count(id)) { allTried = false; break; }
        if (allTried) p.triedTeams.clear();

        EV_INFO << "[DRONE " << myDroneId << "] store-forward: retry "
                << p.retries << "/" << maxRetries << " para " << p.msgId << "\n";
        std::string sel = forwardAlertOnce(p.msgId, p.droneId, p.originIp,
                                           p.posX, p.posY, p.sentAt, p.triedTeams);
        if (!sel.empty()) p.triedTeams.insert(sel);
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

SimpleDroneApp::~SimpleDroneApp()
{
    cancelAndDelete(detectTimer);
    cancelAndDelete(timeoutTimer);
    cancelAndDelete(retryTimer);
}

void SimpleDroneApp::finish()
{
    recordScalar("alertsGenerated",  alertsGenerated);
    recordScalar("alertsSentDirect", alertsSentDirect);
    recordScalar("alertsSentRelay",  alertsSentRelay);
    recordScalar("alertsRelayed",    alertsRelayed);
    recordScalar("alertsAcked",      alertsAcked);
    recordScalar("alertsExpired",    alertsExpired);
    recordScalar("totalRetries",     totalRetries);
    // RTT do ciclo completo (alerta enviado → VictimAck recebido), lado do drone.
    // NÃO é o atraso de entrega 1-via — esse é medido na equipe (meanDeliveryDelay).
    recordScalar("totalRTT",         totalE2EDelay.dbl());
    recordScalar("meanRTT",
        alertsAcked > 0 ? totalE2EDelay.dbl() / alertsAcked : -1.0);
}

} // namespace echosar
