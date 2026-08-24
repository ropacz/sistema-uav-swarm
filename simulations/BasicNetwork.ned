package echosar.simulations;

import inet.environment.common.PhysicalEnvironment;
import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.physicallayer.wireless.ieee80211.packetlevel.Ieee80211ScalarRadioMedium;
import inet.node.aodv.AodvRouter;
import inet.visualizer.canvas.integrated.IntegratedCanvasVisualizer;
import echosar.node.SarDrone;
import echosar.metrics.ExperimentMetrics;
import echosar.scenario.SarScenarioManager;
import echosar.scenario.StaticVictim;

network BasicNetwork
{
    parameters:
        int numDrones = default(10);
        int numTeams  = default(3);
        int numVictims = default(0);
        // Os 1000 px superiores representam a área física de 1 km × 1 km.
        // A faixa inferior de 150 px é reservada aos módulos de infraestrutura.
        @display("bgb=1000,1150;bgg=100,1,grey95;bgc=white");

    submodules:
        // ── Módulos de infraestrutura ─────────────────────────────────────────
        // Agrupados no canto inferior, fora da região inicial do demo visual.
        // O espaçamento considera também os nomes exibidos automaticamente.
        visualizer: IntegratedCanvasVisualizer {
            @display("p=80,1080;i=block/app2;is=vs");
        }
        physicalEnvironment: PhysicalEnvironment {
            @display("p=290,1080;i=misc/town;is=vs");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=500,1080;i=block/cogwheel;is=vs");
        }
        radioMedium: Ieee80211ScalarRadioMedium {
            @display("p=700,1080;i=misc/sun;is=vs");
        }
        scenarioManager: SarScenarioManager {
            @display("p=880,1080;i=block/control;is=vs");
        }
        experimentMetrics: ExperimentMetrics {
            @display("p=970,1080");
        }
        // ── Drones: AodvRouter (FANET ad-hoc com roteamento AODV multi-hop) ───
        drone[numDrones]: SarDrone {
            numApps = 1;
            app[0].typename = "echosar.app.DroneApp";
            // Evita sobrescrever capturas ao repetir a mesma configuração.
            pcapRecorder[*].pcapFile = default("results/pcap/" + expand("${configname}-${runnumber}-") + fullPath() + ".pcapng");
            @display("i=misc/drone,#3399ff,30");
        }
        // ── Embarcações de resgate: AodvRouter (mesmo stack de rede) ──────────
        team[numTeams]: AodvRouter {
            numApps = 1;
            app[0].typename = "echosar.app.TeamApp";
            pcapRecorder[*].pcapFile = default("results/pcap/" + expand("${configname}-${runnumber}-") + fullPath() + ".pcapng");
            @display("i=block/user,#33aa33,35");
        }
        victim[numVictims]: StaticVictim;
}
