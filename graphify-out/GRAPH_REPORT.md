# Graph Report - sistema  (2026-08-22)

## Corpus Check
- 35 files · ~16,520 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 540 nodes · 820 edges · 30 communities (18 shown, 12 thin omitted)
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 36 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `550956fd`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- PendingVictimAlert
- DroneApp.cc
- FitnessParameters
- LifecycleOperation
- BaGaussMarkovMobility
- network_metrics.py
- Cenários científicos da professora
- plot_scenario1_line1.py
- run.sh
- SarScenarioManager
- pcap_batch_to_spreadsheet.py
- RepositionController
- Repository Guidelines
- UdpSocket
- TeamApp.h
- pcap_core.py
- AbstractObstacleSensor
- Packet
- Indication
- simsignal_t
- UdpSocket::ICallback
- TeamApp.cc
- UdpSocket
- LifecycleOperation
- SarMessageSerializers.cc
- analysis/README.md

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 109 edges
2. `TeamApp` - 43 edges
3. `PendingVictimAlert` - 30 edges
4. `FitnessParameters` - 22 edges
5. `BaGaussMarkovMobility` - 19 edges
6. `TeamLinkState` - 17 edges
7. `RepositionFitness` - 14 edges
8. `RepositionController` - 13 edges
9. `BatParameters` - 12 edges
10. `collect()` - 11 edges

## Surprising Connections (you probably didn't know these)
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `pcap_metrics()` --calls--> `compare_group()`  [INFERRED]
  analysis/compare_sca_pcap_scenario1.py → analysis/pcap_batch_to_spreadsheet.py
- `pcap_metrics()` --calls--> `discover_captures()`  [INFERRED]
  analysis/compare_sca_pcap_scenario1.py → analysis/pcap_batch_to_spreadsheet.py
- `collect()` --calls--> `parse_sca()`  [INFERRED]
  analysis/network_metrics.py → analysis/process_results.py
- `aggregate()` --calls--> `ci95()`  [INFERRED]
  analysis/network_metrics.py → analysis/process_results.py

## Import Cycles
- None detected.

## Communities (30 total, 12 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (78): BatParameters, RepositionController, simsignal_t, DroneApp, ackTimeout, alertAttemptsSent, alertsExpired, alertTtl (+70 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (30): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+22 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): cObject, IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (32): cRNG, FeasibilityFunction, FitnessFunction, Coord, Bat, amplitude, fitness, frequency (+24 more)

### Community 4 - "PendingVictimAlert"
Cohesion: 0.05
Nodes (42): ApplicationBase, deque, map, set, simtime_t, string, LinkSample, receptionTime (+34 more)

### Community 5 - "DroneApp.cc"
Cohesion: 0.15
Nodes (23): Packet, cMessage, Coord, string, UdpSocket, calculatePositionUpdatePdr, detectDegradation, estimateTeamPosition (+15 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.08
Nodes (35): tryReposition, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY, areaMinX (+27 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.13
Nodes (18): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, finish (+10 more)

### Community 9 - "network_metrics.py"
Cohesion: 0.12
Nodes (28): aggregate(), collect(), extreme_where(), main(), mean_where(), DataFrame, ratio(), Extract INET network-layer metrics per run and aggregate them across seeds.… (+20 more)

### Community 10 - "Cenários científicos da professora"
Cohesion: 0.14
Nodes (11): Cenários científicos da professora, Desenho solicitado, Execução, Gatilho e Bat Algorithm, Mobilidade e referência física, Métricas e validade, Pergunta e hipóteses, Rádio, obstáculo e FSPL (+3 more)

### Community 13 - "SarScenarioManager"
Cohesion: 0.18
Nodes (11): cModule, map, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections (+3 more)

### Community 14 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.10
Nodes (29): main(), pcap_metrics(), plot_pcap(), DataFrame, Capture, compare_group(), discover_captures(), main() (+21 more)

### Community 15 - "RepositionController"
Cohesion: 0.22
Nodes (5): string, RepositionController, activeAlertId, state, State

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 18 - "TeamApp.h"
Cohesion: 0.50
Nodes (3): ApplicationBase, LifecycleOperation, set

### Community 19 - "pcap_core.py"
Cohesion: 0.10
Nodes (30): compare_direction(), decode_echosar_payload(), find_ipv4_udp(), is_group_destination(), iter_pcapng_packets(), load_capture(), packet_identity_method(), packet_key() (+22 more)

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

## Knowledge Gaps
- **207 isolated node(s):** `ECHOSAR-Net — cenários da professora`, `Análise`, `Pergunta e hipóteses`, `Desenho solicitado`, `Mobilidade e referência física` (+202 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **12 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `PendingVictimAlert`, `DroneApp.cc`, `FitnessParameters`, `LifecycleOperation`, `UdpSocket`?**
  _High betweenness centrality (0.272) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.cc`, `TeamApp.h`, `UdpSocket`, `LifecycleOperation`?**
  _High betweenness centrality (0.104) - this node is a cross-community bridge._
- **Why does `PendingVictimAlert` connect `PendingVictimAlert` to `DroneApp`, `BatParameters`, `DroneApp.cc`, `FitnessParameters`?**
  _High betweenness centrality (0.069) - this node is a cross-community bridge._
- **What connects `ECHOSAR-Net — cenários da professora`, `Análise`, `Pergunta e hipóteses` to the rest of the system?**
  _207 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.02531645569620253 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06451612903225806 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._