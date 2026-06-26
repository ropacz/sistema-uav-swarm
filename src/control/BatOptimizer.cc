#include "BatOptimizer.h"
#include <cmath>
#include <algorithm>

using namespace inet;

namespace echosar {

BatOptimizer::BatOptimizer(omnetpp::cModule *rngModule, const BatParams &params)
    : mod_(rngModule), p_(params)
{
}

double BatOptimizer::uniform01() {
    return mod_->uniform(0.0, 1.0);
}

double BatOptimizer::uniformSym() {
    return mod_->uniform(-1.0, 1.0);
}

// ── Função de aptidão ────────────────────────────────────────────────────────

double BatOptimizer::fitness(const Coord& p, const Coord& p_di,
                              const Coord& p_obs,
                              const std::vector<Coord>& tabu) const
{
    // ρ(p) = exp(-‖p - p_obs‖ / σ_ρ)  — repulsão ao obstáculo ∈ (0,1]
    double distObs = p.distance(p_obs);
    double rho = std::exp(-distObs / p_.sigmaRho);

    // d_des(p) = ‖p - p_di‖ / d_max  — custo de deslocamento ∈ [0,1]
    double ddes = p.distance(p_di) / p_.dMax;
    ddes = std::min(ddes, 1.0);

    double F = p_.w1 * rho + p_.w2 * ddes;

    // Penalidade de tabu: mesma forma de ρ, penaliza regiões já tentadas
    for (const auto& t : tabu) {
        double dt = p.distance(t);
        F += std::exp(-dt / p_.sigmaRho);
    }
    return F;
}

// ── Projeção nos limites do espaço de busca ──────────────────────────────────

Coord BatOptimizer::clamp(const Coord& p, const Coord& p_di) const
{
    Coord q = p;
    // Limitar altitude
    q.z = std::max(p_.zMin, std::min(p_.zMax, q.z));

    // Limitar raio: se ‖q - p_di‖ > d_max, projetar de volta
    double d = q.distance(p_di);
    if (d > p_.dMax && d > 1e-6) {
        Coord dir = (q - p_di) / d;
        q = p_di + dir * p_.dMax;
        q.z = std::max(p_.zMin, std::min(p_.zMax, q.z));
    }
    return q;
}

// ── Algoritmo 1 ─────────────────────────────────────────────────────────────

Coord BatOptimizer::optimize(const Coord& p_di,
                              const Coord& p_obs,
                              const std::vector<Coord>& tabu)
{
    int Nb = p_.Nb;

    // Inicialização
    struct Bat { Coord pos, vel; double f, A, r; };
    std::vector<Bat> bats(Nb);

    for (auto& b : bats) {
        // Posição aleatória dentro de d_max em torno de p_di
        double theta = mod_->uniform(0.0, 2.0 * M_PI);
        double phi   = mod_->uniform(-M_PI / 2.0, M_PI / 2.0);
        double rad   = mod_->uniform(0.0, p_.dMax);
        b.pos = Coord(p_di.x + rad * std::cos(phi) * std::cos(theta),
                      p_di.y + rad * std::cos(phi) * std::sin(theta),
                      p_di.z + rad * std::sin(phi));
        b.pos = clamp(b.pos, p_di);
        b.vel = Coord::ZERO;
        b.f   = p_.fMin;
        b.A   = p_.A0;
        b.r   = p_.r0;
    }

    // Melhor posição global
    Coord pStar = bats[0].pos;
    double fStar = fitness(pStar, p_di, p_obs, tabu);
    for (int j = 1; j < Nb; j++) {
        double fj = fitness(bats[j].pos, p_di, p_obs, tabu);
        if (fj < fStar) { fStar = fj; pStar = bats[j].pos; }
    }

    // Iterações
    int stagnation = 0;
    for (int t = 1; t <= p_.Imax; t++) {
        double prevFStar = fStar;

        // Amplitude média da população (para busca local, Eq. ba_local)
        double Abar = 0;
        for (const auto& b : bats) Abar += b.A;
        Abar /= Nb;

        for (int j = 0; j < Nb; j++) {
            auto& b = bats[j];

            // Eq. ba_freq: frequência aleatória na faixa [fMin, fMax]
            double beta = uniform01();
            b.f = p_.fMin + (p_.fMax - p_.fMin) * beta;

            // Eq. ba_vel: v_j^t = v_j^(t-1) + (p* - p_j^(t-1)) * f_j
            Coord attraction = pStar - b.pos;
            b.vel = b.vel + attraction * b.f;

            // Eq. ba_pos: p_j^t = p_j^(t-1) + v_j^t
            Coord candidate = b.pos + b.vel;
            candidate = clamp(candidate, p_di);

            // Busca local: se rand > r_j, perturba em torno de p*
            if (uniform01() > b.r) {
                // Eq. ba_local: p_local = p* + ε * Ā_t
                double eps = uniformSym();
                candidate = Coord(pStar.x + eps * Abar,
                                  pStar.y + eps * Abar,
                                  pStar.z + eps * Abar);
                candidate = clamp(candidate, p_di);
            }

            // Aceitar nova posição se melhora p* e passa sorteio por amplitude
            double fc = fitness(candidate, p_di, p_obs, tabu);
            if (uniform01() < b.A && fc < fStar) {
                // Eq. ba_loud e ba_rate
                b.A *= p_.alpha;
                b.r  = p_.r0 * (1.0 - std::exp(-p_.gamma_ * t));
                b.pos = candidate;
            }
            if (fc < fStar) {
                fStar = fc;
                pStar = candidate;
            }
        }

        // Parada antecipada por estagnação
        if (fStar < prevFStar - 1e-9)
            stagnation = 0;
        else
            stagnation++;
        if (stagnation >= p_.G) break;
    }

    return pStar;
}

} // namespace echosar
