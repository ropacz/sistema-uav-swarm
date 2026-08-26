#pragma once

#include <map>
#include <set>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/DroneStatus_m.h"
#include "messages/TeamUpdate_m.h"
#include "messages/VictimAck_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAssignment_m.h"
#include "ActiveVictim.h"
#include "PendingVictimAlert.h"
#include "DroneLinkState.h"
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
    omnetpp::cMessage *droneStatusTimer = nullptr;
    std::string droneId;
    std::map<std::string, TeamLinkState> discoveredTeams;
    std::map<std::string, DroneLinkState> discoveredDrones;
    std::map<std::string, ActiveVictim> activeVictims;
    std::map<std::string, PendingVictimAlert> pendingAlerts;
    RepositionController reposition;

    omnetpp::simtime_t retryInterval;
    omnetpp::simtime_t ackTimeout;
    omnetpp::simtime_t alertTtl;
    omnetpp::simtime_t alertInterval;
    /// Último instante em que um novo alerta pode ser criado. Valor negativo
    /// desabilita o corte (útil nos smoke tests que exercitam truncamento).
    omnetpp::simtime_t alertGenerationEndTime;
    omnetpp::simtime_t teamEntryLifetime;
    omnetpp::simtime_t lastKnownTeamRetention;
    omnetpp::simtime_t teamUpdateForwardJitter;
    omnetpp::simtime_t droneStatusInterval;
    omnetpp::simtime_t droneStatusInitialOffset;
    omnetpp::simtime_t droneEntryLifetime;
    omnetpp::simtime_t maintenanceInterval;
    int teamUpdateMaxHops = 3;
    int maxAttempts = 5;
    int repositionAfterUnackedAttempts = 2;
    int applicationIpTtl = 32;
    int appPort = 5000;
    int64_t victimAlertPayloadBytes = 320;
    int64_t droneStatusPayloadBytes = 0;
    int64_t droneStatusSequence = 0;
    int64_t droneStatusUpdatesAccepted = 0;
    int64_t connectivityConstraintsApplied = 0;
    int64_t connectivityPreservedSelections = 0;
    /// Repasses de TeamUpdate ainda não transmitidos, aguardando o jitter.
    std::set<omnetpp::cMessage *> pendingTeamUpdateRelays;
    /// Alguma equipe já foi conhecida por este drone em algum momento. Separa
    /// "nunca conheci" de "conhecia e expirou" na contabilidade de falhas.
    bool everKnewTeam = false;
    bool baEnabled = true;
    FitnessParameters fitnessParameters;
    BatParameters batParameters;

    omnetpp::simsignal_t alertGeneratedSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t alertAttemptSentSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t alertConfirmedSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t alertExpiredSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t operationalFailureSignal = SIMSIGNAL_NULL;
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
    /// Registra diagnósticos locais usados pelos testes de conectividade.
    virtual void finish() override;
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

    /// Mantém ativa a vítima atribuída pelo gerenciador.
    void handleAssignment(VictimAssignment *assignment);
    /// Cria um novo alerta lógico para uma vítima ativa sem alerta pendente.
    void startAlertCycle(ActiveVictim& victim);
    /// Retorna se a janela de geração ainda admite um novo alerta.
    bool canStartAlertCycle() const;
    /// Libera a vítima para o próximo ciclo após ACK ou expiração.
    void completeAlertCycle(const PendingVictimAlert& alert);
    /// Atualiza a última posição recebida de uma equipe descoberta e, quando o
    /// limite de saltos permitir, agenda o repasse da atualização pela FANET.
    void handleTeamUpdate(inet::Packet *packet);
    /// Agenda uma única retransmissão desta atualização após um jitter curto.
    void scheduleTeamUpdateRelay(const TeamUpdateChunk& original,
                                 const std::string& teamAddress);
    /// Remove entradas expiradas, retendo a última posição pelo prazo de
    /// recuperação e marcando-a como desatualizada.
    void expireDiscoveredEntries();
    /// Atualiza a posição anunciada por outro drone, ignorando ecos e reordenação.
    void handleDroneStatus(inet::Packet *packet);
    /// Valida o VictimAck contra o destino histórico e encerra o alerta.
    void handleVictimAck(inet::Packet *packet);
    /// Expira alertas, trata timeouts sem ACK e agenda tentativas regulares.
    void performMaintenance();
    /// Monta e envia uma nova tentativa do VictimAlert para a equipe selecionada.
    void sendAttempt(PendingVictimAlert& alert);
    /// Divulga identidade e posição para a estimativa local de conectividade.
    void sendDroneStatus();
    /// Escolhe a equipe descoberta mais próxima; vazio significa nenhuma visível.
    std::string selectTargetTeam() const;
    /// Faz uma consulta binária do sensor e, no tratamento, executa o BA uma vez.
    void tryReposition(PendingVictimAlert& alert);
    /// Registra a distância efetiva de um reposicionamento concluído.
    void recordCompletedRepositionDistance(const PendingVictimAlert& alert);
    /// Encerra o modo de reposicionamento e retoma a mobilidade Gauss-Markov.
    void resumeMobility();
};

} // namespace echosar
