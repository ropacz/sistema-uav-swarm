# Graph Report - sistema  (2026-08-24)

## Corpus Check
- 38 files · ~16,856 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 555 nodes · 839 edges · 32 communities (21 shown, 11 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 29 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `90d4f0e6`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- StaticVictim
- DroneApp.cc
- FitnessParameters
- TeamApp.h
- BaGaussMarkovMobility
- network_metrics.py
- Cenários científicos da professora
- plot_scenario1_line1.py
- run.sh
- SarScenarioManager
- PendingVictimAlert
- RepositionController
- Repository Guidelines
- UdpSocket
- TeamLinkState
- pcap_batch_to_spreadsheet.py
- DroneApp.h
- LinkSample
- LifecycleOperation
- cMessage
- Indication
- simsignal_t
- TeamApp.cc
- UdpSocket
- LifecycleOperation
- UdpSocket::ICallback
- SarMessageSerializers.cc
- analysis/README.md

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 109 edges
2. `TeamApp` - 43 edges
3. `PendingVictimAlert` - 30 edges
4. `FitnessParameters` - 21 edges
5. `BaGaussMarkovMobility` - 19 edges
6. `TeamLinkState` - 17 edges
7. `RepositionFitness` - 14 edges
8. `RepositionController` - 13 edges
9. `parse_sca()` - 13 edges
10. `BatParameters` - 12 edges

## Surprising Connections (you probably didn't know these)
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `DroneApp` --references--> `PendingVictimAlert`  [EXTRACTED]
  src/app/DroneApp.h → src/app/PendingVictimAlert.h
- `DroneApp` --references--> `TeamLinkState`  [EXTRACTED]
  src/app/DroneApp.h → src/app/TeamLinkState.h
- `sendAttempt` --references--> `PendingVictimAlert`  [EXTRACTED]
  src/app/DroneApp.h → src/app/PendingVictimAlert.h
- `detectDegradation` --references--> `PendingVictimAlert`  [EXTRACTED]
  src/app/DroneApp.h → src/app/PendingVictimAlert.h

## Import Cycles
- None detected.

## Communities (32 total, 11 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.02
Nodes (80): BatParameters, cMessage, FitnessParameters, RepositionController, simsignal_t, DroneApp, ackTimeout, alertAttemptsSent (+72 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (30): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+22 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): cObject, IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (31): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+23 more)

### Community 4 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 5 - "DroneApp.cc"
Cohesion: 0.15
Nodes (23): cMessage, Coord, Packet, string, UdpSocket, calculatePositionUpdatePdr, detectDegradation, estimateTeamPosition (+15 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (36): AbstractObstacleSensor, tryReposition, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY (+28 more)

### Community 7 - "TeamApp.h"
Cohesion: 0.40
Nodes (4): ApplicationBase, LifecycleOperation, UdpSocket, set

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.13
Nodes (18): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, finish (+10 more)

### Community 9 - "network_metrics.py"
Cohesion: 0.13
Nodes (28): aggregate(), collect(), extreme_where(), main(), mean_where(), DataFrame, ratio(), Extract INET network-layer metrics per run and aggregate them across seeds.… (+20 more)

### Community 10 - "Cenários científicos da professora"
Cohesion: 0.10
Nodes (16): Cenários científicos da professora, Desenho solicitado, Execução, Gatilho e Bat Algorithm, Mobilidade e referência física, Métricas e validade, Pergunta e hipóteses, Rádio, obstáculo e FSPL (+8 more)

### Community 13 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 14 - "PendingVictimAlert"
Cohesion: 0.09
Nodes (23): map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts, attemptSentTimes (+15 more)

### Community 15 - "RepositionController"
Cohesion: 0.22
Nodes (5): string, RepositionController, activeAlertId, state, State

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 18 - "TeamLinkState"
Cohesion: 0.17
Nodes (12): deque, Coord, string, TeamLinkState, ipAddress, lastSeen, lastSequence, position (+4 more)

### Community 19 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.06
Nodes (59): main(), pcap_metrics(), plot_pcap(), DataFrame, Capture, compare_group(), discover_captures(), main() (+51 more)

### Community 20 - "DroneApp.h"
Cohesion: 0.36
Nodes (5): ApplicationBase, set, map, string, Coord

### Community 21 - "LinkSample"
Cohesion: 0.40
Nodes (5): simtime_t, LinkSample, receptionTime, rssiDbm, sequence

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

## Knowledge Gaps
- **210 isolated node(s):** `Organização`, `Cenários científicos`, `Validações técnicas`, `Resultados`, `socket` (+205 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **11 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `DroneApp.cc`, `FitnessParameters`, `PendingVictimAlert`, `UdpSocket`, `TeamLinkState`, `DroneApp.h`, `LifecycleOperation`?**
  _High betweenness centrality (0.277) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.cc`, `UdpSocket`, `LifecycleOperation`, `TeamApp.h`?**
  _High betweenness centrality (0.101) - this node is a cross-community bridge._
- **Why does `tryReposition` connect `FitnessParameters` to `DroneApp`, `BatParameters`, `DroneApp.cc`, `PendingVictimAlert`?**
  _High betweenness centrality (0.074) - this node is a cross-community bridge._
- **What connects `Organização`, `Cenários científicos`, `Validações técnicas` to the rest of the system?**
  _210 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.024691358024691357 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06451612903225806 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._