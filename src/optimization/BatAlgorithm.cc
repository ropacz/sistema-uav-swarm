#include "BatAlgorithm.h"

#include <algorithm>
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
    Coord p;
    do {
        p = Coord(sampleUniform(rng, -1, 1), sampleUniform(rng, -1, 1), sampleUniform(rng, -1, 1));
    } while (p.length() > 1 || p.length() == 0);
    return p * (radius * std::cbrt(rng->doubleRand()));
}
}

BatResult BatAlgorithm::optimize(const Coord& center, double maxDistance,
                                 const BatParameters& p, omnetpp::cRNG *rng,
                                 const FitnessFunction& fitness,
                                 const FeasibilityFunction& feasible)
{
    if (p.populationSize <= 0 || p.iterations <= 0 || maxDistance <= 0)
        throw omnetpp::cRuntimeError("Invalid Bat Algorithm parameters");

    std::vector<Bat> bats(p.populationSize);
    BatResult best;
    for (auto& bat : bats) {
        for (int tries = 0; tries < p.initializationAttempts; ++tries) {
            bat.position = center + randomInSphere(rng, maxDistance);
            if (feasible(bat.position))
                break;
        }
        bat.amplitude = p.initialAmplitude;
        bat.pulseRate = p.initialPulseRate;
        if (feasible(bat.position)) {
            bat.fitness = fitness(bat.position);
            best.evaluations++;
            if (bat.fitness < best.fitness)
                best = {bat.position, bat.fitness, best.evaluations, true};
        }
    }
    if (!best.valid && feasible(center)) {
        best = {center, fitness(center), best.evaluations + 1, true};
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
            best.evaluations++;
            if (candidateFitness <= bat.fitness && rng->doubleRand() < bat.amplitude) {
                bat.position = candidate;
                bat.fitness = candidateFitness;
                bat.amplitude *= p.amplitudeDecay;
                bat.pulseRate = p.initialPulseRate *
                    (1 - std::exp(-p.pulseGrowth * (t + 1)));
                if (candidateFitness < best.fitness)
                    best = {candidate, candidateFitness, best.evaluations, true};
            }
        }
    }
    return best;
}

} // namespace echosar
