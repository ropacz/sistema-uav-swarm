package echosar.metrics;

simple ExperimentMetrics
{
    parameters:
        @class(echosar::ExperimentMetrics);
        // Onde gravar o registro por alerta. Acompanha o diretório dos
        // escalares; vazio desabilita o registro.
        string alertRecordDirectory = default("results/omnetpp");
        @display("i=block/table;is=vs");
}
