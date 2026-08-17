package echosar.mobility;

import inet.mobility.single.GaussMarkovMobility;

simple BaGaussMarkovMobility extends GaussMarkovMobility
{
    parameters:
        double initialX @unit(m) = default(uniform(constraintAreaMinX, constraintAreaMaxX));
        double initialY @unit(m) = default(uniform(constraintAreaMinY, constraintAreaMaxY));
        double initialZ @unit(m) = default(uniform(constraintAreaMinZ, constraintAreaMaxZ));
        bool initFromDisplayString = default(false);
        @class(echosar::BaGaussMarkovMobility);
}
