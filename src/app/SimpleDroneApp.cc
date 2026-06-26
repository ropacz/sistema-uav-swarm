#include "SimpleDroneApp.h"

#include <cmath>

#include "inet/mobility/contract/IMobility.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "inet/networklayer/contract/ipv4/Ipv4Address.h"
#include "inet/networklayer/ipv4/Ipv4InterfaceData.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
#include "control/ObstacleSensor.h"
#include "mobility/BatControlledMobility.h"

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(SimpleDroneApp);

// ── Ciclo de vida ─────────────────────────────────────────────────────────────

void SimpleDroneApp::initialize(int stage)
{
    ApplicationBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        myDroneId     = par("myDroneId").stdstringValue();
        victimInterval = par("victimInterval");
        teamTimeout    = par("teamTimeout");
        retryInterval  = par("retryInterval");
        maxRetries     = par("maxRetries");

        enableBatReposition = par("enableBatReposition");
        if (enableBatReposition) {
            windowSize          = par("windowSize");
            maxLosses           = par("maxLosses");
            rssiRef             = par("rssiRef");
            refDistance         = par("refDistance");
            pathLossExp         = par("pathLossExp");
            rssiMargin_dB       = par("rssiMargin");
            probeTimeout_s      = par("probeTimeout");
            maxRepositionTries  = par("maxRepositionTries");

            batParams_.Nb     = par("batPop");
            batParams_.Imax   = par("batIters");
            batParams_.G      = par("batStagnation");
            batParams_.w1     = par("wRepulsion");
            batParams_.w2     = par("wDisplacement");
            batParams_.sigmaRho = par("sigmaRho");
            batParams_.dMax   = par("dMaxReposition");
            batParams_.zMin   = par("zMinReposition");
            batParams_.zMax   = par("zMaxReposition");
            batParams_.alpha  = par("batAlpha");
            batParams_.gamma_ = par("batGamma");
        }
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        if (myDroneId.empty())
            myDroneId = getParentModule()->getFullName();

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

        int idx = getParentModule()->getIndex();

        teamSocket.setOutputGate(gate("socketOut"));
        teamSocket.setCallback(this);
        teamSocket.setBroadcast(true);
        teamSocket.bind(TEAM_UPDATE_PORT);

        ackSocket.setOutputGate(gate("socketOut"));
        ackSocket.setCallback(this);
        ackSocket.bind(9000 + idx * 3);

        alertSocket.setOutputGate(gate("socketOut"));
        alertSocket.setCallback(this);
        alertSocket.bind(9001 + idx * 3);

        relaySocket.setOutputGate(gate("socketOut"));
        relaySocket.setCallback(this);
        relaySocket.setBroadcast(true);
        relaySocket.bind(RELAY_PORT);

        fwdSocket.setOutputGate(gate("socketOut"));
        fwdSocket.setCallback(this);
        fwdSocket.setBroadcast(true);
        fwdSocket.bind(9002 + idx * 3);

        ackRxSocket.setOutputGate(gate("socketOut"));
        ackRxSocket.setCallback(this);
        ackRxSocket.bind(ACK_PORT);

        if (enableBatReposition) {
            probeRxSocket.setOutputGate(gate("socketOut"));
            probeRxSocket.setCallback(this);
            probeRxSocket.bind(PROBE_ACK_PORT);

            // PhysicalEnvironment: ^=drone[i], ^.^=BasicNetwork
            auto *m = getModuleByPath("^.^.physicalEnvironment");
            if (m)
                physEnv = check_and_cast<physicalenvironment::IPhysicalEnvironment*>(m);

            batOptimizer = new BatOptimizer(this, batParams_);

            repositionTimer = new cMessage("reposition");
            probeTimer      = new cMessage("probe-timeout");
        }

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
        } else if (msg == repositionTimer) {
            onRepositionTimer();
        } else if (msg == probeTimer) {
            onProbeTimer();
        } else {
            delete msg;
        }
    } else if (teamSocket.belongsToSocket(msg)) {
        teamSocket.processMessage(msg);
    } else if (relaySocket.belongsToSocket(msg)) {
        relaySocket.processMessage(msg);
    } else if (ackRxSocket.belongsToSocket(msg)) {
        ackRxSocket.processMessage(msg);
    } else if (alertSocket.belongsToSocket(msg)) {
        alertSocket.processMessage(msg);
    } else if (ackSocket.belongsToSocket(msg)) {
        ackSocket.processMessage(msg);
    } else if (fwdSocket.belongsToSocket(msg)) {
        fwdSocket.processMessage(msg);
    } else if (enableBatReposition && probeRxSocket.belongsToSocket(msg)) {
        probeRxSocket.processMessage(msg);
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
    else if (socket == &probeRxSocket && pkt->hasAtFront<ProbeAckChunk>())
        handleProbeAck(pkt);
    else
        delete pkt;
}

