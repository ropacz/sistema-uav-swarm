package echosar.app;

import inet.applications.contract.IApp;

simple TeamApp like IApp
{
    parameters:
        @class(echosar::TeamApp);
        string teamId = default("");
        int appPort = default(5000);
        int positionUpdatePayloadBytes = default(160);
        int victimAckPayloadBytes = default(96);
        double positionUpdateInterval @unit(s) = default(1s);
        double initialJitter @unit(s) = default(this.positionUpdateInterval);
        int applicationIpTtl = default(32);
        // Injeção determinística de falha: nenhum VictimAck antes deste instante.
        double ackStartTime @unit(s) = default(0s);
        @signal[deliveryDelay](type="simtime_t");
        @statistic[deliveryDelay](source="deliveryDelay"; record=vector,histogram,mean; unit="s");
        @signal[hopCount](type=long);
        @statistic[hopCount](source="hopCount"; record=vector,histogram,count,sum,mean);
        @signal[victimAlertDelivered](type=echosar::AlertMetricEvent);
        @display("i=block/user,green,50");
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}
