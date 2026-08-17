package echosar.scenario;

simple StaticVictim
{
    parameters:
        string victimId;
        double positionX @unit(m);
        double positionY @unit(m);
        double positionZ @unit(m) = default(0m);
        double detectionTime @unit(s);
        @class(echosar::StaticVictim);
        @display("i=abstract/person,red;is=s");
}
