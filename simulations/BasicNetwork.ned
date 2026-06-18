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
        // bgb: canvas Qtenv 5 km × 5 km; bgg: grade 500 m; fundo neutro
        @display("bgb=5000,5000;bgg=500,1,grey95;bgc=white");

    submodules:
        // ── Módulos de infraestrutura ─────────────────────────────────────────
        // Posicionados à esquerda do canvas (x<0): fora da área de mobilidade
        // (constraintAreaMinX=0) — nunca sobrepostos com nós móveis.
        // is=s (small) para ficarem visíveis sem poluir a área de simulação.
        visualizer: IntegratedCanvasVisualizer {
            @display("p=-200,1000;i=block/app2;is=s");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=-200,1200;i=misc/town;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=-200,1400;i=block/cogwheel_s;is=s");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=-200,1600;i=misc/sun;is=s");
        }
        // ── Drones: AodvRouter (FANET ad-hoc com roteamento AODV multi-hop) ───
        drone[numDrones]: AodvRouter {
            numApps = 1;
            app[0].typename = "echosar.app.SimpleDroneApp";
            @display("i=misc/drone,#3399ff,50");
        }
        // ── Embarcações de resgate: AodvRouter (mesmo stack de rede) ──────────
        team[numTeams]: AodvRouter {
            numApps = 1;
            app[0].typename = "echosar.app.SimpleTeamApp";
            @display("i=block/user,#33aa33,60");
        }
}
