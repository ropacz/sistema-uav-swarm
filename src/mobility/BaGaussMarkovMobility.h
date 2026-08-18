#pragma once

#include "inet/mobility/single/GaussMarkovMobility.h"

namespace echosar {

class BaGaussMarkovMobility : public inet::GaussMarkovMobility
{
  protected:
    bool baOverride = false;
    bool holding = false;
    int waypointId = 0;

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
