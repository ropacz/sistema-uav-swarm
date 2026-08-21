#pragma once

#include <deque>
#include <map>
#include <set>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/PositionUpdate_m.h"
#include "messages/VictimAck_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAssignment_m.h"
#include "optimization/BatAlgorithm.h"
#include "optimization/RepositionFitness.h"

namespace echosar {

struct LinkSample {
    int64_t sequence = 0;
    omnetpp::simtime_t receptionTime;
    double rssiDbm = NAN;
};

struct TeamLinkState {
    std::string ipAddress;
    inet::Coord position;
    omnetpp::simtime_t lastSeen = -1;
    std::deque<LinkSample> samples;
};

struct PendingVictimAlert {
    std::string alertId;
    std::string victimId;
    inet::Coord victimPosition;
    omnetpp::simtime_t generationTime;
    omnetpp::simtime_t ackDeadline = -1;
    omnetpp::simtime_t nextAttempt = -1;
    int attempts = 0;
    int64_t sequence = 0;
    std::string targetTeamId;
    std::map<std::string, omnetpp::simtime_t> attemptSentTimes;
    bool degradationEvaluated = false;
    int baCycles = 0;
    std::set<std::string> testedPositions;
    std::string validationMessageId;
    omnetpp::simtime_t repositionStart = -1;
    double preRepositionPdr = NAN;
    double preRepositionRssi = NAN;
};

class DroneApp : public inet::ApplicationBase, public inet::UdpSocket::ICallback
{
  protected:
    enum class RepositionState { IDLE, MOVING, AWAITING_VALIDATION };

    inet::UdpSocket socket;
    omnetpp::cMessage *maintenanceTimer = nullptr;
    omnetpp::cMessage *movementCompleteTimer = nullptr;
    std::string droneId;
    std::string ipAddress;
    std::map<std::string, TeamLinkState> teams;
    std::map<std::string, PendingVictimAlert> pendingAlerts;
    std::string activeRepositionAlertId;
    RepositionState repositionState = RepositionState::IDLE;

    omnetpp::simtime_t retryInterval;
    omnetpp::simtime_t ackTimeout;
    omnetpp::simtime_t alertTtl;
    omnetpp::simtime_t linkWindow;
    omnetpp::simtime_t teamSilenceTimeout;
    omnetpp::simtime_t maintenanceInterval;
    int maxAttempts = 5;
    int maxBaCycles = 2;
    int applicationIpTtl = 32;
    int appPort = 5000;
    int64_t victimAlertPayloadBytes = 320;
    double pdrThreshold = 0.8;
    double rssiThresholdDbm = -80;
    bool baEnabled = true;
    double maximumRepositionDistance = 25;
    double minimumAltitude = 6;
    double maximumAltitude = 20;
    FitnessParameters fitnessParameters;
    BatParameters batParameters;

