package echosar.simulations;

import inet.environment.common.PhysicalEnvironment;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.node.inet.WirelessHost;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

network BasicNetwork
{
    parameters:
        int numDrones = default(10);
        int numTeams  = default(3);
        @display("bgb=5000,5000;bgg=500,1,grey95");

    submodules:
        visualizer: IntegratedCanvasVisualizer {
            @display("p=40,40");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=40,280;i=misc/town");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=40,120;i=block/cogwheel_s");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=40,200;i=misc/sun");
        }
        drone[numDrones]: WirelessHost {
            numApps = 1;
            app[0].typename = "echosar.app.SimpleDroneApp";
            @display("i=misc/drone,#00aaff,40");
        }
        team[numTeams]: WirelessHost {
            numApps = 1;
            app[0].typename = "echosar.app.SimpleTeamApp";
            @display("i=block/user,green,50");
        }
}
