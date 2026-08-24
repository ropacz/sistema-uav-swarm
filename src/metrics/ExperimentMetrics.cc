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
            generationTimes[event->alertId] = event->referenceTime;
    }
    else if (signalId == attemptSentSignal) {
        alertAttemptsSent++;
        attemptsByAlert[event->alertId]++;
    }
    else if (signalId == deliveredSignal) {
        // A primeira equipe que recebe o alertId define entrega e atraso fim a fim.
        if (deliveredAlertIds.insert(event->alertId).second) {
            auto generation = generationTimes.find(event->alertId);
            simtime_t start = generation == generationTimes.end()
                ? event->referenceTime : generation->second;
            deliveryDelaySum += simTime() - start;
            deliveryDelayCount++;
        }
    }
    else if (signalId == confirmedSignal)
        confirmedAlertIds.insert(event->alertId);
    else if (signalId == expiredSignal)
        expiredAlertIds.insert(event->alertId);
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
            if (startedRepositionAlertIds.insert(event->alertId).second)
                repositionsStarted++;
        }
        else if (event->category == "completed") {
            if (!startedRepositionAlertIds.count(event->alertId))
                throw cRuntimeError("Completed reposition for unknown alert '%s'",
                                    event->alertId.c_str());
            if (completedRepositionAlertIds.insert(event->alertId).second)
                repositionsCompleted++;
        }
        else if (event->category == "distance") {
            if (!startedRepositionAlertIds.count(event->alertId))
                throw cRuntimeError("Distance for unknown reposition alert '%s'",
                                    event->alertId.c_str());
            if (measuredRepositionAlertIds.insert(event->alertId).second) {
                repositionDistanceSum += event->value;
                repositionDistanceCount++;
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

    // Relações que detectam contagem duplicada e estados impossíveis.
    if (deliveredAlertIds.size() > generatedAlertIds.size() ||
        confirmedAlertIds.size() > deliveredAlertIds.size() ||
        confirmedAlertIds.size() + expiredAlertIds.size() > generatedAlertIds.size() ||
        repositionsCompleted > repositionsStarted ||
        repositionsStarted > baActivations ||
        obstaclesDetected > repositionTriggers ||
        repositionDistanceCount > repositionsStarted)
        throw cRuntimeError("ExperimentMetrics invariant violation: generated=%zu, "
                            "delivered=%zu, confirmed=%zu, expired=%zu, triggers=%d, "
                            "obstacles=%d, BA=%d, started=%d, completed=%d, distances=%d",
                            generatedAlertIds.size(), deliveredAlertIds.size(),
                            confirmedAlertIds.size(), expiredAlertIds.size(),
                            repositionTriggers, obstaclesDetected, baActivations,
                            repositionsStarted, repositionsCompleted,
                            repositionDistanceCount);

    recordScalar("alertsGenerated", generatedAlertIds.size());
    recordScalar("alertsDelivered", deliveredAlertIds.size());
    recordScalar("alertsConfirmed", confirmedAlertIds.size());
    recordScalar("alertsExpired", expiredAlertIds.size());
    recordScalar("alertAttemptsSent", alertAttemptsSent);
    recordScalar("applicationRetries", applicationRetries);
    recordScalar("deliveryDelaySum", deliveryDelaySum.dbl());
    recordScalar("deliveryDelayCount", deliveryDelayCount);
    recordScalar("pdr", pdr);
    recordScalar("confirmationRate", confirmationRate);
    recordScalar("repositionTriggers", repositionTriggers);
    recordScalar("obstaclesDetected", obstaclesDetected);
    recordScalar("baActivations", baActivations);
    recordScalar("repositionsStarted", repositionsStarted);
    recordScalar("repositionsCompleted", repositionsCompleted);
    recordScalar("repositionDistanceSum", repositionDistanceSum);
    recordScalar("repositionDistanceCount", repositionDistanceCount);
}

} // namespace echosar
