#pragma once
#include <vector>
#include <omnetpp/cmodule.h>
#include "inet/common/geometry/common/Coord.h"

namespace echosar {

struct BatParams {
    int    Nb      = 40;    // população de morcegos
    int    Imax    = 30;    // iterações máximas
    int    G       = 8;     // tolerância de estagnação (parada antecipada)
    double w1      = 0.6;   // peso de repulsão ao obstáculo
    double w2      = 0.4;   // peso do custo de deslocamento (w1+w2=1)
    double sigmaRho = 150.0; // raio de influência da repulsão (m), σ_ρ
    double dMax    = 300.0; // raio máximo de reposicionamento (m)
    double zMin    = 80.0;  // altitude mínima (m)
    double zMax    = 120.0; // altitude máxima (m)
    double fMin    = 0.0;   // frequência mínima (escala física relativa a dMax)
    double fMax    = 1.0;   // frequência máxima
    double alpha   = 0.9;   // fator de redução da amplitude A
    double gamma_  = 0.9;   // taxa de crescimento de r (nota: não é η de propagação)
    double r0      = 0.1;   // taxa inicial de emissão de pulsos
    double A0      = 1.0;   // amplitude sonora inicial
};

// Bat Algorithm puro — sem dependência de cModule de tempo de simulação.
// O cModule é passado APENAS para o RNG do OMNeT++ (reprodutibilidade por seed).
// Algoritmo 1 da dissertação: Eqs. ba_freq, ba_vel, ba_pos, ba_local, ba_loud, ba_rate.
class BatOptimizer {
  public:
    explicit BatOptimizer(omnetpp::cModule *rngModule, const BatParams &params = {});

    // Encontra a melhor posição p* de reposicionamento.
    // p_di  : posição atual do drone
    // p_obs : posição do obstáculo detectado pelo sensor
    // tabu  : posições já tentadas que falharam (memória de tabu espacial)
    inet::Coord optimize(const inet::Coord& p_di,
                         const inet::Coord& p_obs,
                         const std::vector<inet::Coord>& tabu);

  private:
    omnetpp::cModule *mod_;
    BatParams p_;

    double uniform01();
    double uniformSym();   // [-1, 1]

    // F(p) = w1·ρ(p) + w2·d_des(p) + penalidade_tabu(p)
    double fitness(const inet::Coord& p, const inet::Coord& p_di,
                   const inet::Coord& p_obs, const std::vector<inet::Coord>& tabu) const;

    // Projeta p dentro do raio d_max em torno de p_di e nos limites de altitude.
    inet::Coord clamp(const inet::Coord& p, const inet::Coord& p_di) const;
};

} // namespace echosar
