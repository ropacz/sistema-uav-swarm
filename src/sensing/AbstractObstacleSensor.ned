package echosar.sensing;

simple AbstractObstacleSensor
{
    parameters:
        string physicalEnvironmentModule = default("^.^.physicalEnvironment");
        double minimumRange @unit(m) = default(0.7m);
        double maximumRange @unit(m) = default(30m);
        @class(echosar::AbstractObstacleSensor);
        @display("i=block/camera;is=s");
}
