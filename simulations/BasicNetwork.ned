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
        // bgb: área de simulação 5 km × 5 km; bgg: grade 500 m; bgi: fundo neutro
        @display("bgb=5000,5000;bgg=500,1,grey95;bgc=white");

    submodules:
        // ── Módulos de infraestrutura ─────────────────────────────────────────
        // Posicionados numa coluna à esquerda da área de mobilidade (x=250),
        // distribuídos verticalmente a cada 1200 unidades para evitar sobreposição.
        // As equipes ficam no mínimo em x=600 — distância segura de 350 unidades.
        visualizer: IntegratedCanvasVisualizer {
            @display("p=250,600;i=block/app2;is=s");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=250,1800;i=misc/town;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=250,3000;i=block/cogwheel_s;is=s");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=250,4200;i=misc/sun;is=s");
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
