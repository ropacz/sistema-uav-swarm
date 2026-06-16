package echosar.app;

import inet.applications.contract.IApp;

simple SimpleDroneApp like IApp
{
    parameters:
        @class(echosar::SimpleDroneApp);
        string myDroneId = default("");
        double victimInterval @unit(s) = default(20s);   // intervalo médio de detecção SAR
        double teamTimeout @unit(s)    = default(30s);   // timer Link State [SCI-2019]
        double retryInterval @unit(s)  = default(10s);   // intervalo entre retries store-forward
        int    maxRetries              = default(5);      // máximo de retries antes de descartar
        @display("i=misc/drone");
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}
