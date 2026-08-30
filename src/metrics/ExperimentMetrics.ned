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
        // Deslocamento mínimo para um reposicionamento concluído ser contado
        // como operacionalmente relevante em effectiveRepositions. Puramente
        // classificatório: não altera o comportamento da simulação, e a
        // distância bruta continua sendo somada em repositionDistanceSum.
        double effectiveRepositionThreshold @unit(m) = default(1m);
        @display("i=block/table;is=vs");
}
