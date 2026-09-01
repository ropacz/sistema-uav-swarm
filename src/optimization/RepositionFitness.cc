#include "RepositionFitness.h"

#include <algorithm>
#include <cmath>

#include "camera/AbstractObstacleSensor.h"

using inet::Coord;

namespace echosar {

RepositionFitness::RepositionFitness(const FitnessParameters& parameters,
                                     const AbstractObstacleSensor *sensor,
                                     const Coord& current,
                                     const Coord& teamPosition,
                                     const std::optional<Coord>& obstaclePoint,
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
    // A distância até a última posição conhecida da equipe é uma aproximação
    // geométrica da qualidade esperada do enlace — não uma estimativa de RSSI,
    // que o BA não conhece.
    double teamDistanceCost = std::clamp(
        candidate.distance(teamPosition) / parameters.linkNormalizationDistance, 0.0, 1.0);
    // Repulsão local em torno da única superfície que a câmera observou. Sem
    // observação o termo é constante e não influencia a ordenação dos
    // candidatos — valores absolutos de aptidão não são comparáveis entre uma
    // ativação com observação e outra sem.
    //
    // Não há aqui o teste de "candidato ainda obstruído": ele seria redundante,
    // porque cost() só é avaliada em candidatos que feasible() já aprovou, e
    // aquela exige linha de visada livre até a equipe. Repeti-lo custaria um
    // raycasting por avaliação para um valor que é sempre o mesmo.
    double obstacleCost = obstaclePoint.has_value()
        ? std::exp(-candidate.distance(*obstaclePoint) / parameters.obstacleSigma)
        : 0.0;
    double movementCost = std::clamp(
        candidate.distance(current) / parameters.maximumRepositionDistance, 0.0, 1.0);
    return parameters.wLink * teamDistanceCost + parameters.wObstacle * obstacleCost +
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
        candidate.distance(current) > parameters.maximumRepositionDistance)
        return false;
    // Afastamento mínimo da superfície observada. É repulsão em torno desse
    // ponto; a garantia de trajeto livre vem do corredor testado no fim.
    if (obstaclePoint.has_value() &&
        candidate.distance(*obstaclePoint) < parameters.obstacleSafetyMargin)
        return false;
    if (now + travelTime(current, candidate) > parameters.flightTimeLimit)
        return false;
    // A restrição só existe quando o drone tinha conectividade estimada antes
    // do movimento. Um drone já isolado não é impedido de buscar uma saída.
    if (preserveConnectivity && !predictsNeighbor(candidate))
        return false;
    // Avaliador geométrico idealizado do simulador (§14; D4/E4). São duas
    // perguntas de natureza diferente, e por isso duas geometrias diferentes:
    //
    // O trajeto é uma questão de colisão, e o drone tem volume. Exige um
    // corredor livre de raio droneRadius + obstacleSafetyMargin em torno do
    // eixo — testar só a linha central aceitaria uma rota que passa rente à
    // parede e leva as hélices junto.
    //
    // A linha de visada até a equipe é uma questão de propagação, e o que
    // importa ali é o eixo: um segmento fino é a geometria certa, porque a
    // pergunta é se o sinal atravessa, não se o drone cabe.
    //
    // Nada disso é o que o verificador observa — a detecção que ativou o
    // mecanismo essa sim respeitou a faixa de validade declarada.
    return sensor->clearCorridorGroundTruth(
               current, candidate,
               parameters.droneRadius + parameters.obstacleSafetyMargin) &&
           !sensor->intersectsAnyObstacleGroundTruth(candidate, teamPosition);
}

} // namespace echosar
