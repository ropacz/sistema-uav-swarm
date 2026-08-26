#pragma once

#include <map>
#include <set>
#include <string>

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
class ExperimentMetrics : public omnetpp::cSimpleModule, public omnetpp::cListener
{
  protected:
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

    omnetpp::simtime_t deliveryDelaySum = 0;
    omnetpp::simtime_t confirmationDelaySum = 0;
    omnetpp::simtime_t repositionDurationSum = 0;
    int deliveryDelayCount = 0;
    int confirmationDelayCount = 0;
    int noKnownTeamFailures = 0;
    int hopCountSum = 0;
    int hopCountCount = 0;
    int multiHopDeliveries = 0;
    int intermediateForwardings = 0;
    int repositionTriggers = 0;
    int obstaclesDetected = 0;
    int baActivations = 0;
    int repositionsStarted = 0;
    int repositionsCompleted = 0;
    double repositionDistanceSum = 0;

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
