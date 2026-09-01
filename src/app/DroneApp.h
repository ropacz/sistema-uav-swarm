#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/DroneStatus_m.h"
#include "messages/RecoveryProbe_m.h"
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
    int64_t victimAlertPhotoBytes = 0;
    int64_t recoveryProbePayloadBytes = 128;
    omnetpp::simtime_t recoveryProbeTimeout;
    int recoveryProbeMaxAttempts = 2;
    bool recoveryProbeEnabled = false;
    /// Sequencial local que torna único o probeId de cada sondagem, para que
    /// uma resposta atrasada da tentativa anterior não confirme a atual.
    int64_t recoveryProbeSequence = 0;
    int victimAlertPhotoWidth = 160;
    int victimAlertPhotoHeight = 120;
    omnetpp::simtime_t rssiWindow;
    omnetpp::simtime_t directUpdateTimeout;
    double rssiReferenceDbm = -30.05;
    double rssiReferenceDistance = 1;
    double losPathLossExponent = 2;
    double excessLossThresholdDb = 6;
    /// Exige a indicação S_ij antes de consultar o sensor. Falso preserva o
    /// mecanismo atual, em que só as tentativas sem ACK decidem quando avaliar.
    bool requireObstructionIndication = false;
    int64_t directRssiSamples = 0;
    /// Soma da atenuação excedente das amostras diretas. Dividida por
    /// directRssiSamples dá a média de Delta observada na execução.
    double directRssiExcessLossSum = 0;
    int64_t forwardedTeamUpdatesIgnoredForRssi = 0;
    int64_t rssiDegradationIndications = 0;
    int64_t directUpdateTimeoutIndications = 0;
    int64_t possibleObstructionIndications = 0;
    /// Consultas ao sensor barradas pelo parâmetro acima. Mede o efeito do
    /// gate; sem ele não se distingue "não houve gatilho" de "houve gatilho
    /// sem indicação".
    int64_t obstructionGateSuppressions = 0;
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
    omnetpp::simsignal_t recoveryProbeSignal = SIMSIGNAL_NULL;

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
    /// Guarda o RSSI e a distância de uma recepção direta. Sem a etiqueta
    /// SignalPowerInd — ou com potência não positiva — nada é registrado.
    void recordDirectRssiSample(const inet::Packet *packet, TeamLinkState& team,
                                const inet::Coord& teamPosition);
    /// RSSI previsto pelo Log-Distance sem obstrução para uma dada distância.
    double expectedRssiDbm(double distanceMeters) const;
    /// Descarta as amostras que saíram da janela de observação.
    void discardExpiredRssiSamples(TeamLinkState& team);
    /// Média da atenuação excedente em relação ao Log-Distance sem obstrução.
    /// Vazio quando a janela não tem amostra direta.
    std::optional<double> computeExcessAttenuation(TeamLinkState& team);
    /// Calcula S_ij: atenuação acima do limiar, ou ausência prolongada de
    /// recepção direta. Contabiliza cada episódio uma única vez.
    bool evaluatePossibleObstruction(TeamLinkState& team);
    /// Atualiza a posição anunciada por outro drone, ignorando ecos e reordenação.
    void handleDroneStatus(inet::Packet *packet);
    /// Valida o VictimAck contra o destino histórico e encerra o alerta.
    void handleVictimAck(inet::Packet *packet);
    /// Expira alertas, trata timeouts sem ACK e agenda tentativas regulares.
    void performMaintenance();
    /// Monta e envia uma nova tentativa do VictimAlert para a equipe selecionada.
    void sendAttempt(PendingVictimAlert& alert);
    /// Anexa a miniatura da vítima ao alerta, ou a deixa vazia quando a captura
    /// está desabilitada. Reconstrói sempre o mesmo conteúdo para um dado
    /// photoId, portanto todas as tentativas reenviam a imagem da captura.
    void attachVictimPhoto(const inet::Ptr<VictimAlertChunk>& message,
                           const PendingVictimAlert& alert) const;
    /// Divulga identidade e posição para a estimativa local de conectividade.
    void sendDroneStatus();
    /// Abre a verificação do enlace ao fim do deslocamento: enquanto ela estiver
    /// pendente o alerta não é transmitido.
    void startRecoveryProbe(PendingVictimAlert& alert);
    /// Emite uma sondagem para a equipe atualmente selecionada. Retorna falso
    /// quando não há equipe endereçável, caso em que a verificação é encerrada.
    bool sendRecoveryProbe(PendingVictimAlert& alert);
    /// Reavalia as sondagens sem resposta: reenvia enquanto houver tentativas e,
    /// esgotadas, devolve o alerta ao retry normal.
    void expireRecoveryProbes();
    /// Trata a resposta da equipe e libera a transmissão do alerta.
    void handleRecoveryProbe(inet::Packet *packet);
    /// Encerra a verificação registrando seu desfecho.
    void finishRecoveryProbe(PendingVictimAlert& alert, const char *outcome);
    /// Escolhe a equipe descoberta mais próxima; vazio significa nenhuma visível.
    std::string selectTargetTeam() const;
    /// Faz uma consulta binária do sensor e, no tratamento, executa o BA uma vez.
    /// Avalia e, se couber, inicia o reposicionamento do alerta. Retorna true
    /// quando a decisão é definitiva (iniciada, desabilitada, sem obstáculo
    /// observado ou sem candidato viável) e false quando a recusa é temporária
    /// e o alerta deve tentar de novo na próxima oportunidade.
    bool tryReposition(PendingVictimAlert& alert);
    /// Registra a distância efetiva de um reposicionamento concluído.
    void recordCompletedRepositionDistance(const PendingVictimAlert& alert);
    /// Encerra o modo de reposicionamento e retoma a mobilidade Gauss-Markov.
    void resumeMobility();
};

} // namespace echosar
