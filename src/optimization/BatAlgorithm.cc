#include "BatAlgorithm.h"

#include <cmath>

using inet::Coord;

namespace echosar {

namespace {
struct Bat {
    Coord position;
    Coord velocity;
    double frequency = 0;
    double amplitude = 0;
    double pulseRate = 0;
    double fitness = INFINITY;
};

double sampleUniform(omnetpp::cRNG *rng, double a, double b)
{
    return a + (b - a) * rng->doubleRand();
}

Coord randomInSphere(omnetpp::cRNG *rng, double radius)
{
    // A rejeição do cubo já produz distribuição uniforme no volume da esfera.
    // Aplicar outro fator radial concentraria candidatos artificialmente no centro.
    Coord p;
    do {
        p = Coord(sampleUniform(rng, -1, 1), sampleUniform(rng, -1, 1), sampleUniform(rng, -1, 1));
    } while (p.length() > 1 || p.length() == 0);
    return p * radius;
}

// Limite de tentativas só para achar um ponto dentro de área/altitude — checagem
// geométrica barata (sem raycasting de obstáculo), separada do orçamento de
// initializationAttempts. Alto o bastante para não ser o gargalo em cenários
// razoáveis; se esgotar, devolve a última amostra mesmo fora do domínio e deixa
// feasible() rejeitá-la normalmente — nunca pior que o comportamento anterior.
constexpr int kDomainSamplingAttempts = 500;

Coord sampleInDomain(omnetpp::cRNG *rng, const Coord& center, double maxDistance,
                     const BatAlgorithm::DomainFunction& inDomain)
{
    Coord candidate = center + randomInSphere(rng, maxDistance);
    for (int attempt = 1; attempt < kDomainSamplingAttempts && !inDomain(candidate); ++attempt)
        candidate = center + randomInSphere(rng, maxDistance);
    return candidate;
}
}

BatResult BatAlgorithm::optimize(const Coord& center, double maxDistance,
                                 const BatParameters& p, omnetpp::cRNG *rng,
                                 const FitnessFunction& fitness,
                                 const FeasibilityFunction& feasible,
                                 const DomainFunction& inDomain)
{
    if (p.populationSize <= 0 || p.iterations <= 0 || maxDistance <= 0)
        throw omnetpp::cRuntimeError("Invalid Bat Algorithm parameters");

    std::vector<Bat> bats(p.populationSize);
    BatResult best;
    // Cada morcego inicia em uma posição viável dentro da região de busca. A
    // amostra é pré-filtrada por área/altitude (sampleInDomain) antes de gastar
    // uma tentativa em feasible(), que também testa obstáculo/conectividade.
    for (auto& bat : bats) {
        for (int tries = 0; tries < p.initializationAttempts; ++tries) {
            bat.position = sampleInDomain(rng, center, maxDistance, inDomain);
            if (feasible(bat.position))
                break;
        }
        bat.amplitude = p.initialAmplitude;
        bat.pulseRate = p.initialPulseRate;
        if (feasible(bat.position)) {
            bat.fitness = fitness(bat.position);
            if (bat.fitness < best.fitness)
                best = {bat.position, bat.fitness, true};
        }
    }
    // O centro preserva uma solução válida quando nenhum candidato aleatório serve.
    if (!best.valid && feasible(center)) {
        best = {center, fitness(center), true};
    }

    for (int t = 0; t < p.iterations && best.valid; ++t) {
        double meanAmplitude = 0;
        for (const auto& bat : bats)
            meanAmplitude += bat.amplitude;
        meanAmplitude /= bats.size();

        for (auto& bat : bats) {
            bat.frequency = p.frequencyMin +
                (p.frequencyMax - p.frequencyMin) * rng->doubleRand();
            // Yang (2010) convention: v(t)=v(t-1)+(x(t-1)-x*)f.
            bat.velocity += (bat.position - best.position) * bat.frequency;
            Coord candidate = bat.position + bat.velocity;
            if (rng->doubleRand() > bat.pulseRate)
                candidate = best.position + randomInSphere(rng,
                    maxDistance * p.localSearchScale * meanAmplitude);
            if (candidate.distance(center) > maxDistance || !feasible(candidate))
                continue;
            double candidateFitness = fitness(candidate);
            // Uma solução só é aceita se melhorar o morcego e passar pelo teste de amplitude.
            if (candidateFitness <= bat.fitness && rng->doubleRand() < bat.amplitude) {
                bat.position = candidate;
                bat.fitness = candidateFitness;
                bat.amplitude *= p.amplitudeDecay;
                bat.pulseRate = p.initialPulseRate *
                    (1 - std::exp(-p.pulseGrowth * (t + 1)));
                if (candidateFitness < best.fitness)
                    best = {candidate, candidateFitness, true};
            }
        }
    }
    return best;
}

} // namespace echosar
