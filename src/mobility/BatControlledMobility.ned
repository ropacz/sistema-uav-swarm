package echosar.mobility;

import inet.mobility.base.MovingMobilityBase;

// Mobilidade controlada para o Algoritmo do Morcego (BA).
// Em modo CRUISE comporta-se como GaussMarkovMobility.
// SimpleDroneApp chama hover()/moveTo()/resumeCruise() via interface C++.
// updateInterval, constraintArea*, initialX/Y/Z, displayStringTextFormat
// são herdados de MovingMobilityBase / MobilityBase.
simple BatControlledMobility extends MovingMobilityBase
{
    parameters:
        @class(echosar::BatControlledMobility);
        @display("i=block/cogwheel");

        // Cruise (Gauss-Markov)
        double speed @unit(mps)       = default(uniform(8mps, 15mps));
        double speedStdDev @unit(mps) = default(0mps);
        double alpha                  = default(0.75);
        double angleStdDev @unit(rad) = default(0.5236rad);  // 30 graus
        double margin @unit(m)        = default(100m);
        // updateInterval herdado de MovingMobilityBase (default 0.1s, sobrescrito no .ini para 0.5s)
}
