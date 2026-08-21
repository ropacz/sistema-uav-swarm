#include "BaGaussMarkovMobility.h"

#include <algorithm>

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(BaGaussMarkovMobility);

void BaGaussMarkovMobility::initialize(int stage)
{
    GaussMarkovMobility::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        elevationMean = deg(par("elevation"));
        elevationStdDev = deg(par("elevationStdDev"));
        elevation = elevationMean;
    }
}

void BaGaussMarkovMobility::move()
{
    preventBorderHugging();
    LineSegmentsMobilityBase::move();
    // A sobrecarga com elevação reflete também nos limites verticais.
    handleIfOutside(REFLECT, lastPosition, lastVelocity, angle, elevation);
    minimumObservedZ = std::min(minimumObservedZ, lastPosition.z);
    maximumObservedZ = std::max(maximumObservedZ, lastPosition.z);
}

void BaGaussMarkovMobility::finish()
{
    recordScalar("minimumObservedZ", minimumObservedZ);
    recordScalar("maximumObservedZ", maximumObservedZ);
    GaussMarkovMobility::finish();
}

void BaGaussMarkovMobility::setTargetPosition()
{
    // Ao terminar a perna do BA, mantém a posição até a aplicação validar o enlace.
    if (baOverride) {
        baOverride = false;
        holding = true;
        stationary = true;
        nextChange = -1;
        lastVelocity = Coord::ZERO;
        return;
    }
    waypointId++;
    speed = alpha * speed
        + (1.0 - alpha) * speedMean
        + sqrt(1.0 - alpha * alpha) * normal(0.0, 1.0) * speedStdDev;
    angle = alpha * angle
        + (1.0 - alpha) * angleMean
        + rad(sqrt(1.0 - alpha * alpha) * normal(0.0, 1.0) * angleStdDev);
    elevation = alpha * elevation
        + (1.0 - alpha) * elevationMean
        + rad(sqrt(1.0 - alpha * alpha) * normal(0.0, 1.0) * elevationStdDev);

    double azimuth = rad(angle).get();
    double vertical = rad(elevation).get();
    Coord direction(cos(vertical) * cos(azimuth),
                    cos(vertical) * sin(azimuth),
                    sin(vertical));
    nextChange = simTime() + updateInterval;
    targetPosition = lastPosition + direction * (speed * updateInterval.dbl());
}

void BaGaussMarkovMobility::moveTo(const Coord& destination, double horizontalSpeed,
                                   double climbSpeed, double descentSpeed)
{
    // Chamado pela aplicação, que roda no contexto de outro módulo. Sem trocar
    // o contexto, o evento de mobilidade seria agendado e possuído pelo
    // DroneApp, e o OMNeT++ aborta a execução.
    Enter_Method_Silent();
    getCurrentPosition();
    Coord delta = destination - lastPosition;
    double horizontalTime = std::hypot(delta.x, delta.y) / horizontalSpeed;
    double verticalSpeed = delta.z >= 0 ? climbSpeed : descentSpeed;
    double verticalTime = std::abs(delta.z) / verticalSpeed;
    // Os eixos evoluem simultaneamente; o eixo mais lento determina a chegada.
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
    // Também chamado a partir da aplicação — ver moveTo().
    Enter_Method_Silent();
    // Um ACK pode chegar durante o trajeto. Retoma o Gauss-Markov a partir da
    // posição interpolada, sem deixar um comando do BA pendente.
    getCurrentPosition();
    baOverride = false;
    holding = false;
    stationary = false;
    setTargetPosition();
    scheduleUpdate();
}

} // namespace echosar
