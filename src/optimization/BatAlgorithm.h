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
    int evaluations = 0;
    bool valid = false;
};

class BatAlgorithm
{
  public:
    using FitnessFunction = std::function<double(const inet::Coord&)>;
    using FeasibilityFunction = std::function<bool(const inet::Coord&)>;

    /// Busca a melhor posição viável dentro da esfera centrada no drone.
    static BatResult optimize(const inet::Coord& center, double maxDistance,
                              const BatParameters& parameters,
                              omnetpp::cRNG *rng,
                              const FitnessFunction& fitness,
                              const FeasibilityFunction& feasible);
};

} // namespace echosar
