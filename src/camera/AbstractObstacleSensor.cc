#include "AbstractObstacleSensor.h"

#include <cmath>
#include <limits>

#include "inet/common/IVisitor.h"
#include "inet/common/geometry/common/RotationMatrix.h"
#include "inet/common/geometry/object/LineSegment.h"

using namespace omnetpp;
using namespace inet;
using namespace inet::physicalenvironment;

namespace echosar {

Define_Module(AbstractObstacleSensor);

void AbstractObstacleSensor::initialize()
{
    environment.reference(this, "physicalEnvironmentModule", true);
    minimumRange = par("minimumRange");
    maximumRange = par("maximumRange");
    if (minimumRange < 0)
        throw cRuntimeError("minimumRange must not be negative");
    if (maximumRange >= 0 && maximumRange <= minimumRange)
        throw cRuntimeError("maximumRange must exceed minimumRange, or be negative for no limit");
}

namespace {
class IntersectionVisitor : public IVisitor {
  public:
    const Coord& origin;
    const Coord& destination;
    mutable const IPhysicalObject *closest = nullptr;
    mutable Coord closestPoint;
    mutable double closestDistance = std::numeric_limits<double>::infinity();

    IntersectionVisitor(const Coord& origin, const Coord& destination) :
        origin(origin), destination(destination) {}

    void visit(const cObject *object) const override {
        auto physicalObject = check_and_cast<const IPhysicalObject *>(object);
        // A interseção é calculada no sistema local do objeto e convertida ao mundo.
        RotationMatrix rotation(physicalObject->getOrientation().toEulerAngles());
        LineSegment local(rotation.rotateVectorInverse(origin - physicalObject->getPosition()),
                          rotation.rotateVectorInverse(destination - physicalObject->getPosition()));
        Coord p1, p2, n1, n2;
        if (!physicalObject->getShape()->computeIntersection(local, p1, p2, n1, n2))
            return;
        Coord world1 = rotation.rotateVector(p1) + physicalObject->getPosition();
        Coord world2 = rotation.rotateVector(p2) + physicalObject->getPosition();
        Coord candidate = origin.distance(world1) <= origin.distance(world2) ? world1 : world2;
        double distance = origin.distance(candidate);
        // Somente o primeiro obstáculo na linha de visada é relevante ao sensor.
        if (distance < closestDistance) {
            closest = physicalObject;
            closestPoint = candidate;
            closestDistance = distance;
        }
    }
};
}

ObstacleObservation AbstractObstacleSensor::inspect(const Coord& dronePosition,
                                                    const Coord& inspectionTarget) const
{
    ObstacleObservation result;
    Coord direction = inspectionTarget - dronePosition;
    double targetDistance = direction.length();
    if (targetDistance == 0) {
        result.reason = "invalidTargetDirection";
        return result;
    }
    // maximumRange < 0 mantém o oráculo geométrico idealizado (mecanismo
    // científico, ver docs/desvios_e_extensoes.md, D4): o segmento avaliado
    // vai até o alvo, sem limite de alcance. Com maximumRange >= 0 (sensor
    // físico, padrão), a câmera não enxerga além do seu alcance: o segmento
    // inspecionado é truncado ali. Se o alvo estiver mais distante, o
    // restante do caminho simplesmente não foi observado — não é "livre".
    result.fullyObserved = maximumRange < 0 || targetDistance <= maximumRange;
    Coord sensorEnd = inspectionTarget;
    if (!result.fullyObserved)
        sensorEnd = dronePosition + direction * (maximumRange / targetDistance);
    IntersectionVisitor visitor(dronePosition, sensorEnd);
    environment->visitObjects(&visitor, LineSegment(dronePosition, sensorEnd));
    if (!visitor.closest) {
        // Distinguir os dois sentidos de "sem obstáculo": o trecho além do
        // alcance não foi observado, e não pode ser reportado como livre.
        result.reason = result.fullyObserved ? "clearToTarget" : "clearWithinSensorRange";
        return result;
    }
    // Fora da faixa de validade declarada o modelo não afirma nada. Reportar
    // "detectado" aqui e ainda devolver o ponto de superfície entregaria ao BA
    // a geometria exata do simulador sob o rótulo de uma observação — que é
    // justamente o que a faixa mínima existe para negar. A recusa fica
    // registrada em reason, então some da detecção sem sumir da auditoria.
    // Só se aplica no modo com faixa; o oráculo idealizado (maximumRange < 0)
    // não tem limite inferior.
    if (maximumRange >= 0 && visitor.closestDistance < minimumRange) {
        result.reason = "outsideCalibratedRange";
        return result;
    }
    result.nearestSurfacePoint = visitor.closestPoint;
    result.distance = visitor.closestDistance;
    result.detected = true;
    result.distanceValid = true;
    result.reason = "obstacleConfirmed";
    return result;
}

bool AbstractObstacleSensor::clearCorridorGroundTruth(const Coord& a, const Coord& b,
                                                     double clearance) const
{
    if (intersectsAnyObstacleGroundTruth(a, b))
        return false;
    Coord axis = b - a;
    double length = axis.length();
    // Sem eixo ou sem folga pedida, o corredor degenera na própria linha
    // central, que o teste acima já cobriu.
    if (length == 0 || clearance <= 0)
        return true;
    Coord unit = axis / length;
    // Qualquer vetor não paralelo ao eixo serve de semente para a base
    // perpendicular; escolher pelo menor componente evita o produto vetorial
    // degenerado quando o eixo é quase vertical.
    Coord seed = std::abs(unit.z) < 0.9 ? Coord(0, 0, 1) : Coord(1, 0, 0);
    Coord u = (unit % seed).getNormalized() * clearance;
    Coord v = (unit % u).getNormalized() * clearance;
    for (const Coord& offset : {u, -u, v, -v})
        if (intersectsAnyObstacleGroundTruth(a + offset, b + offset))
            return false;
    return true;
}

bool AbstractObstacleSensor::intersectsAnyObstacleGroundTruth(const Coord& a, const Coord& b) const
{
    // Deliberadamente sem minimumRange/maximumRange: este predicado representa
    // a geometria do cenário conhecida pelo simulador, não o que a câmera vê.
    IntersectionVisitor visitor(a, b);
    environment->visitObjects(&visitor, LineSegment(a, b));
    return visitor.closest != nullptr;
}

} // namespace echosar
