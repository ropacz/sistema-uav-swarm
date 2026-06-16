package echosar.app;

import inet.applications.contract.IApp;

simple SimpleTeamApp like IApp
{
    parameters:
        @class(echosar::SimpleTeamApp);
        string myTeamId = default("");
        double sendInterval @unit(s) = default(5s);
        double teamSpeed @unit(mps)  = default(7mps);   // velocidade de deslocamento da equipe
        double serviceTime @unit(s)  = default(120s);   // tempo de atendimento no local da vítima
        @display("i=block/user,green,50");
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}
