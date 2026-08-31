#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "omnetpp/clistener.h"
#include "omnetpp/cmessage.h"
#include "omnetpp/csimplemodule.h"
#include "omnetpp/simtime.h"

namespace echosar {

/**
 * Agrega somente os resultados necessários ao experimento principal.
 *
 * DroneApp e TeamApp emitem fatos da aplicação. Este módulo deduplica esses
 * fatos por alertId e produz uma única visão fim a fim por execução.
 */
/// Uma linha por alerta lógico. Retransmissões e recebimentos por várias
/// equipes não criam linhas novas: o alertId é a chave.
struct AlertRecord {
    std::string alertId;
    std::string victimId;
    std::string droneId;
    omnetpp::simtime_t generationTime;
    bool delivered = false;
    std::string receivingTeamId;
    bool acknowledged = false;
    std::string ackTeamId;
    int attempts = 0;
};

class ExperimentMetrics : public omnetpp::cSimpleModule, public omnetpp::cListener
{
  protected:
    /// Ordem de geração preservada para que o CSV seja estável entre execuções.
    std::vector<std::string> alertOrder;
    std::map<std::string, AlertRecord> alertRecords;
    std::set<std::string> generatedAlertIds;
    std::set<std::string> deliveredAlertIds;
    std::set<std::string> confirmedAlertIds;
    std::set<std::string> expiredAlertIds;
    std::set<std::string> startedRepositionAlertIds;
    std::set<std::string> completedRepositionAlertIds;
    std::set<std::string> measuredRepositionAlertIds;
    std::set<std::string> sentAttemptIds;
    std::set<std::string> receivedAttemptIds;
    std::set<std::string> alertsWithoutKnownTeam;
    std::map<std::string, int> attemptsByAlert;
    std::map<std::string, omnetpp::simtime_t> creationTimes;
    std::map<std::string, omnetpp::simtime_t> repositionStartTimes;

    // Métricas mantidas por hora: atendimento e perda (ver AlertRecord acima
    // e writeAlertRecords) precisam apenas de geração/entrega/confirmação por
    // alertId. Tudo abaixo permanece só porque os 8 smoke tests obrigatórios
    // (diretriz §28) ainda leem esses escalares — ver a lista de candidatos a
    // remoção futura no topo de ExperimentMetrics.cc, junto com qual smoke
    // test prende cada um.
    omnetpp::simtime_t confirmationDelaySum = 0;
    omnetpp::simtime_t repositionDurationSum = 0;
    int confirmationDelayCount = 0;
    // Diagnósticos por evento: contam oportunidades de envio e timeouts, não
    // alertas. Podem ser maiores que o total de alertas gerados.
    int neverKnownTeamSelectionEvents = 0;
    int expiredKnownTeamSelectionEvents = 0;
    int hopCountSum = 0;
    int hopCountCount = 0;
    int multiHopDeliveries = 0;
    int intermediateForwardings = 0;
    int repositionTriggers = 0;
    /// Quantas vezes o modelo visual foi consultado. Separado de
    /// obstaclesDetected porque, desde que a detecção deixou de condicionar o
    /// BA (D7), "nenhum obstáculo detectado" e "câmera nunca consultada" são
    /// situações distintas que o funil precisa distinguir.
    int sensorEvaluations = 0;
    int obstaclesDetected = 0;
    int baActivations = 0;
    int repositionsStarted = 0;
    int repositionsCompleted = 0;
    double repositionDistanceSum = 0;
    /// Reposicionamentos concluídos cujo deslocamento excedeu
    /// effectiveRepositionThreshold. Classificação analítica: um deslocamento
    /// de poucos centímetros é numericamente um reposicionamento, mas não um
    /// reposicionamento operacionalmente relevante. O valor bruto continua em
    /// repositionDistanceSum.
    int effectiveRepositions = 0;
    double effectiveRepositionThreshold = 1;
    /// Funil da sondagem de recuperação. Uma verificação é aberta por
    /// reposicionamento concluído (recoveryProbeChecks) e fecha em exatamente
    /// um dos quatro desfechos. recoveryProbesSent conta pacotes, não
    /// verificações: uma verificação emite de zero (equipe já ausente) a
    /// recoveryProbeMaxAttempts sondagens antes de fechar.
    int recoveryProbeChecks = 0;
    int recoveryProbesSent = 0;
    int recoveryProbesConfirmed = 0;
    int recoveryProbesFailed = 0;
    int recoveryProbesUnreachable = 0;
    /// Verificação encerrada porque o alerta terminou antes dela — ACK de uma
    /// tentativa anterior, ou expiração do TTL.
    int recoveryProbesAbandoned = 0;

    omnetpp::simsignal_t generatedSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t attemptSentSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t deliveredSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t confirmedSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t expiredSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t operationalFailureSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t repositionTriggerSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t sensorSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t baActivationSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t repositionSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t recoveryProbeSignal = SIMSIGNAL_NULL;

    using omnetpp::cListener::finish;
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *message) override;
    virtual void finish() override;
    /// Grava uma linha por alerta ao lado dos escalares da execução.
    void writeAlertRecords() const;
    virtual void receiveSignal(omnetpp::cComponent *source,
                               omnetpp::simsignal_t signalId,
                               omnetpp::cObject *value,
                               omnetpp::cObject *details) override;
};

} // namespace echosar
