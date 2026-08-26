package echosar.sensing;

simple AbstractObstacleSensor
{
    parameters:
        string physicalEnvironmentModule = default("^.^.physicalEnvironment");
        double minimumRange @unit(m) = default(0.7m);
        // Negativo desabilita o limite: o mecanismo passa a avaliar a obstrução
        // geométrica do enlace inteiro. Os 30 m do Phantom 4 permanecem como
        // referência de prevenção de colisões, não como limite de identificação.
        double maximumRange @unit(m) = default(-1m);
        @class(echosar::AbstractObstacleSensor);
        @display("i=block/camera;is=s");
}
