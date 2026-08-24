#pragma once

#include <map>
#include <set>
#include <string>

#include "omnetpp/clistener.h"
#include "omnetpp/cmessage.h"
#include "omnetpp/csimplemodule.h"
#include "omnetpp/simtime.h"

namespace echosar {

/** Agrega, por alertId, as métricas fim a fim de uma execução. */
class ExperimentMetrics : public omnetpp::cSimpleModule, public omnetpp::cListener
{
  protected:
    std::set<std::string> generatedAlertIds;
    std::set<std::string> deliveredAlertIds;
    std::set<std::string> confirmedAlertIds;
    std::set<std::string> expiredAlertIds;
    std::set<std::string> deliveredMessageIds;
    // Identidades de ciclo tornam os eventos de movimento idempotentes e
    // permitem rejeitar uma conclusão atribuída a um ciclo inexistente.
    std::set<std::string> startedRepositionCycleIds;
    std::set<std::string> completedRepositionCycleIds;
    std::set<std::string> terminalRepositionCycleIds;
    std::set<std::string> measuredRepositionCycleIds;
    std::map<std::string, int> attemptsByAlert;
    std::map<std::string, omnetpp::simtime_t> generationTimes;
    std::map<std::string, omnetpp::simtime_t> attemptSentTimes;

    omnetpp::simtime_t deliveryDelaySum = 0;
    omnetpp::simtime_t attemptDeliveryDelaySum = 0;
    omnetpp::simtime_t rttSum = 0;
    omnetpp::simtime_t recoveryTimeSum = 0;
    omnetpp::simtime_t validatedRecoveryTimeSum = 0;
    int validatedRecoveryTimeCount = 0;
    int deliveryDelayCount = 0;
    int rttCount = 0;
    int alertAttemptsSent = 0;
    int degradationIndications = 0;
    int sensorConfirmations = 0;
    int sensorRejections = 0;
    int teamUnknownForReposition = 0;
    int sensorClearLineOfSight = 0;
    int sensorOutsideRange = 0;
    int baActivations = 0;
    int repositionsStarted = 0;
    int repositionsCompleted = 0;
    int repositionsValidated = 0;
    int repositionsRecoveredDuringMovement = 0;
    int repositionsRecoveredAfterArrival = 0;
    int repositionsExpired = 0;
    int repositionsFailedBeforeMovement = 0;
    int baNoFeasibleSolution = 0;
    int baRedundantCandidate = 0;
    int repositionDistanceCount = 0;
    double repositionDistanceSum = 0;
    double commandedRepositionDistanceSum = 0;

    omnetpp::simsignal_t generatedSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t attemptSentSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t deliveredSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t confirmedSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t expiredSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t degradationSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t sensorSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t baActivationSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t repositionSignal = SIMSIGNAL_NULL;

    using omnetpp::cListener::finish;
    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *message) override;
    virtual void finish() override;
    virtual void receiveSignal(omnetpp::cComponent *source,
                               omnetpp::simsignal_t signalId,
                               omnetpp::cObject *value,
                               omnetpp::cObject *details) override;
};

} // namespace echosar
