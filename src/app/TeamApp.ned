package echosar.app;

import inet.applications.contract.IApp;

simple TeamApp like IApp
{
    parameters:
        @class(echosar::TeamApp);
        string teamId = default("");
        int appPort = default(5000);
        int teamUpdatePayloadBytes = default(160);
        int victimAckPayloadBytes;
        // Payload da resposta à sondagem de recuperação. Deve acompanhar o
        // recoveryProbePayloadBytes do drone: as duas metades do eco.
        int recoveryProbePayloadBytes = default(128);
        double teamUpdateInterval @unit(s) = default(1s);
        double initialJitter @unit(s) = default(this.teamUpdateInterval);
        int applicationIpTtl = default(32);
        // Injeção determinística de falha: nenhum VictimAck antes deste instante.
        double ackStartTime @unit(s) = default(0s);
        @signal[victimAlertDelivered](type=echosar::AlertMetricEvent);
        @display("i=block/user,green,50");
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}
