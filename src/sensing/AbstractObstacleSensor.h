#pragma once

#include <string>

#include "inet/common/ModuleRefByPar.h"
#include "inet/environment/contract/IPhysicalEnvironment.h"
#include "omnetpp.h"

namespace echosar {

struct ObstacleObservation {
    bool confirmed = false;
    int obstacleId = -1;
    std::string obstacleName;
    inet::Coord center;
    inet::Coord nearestSurfacePoint;
    double distance = NAN;
    omnetpp::simtime_t timestamp;
    std::string reason;
};

class AbstractObstacleSensor : public omnetpp::cSimpleModule
{
  protected:
    inet::ModuleRefByPar<inet::physicalenvironment::IPhysicalEnvironment> environment;
    double minimumRange = 0.7;
    double maximumRange = 30;

    virtual void initialize() override;
    virtual void handleMessage(omnetpp::cMessage *message) override { delete message; }

  public:
    ObstacleObservation inspect(const inet::Coord& dronePosition,
                                const inet::Coord& teamPosition) const;
    bool intersectsAnyObstacle(const inet::Coord& a, const inet::Coord& b) const;
};

} // namespace echosar
