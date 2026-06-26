package echosar.app;

import inet.applications.contract.IApp;

simple SimpleTeamApp like IApp
{
    parameters:
        @class(echosar::SimpleTeamApp);
        string myTeamId = default("");
        double sendInterval @unit(s)  = default(5s);
        // beaconJitter: offset aleatório do primeiro beacon — CRÍTICO.
        // Com múltiplas equipes, beacons sincronizados geram rajadas de broadcast
        // que aumentam a contenção no meio sem fio e degradam a recepção dos
        // TeamUpdate, deixando a teamTable vazia nos drones → PDR próximo de zero.
        // O jitter distribui as transmissões iniciais no tempo e evita essa
        // regressão. Valor zero é rejeitado em initialize(). Mantenha = sendInterval.
        double beaconJitter @unit(s)  = default(this.sendInterval);
        double teamSpeed @unit(mps)   = default(7mps);
        double serviceTime @unit(s)   = default(120s);
        bool   enableProbeAck         = default(false); // [BA] envia ProbeAck ao drone após DroneStatus
        @display("i=block/user,green,50");
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}
