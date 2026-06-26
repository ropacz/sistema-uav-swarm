#pragma once
#include "inet/mobility/base/MovingMobilityBase.h"

namespace echosar {

// Mobilidade controlada para o mecanismo BA.
// Modos:
//   CRUISE     — Gauss-Markov adaptado (baseline idêntico ao GaussMarkovMobility)
//   HOVER      — posição congelada (estabiliza para leitura sensorial e cálculo BA)
//   MOVE_TO    — desloca-se em linha reta até p* à velocidade de cruzeiro
//
// API pública (chamada por SimpleDroneApp):
//   hover()        — entra em HOVER imediatamente
//   moveTo(p)      — vai até p em linha reta e sinaliza quando chegar (hasArrived)
//   resumeCruise() — volta ao modo CRUISE
//   hasArrived()   — true após MOVE_TO concluído; limpado em cada chamada

class BatControlledMobility : public inet::MovingMobilityBase
{
  public:
    enum class Mode { CRUISE, HOVER, MOVE_TO };

    // API para SimpleDroneApp (apenas set de estado — sem scheduling cross-module) ──
    void hover()                        { mode_ = Mode::HOVER; }
    void moveTo(const inet::Coord& t)   { cmdTarget_ = t; arrived_ = false; mode_ = Mode::MOVE_TO; }
    void resumeCruise()                 { mode_ = Mode::CRUISE; }
    bool hasArrived()                   { bool a = arrived_; arrived_ = false; return a; }

  protected:
    // ── Parâmetros NED ───────────────────────────────────────────────────────
    double speedMean_    = 11.5;
    double speedStdDev_  = 2.0;
    double alpha_        = 0.75;
    double angleStdDev_  = 0.5236;  // 30 graus em rad
    double margin_       = 100.0;
    double updateInterval_ = 0.5;

    // ── Estado de movimento ──────────────────────────────────────────────────
    Mode   mode_          = Mode::CRUISE;
    inet::Coord cmdTarget_;
    bool   arrived_       = false;

    // Gauss-Markov state
    double speed_    = 0.0;
    double angleMean_ = 0.0;
    double angle_    = 0.0;

    // ── MovingMobilityBase ───────────────────────────────────────────────────
    virtual void initialize(int stage) override;
    virtual void move() override;
    virtual double getMaxSpeed() const override { return speedMean_ + 3 * speedStdDev_; }

  private:
    void stepGaussMarkov();  // atualiza posição/velocidade no modo CRUISE
};

} // namespace echosar
