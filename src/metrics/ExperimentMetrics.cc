#include "ExperimentMetrics.h"

#include <algorithm>

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
    degradationSignal = registerSignal("victimDegradationIndicated");
    sensorSignal = registerSignal("victimSensorEvaluated");
    baActivationSignal = registerSignal("victimBaActivated");
    repositionSignal = registerSignal("victimRepositionEvent");

    auto network = getSimulation()->getSystemModule();
    network->subscribe(generatedSignal, this);
    network->subscribe(attemptSentSignal, this);
    network->subscribe(deliveredSignal, this);
    network->subscribe(confirmedSignal, this);
    network->subscribe(expiredSignal, this);
    network->subscribe(degradationSignal, this);
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
        if (!event->messageId.empty())
            attemptSentTimes[event->messageId] = event->referenceTime;
    }
    else if (signalId == deliveredSignal) {
        if (!event->messageId.empty() && deliveredMessageIds.insert(event->messageId).second) {
            attemptDeliveryDelaySum += simTime() - event->secondaryTime;
        }
        if (deliveredAlertIds.insert(event->alertId).second) {
            auto generation = generationTimes.find(event->alertId);
            simtime_t start = generation == generationTimes.end()
                ? event->referenceTime : generation->second;
            deliveryDelaySum += simTime() - start;
            deliveryDelayCount++;
        }
    }
    else if (signalId == confirmedSignal) {
        if (confirmedAlertIds.insert(event->alertId).second) {
            auto sent = attemptSentTimes.find(event->messageId);
            simtime_t start = sent == attemptSentTimes.end()
                ? event->referenceTime : sent->second;
            rttSum += simTime() - start;
            rttCount++;
        }
    }
    else if (signalId == expiredSignal)
        expiredAlertIds.insert(event->alertId);
    else if (signalId == degradationSignal)
        degradationIndications++;
    else if (signalId == sensorSignal) {
        if (event->category == "confirmed")
            sensorConfirmations++;
        else if (event->category == "teamUnknown")
            teamUnknownForReposition++;
        else {
            sensorRejections++;
            if (event->category == "clearLineOfSight")
                sensorClearLineOfSight++;
            else if (event->category == "outsideVisualRange")
                sensorOutsideRange++;
        }
    }
    else if (signalId == baActivationSignal)
        baActivations++;
    else if (signalId == repositionSignal) {
        if (event->category == "started") {
            repositionsStarted++;
            commandedRepositionDistanceSum += event->value;
        }
        else if (event->category == "completed")
            repositionsCompleted++;
        else if (event->category == "validated" ||
                 event->category == "recoveredDuringMovement" ||
                 event->category == "recoveredAfterArrival") {
            if (event->category == "validated")
                repositionsValidated++;
            else if (event->category == "recoveredDuringMovement")
                repositionsRecoveredDuringMovement++;
            else
                repositionsRecoveredAfterArrival++;
            recoveryTimeSum += simTime() - event->referenceTime;
        }
        else if (event->category == "expired")
            repositionsExpired++;
        else if (event->category == "noFeasibleSolution" ||
                 event->category == "redundantCandidate") {
            repositionsFailedBeforeMovement++;
            if (event->category == "noFeasibleSolution")
                baNoFeasibleSolution++;
            else
                baRedundantCandidate++;
        }
        else if (event->category == "distance") {
            repositionDistanceSum += event->value;
            repositionDistanceCount++;
        }
    }
}

