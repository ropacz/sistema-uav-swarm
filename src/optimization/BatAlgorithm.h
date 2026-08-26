#pragma once

#include <functional>
#include <vector>

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp.h"

namespace echosar {

struct BatParameters {
    int populationSize = 20;
    int iterations = 50;
    int initializationAttempts = 100;
    double frequencyMin = 0;
    double frequencyMax = 2;
    double initialAmplitude = 0.9;
    double initialPulseRate = 0.5;
    double amplitudeDecay = 0.9;
    double pulseGrowth = 0.9;
    double localSearchScale = 0.1;
};

struct BatResult {
    inet::Coord position;
    double fitness = INFINITY;
    bool valid = false;
};

class BatAlgorithm
{
  public:
    using FitnessFunction = std::function<double(const inet::Coord&)>;
    using FeasibilityFunction = std::function<bool(const inet::Coord&)>;
    using DomainFunction = std::function<bool(const inet::Coord&)>;

    /// Busca a melhor posição viável dentro da esfera centrada no drone.
    ///
    /// A inicialização amostra por rejeição dentro de `inDomain` (área e
    /// altitude — checagem barata, sem raycasting) antes de gastar uma
    /// avaliação de `feasible`/`fitness` (checagem cara: obstáculo,
    /// conectividade, linha de visada) em cada morcego. Sem essa separação,
    /// a maior parte do orçamento de inicialização é desperdiçada em pontos
    /// trivialmente fora do domínio quando a esfera de busca (raio
    /// maximumRepositionDistance) é grande em relação à faixa vertical válida
    /// ou o drone está perto da borda do cenário — medido empiricamente: 96%
    /// das avaliações descartadas antes mesmo do teste geométrico de
    /// obstáculo. Ver docs/desvios_e_extensoes.md.
    static BatResult optimize(const inet::Coord& center, double maxDistance,
                              const BatParameters& parameters,
                              omnetpp::cRNG *rng,
                              const FitnessFunction& fitness,
                              const FeasibilityFunction& feasible,
                              const DomainFunction& inDomain);
};

} // namespace echosar