// ── Passo 3/4: recebe TeamUpdate ─────────────────────────────────────────────

void SimpleDroneApp::handleTeamUpdate(Packet *pkt)
{
    auto chunk   = pkt->peekAtFront<TeamUpdateChunk>();
    auto srcAddr = pkt->getTag<L3AddressInd>()->getSrcAddress();

    std::string teamId = chunk->getTeamId();
    auto& e    = teamTable[teamId];
    e.ip        = chunk->getIpAddress();
    e.posX      = chunk->getPosX();
    e.posY      = chunk->getPosY();
    e.available = chunk->getAvailable();
    e.lastSeen  = simTime();

    EV_INFO << "[DRONE " << myDroneId << "] tabela: " << teamId
            << " ip=" << e.ip << " disponivel=" << e.available << "\n";

    // BA: ler RSSI do pacote recebido (sentido equipe→drone)
    if (enableBatReposition) {
        auto sigTag = pkt->findTag<SignalPowerInd>();  // retorna Ptr<const SignalPowerInd>
        if (sigTag) {
            double rssi_dBm = 10.0 * std::log10(sigTag->getPower().get() * 1000.0);
            linkMonitors[teamId].N_W = windowSize;
            linkMonitors[teamId].pushRssi(rssi_dBm);
        }
    }

    // Passo 5: envia DroneStatus unicast para a equipe
    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();
    simtime_t now = simTime();

    auto resp = makeShared<DroneStatusChunk>();
    resp->setChunkLength(B(128));
    resp->setDroneId(myDroneId.c_str());
    resp->setSentAt(now);
    resp->setPosX(pos.x);
    resp->setPosY(pos.y);
    resp->setPosZ(pos.z);

    ackSocket.sendTo(new Packet("DroneStatus", resp),
                     srcAddr.toIpv4(), DRONE_STATUS_PORT);

    // BA: registrar tentativa na janela ARR e avaliar Γ
    if (enableBatReposition) {
        linkMonitors[teamId].N_W = windowSize;
        linkMonitors[teamId].pushAttempt(now);
        checkGamma(teamId, e.posX, e.posY);
    }

    delete pkt;
}

// ── BA: ProbeAck recebido da equipe ──────────────────────────────────────────

void SimpleDroneApp::handleProbeAck(Packet *pkt)
{
    auto chunk = pkt->peekAtFront<ProbeAckChunk>();
    std::string teamId = chunk->getTeamId();
    simtime_t   sentAt = chunk->getSentAt();

    // Marcar ACK na janela ARR do respectivo LinkMonitor
    if (linkMonitors.count(teamId))
        linkMonitors[teamId].markAck(sentAt);

    // Validação pós-reposicionamento
    if (droneState == DroneState::VALIDATING && teamId == activeTeamId) {
        EV_INFO << "[DRONE " << myDroneId << "] BA: ProbeAck recebido de "
                << teamId << " → reposicionamento bem-sucedido\n";
        cancelEvent(probeTimer);
        repositionsSucceeded++;
        totalRecoveryTime += simTime() - gammaTriggeredAt;

        auto *batMob = dynamic_cast<BatControlledMobility*>(
            getParentModule()->getSubmodule("mobility"));
        if (batMob) batMob->resumeCruise();

        tabuPoints.clear();
        repositionTries = 0;
        droneState = DroneState::CRUISE;
    }

    delete pkt;
}

// ── BA: avalia Γ_ij após atualização da janela ───────────────────────────────

void SimpleDroneApp::checkGamma(const std::string& teamId,
                                 double teamX, double teamY)
{
    if (droneState != DroneState::CRUISE) return;  // já em reposicionamento
    if (!linkMonitors.count(teamId)) return;
    auto& lm = linkMonitors[teamId];
    if (!lm.windowComplete()) return;

    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();
    double dx = teamX - pos.x, dy = teamY - pos.y;
    double dist = std::sqrt(dx*dx + dy*dy + pos.z*pos.z);  // distância 3D

    double thetaARR = 1.0 - (double)maxLosses / windowSize;
    bool gamma = lm.gamma(rssiRef, refDistance, pathLossExp, rssiMargin_dB, dist, thetaARR);

    if (!gamma) return;

    // Γ = 1: aciona sensor de obstáculo
    gammaTriggers++;
    gammaTriggeredAt = simTime();
    EV_INFO << "[DRONE " << myDroneId << "] BA: Γ=1 para " << teamId
            << " RSSI_avg=" << lm.rssiAvg() << " ARR=" << lm.arr() << "\n";

    activeTeamId  = teamId;
    activeTeamPos = Coord(teamX, teamY, 0.0);

    // Verificação sensorial (instantânea — abstração do LiDAR)
    auto pObs = senseObstacleOnLoS(pos, activeTeamPos, physEnv);
    if (!pObs) {
        EV_INFO << "[DRONE " << myDroneId
                << "] BA: sensor sem obstáculo — degradação por mobilidade\n";
        sensorAbortsNoObstacle++;
        return;
    }
    activeObsPos = *pObs;
    repositionTries = 0;
    tabuPoints.clear();
    startOptimize();
}

