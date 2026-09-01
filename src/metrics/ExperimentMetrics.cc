#include "ExperimentMetrics.h"

#include <algorithm>
#include <fstream>

#include "omnetpp/cconfiguration.h"

#include "AlertMetricEvent.h"

using namespace omnetpp;

namespace echosar {

// Este módulo grava, por hora, só o necessário para atendimento e perda
// (AlertRecord, escrito em writeAlertRecords) mais os escalares que os 8
// smoke tests obrigatórios (diretriz §28) exigem — conferido um a um contra
// analysis/validation/validate_*.py antes de remover qualquer coisa.
//
// Candidatos a remoção futura, quando/se os smoke tests forem revistos:
//   hopCountSum/hopCountCount, multiHopDeliveries, intermediateForwardings
//     -> ba-smoke-test, alert-lifecycle-smoke-test, multihop-smoke-test
//   repositionTriggers, obstaclesDetected, baActivations,
//   repositionsStarted/Completed, repositionDistanceSum, repositionDurationSum
//     -> ba-smoke-test, sensor-range-smoke-test, reposition-interrupted-smoke-test
//   neverKnownTeamSelectionEvents, expiredKnownTeamSelectionEvents,
//   alertsWithoutKnownTeam, alertsExpired
//     -> no-known-team-smoke-test, alert-lifecycle-smoke-test
//   confirmationDelaySum/confirmationDelayCount
//     -> ba-smoke-test, alert-lifecycle-smoke-test, multihop-smoke-test
// Já removidos por não serem lidos por nenhum smoke test nem pela análise:
// deliveryDelaySum/deliveryDelayCount, knownTeamNoAckTimeoutEvents (o sinal
// "knownTeamNoAck" continua sendo aceito abaixo, só não é mais contado), e os
// escalares derivados pdr/confirmationRate. Estes dois eram pior que inertes:
// duplicavam, já divididos, as taxas que analysis/core/experiment_metrics.py
// recalcula dos contadores brutos justamente para manter o relatório
// auditável. Quem lesse o escalar pronto pularia esse recálculo, e as duas
// fontes poderiam divergir sem nada acusar.

Register_Class(AlertMetricEvent);
Define_Module(ExperimentMetrics);

void ExperimentMetrics::initialize()
{
    effectiveRepositionThreshold = par("effectiveRepositionThreshold");
    if (effectiveRepositionThreshold < 0)
        throw cRuntimeError("effectiveRepositionThreshold must not be negative");

    generatedSignal = registerSignal("victimAlertGenerated");
    attemptSentSignal = registerSignal("victimAlertAttemptSent");
    deliveredSignal = registerSignal("victimAlertDelivered");
    confirmedSignal = registerSignal("victimAlertConfirmed");
    expiredSignal = registerSignal("victimAlertExpired");
    operationalFailureSignal = registerSignal("victimAlertOperationalFailure");
    repositionTriggerSignal = registerSignal("victimRepositionTriggered");
    sensorSignal = registerSignal("victimSensorEvaluated");
    baActivationSignal = registerSignal("victimBaActivated");
    repositionSignal = registerSignal("victimRepositionEvent");
    recoveryProbeSignal = registerSignal("victimRecoveryProbe");

    // Assinar no módulo raiz permite observar todos os UAVs e equipes sem
    // acoplar as aplicações diretamente ao coletor.
    auto network = getSimulation()->getSystemModule();
    network->subscribe(generatedSignal, this);
    network->subscribe(attemptSentSignal, this);
    network->subscribe(deliveredSignal, this);
    network->subscribe(confirmedSignal, this);
    network->subscribe(expiredSignal, this);
    network->subscribe(operationalFailureSignal, this);
    network->subscribe(repositionTriggerSignal, this);
    network->subscribe(sensorSignal, this);
    network->subscribe(baActivationSignal, this);
    network->subscribe(repositionSignal, this);
    network->subscribe(recoveryProbeSignal, this);
}

void ExperimentMetrics::handleMessage(cMessage *message)
{
    throw cRuntimeError("ExperimentMetrics is passive and cannot receive messages (%s)",
                        message->getName());
}

void ExperimentMetrics::receiveSignal(cComponent *, simsignal_t signalId,
                                      cObject *value, cObject *)
{
    auto event = dynamic_cast<AlertMetricEvent *>(value);
    if (!event || event->alertId.empty())
        throw cRuntimeError("Signal '%s' requires a non-empty AlertMetricEvent",
                            getSignalName(signalId));

    if (signalId == generatedSignal) {
        if (generatedAlertIds.insert(event->alertId).second) {
            creationTimes[event->alertId] = event->referenceTime;
            alertOrder.push_back(event->alertId);
            auto& record = alertRecords[event->alertId];
            record.alertId = event->alertId;
            record.victimId = event->victimId;
            record.droneId = event->droneId;
            record.generationTime = event->referenceTime;
        }
    }
    else if (signalId == attemptSentSignal) {
        if (!generatedAlertIds.count(event->alertId))
            throw cRuntimeError("Attempt for unknown alert '%s'",
                                event->alertId.c_str());
        if (event->messageId.empty())
            throw cRuntimeError("Attempt signal for alert '%s' has no messageId",
                                event->alertId.c_str());
        if (sentAttemptIds.insert(event->messageId).second) {
            attemptsByAlert[event->alertId]++;
            alertRecords[event->alertId].attempts++;
        }
    }
    else if (signalId == deliveredSignal) {
        if (event->messageId.empty())
            throw cRuntimeError("Delivery signal for alert '%s' has no messageId",
                                event->alertId.c_str());
        receivedAttemptIds.insert(event->messageId);
        // A primeira equipe que recebe o alertId define entrega e atraso fim a fim.
        if (deliveredAlertIds.insert(event->alertId).second) {
            // A primeira equipe a receber define a entrega; recebimentos
            // posteriores do mesmo alertId não criam nova entrega.
            auto& record = alertRecords[event->alertId];
            record.delivered = true;
            record.receivingTeamId = event->teamId;
            int hopCount = static_cast<int>(event->value);
            if (hopCount < 1 || event->value != hopCount)
                throw cRuntimeError("Invalid hop count %.3f for alert '%s'",
                                    event->value, event->alertId.c_str());
            if (!creationTimes.count(event->alertId))
                throw cRuntimeError("Delivery for unknown alert '%s'",
                                    event->alertId.c_str());
            hopCountSum += hopCount;
            hopCountCount++;
            if (hopCount > 1)
                multiHopDeliveries++;
            intermediateForwardings += hopCount - 1;
        }
    }
    else if (signalId == confirmedSignal) {
        if (confirmedAlertIds.insert(event->alertId).second) {
            auto& record = alertRecords[event->alertId];
            record.acknowledged = true;
            record.ackTeamId = event->teamId;
            auto creation = creationTimes.find(event->alertId);
            if (creation == creationTimes.end())
                throw cRuntimeError("Confirmation for unknown alert '%s'",
                                    event->alertId.c_str());
            confirmationDelaySum += simTime() - creation->second;
            confirmationDelayCount++;
        }
    }
    else if (signalId == expiredSignal) {
        if (!generatedAlertIds.count(event->alertId))
            throw cRuntimeError("Expiration for unknown alert '%s'",
                                event->alertId.c_str());
        expiredAlertIds.insert(event->alertId);
    }
    else if (signalId == operationalFailureSignal) {
        if (!generatedAlertIds.count(event->alertId))
            throw cRuntimeError("Operational failure for unknown alert '%s'",
                                event->alertId.c_str());
        // Três diagnósticos operacionais distintos, contados por evento e não
        // por alerta: um mesmo alerta pode falhar a seleção de destino em várias
        // oportunidades de envio, e por isso estes contadores podem exceder o
        // número de alertas gerados. Os dois primeiros indicam que o alerta não
        // chegou a ser transmitido; o terceiro, que foi transmitido e expirou
        // sem confirmação.
        if (event->category == "neverKnownTeam") {
            neverKnownTeamSelectionEvents++;
            alertsWithoutKnownTeam.insert(event->alertId);
        }
        else if (event->category == "expiredKnownTeam") {
            expiredKnownTeamSelectionEvents++;
            alertsWithoutKnownTeam.insert(event->alertId);
        }
        else if (event->category == "knownTeamNoAck") {
            // Reconhecido e descartado: nenhum smoke test nem a planilha de
            // atendimento/perda usa essa contagem hoje (candidato listado no
            // topo do arquivo). Ainda precisa ser aceito aqui, senão o sinal
            // que DroneApp::performMaintenance emite cairia no "else" abaixo.
        }
        else
            throw cRuntimeError("Unknown operational failure '%s'",
                                event->category.c_str());
    }
    else if (signalId == repositionTriggerSignal)
        repositionTriggers++;
    else if (signalId == sensorSignal) {
        sensorEvaluations++;
        if (event->category == "detected")
            obstaclesDetected++;
    }
    else if (signalId == baActivationSignal)
        baActivations++;
    else if (signalId == recoveryProbeSignal) {
        if (event->category == "started")
            recoveryProbeChecks++;
        else if (event->category == "sent")
            recoveryProbesSent++;
        else if (event->category == "confirmed")
            recoveryProbesConfirmed++;
        else if (event->category == "failed")
            recoveryProbesFailed++;
        else if (event->category == "unreachable")
            recoveryProbesUnreachable++;
        else if (event->category == "abandoned")
            recoveryProbesAbandoned++;
        else
            throw cRuntimeError("Unknown recovery probe category '%s'",
                                event->category.c_str());
    }
    else if (signalId == repositionSignal) {
        // Há no máximo uma decisão por alerta, portanto alertId identifica o ciclo.
        if (event->category == "started") {
            if (startedRepositionAlertIds.insert(event->alertId).second) {
                repositionsStarted++;
                repositionStartTimes[event->alertId] = event->referenceTime;
            }
        }
        else if (event->category == "completed") {
            if (!startedRepositionAlertIds.count(event->alertId))
                throw cRuntimeError("Completed reposition for unknown alert '%s'",
                                    event->alertId.c_str());
            if (completedRepositionAlertIds.insert(event->alertId).second) {
                if (!measuredRepositionAlertIds.count(event->alertId))
                    throw cRuntimeError("Completed reposition without distance for '%s'",
                                        event->alertId.c_str());
                auto start = repositionStartTimes.find(event->alertId);
                if (start == repositionStartTimes.end())
                    throw cRuntimeError("Completed reposition without start time for '%s'",
                                        event->alertId.c_str());
                if (event->referenceTime < start->second)
                    throw cRuntimeError("Negative reposition duration for '%s'",
                                        event->alertId.c_str());
                repositionsCompleted++;
                repositionDurationSum += event->referenceTime - start->second;
            }
        }
        else if (event->category == "distance") {
            if (!startedRepositionAlertIds.count(event->alertId))
                throw cRuntimeError("Distance for unknown reposition alert '%s'",
                                    event->alertId.c_str());
            if (event->value < 0)
                throw cRuntimeError("Negative reposition distance for '%s'",
                                    event->alertId.c_str());
            if (measuredRepositionAlertIds.insert(event->alertId).second) {
                repositionDistanceSum += event->value;
                // Classificação analítica; o valor bruto acima é preservado.
                if (event->value > effectiveRepositionThreshold)
                    effectiveRepositions++;
            }
        }
    }
}

void ExperimentMetrics::writeAlertRecords() const
{
    std::string directory = par("alertRecordDirectory").stdstringValue();
    if (directory.empty())
        return;
    auto *config = getEnvir()->getConfigEx();
    std::string name = directory + "/" + config->getVariable("configname") + "-" +
        config->getVariable("runnumber") + "-alerts.csv";
    std::ofstream file(name);
    if (!file)
        throw cRuntimeError("Cannot write alert records to '%s'", name.c_str());
    file << "alertId,victimId,droneId,generationTime,delivered,"
            "receivingTeamId,acknowledged,ackTeamId,retryCount\n";
    for (const auto& alertId : alertOrder) {
        const auto& record = alertRecords.at(alertId);
        // retryCount conta retransmissões, não tentativas: a primeira não é retry.
        file << record.alertId << ',' << record.victimId << ',' << record.droneId
             << ',' << record.generationTime.dbl()
             << ',' << (record.delivered ? 1 : 0) << ',' << record.receivingTeamId
             << ',' << (record.acknowledged ? 1 : 0) << ',' << record.ackTeamId
             << ',' << std::max(0, record.attempts - 1) << '\n';
    }
}

void ExperimentMetrics::finish()
{
    writeAlertRecords();

    int applicationRetries = 0;
    for (const auto& [alertId, attempts] : attemptsByAlert)
        applicationRetries += std::max(0, attempts - 1);

    int recoveryProbeOutcomes = recoveryProbesConfirmed + recoveryProbesFailed +
        recoveryProbesUnreachable + recoveryProbesAbandoned;

    auto isSubset = [](const auto& subset, const auto& superset) {
        return std::includes(superset.begin(), superset.end(),
                             subset.begin(), subset.end());
    };
    bool confirmedAndExpiredOverlap = std::any_of(
        confirmedAlertIds.begin(), confirmedAlertIds.end(),
        [&](const auto& id) { return expiredAlertIds.count(id); });
    bool incompleteAlerts = confirmedAlertIds.size() + expiredAlertIds.size() !=
        generatedAlertIds.size();

    // Relações que detectam contagem duplicada e estados impossíveis.
    if (!isSubset(deliveredAlertIds, generatedAlertIds) ||
        !isSubset(confirmedAlertIds, deliveredAlertIds) ||
        !isSubset(expiredAlertIds, generatedAlertIds) ||
        !isSubset(receivedAttemptIds, sentAttemptIds) ||
        confirmedAndExpiredOverlap ||
        (par("requireClosedAlerts").boolValue() && incompleteAlerts) ||
        confirmedAlertIds.size() + expiredAlertIds.size() > generatedAlertIds.size() ||
        repositionsCompleted > repositionsStarted ||
        repositionsStarted > baActivations ||
        // O funil da câmera: consultada no máximo uma vez por gatilho, e a
        // detecção é um subconjunto das consultas.
        sensorEvaluations > repositionTriggers ||
        obstaclesDetected > sensorEvaluations ||
        effectiveRepositions > repositionsCompleted ||
        // O funil da sondagem: uma verificação por reposicionamento concluído,
        // cada uma com um único desfecho. Contar mais desfechos que aberturas
        // seria contagem dupla e é sempre erro. A igualdade só é exigida onde
        // os alertas também precisam ter fechado: uma execução truncada pode
        // terminar com a última verificação ainda em curso.
        recoveryProbeChecks > repositionsCompleted ||
        recoveryProbeOutcomes > recoveryProbeChecks ||
        (par("requireClosedAlerts").boolValue() &&
            recoveryProbeOutcomes != recoveryProbeChecks) ||
        // Confirmar ou esgotar exige ter perguntado; desistir e não ter destino
        // não exigem, então ficam fora desta cota.
        recoveryProbesConfirmed + recoveryProbesFailed > recoveryProbesSent ||
        measuredRepositionAlertIds != completedRepositionAlertIds ||
        confirmationDelayCount != static_cast<int>(confirmedAlertIds.size()) ||
        hopCountCount != static_cast<int>(deliveredAlertIds.size()) ||
        multiHopDeliveries > hopCountCount ||
        alertsWithoutKnownTeam.size() > generatedAlertIds.size())
        throw cRuntimeError("ExperimentMetrics invariant violation: generated=%zu, "
                            "delivered=%zu, confirmed=%zu, expired=%zu, triggers=%d, "
                            "obstacles=%d, BA=%d, started=%d, completed=%d, distances=%zu",
                            generatedAlertIds.size(), deliveredAlertIds.size(),
                            confirmedAlertIds.size(), expiredAlertIds.size(),
                            repositionTriggers, obstaclesDetected, baActivations,
                            repositionsStarted, repositionsCompleted,
                            measuredRepositionAlertIds.size());

    recordScalar("alertsGenerated", generatedAlertIds.size());
    recordScalar("alertsDelivered", deliveredAlertIds.size());
    recordScalar("alertsConfirmed", confirmedAlertIds.size());
    recordScalar("alertsExpired", expiredAlertIds.size());
    recordScalar("alertAttemptsSent", sentAttemptIds.size());
    recordScalar("attemptsReceived", receivedAttemptIds.size());
    recordScalar("applicationRetries", applicationRetries);
    recordScalar("confirmationDelaySum", confirmationDelaySum.dbl());
    recordScalar("confirmationDelayCount", confirmationDelayCount);
    recordScalar("hopCountSum", hopCountSum);
    recordScalar("hopCountCount", hopCountCount);
    recordScalar("multiHopDeliveries", multiHopDeliveries);
    recordScalar("intermediateForwardings", intermediateForwardings);
    recordScalar("neverKnownTeamSelectionEvents", neverKnownTeamSelectionEvents);
    recordScalar("expiredKnownTeamSelectionEvents", expiredKnownTeamSelectionEvents);
    recordScalar("alertsWithoutKnownTeam", alertsWithoutKnownTeam.size());
    recordScalar("repositionTriggers", repositionTriggers);
    recordScalar("sensorEvaluations", sensorEvaluations);
    recordScalar("obstaclesDetected", obstaclesDetected);
    recordScalar("baActivations", baActivations);
    recordScalar("repositionsStarted", repositionsStarted);
    recordScalar("repositionsCompleted", repositionsCompleted);
    recordScalar("repositionDistanceSum", repositionDistanceSum);
    recordScalar("effectiveRepositions", effectiveRepositions);
    recordScalar("repositionDurationSum", repositionDurationSum.dbl());
    recordScalar("recoveryProbeChecks", recoveryProbeChecks);
    recordScalar("recoveryProbesSent", recoveryProbesSent);
    recordScalar("recoveryProbesConfirmed", recoveryProbesConfirmed);
    recordScalar("recoveryProbesFailed", recoveryProbesFailed);
    recordScalar("recoveryProbesUnreachable", recoveryProbesUnreachable);
    recordScalar("recoveryProbesAbandoned", recoveryProbesAbandoned);
}

} // namespace echosar
