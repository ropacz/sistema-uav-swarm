#include "BaGaussMarkovMobility.h"

#include <algorithm>

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(BaGaussMarkovMobility);

void BaGaussMarkovMobility::setTargetPosition()
{
    if (baOverride) {
        baOverride = false;
        holding = true;
        stationary = true;
        nextChange = -1;
        lastVelocity = Coord::ZERO;
        return;
    }
    waypointId++;
    GaussMarkovMobility::setTargetPosition();
}

void BaGaussMarkovMobility::moveTo(const Coord& destination, double horizontalSpeed,
                                   double climbSpeed, double descentSpeed)
{
    getCurrentPosition();
    Coord delta = destination - lastPosition;
    double horizontalTime = std::hypot(delta.x, delta.y) / horizontalSpeed;
    double verticalSpeed = delta.z >= 0 ? climbSpeed : descentSpeed;
    double verticalTime = std::abs(delta.z) / verticalSpeed;
    double travelTime = std::max(horizontalTime, verticalTime);
    if (travelTime <= 0) {
        holding = true;
        return;
    }
    targetPosition = destination;
    nextChange = simTime() + travelTime;
    lastVelocity = delta / travelTime;
    stationary = false;
    holding = false;
    baOverride = true;
    waypointId++;
    scheduleUpdate();
}

void BaGaussMarkovMobility::resumeNormal()
{
    // May be called while the BA leg is still in progress (for example when an
    // ACK arrives before reaching the candidate). Rebase Gauss-Markov at the
    // interpolated current position instead of leaving a pending BA override.
    getCurrentPosition();
    baOverride = false;
    holding = false;
    stationary = false;
    GaussMarkovMobility::setTargetPosition();
    scheduleUpdate();
}

} // namespace echosar