// ── BA: otimização e comando de deslocamento ─────────────────────────────────

void SimpleDroneApp::startOptimize()
{
    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord dronePos = mob->getCurrentPosition();

    EV_INFO << "[DRONE " << myDroneId << "] BA: executando BatOptimizer"
            << " p_obs=(" << activeObsPos.x << "," << activeObsPos.y << "," << activeObsPos.z << ")"
            << " tries=" << repositionTries << "\n";

    Coord pStar = batOptimizer->optimize(dronePos, activeObsPos, tabuPoints);
    activeTargetPos = pStar;

    EV_INFO << "[DRONE " << myDroneId << "] BA: p*=("
            << pStar.x << "," << pStar.y << "," << pStar.z << ")\n";

    // Comanda mobilidade
    auto *batMob = dynamic_cast<BatControlledMobility*>(
        getParentModule()->getSubmodule("mobility"));
    if (batMob) {
        batMob->moveTo(pStar);
    } else {
        // Fallback: sem mobilidade controlada — reposicionamento lógico (teleporte)
        EV_WARN << "[DRONE " << myDroneId
                << "] BA: BatControlledMobility não encontrado — reposicionamento lógico\n";
    }

    double dist = dronePos.distance(pStar);
    totalRepositionDistance += dist;
    repositionsAttempted++;

    // Timer de chegada estimada: dist/10m/s (velocidade média) + 2s de folga
    double travelEst = dist / 10.0 + 2.0;
    cancelEvent(repositionTimer);
    scheduleAt(simTime() + travelEst, repositionTimer);
    droneState = DroneState::REPOSITIONING;
}

// ── BA: timer de chegada estimada ────────────────────────────────────────────

void SimpleDroneApp::onRepositionTimer()
{
    if (droneState != DroneState::REPOSITIONING) return;

    auto *batMob = dynamic_cast<BatControlledMobility*>(
        getParentModule()->getSubmodule("mobility"));

    // Se ainda não chegou, aguarda mais 1s
    if (batMob && !batMob->hasArrived()) {
        auto *mob = check_and_cast<IMobility*>(getParentModule()->getSubmodule("mobility"));
        double remaining = mob->getCurrentPosition().distance(activeTargetPos);
        if (remaining > 5.0) {
            scheduleAt(simTime() + 1.0, repositionTimer);
            return;
        }
    }

    EV_INFO << "[DRONE " << myDroneId << "] BA: chegou em p* — enviando probe\n";
    droneState = DroneState::VALIDATING;
    sendProbeToActiveTeam();
    scheduleAt(simTime() + probeTimeout_s, probeTimer);
}

// ── BA: envia DroneStatus como probe de validação ────────────────────────────

void SimpleDroneApp::sendProbeToActiveTeam()
{
    if (!teamTable.count(activeTeamId)) return;
    auto& e = teamTable[activeTeamId];
    if (e.ip.empty()) return;

    auto *mob = check_and_cast<IMobility *>(getParentModule()->getSubmodule("mobility"));
    Coord pos = mob->getCurrentPosition();
    simtime_t now = simTime();

    auto resp = makeShared<DroneStatusChunk>();
    resp->setChunkLength(B(128));
    resp->setDroneId(myDroneId.c_str());
    resp->setSentAt(now);
    resp->setPosX(pos.x);
    resp->setPosY(pos.y);
    resp->setPosZ(pos.z);

    ackSocket.sendTo(new Packet("DroneStatus", resp),
                     Ipv4Address(e.ip.c_str()), DRONE_STATUS_PORT);

    linkMonitors[activeTeamId].pushAttempt(now);
    EV_INFO << "[DRONE " << myDroneId << "] BA: probe enviado para " << activeTeamId << "\n";
}

// ── BA: timeout do probe de validação ────────────────────────────────────────

