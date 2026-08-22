# Graph Report - sistema  (2026-08-22)

## Corpus Check
- 35 files · ~16,520 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 530 nodes · 827 edges · 26 communities (20 shown, 6 thin omitted)
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 29 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `321a3ff5`
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
- TeamLinkState
- RepositionController
- Repository Guidelines
- DroneApp.h
- StaticVictim
- pcap_batch_to_spreadsheet.py
- handleAssignment
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
7. `RepositionController` - 14 edges
8. `RepositionFitness` - 14 edges
9. `parse_sca()` - 13 edges
10. `BatParameters` - 13 edges

## Surprising Connections (you probably didn't know these)
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `MultiseedStatisticsTests` --uses--> `Capture`  [INFERRED]
  analysis/tests/test_pcap_multiseed.py → analysis/pcap_batch_to_spreadsheet.py
- `decode_echosar_payload()` --calls--> `text()`  [INFERRED]
  analysis/pcap_core.py → analysis/tests/test_pcap_analysis.py
- `DroneApp` --references--> `RepositionController`  [EXTRACTED]
  src/app/DroneApp.h → src/app/RepositionController.h
- `DroneApp` --references--> `BatParameters`  [EXTRACTED]
  src/app/DroneApp.h → src/optimization/BatAlgorithm.h

## Import Cycles
- None detected.

## Communities (26 total, 6 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (76): DroneApp, ackTimeout, alertAttemptsSent, alertsExpired, alertTtl, applicationIpTtl, appPort, baActivations (+68 more)

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
Cohesion: 0.10
Nodes (20): PendingVictimAlert, ackDeadline, alertId, attempts, attemptSentTimes, baCycles, degradationEvaluated, generationTime (+12 more)

### Community 5 - "DroneApp.cc"
Cohesion: 0.18
Nodes (19): cMessage, Packet, string, UdpSocket, calculatePositionUpdatePdr, detectDegradation, finish, handleMessageWhenUp (+11 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (36): AbstractObstacleSensor, tryReposition, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY (+28 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.13
Nodes (18): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, finish (+10 more)

### Community 9 - "network_metrics.py"
Cohesion: 0.13
Nodes (28): aggregate(), collect(), extreme_where(), main(), mean_where(), DataFrame, ratio(), Extract INET network-layer metrics per run and aggregate them across seeds.… (+20 more)

### Community 10 - "Cenários científicos da professora"
Cohesion: 0.14
Nodes (11): Cenários científicos da professora, Desenho solicitado, Execução, Gatilho e Bat Algorithm, Mobilidade e referência física, Métricas e validade, Pergunta e hipóteses, Rádio, obstáculo e FSPL (+3 more)

### Community 13 - "SarScenarioManager"
Cohesion: 0.18
Nodes (11): cModule, map, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections (+3 more)

### Community 14 - "TeamLinkState"
Cohesion: 0.15
Nodes (14): simtime_t, LinkSample, receptionTime, rssiDbm, sequence, TeamLinkState, ipAddress, lastSeen (+6 more)

### Community 15 - "RepositionController"
Cohesion: 0.22
Nodes (5): string, RepositionController, activeAlertId, state, State

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "DroneApp.h"
Cohesion: 0.25
Nodes (6): deque, ApplicationBase, Indication, set, string, UdpSocket

### Community 18 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 19 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.06
Nodes (59): main(), pcap_metrics(), plot_pcap(), DataFrame, Capture, compare_group(), discover_captures(), main() (+51 more)

### Community 20 - "handleAssignment"
Cohesion: 0.50
Nodes (4): Coord, estimateTeamPosition, handleAssignment, VictimAssignment

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

## Knowledge Gaps
- **207 isolated node(s):** `run.sh script`, `sequence`, `receptionTime`, `rssiDbm`, `ipAddress` (+202 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **6 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `BatParameters`, `PendingVictimAlert`, `DroneApp.cc`, `FitnessParameters`, `LifecycleOperation`, `SarScenarioManager`, `TeamLinkState`, `RepositionController`, `DroneApp.h`, `handleAssignment`?**
  _High betweenness centrality (0.317) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `DroneApp.h`, `TeamApp.cc`, `UdpSocket`, `LifecycleOperation`?**
  _High betweenness centrality (0.106) - this node is a cross-community bridge._
- **Why does `FitnessParameters` connect `FitnessParameters` to `DroneApp`?**
  _High betweenness centrality (0.070) - this node is a cross-community bridge._
- **What connects `run.sh script`, `sequence`, `receptionTime` to the rest of the system?**
  _207 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.025974025974025976 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06451612903225806 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._