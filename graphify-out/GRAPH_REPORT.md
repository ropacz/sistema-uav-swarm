# Graph Report - sistema  (2026-08-24)

## Corpus Check
- 42 files · ~18,525 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 643 nodes · 948 edges · 34 communities (23 shown, 11 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 32 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `9e822ecd`
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
- LifecycleOperation
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
- string
- AlertMetricEvent
- ExperimentMetrics.h
- ExperimentMetrics
- cSimpleModule
- receiveSignal
- TeamApp.cc
- UdpSocket
- LifecycleOperation
- TeamApp.h
- SarMessageSerializers.cc
- DataFrame
- Coord
- analysis/README.md

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 117 edges
2. `ExperimentMetrics` - 55 edges
3. `TeamApp` - 44 edges
4. `PendingVictimAlert` - 25 edges
5. `FitnessParameters` - 21 edges
6. `BaGaussMarkovMobility` - 19 edges
7. `RepositionFitness` - 15 edges
8. `TeamLinkState` - 14 edges
9. `collect()` - 13 edges
10. `RepositionController` - 13 edges

## Surprising Connections (you probably didn't know these)
- `tryReposition` --references--> `RepositionFitness`  [INFERRED]
  src/app/DroneApp.h → src/optimization/RepositionFitness.h
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `collect()` --calls--> `parse_sca()`  [INFERRED]
  analysis/network_metrics.py → analysis/process_results.py
- `aggregate()` --calls--> `ci95()`  [INFERRED]
  analysis/network_metrics.py → analysis/process_results.py
- `record()` --calls--> `parse_sca()`  [INFERRED]
  analysis/report_professor_scaling_test.py → analysis/process_results.py

## Import Cycles
- None detected.

## Communities (34 total, 11 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.02
Nodes (89): BatParameters, FitnessParameters, RepositionController, DroneApp, ackTimeout, alertAttemptSentSignal, alertAttemptsSent, alertConfirmedSignal (+81 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (30): ApplicationBase, cMessage, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp, ackStartTime (+22 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (31): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+23 more)

### Community 4 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 5 - "DroneApp.cc"
Cohesion: 0.14
Nodes (26): Coord, cMessage, Packet, PendingVictimAlert, string, TeamLinkState, UdpSocket, calculatePositionUpdatePdr (+18 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (35): AbstractObstacleSensor, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY, areaMinX (+27 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.13
Nodes (18): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, finish (+10 more)

### Community 9 - "network_metrics.py"
Cohesion: 0.11
Nodes (32): aggregate(), collect(), extreme_where(), global_or_legacy(), global_scalar(), main(), mean_where(), ratio() (+24 more)

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
Cohesion: 0.13
Nodes (16): Coord, simtime_t, string, LinkSample, receptionTime, rssiDbm, sequence, TeamLinkState (+8 more)

### Community 19 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.06
Nodes (59): main(), pcap_metrics(), plot_pcap(), DataFrame, Capture, compare_group(), discover_captures(), main() (+51 more)

### Community 20 - "string"
Cohesion: 0.33
Nodes (5): deque, map, string, Coord, set

### Community 21 - "AlertMetricEvent"
Cohesion: 0.18
Nodes (10): AlertMetricEvent, alertId, category, messageId, referenceTime, secondaryTime, value, cObject (+2 more)

### Community 22 - "ExperimentMetrics.h"
Cohesion: 0.22
Nodes (7): cListener, cSimpleModule, cMessage, cMessage, finish, handleMessage, initialize

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (49): ExperimentMetrics, alertAttemptsSent, attemptDeliveryDelaySum, attemptsByAlert, attemptSentSignal, attemptSentTimes, baActivations, baActivationSignal (+41 more)

### Community 25 - "receiveSignal"
Cohesion: 0.50
Nodes (4): cComponent, cObject, simsignal_t, receiveSignal

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

## Knowledge Gaps
- **268 isolated node(s):** `Organização`, `Pergunta e hipóteses`, `Desenho solicitado`, `Mobilidade e referência física`, `Rádio, obstáculo e FSPL` (+263 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **11 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `DroneApp.cc`, `LifecycleOperation`, `UdpSocket`, `string`, `ExperimentMetrics.h`, `TeamApp.h`?**
  _High betweenness centrality (0.249) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `receiveSignal`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.108) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.cc`, `UdpSocket`, `LifecycleOperation`, `TeamApp.h`?**
  _High betweenness centrality (0.090) - this node is a cross-community bridge._
- **What connects `Organização`, `Pergunta e hipóteses`, `Desenho solicitado` to the rest of the system?**
  _268 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.022222222222222223 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06451612903225806 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._