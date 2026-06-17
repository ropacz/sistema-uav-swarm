package echosar.simulations;

import inet.environment.common.PhysicalEnvironment;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.node.aodv.AodvRouter;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;

network BasicNetwork
{
    parameters:
        int numDrones = default(10);
        int numTeams  = default(3);
        // bgb: canvas Qtenv 2 km × 2 km; bgg: grade 200 m; fundo neutro
        @display("bgb=2000,2000;bgg=200,1,grey95;bgc=white");

    submodules:
        // ── Módulos de infraestrutura ─────────────────────────────────────────
        // Posicionados à esquerda da área (x=75), espaçados 350 unidades em Y.
        visualizer: IntegratedCanvasVisualizer {
            @display("p=75,150;i=block/app2;is=s");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=75,500;i=misc/town;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=75,850;i=block/cogwheel_s;is=s");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=75,1200;i=misc/sun;is=s");
        }
        // ── Drones: AodvRouter (FANET ad-hoc com roteamento AODV multi-hop) ───
        drone[numDrones]: AodvRouter {
            numApps = 1;
            app[0].typename = "echosar.app.SimpleDroneApp";
            @display("i=misc/drone,#3399ff,40");
        }
        // ── Embarcações de resgate: AodvRouter (mesmo stack de rede) ──────────
        team[numTeams]: AodvRouter {
            numApps = 1;
            app[0].typename = "echosar.app.SimpleTeamApp";
            @display("i=block/user,#33aa33,50");
        }
}
