#include "ExperimentMetrics.h"

#include <algorithm>
#include <limits>

#include "AlertMetricEvent.h"

using namespace omnetpp;

namespace echosar {

Register_Class(AlertMetricEvent);
Define_Module(ExperimentMetrics);

void ExperimentMetrics::initialize()
{
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
        if (generatedAlertIds.insert(event->alertId).second)
            creationTimes[event->alertId] = event->referenceTime;
    }
    else if (signalId == attemptSentSignal) {
        if (!generatedAlertIds.count(event->alertId))
            throw cRuntimeError("Attempt for unknown alert '%s'",
                                event->alertId.c_str());
        if (event->messageId.empty())
            throw cRuntimeError("Attempt signal for alert '%s' has no messageId",
                                event->alertId.c_str());
        if (sentAttemptIds.insert(event->messageId).second)
            attemptsByAlert[event->alertId]++;
    }
    else if (signalId == deliveredSignal) {
        if (event->messageId.empty())
            throw cRuntimeError("Delivery signal for alert '%s' has no messageId",
                                event->alertId.c_str());
        receivedAttemptIds.insert(event->messageId);
        // A primeira equipe que recebe o alertId define entrega e atraso fim a fim.
        if (deliveredAlertIds.insert(event->alertId).second) {
            int hopCount = static_cast<int>(event->value);
            if (hopCount < 1 || event->value != hopCount)
                throw cRuntimeError("Invalid hop count %.3f for alert '%s'",
                                    event->value, event->alertId.c_str());
            auto creation = creationTimes.find(event->alertId);
            if (creation == creationTimes.end())
                throw cRuntimeError("Delivery for unknown alert '%s'",
                                    event->alertId.c_str());
            deliveryDelaySum += simTime() - creation->second;
            deliveryDelayCount++;
            hopCountSum += hopCount;
            hopCountCount++;
            if (hopCount > 1)
                multiHopDeliveries++;
            intermediateForwardings += hopCount - 1;
        }
    }
    else if (signalId == confirmedSignal) {
        if (confirmedAlertIds.insert(event->alertId).second) {
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
        else if (event->category == "knownTeamNoAck")
            knownTeamNoAckTimeoutEvents++;
        else
            throw cRuntimeError("Unknown operational failure '%s'",
                                event->category.c_str());
    }
    else if (signalId == repositionTriggerSignal)
        repositionTriggers++;
    else if (signalId == sensorSignal) {
        if (event->category == "detected")
            obstaclesDetected++;
    }
    else if (signalId == baActivationSignal)
        baActivations++;
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
            }
        }
    }
}

void ExperimentMetrics::finish()
{
    int applicationRetries = 0;
    for (const auto& [alertId, attempts] : attemptsByAlert)
        applicationRetries += std::max(0, attempts - 1);

    const double undefined = std::numeric_limits<double>::quiet_NaN();
    double generated = generatedAlertIds.size();
    double pdr = generated > 0 ? deliveredAlertIds.size() / generated : undefined;
    double confirmationRate = generated > 0
        ? confirmedAlertIds.size() / generated : undefined;

    auto isSubset = [](const auto& subset, const auto& superset) {
        return std::includes(superset.begin(), superset.end(),
                             subset.begin(), subset.end());
    };
    bool confirmedAndExpiredOverlap = std::any_of(
        confirmedAlertIds.begin(), confirmedAlertIds.end(),
        [&](const auto& id) { return expiredAlertIds.count(id); });

    // Relações que detectam contagem duplicada e estados impossíveis.
    if (!isSubset(deliveredAlertIds, generatedAlertIds) ||
        !isSubset(confirmedAlertIds, deliveredAlertIds) ||
        !isSubset(expiredAlertIds, generatedAlertIds) ||
        !isSubset(receivedAttemptIds, sentAttemptIds) ||
        confirmedAndExpiredOverlap ||
        confirmedAlertIds.size() + expiredAlertIds.size() > generatedAlertIds.size() ||
        repositionsCompleted > repositionsStarted ||
        repositionsStarted > baActivations ||
        obstaclesDetected > repositionTriggers ||
        measuredRepositionAlertIds != completedRepositionAlertIds ||
        confirmationDelayCount != static_cast<int>(confirmedAlertIds.size()) ||
        hopCountCount != static_cast<int>(deliveredAlertIds.size()) ||
        multiHopDeliveries > hopCountCount ||
        alertsWithoutKnownTeam.size() > generatedAlertIds.size() ||
        knownTeamNoAckTimeoutEvents > static_cast<int>(expiredAlertIds.size()))
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
    recordScalar("deliveryDelaySum", deliveryDelaySum.dbl());
    recordScalar("deliveryDelayCount", deliveryDelayCount);
    recordScalar("confirmationDelaySum", confirmationDelaySum.dbl());
    recordScalar("confirmationDelayCount", confirmationDelayCount);
    recordScalar("hopCountSum", hopCountSum);
    recordScalar("hopCountCount", hopCountCount);
    recordScalar("multiHopDeliveries", multiHopDeliveries);
    recordScalar("intermediateForwardings", intermediateForwardings);
    recordScalar("neverKnownTeamSelectionEvents", neverKnownTeamSelectionEvents);
    recordScalar("expiredKnownTeamSelectionEvents", expiredKnownTeamSelectionEvents);
    recordScalar("knownTeamNoAckTimeoutEvents", knownTeamNoAckTimeoutEvents);
    recordScalar("alertsWithoutKnownTeam", alertsWithoutKnownTeam.size());
    recordScalar("pdr", pdr);
    recordScalar("confirmationRate", confirmationRate);
    recordScalar("repositionTriggers", repositionTriggers);
    recordScalar("obstaclesDetected", obstaclesDetected);
    recordScalar("baActivations", baActivations);
    recordScalar("repositionsStarted", repositionsStarted);
    recordScalar("repositionsCompleted", repositionsCompleted);
    recordScalar("repositionDistanceSum", repositionDistanceSum);
    recordScalar("repositionDurationSum", repositionDurationSum.dbl());
}

} // namespace echosar
