#include "BatControlledMobility.h"
#include <cmath>
#include <algorithm>

using namespace inet;
using omnetpp::simTime;

namespace echosar {

Define_Module(BatControlledMobility);

void BatControlledMobility::initialize(int stage)
{
    MovingMobilityBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        speedMean_     = par("speed");
        speedStdDev_   = par("speedStdDev");
        alpha_         = par("alpha");
        angleStdDev_   = deg(par("angleStdDev")).get();
        margin_        = par("margin");
        updateInterval_ = par("updateInterval").doubleValue();

        speed_     = speedMean_;
        angleMean_ = 0.0;
        angle_     = uniform(0.0, 2.0 * M_PI);
    }
}

// ── Gauss-Markov step (modo CRUISE) ──────────────────────────────────────────

void BatControlledMobility::stepGaussMarkov()
{
    // Repulsão de bordas — ajusta angleMean para afastar das bordas (como GaussMarkov)
    bool left   = lastPosition.x < constraintAreaMin.x + margin_;
    bool right  = lastPosition.x >= constraintAreaMax.x - margin_;
    bool top    = lastPosition.y < constraintAreaMin.y + margin_;
    bool bottom = lastPosition.y >= constraintAreaMax.y - margin_;

    if (top || bottom) {
        angleMean_ = bottom ? M_PI * 1.5 : M_PI * 0.5;
        if (left)  angleMean_ -= M_PI / 4.0;
        if (right) angleMean_ += M_PI / 4.0;
    } else if (left) {
        angleMean_ = 0.0;
    } else if (right) {
        angleMean_ = M_PI;
    }

    // Eq. ba_freq análoga — aqui aplica Gauss-Markov sobre velocidade e ângulo
    speed_ = alpha_ * speed_
           + (1.0 - alpha_) * speedMean_
           + std::sqrt(1.0 - alpha_ * alpha_) * normal(0.0, 1.0) * speedStdDev_;
    speed_ = std::max(0.1, speed_);

    angle_ = alpha_ * angle_
           + (1.0 - alpha_) * angleMean_
           + std::sqrt(1.0 - alpha_ * alpha_) * normal(0.0, 1.0) * angleStdDev_;

    Coord dir(std::cos(angle_), std::sin(angle_), 0.0);
    lastVelocity = dir * speed_;

    // Mantém altitude na faixa [zMin, zMax] com variação suave
    double dz = normal(0.0, 1.0) * 0.5;
    double newZ = std::max(constraintAreaMin.z,
                  std::min(constraintAreaMax.z, lastPosition.z + dz));
    lastVelocity.z = (newZ - lastPosition.z) / updateInterval_;

    nextChange = simTime() + updateInterval_;
    lastPosition += lastVelocity * updateInterval_;
    handleIfOutside(REFLECT, lastPosition, lastVelocity);
}

// ── move() — chamado pelo MovingMobilityBase em cada timer ────────────────────

void BatControlledMobility::move()
{
    switch (mode_) {
    case Mode::CRUISE:
        stepGaussMarkov();
        break;

    case Mode::HOVER:
        // Permanece estático; continua agendando para detectar retorno ao CRUISE
        lastVelocity = Coord::ZERO;
        nextChange = simTime() + updateInterval_;
        break;

    case Mode::MOVE_TO: {
        double dist = lastPosition.distance(cmdTarget_);
        if (dist < 1.0) {
            // Chegou
            lastPosition = cmdTarget_;
            lastVelocity = Coord::ZERO;
            arrived_     = true;
            mode_        = Mode::HOVER;
            nextChange   = simTime() + 1e9;
        } else {
            double step = std::min(speed_ * updateInterval_, dist);
            Coord dir   = (cmdTarget_ - lastPosition) / dist;
            lastPosition += dir * step;
            lastVelocity  = dir * speed_;
            nextChange    = simTime() + std::min(updateInterval_, dist / speed_);
        }
        break;
    }
    }
    lastUpdate = simTime();
}

// Métodos públicos (hover/moveTo/resumeCruise/hasArrived) definidos inline no header.

} // namespace echosar