void ExperimentMetrics::finish()
{
    int applicationRetries = 0;
    for (const auto& [alertId, attempts] : attemptsByAlert)
        applicationRetries += std::max(0, attempts - 1);

    double generated = generatedAlertIds.size();
    double pdr = generated > 0 ? deliveredAlertIds.size() / generated : 0;
    double confirmationRate = generated > 0 ? confirmedAlertIds.size() / generated : 0;
    double attemptDeliveryRate = alertAttemptsSent > 0
        ? static_cast<double>(deliveredMessageIds.size()) / alertAttemptsSent : 0;
    int recoveredWithoutValidation = repositionsRecoveredDuringMovement +
        repositionsRecoveredAfterArrival;
    int successfulRepositions = repositionsValidated + recoveredWithoutValidation;
    double repositionSuccessRate = repositionsStarted > 0
        ? static_cast<double>(successfulRepositions) / repositionsStarted : 0;

    if (deliveredAlertIds.size() > generatedAlertIds.size() ||
        confirmedAlertIds.size() > deliveredAlertIds.size() ||
        confirmedAlertIds.size() + expiredAlertIds.size() > generatedAlertIds.size() ||
        deliveredMessageIds.size() > static_cast<size_t>(alertAttemptsSent) ||
        successfulRepositions > repositionsStarted)
        throw cRuntimeError("ExperimentMetrics invariant violation: generated=%zu, "
                            "delivered=%zu, confirmed=%zu, expired=%zu, attempts=%d, "
                            "attemptsDelivered=%zu, repositionsStarted=%d, successes=%d",
                            generatedAlertIds.size(), deliveredAlertIds.size(),
                            confirmedAlertIds.size(), expiredAlertIds.size(),
                            alertAttemptsSent, deliveredMessageIds.size(),
                            repositionsStarted, successfulRepositions);

    recordScalar("alertsGenerated", generatedAlertIds.size());
    recordScalar("alertsDelivered", deliveredAlertIds.size());
    recordScalar("alertsConfirmed", confirmedAlertIds.size());
    recordScalar("alertsExpired", expiredAlertIds.size());
    recordScalar("alertAttemptsSent", alertAttemptsSent);
    recordScalar("alertAttemptsDelivered", deliveredMessageIds.size());
    recordScalar("alertAttemptsLost",
                 alertAttemptsSent - static_cast<int>(deliveredMessageIds.size()));
    recordScalar("applicationRetries", applicationRetries);
    recordScalar("deliveryDelaySum", deliveryDelaySum.dbl());
    recordScalar("deliveryDelayCount", deliveryDelayCount);
    recordScalar("attemptDeliveryDelaySum", attemptDeliveryDelaySum.dbl());
    recordScalar("attemptDeliveryDelayCount", deliveredMessageIds.size());
    recordScalar("rttSum", rttSum.dbl());
    recordScalar("rttCount", rttCount);
    recordScalar("pdr", pdr);
    recordScalar("packetLossRate", 1 - pdr);
    recordScalar("confirmationRate", confirmationRate);
    recordScalar("attemptDeliveryRate", attemptDeliveryRate);
    recordScalar("attemptLossRate", 1 - attemptDeliveryRate);
    recordScalar("degradationIndications", degradationIndications);
    recordScalar("sensorConfirmations", sensorConfirmations);
    recordScalar("sensorRejections", sensorRejections);
    recordScalar("teamUnknownForReposition", teamUnknownForReposition);
    recordScalar("sensorClearLineOfSight", sensorClearLineOfSight);
    recordScalar("sensorOutsideRange", sensorOutsideRange);
    recordScalar("baActivations", baActivations);
    recordScalar("repositionsStarted", repositionsStarted);
    recordScalar("repositionsCompleted", repositionsCompleted);
    recordScalar("repositionsValidated", repositionsValidated);
    recordScalar("repositionsRecoveredDuringMovement", repositionsRecoveredDuringMovement);
    recordScalar("repositionsRecoveredAfterArrival", repositionsRecoveredAfterArrival);
    recordScalar("repositionsRecoveredWithoutValidation", recoveredWithoutValidation);
    recordScalar("repositionsExpired", repositionsExpired);
    recordScalar("repositionsFailedBeforeMovement", repositionsFailedBeforeMovement);
    recordScalar("baNoFeasibleSolution", baNoFeasibleSolution);
    recordScalar("baRedundantCandidate", baRedundantCandidate);
    recordScalar("successfulRepositions", successfulRepositions);
    recordScalar("repositionSuccessRate", repositionSuccessRate);
    recordScalar("recoveryTimeSum", recoveryTimeSum.dbl());
    recordScalar("recoveryTimeCount", successfulRepositions);
    recordScalar("repositionDistanceSum", repositionDistanceSum);
    recordScalar("repositionDistanceCount", repositionDistanceCount);
    recordScalar("commandedRepositionDistanceSum", commandedRepositionDistanceSum);
}

} // namespace echosar
