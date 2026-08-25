#pragma once

#include <map>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/PositionUpdate_m.h"
#include "messages/VictimAck_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAssignment_m.h"
#include "PendingVictimAlert.h"
#include "RepositionController.h"
#include "TeamLinkState.h"
#include "optimization/BatAlgorithm.h"
#include "optimization/RepositionFitness.h"

namespace echosar {

class DroneApp : public inet::ApplicationBase, public inet::UdpSocket::ICallback
{
  protected:
    inet::UdpSocket socket;
    omnetpp::cMessage *maintenanceTimer = nullptr;
    omnetpp::cMessage *movementCompleteTimer = nullptr;
    std::string droneId;
    std::map<std::string, TeamLinkState> discoveredTeams;
    std::map<std::string, PendingVictimAlert> pendingAlerts;
    RepositionController reposition;

    omnetpp::simtime_t retryInterval;
    omnetpp::simtime_t ackTimeout;
    omnetpp::simtime_t alertTtl;
    omnetpp::simtime_t teamEntryLifetime;
    omnetpp::simtime_t maintenanceInterval;
    int maxAttempts = 5;
    int repositionAfterUnackedAttempts = 2;
    int applicationIpTtl = 32;
    int appPort = 5000;
    int64_t victimAlertPayloadBytes = 320;
    bool baEnabled = true;
    FitnessParameters fitnessParameters;
    BatParameters batParameters;

    omnetpp::simsignal_t alertGeneratedSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t alertAttemptSentSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t alertConfirmedSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t alertExpiredSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t repositionTriggerSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t sensorEvaluationSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t baActivationSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t repositionEventSignal = SIMSIGNAL_NULL;

    /// Libera timers e encerra o socket sem deixar mensagens pendentes.
    virtual ~DroneApp();
    /// Informa ao OMNeT++ quantos estágios de inicialização do INET serão usados.
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    /// Lê parâmetros, registra métricas e configura socket e timers.
    virtual void initialize(int stage) override;
    /// Verifica cada parâmetro isoladamente, nomeando o que estiver inválido.
    void validateParameters() const;
    /// Despacha timers de manutenção, conclusão de movimento e eventos do socket.
    virtual void handleMessageWhenUp(omnetpp::cMessage *message) override;
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
    /// Atualiza a última posição recebida de uma equipe descoberta.
    void handlePositionUpdate(inet::Packet *packet);
    /// Valida o VictimAck contra o destino histórico e encerra o alerta.
    void handleVictimAck(inet::Packet *packet);
    /// Expira alertas, trata timeouts sem ACK e agenda tentativas regulares.
    void performMaintenance();
    /// Monta e envia uma nova tentativa do VictimAlert para a equipe selecionada.
    void sendAttempt(PendingVictimAlert& alert);
    /// Escolhe a equipe descoberta mais próxima; vazio significa nenhuma visível.
    std::string selectTargetTeam() const;
    /// Faz uma consulta binária do sensor e, no tratamento, executa o BA uma vez.
    void tryReposition(PendingVictimAlert& alert);
    /// Acumula uma vez a distância efetivamente percorrida no comando corrente.
    void recordActualRepositionDistance(PendingVictimAlert& alert);
    /// Encerra o modo de reposicionamento e retoma a mobilidade Gauss-Markov.
    void resumeMobility();
};

} // namespace echosar
