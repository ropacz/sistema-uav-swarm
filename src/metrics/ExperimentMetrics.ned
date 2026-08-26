package echosar.metrics;

simple ExperimentMetrics
{
    parameters:
        @class(echosar::ExperimentMetrics);
        // Onde gravar o registro por alerta. Acompanha o diretório dos
        // escalares; vazio desabilita o registro.
        string alertRecordDirectory = default("results/omnetpp");
        // Cenários científicos exigem que todos os alertas tenham alcançado um
        // estado terminal. Testes de truncamento podem manter o default falso.
        bool requireClosedAlerts = default(false);
        @display("i=block/table;is=vs");
}
