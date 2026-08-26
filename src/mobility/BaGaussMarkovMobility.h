#pragma once

#include "inet/mobility/single/GaussMarkovMobility.h"

namespace echosar {

class BaGaussMarkovMobility : public inet::GaussMarkovMobility
{
  protected:
    bool baOverride = false;
    inet::rad elevation = inet::rad(0);
    inet::rad elevationMean = inet::rad(0);
    inet::rad elevationStdDev = inet::rad(0);

    virtual void initialize(int stage) override;
    virtual void move() override;

    /// Alterna entre concluir a perna comandada pelo BA e gerar um alvo Gauss-Markov.
    virtual void setTargetPosition() override;

  public:
    /// Inicia um deslocamento gradual até o candidato respeitando velocidades por
    /// eixo e devolve o tempo de trajeto que a mobilidade vai efetivamente usar.
    /// Zero significa que nenhum movimento foi iniciado.
    double moveTo(const inet::Coord& destination, double horizontalSpeed,
                  double climbSpeed, double descentSpeed);
    /// Cancela a espera/comando do BA e retoma a trajetória estocástica normal.
    void resumeNormal();
};

} // namespace echosar
