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
                                                    const Coord& teamPosition) const
{
    ObstacleObservation result;
    Coord direction = teamPosition - dronePosition;
    if (direction.length() == 0) {
        result.reason = "invalidTargetDirection";
        return result;
    }
    // A verificação é sobre o segmento entre a posição do drone e a última
    // posição conhecida da equipe: é esse percurso que o sinal atravessa.
    IntersectionVisitor visitor(dronePosition, teamPosition);
    environment->visitObjects(&visitor, LineSegment(dronePosition, teamPosition));
    if (!visitor.closest) {
        result.reason = "clearLineOfSight";
        return result;
    }
    result.nearestSurfacePoint = visitor.closestPoint;
    result.distance = visitor.closestDistance;
    // minimumRange e maximumRange modelam a zona morta e o alcance do sensor
    // físico de prevenção de colisões; só se aplicam juntos, quando o sensor
    // está configurado como físico (maximumRange >= 0). Com maximumRange < 0
    // o módulo age como oráculo geométrico idealizado (mecanismo científico,
    // ver docs/desvios_e_extensoes.md, D4): um edifício que bloqueia o
    // enlace é identificado pela geometria do percurso, sem zona morta nem
    // limite de distância — um drone encostado na parede não deixaria de
    // confirmar o obstáculo por estar "perto demais".
    if (maximumRange >= 0 &&
        (result.distance < minimumRange || result.distance > maximumRange)) {
        result.reason = "outsideVisualRange";
        return result;
    }
    result.confirmed = true;
    result.reason = "obstacleConfirmed";
    return result;
}

bool AbstractObstacleSensor::intersectsAnyObstacle(const Coord& a, const Coord& b) const
{
    IntersectionVisitor visitor(a, b);
    environment->visitObjects(&visitor, LineSegment(a, b));
    return visitor.closest != nullptr;
}

} // namespace echosar
