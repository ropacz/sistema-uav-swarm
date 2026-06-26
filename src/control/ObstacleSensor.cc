#include "ObstacleSensor.h"
#include "inet/common/geometry/common/RotationMatrix.h"
#include "inet/common/geometry/object/LineSegment.h"
#include "inet/environment/common/PhysicalObject.h"

using namespace inet;
using namespace inet::physicalenvironment;

namespace echosar {

std::optional<Coord> senseObstacleOnLoS(const Coord& dronePos,
                                         const Coord& teamPos,
                                         IPhysicalEnvironment* physEnv)
{
    if (!physEnv) return std::nullopt;

    bool found = false;
    Coord closest;
    double minDist = 1e30;

    int n = physEnv->getNumObjects();
    for (int i = 0; i < n; i++) {
        const IPhysicalObject *obj = physEnv->getObject(i);
        const ShapeBase  *shape       = obj->getShape();
        const Coord&      position    = obj->getPosition();
        const Quaternion& orientation = obj->getOrientation();

        // Transforma o segmento LoS para o referencial local do objeto.
        // Reproduz exatamente o padrão de DielectricObstacleLoss::computeObjectLoss().
        RotationMatrix rot(orientation.toEulerAngles());
        LineSegment seg(rot.rotateVectorInverse(dronePos - position),
                        rot.rotateVectorInverse(teamPos  - position));

        Coord i1, i2, n1, n2;
        bool hit = shape->computeIntersection(seg, i1, i2, n1, n2);
        if (hit && i1 != i2) {
            // Distância do drone ao centro do objeto (como p_obs no referencial global)
            double d = dronePos.distance(position);
            if (d < minDist) {
                minDist  = d;
                closest  = position;   // Eq. p_obs: centro do obstáculo
                found    = true;
            }
        }
    }

    if (found) return closest;
    return std::nullopt;
}

} // namespace echosar
