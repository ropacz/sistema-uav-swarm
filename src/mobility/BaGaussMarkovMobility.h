#pragma once

#include "inet/mobility/single/GaussMarkovMobility.h"
#include <limits>

namespace echosar {

class BaGaussMarkovMobility : public inet::GaussMarkovMobility
{
  protected:
    bool baOverride = false;
    bool holding = false;
    int waypointId = 0;
    inet::rad elevation = inet::rad(0);
    inet::rad elevationMean = inet::rad(0);
    inet::rad elevationStdDev = inet::rad(0);
    double minimumObservedZ = std::numeric_limits<double>::infinity();
    double maximumObservedZ = -std::numeric_limits<double>::infinity();

    virtual void initialize(int stage) override;
    virtual void move() override;
    virtual void finish() override;

    /// Alterna entre concluir a perna comandada pelo BA e gerar um alvo Gauss-Markov.
    virtual void setTargetPosition() override;

  public:
    /// Inicia um deslocamento gradual até o candidato respeitando velocidades por eixo.
    void moveTo(const inet::Coord& destination, double horizontalSpeed,
                double climbSpeed, double descentSpeed);
    /// Cancela a espera/comando do BA e retoma a trajetória estocástica normal.
    void resumeNormal();
    /// Indica se o drone aguarda a validação do enlace na posição candidata.
    bool isHolding() const { return holding; }
    /// Retorna o identificador monotônico do segmento de mobilidade atual.
    int getWaypointId() const { return waypointId; }
};

} // namespace echosar
