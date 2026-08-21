package echosar.mobility;

import inet.mobility.single.GaussMarkovMobility;

simple BaGaussMarkovMobility extends GaussMarkovMobility
{
    parameters:
        double initialX @unit(m) = default(uniform(constraintAreaMinX, constraintAreaMaxX));
        double initialY @unit(m) = default(uniform(constraintAreaMinY, constraintAreaMaxY));
        double initialZ @unit(m) = default(uniform(constraintAreaMinZ, constraintAreaMaxZ));
        // O GaussMarkovMobility nativo do INET gera somente azimute XY. Estes
        // parâmetros acrescentam a componente vertical correlacionada.
        double elevation @unit(deg) = default(0deg);
        double elevationStdDev @unit(deg) = default(5deg);
        bool initFromDisplayString = default(false);
        @class(echosar::BaGaussMarkovMobility);
}
