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
                                     omnetpp::simtime_t now,
                                     bool useObstacleModel) :
    parameters(parameters), sensor(sensor), useObstacleModel(useObstacleModel),
    current(current), teamPosition(teamPosition),
    obstaclePoint(obstaclePoint), now(now)
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
    double proximity = useObstacleModel ?
        std::exp(-candidate.distance(obstaclePoint) / parameters.obstacleSigma) : 0.0;
    // Um candidato que permanece obstruído recebe custo máximo de obstáculo,
    // por mais distante que esteja da superfície detectada.
    double obstacleCost = useObstacleModel ? std::max(
        proximity, sensor->intersectsAnyObstacle(candidate, teamPosition) ? 1.0 : 0.0) : 0.0;
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
        (useObstacleModel && candidate.distance(obstaclePoint) < parameters.obstacleSafetyMargin))
        return false;
    if (now + travelTime(current, candidate) > parameters.flightTimeLimit)
        return false;
    // O trajeto do drone deve ser livre e a posição final precisa de linha de
    // visada até a equipe estimada. Uma posição ainda obstruída não cumpre a
    // finalidade do reposicionamento e não deve competir apenas por penalidade.
    return !useObstacleModel ||
           (!sensor->intersectsAnyObstacle(current, candidate) &&
            !sensor->intersectsAnyObstacle(candidate, teamPosition));
}

} // namespace echosar
