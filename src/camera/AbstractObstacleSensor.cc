#include "AbstractObstacleSensor.h"

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

    // Parâmetros declarativos da câmera de referência: validados para recusar
    // configuração incoerente, mas ainda não aplicados à inspeção (D4, L2).
    // Os ângulos são lidos explicitamente em graus para não depender da
    // unidade angular usada no .ini.
    double fieldOfViewHorizontal = par("fieldOfViewHorizontal").doubleValueInUnit("deg");
    double fieldOfViewVertical = par("fieldOfViewVertical").doubleValueInUnit("deg");
    double measurementFrequency = par("measurementFrequency").doubleValueInUnit("Hz");
    if (fieldOfViewHorizontal <= 0 || fieldOfViewHorizontal > 360)
        throw cRuntimeError("fieldOfViewHorizontal must be in (0, 360] degrees");
    if (fieldOfViewVertical <= 0 || fieldOfViewVertical > 180)
        throw cRuntimeError("fieldOfViewVertical must be in (0, 180] degrees");
    if (measurementFrequency <= 0)
        throw cRuntimeError("measurementFrequency must be positive");
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
    bool fullyObserved = maximumRange < 0 || targetDistance <= maximumRange;
    Coord sensorEnd = inspectionTarget;
    if (!fullyObserved)
        sensorEnd = dronePosition + direction * (maximumRange / targetDistance);
    IntersectionVisitor visitor(dronePosition, sensorEnd);
    environment->visitObjects(&visitor, LineSegment(dronePosition, sensorEnd));
    if (!visitor.closest) {
        // Distinguir os dois sentidos de "sem obstáculo": o trecho além do
        // alcance não foi observado, e não pode ser reportado como livre.
        result.reason = fullyObserved ? "clearToTarget" : "clearWithinSensorRange";
        return result;
    }
    result.nearestSurfacePoint = visitor.closestPoint;
    result.distance = visitor.closestDistance;
    result.confirmed = true;
    // Zona morta física: abaixo do alcance mínimo mensurável, a câmera
    // estereoscópica não estima distância com confiabilidade, mas continua
    // enxergando que há um obstáculo ali — um drone encostado numa parede não
    // deixa de notar o obstáculo por estar perto demais, então a confirmação
    // vale nos dois casos. Só se aplica no modo sensor físico; o oráculo
    // idealizado (maximumRange < 0) não tem zona morta.
    bool insideDeadZone = maximumRange >= 0 && result.distance < minimumRange;
    result.reason = insideDeadZone ? "obstacleTooCloseToMeasure" : "obstacleConfirmed";
    return result;
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
