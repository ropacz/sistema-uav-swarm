#include "BaGaussMarkovMobility.h"

#include <algorithm>

using namespace omnetpp;
using namespace inet;

namespace echosar {

Define_Module(BaGaussMarkovMobility);

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
    // Um ACK pode chegar durante o trajeto. Retoma o Gauss-Markov a partir da
    // posição interpolada, sem deixar um comando do BA pendente.
    getCurrentPosition();
    baOverride = false;
    holding = false;
    stationary = false;
    GaussMarkovMobility::setTargetPosition();
    scheduleUpdate();
}

} // namespace echosar