    int uniqueAlertsGenerated = 0;
    int alertAttemptsSent = 0;
    int uniqueAlertsAcked = 0;
    int alertsExpired = 0;
    int degradationIndications = 0;
    int sensorConfirmations = 0;
    // Rejeição sensorial propriamente dita: houve consulta e o sensor não
    // confirmou obstáculo dentro do alcance. Não inclui os casos em que a
    // consulta sequer foi possível — ver teamUnknownForReposition.
    int sensorRejections = 0;
    // Degradação indicada sem posição conhecida da equipe: o sensor não pode
    // ser consultado porque não há linha de visada a traçar.
    int teamUnknownForReposition = 0;
    // Decomposição de sensorRejections pelo motivo devolvido pelo sensor.
    // Distinguem duas situações opostas: a linha de visada estava livre, ou
    // havia obstáculo mas fora do alcance configurado.
    int sensorClearLineOfSight = 0;
    int sensorOutsideRange = 0;
    int baActivations = 0;
    int successfulRepositions = 0;
    // Total de reposicionamentos malsucedidos; decomposto pelas quatro causas
    // abaixo, cuja soma é igual a este contador.
    int failedRepositions = 0;
    // O BA não devolveu posição utilizável, ou a mobilidade não é comandável.
    int baNoFeasibleSolution = 0;
    // Candidato já testado neste alerta, ou deslocamento numericamente nulo.
    int baRedundantCandidate = 0;
    // TTL ou tentativas esgotaram enquanto o reposicionamento estava ativo.
    int repositionExpiredBeforeAck = 0;
    // O alerta foi confirmado por uma tentativa anterior à chegada: entrega
    // bem-sucedida, mas sem validar a posição escolhida pelo BA.
    int repositionAckedBeforeValidation = 0;
    double baDistance = 0;
    omnetpp::simtime_t totalRtt = 0;
    omnetpp::simtime_t totalRecoveryTime = 0;
    int recoverySamples = 0;
    int repositionValidationSamples = 0;
    double preRepositionPdrSum = 0;
    double postRepositionPdrSum = 0;
    double preRepositionRssiSum = 0;
    double postRepositionRssiSum = 0;
    int preRepositionRssiSamples = 0;
    int postRepositionRssiSamples = 0;

    omnetpp::simsignal_t rssiSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t pdrSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t repositionDistanceSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t recoveryTimeSignal = SIMSIGNAL_NULL;

    /// Libera timers e encerra o socket sem deixar mensagens pendentes.
    virtual ~DroneApp();
    /// Informa ao OMNeT++ quantos estágios de inicialização do INET serão usados.
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    /// Lê parâmetros, registra métricas e configura endereço, socket e timers.
    virtual void initialize(int stage) override;
    /// Despacha timers de manutenção, conclusão de movimento e eventos do socket.
    virtual void handleMessageWhenUp(omnetpp::cMessage *message) override;
    /// Grava escalares acumulados ao final da simulação.
    virtual void finish() override;
    /// Hooks de ciclo de vida mantidos vazios porque o cenário não reinicia aplicações.
    virtual void handleStartOperation(inet::LifecycleOperation *) override {}
    virtual void handleStopOperation(inet::LifecycleOperation *) override {}
    virtual void handleCrashOperation(inet::LifecycleOperation *) override {}
    /// Classifica os pacotes UDP recebidos e encaminha ao tratador apropriado.
    virtual void socketDataArrived(inet::UdpSocket *, inet::Packet *packet) override;
    /// Consome indicações de erro e fechamento exigidas pelo callback UDP.
    virtual void socketErrorArrived(inet::UdpSocket *, inet::Indication *indication) override { delete indication; }
    virtual void socketClosed(inet::UdpSocket *) override {}

    /// Cria um alerta pendente para a vítima atribuída pelo gerenciador.
    void handleAssignment(VictimAssignment *assignment);
    /// Atualiza posição, sequência, RSSI e janela de recepção de uma equipe.
    void handlePositionUpdate(inet::Packet *packet);
    /// Valida o VictimAck, encerra o alerta e registra RTT ou recuperação pelo BA.
    void handleVictimAck(inet::Packet *packet);
    /// Expira alertas, avalia degradação e agenda as tentativas regulares.
    void performMaintenance();
    /// Monta e envia uma nova tentativa do VictimAlert para a equipe selecionada.
    void sendAttempt(PendingVictimAlert& alert);
    /// Escolhe a equipe posicionada mais próxima; menor ID é o fallback sem posição.
    std::string selectTargetTeam() const;
    /// Calcula PDR/RSSI da janela e indica degradação enquanto falta AppACK.
    bool detectDegradation(const PendingVictimAlert& alert, double& pdr, double& rssi) const;
    /// Confirma o obstáculo, executa o BA e inicia o deslocamento quando permitido.
    void tryReposition(PendingVictimAlert& alert, double prePdr, double preRssi);
    /// Encerra o modo de reposicionamento e retoma a mobilidade Gauss-Markov.
    void resumeMobility();
};

} // namespace echosar