void SimpleDroneApp::onProbeTimer()
{
    if (droneState != DroneState::VALIDATING) return;

    EV_INFO << "[DRONE " << myDroneId << "] BA: probe timeout — "
            << activeTeamId << " não respondeu\n";

    repositionTries++;
    if (repositionTries < maxRepositionTries) {
        // Tabu: adiciona p* à lista e re-otimiza
        tabuPoints.push_back(activeTargetPos);
        tabuActivations++;
        EV_INFO << "[DRONE " << myDroneId << "] BA: tabu ativado, "
                << "tentativa " << repositionTries << "/" << maxRepositionTries << "\n";
        droneState = DroneState::CRUISE;  // startOptimize redefine para REPOSITIONING
        startOptimize();
    } else {
        EV_WARN << "[DRONE " << myDroneId
                << "] BA: máximo de tentativas atingido — abandonando\n";
        auto *batMob = dynamic_cast<BatControlledMobility*>(
            getParentModule()->getSubmodule("mobility"));
        if (batMob) batMob->resumeCruise();
        tabuPoints.clear();
        repositionTries = 0;
        droneState = DroneState::CRUISE;
    }
}

// ── Passo 7: drone detecta vítima ────────────────────────────────────────────

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

// ── Passos 10/11/9: seleciona equipe e encaminha ─────────────────────────────

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

    alertsSentRelay++;
    fwdSocket.sendTo(new Packet("VictimAlert", chunk),
                     Ipv4Address::ALLONES_ADDRESS, RELAY_PORT);
    EV_INFO << "[DRONE " << myDroneId << "] relay: " << msgId
            << " → broadcast (sem equipe elegível)\n";
    return "";
}

// ── Passos 8/9: relay ─────────────────────────────────────────────────────────

void SimpleDroneApp::handleVictimAlertRelay(Packet *pkt)
{
    auto chunk = pkt->peekAtFront<VictimAlertChunk>();
    std::string msgId = chunk->getMsgId();

    if (seenAlerts.count(msgId)) {
        EV_INFO << "[DRONE " << myDroneId << "] dedup: descartando " << msgId << "\n";
        delete pkt;
        return;
    }
    seenAlerts.insert(msgId);
    alertsRelayed++;

    EV_INFO << "[DRONE " << myDroneId << "] relay recebido: " << msgId
            << " de " << chunk->getDroneId() << "\n";

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

// ── Passo 12: VictimAck ───────────────────────────────────────────────────────

void SimpleDroneApp::handleVictimAck(Packet *pkt)
{
    auto chunk = pkt->peekAtFront<VictimAckChunk>();
    std::string msgId = chunk->getMsgId();

    EV_INFO << "[DRONE " << myDroneId << "] VictimAck recebido para " << msgId
            << " de " << chunk->getTeamId() << "\n";

    for (const auto& p : pendingAlerts)
        if (p.msgId == msgId) {
            totalE2EDelay += simTime() - p.sentAt;
            alertsAcked++;
            break;
        }

    auto it = std::remove_if(pendingAlerts.begin(), pendingAlerts.end(),
                              [&](const PendingAlert& p){ return p.msgId == msgId; });
    pendingAlerts.erase(it, pendingAlerts.end());
    delete pkt;
}

// ── Passo 15: retry store-forward ─────────────────────────────────────────────

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

// ── Passo 13: timeout de equipes ─────────────────────────────────────────────

void SimpleDroneApp::checkTimeouts()
{
    simtime_t now = simTime();
    for (auto it = teamTable.begin(); it != teamTable.end(); ) {
        if (now - it->second.lastSeen > teamTimeout) {
            EV_INFO << "[DRONE " << myDroneId << "] timeout: removendo " << it->first << "\n";
            if (enableBatReposition) linkMonitors.erase(it->first);
            it = teamTable.erase(it);
        } else {
            ++it;
        }
    }
}

// ── Destrutor e finish ────────────────────────────────────────────────────────

SimpleDroneApp::~SimpleDroneApp()
{
    cancelAndDelete(detectTimer);
    cancelAndDelete(timeoutTimer);
    cancelAndDelete(retryTimer);
    if (repositionTimer) cancelAndDelete(repositionTimer);
    if (probeTimer)      cancelAndDelete(probeTimer);
    delete batOptimizer;
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
    recordScalar("totalRTT",         totalE2EDelay.dbl());
    recordScalar("meanRTT",
        alertsAcked > 0 ? totalE2EDelay.dbl() / alertsAcked : -1.0);

    if (enableBatReposition) {
        recordScalar("gammaTriggers",          gammaTriggers);
        recordScalar("repositionsAttempted",   repositionsAttempted);
        recordScalar("repositionsSucceeded",   repositionsSucceeded);
        recordScalar("sensorAbortsNoObstacle", sensorAbortsNoObstacle);
        recordScalar("tabuActivations",        tabuActivations);
        recordScalar("totalRepositionDistance", totalRepositionDistance);
        recordScalar("totalRecoveryTime",      totalRecoveryTime.dbl());
        recordScalar("meanRecoveryTime",
            repositionsSucceeded > 0
                ? totalRecoveryTime.dbl() / repositionsSucceeded : -1.0);
    }
}

} // namespace echosar
