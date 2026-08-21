#include "RepositionFitness.h"

#include <algorithm>
#include <cmath>

#include "sensing/AbstractObstacleSensor.h"

using inet::Coord;

namespace echosar {

RepositionFitness::RepositionFitness(const FitnessParameters& parameters,
                                     const AbstractObstacleSensor *sensor,
                                     const Coord& current,
                                     const Coord& teamPosition,
                                     const Coord& obstaclePoint,
                                     omnetpp::simtime_t now) :
    parameters(parameters), sensor(sensor), current(current),
    teamPosition(teamPosition), obstaclePoint(obstaclePoint), now(now)
{
}

double RepositionFitness::travelTime(const Coord& from, const Coord& to) const
{
    // Os eixos evoluem simultaneamente; o mais lento determina a chegada.
    double horizontal = std::hypot(to.x - from.x, to.y - from.y) / parameters.horizontalSpeed;
    double vertical = std::abs(to.z - from.z) /
        (to.z >= from.z ? parameters.climbSpeed : parameters.descentSpeed);
    return std::max(horizontal, vertical);
}

double RepositionFitness::cost(const Coord& candidate) const
{
    // Usa apenas qualidade estimada; o RSSI futuro não é conhecido pelo BA.
    double linkCost = std::clamp(
        candidate.distance(teamPosition) / parameters.linkNormalizationDistance, 0.0, 1.0);
    double proximity = std::exp(-candidate.distance(obstaclePoint) / parameters.obstacleSigma);
    // Um candidato que permanece obstruído recebe custo máximo de obstáculo,
    // por mais distante que esteja da superfície detectada.
    double obstacleCost = std::max(
        proximity, sensor->intersectsAnyObstacle(candidate, teamPosition) ? 1.0 : 0.0);
    double movementCost = std::clamp(
        candidate.distance(current) / parameters.maximumRepositionDistance, 0.0, 1.0);
    return parameters.wLink * linkCost + parameters.wObstacle * obstacleCost +
           parameters.wMove * movementCost;
}

bool RepositionFitness::feasible(const Coord& candidate) const
{
    if (candidate.x < parameters.areaMinX || candidate.x > parameters.areaMaxX ||
        candidate.y < parameters.areaMinY || candidate.y > parameters.areaMaxY ||
        candidate.z < parameters.minimumAltitude || candidate.z > parameters.maximumAltitude ||
        candidate.distance(current) > parameters.maximumRepositionDistance ||
        candidate.distance(obstaclePoint) < parameters.obstacleSafetyMargin)
        return false;
    if (now + travelTime(current, candidate) > parameters.flightTimeLimit)
        return false;
    return !sensor->intersectsAnyObstacle(current, candidate);
}

} // namespace echosar
