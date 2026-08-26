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
                                     const std::vector<Coord>& neighborPositions,
                                     bool preserveConnectivity,
                                     omnetpp::simtime_t now) :
    parameters(parameters), sensor(sensor), current(current), teamPosition(teamPosition),
    obstaclePoint(obstaclePoint), neighborPositions(neighborPositions),
    preserveConnectivity(preserveConnectivity), now(now)
{
}

bool RepositionFitness::predictsNeighbor(const Coord& candidate) const
{
    return std::any_of(neighborPositions.begin(), neighborPositions.end(),
        [&](const Coord& position) {
            return candidate.distance(position) <= parameters.communicationRange;
        });
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
    double proximity =
        std::exp(-candidate.distance(obstaclePoint) / parameters.obstacleSigma);
    // Um candidato que permanece obstruído recebe custo máximo de obstáculo,
    // por mais distante que esteja da superfície detectada.
    double obstacleCost = std::max(
        proximity, sensor->intersectsAnyObstacle(candidate, teamPosition) ? 1.0 : 0.0);
    double movementCost = std::clamp(
        candidate.distance(current) / parameters.maximumRepositionDistance, 0.0, 1.0);
    return parameters.wLink * linkCost + parameters.wObstacle * obstacleCost +
           parameters.wMove * movementCost;
}

bool RepositionFitness::inDomain(const Coord& candidate) const
{
    return candidate.x >= parameters.areaMinX && candidate.x <= parameters.areaMaxX &&
           candidate.y >= parameters.areaMinY && candidate.y <= parameters.areaMaxY &&
           candidate.z >= parameters.minimumAltitude && candidate.z <= parameters.maximumAltitude;
}

bool RepositionFitness::feasible(const Coord& candidate) const
{
    if (!inDomain(candidate) ||
        candidate.distance(current) > parameters.maximumRepositionDistance ||
        candidate.distance(obstaclePoint) < parameters.obstacleSafetyMargin)
        return false;
    if (now + travelTime(current, candidate) > parameters.flightTimeLimit)
        return false;
    // A restrição só existe quando o drone tinha conectividade estimada antes
    // do movimento. Um drone já isolado não é impedido de buscar uma saída.
    if (preserveConnectivity && !predictsNeighbor(candidate))
        return false;
    // O trajeto do drone deve ser livre e a posição final precisa de linha de
    // visada até a equipe estimada. Uma posição ainda obstruída não cumpre a
    // finalidade do reposicionamento e não deve competir apenas por penalidade.
    return !sensor->intersectsAnyObstacle(current, candidate) &&
           !sensor->intersectsAnyObstacle(candidate, teamPosition);
}

} // namespace echosar
