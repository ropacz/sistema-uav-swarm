#include "SarScenarioManager.h"

#include <limits>
#include <set>

#include "inet/common/InitStages.h"
#include "inet/mobility/contract/IMobility.h"
#include "messages/VictimAssignment_m.h"

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(SarScenarioManager);

int SarScenarioManager::numInitStages() const
{
    return NUM_INIT_STAGES;
}

void SarScenarioManager::initialize(int stage)
{
    if (stage != INITSTAGE_APPLICATION_LAYER)
        return;
    auto network = getParentModule();
    std::set<std::string> ids;
    std::set<std::string> nodeIds;
    for (const char *vectorName : {"drone", "team"}) {
        int nodeCount = network->getSubmoduleVectorSize(vectorName);
        for (int i = 0; i < nodeCount; ++i) {
            auto node = network->getSubmodule(vectorName, i);
            auto app = node->getSubmodule("app", 0);
            const char *parameterName = std::string(vectorName) == "drone" ? "droneId" : "teamId";
            std::string id = app->par(parameterName).stdstringValue();
            if (id.empty()) id = node->getFullName();
            if (!nodeIds.insert(id).second)
                throw cRuntimeError("Drone/team IDs must be unique: '%s'", id.c_str());
        }
    }
    int count = network->getSubmoduleVectorSize("victim");
    for (int i = 0; i < count; ++i) {
        auto victim = network->getSubmodule("victim", i);
        std::string id = victim->par("victimId").stdstringValue();
        if (id.empty() || !ids.insert(id).second)
            throw cRuntimeError("Victim IDs must be non-empty and unique: '%s'", id.c_str());
        auto event = new cMessage(("detect-" + id).c_str());
        detections[event] = victim;
        scheduleAt(victim->par("detectionTime"), event);
    }
}

void SarScenarioManager::handleMessage(cMessage *message)
{
    auto victim = detections.at(message);
    detections.erase(message);
    auto network = getParentModule();
    Coord victimPosition(victim->par("positionX").doubleValueInUnit("m"),
                         victim->par("positionY").doubleValueInUnit("m"),
                         victim->par("positionZ").doubleValueInUnit("m"));
    cModule *selectedApp = nullptr;
    double bestDistance = std::numeric_limits<double>::infinity();
    int bestIndex = std::numeric_limits<int>::max();
    std::string bestId;
    int count = network->getSubmoduleVectorSize("drone");
    for (int i = 0; i < count; ++i) {
        auto drone = network->getSubmodule("drone", i);
        auto app = drone->getSubmodule("app", 0);
        std::string droneId = app->par("droneId").stdstringValue();
        if (droneId.empty()) droneId = drone->getFullName();
        auto mobility = check_and_cast<IMobility *>(drone->getSubmodule("mobility"));
        double distance = mobility->getCurrentPosition().distance(victimPosition);
        if (distance < bestDistance || (distance == bestDistance && (bestId.empty() || droneId < bestId))) {
            selectedApp = app;
            bestDistance = distance;
            bestIndex = i;
            bestId = droneId;
        }
    }
    if (!selectedApp)
        throw cRuntimeError("No active drone available for victim assignment");

    auto assignment = new VictimAssignment("victimAssignment");
    std::string victimId = victim->par("victimId").stdstringValue();
    assignment->setAlertId((victimId + "-event").c_str());
    assignment->setVictimId(victimId.c_str());
    assignment->setVictimPositionX(victimPosition.x);
    assignment->setVictimPositionY(victimPosition.y);
    assignment->setVictimPositionZ(victimPosition.z);
    assignment->setDetectionTimestamp(simTime());
    sendDirect(assignment, selectedApp, "assignmentIn");
    EV_INFO << "Victim " << victimId << " assigned to drone[" << bestIndex << "]\n";
    delete message;
}

SarScenarioManager::~SarScenarioManager()
{
    for (auto& entry : detections)
        cancelAndDelete(entry.first);
}

} // namespace echosar
