# Graph Report - sistema  (2026-08-24)

## Corpus Check
- 50 files · ~20,788 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 695 nodes · 1027 edges · 43 communities (25 shown, 18 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 27 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `8653a1fc`
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
- TeamApp.h
- TeamLinkState
- pcap_batch_to_spreadsheet.py
- string
- AlertMetricEvent
- ExperimentMetrics.h
- ExperimentMetrics
- receiveSignal
- core/__init__.py
- TeamApp.cc
- UdpSocket
- LifecycleOperation
- analysis/__init__.py
- SarMessageSerializers.cc
- pcap/__init__.py
- plots/__init__.py
- reports/__init__.py
- analysis/README.md
- validation/__init__.py
- report_main_experiment.py
- Path
- Packet
- Indication
- UdpSocket::ICallback
- cObject
- cSimpleModule

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 118 edges
2. `ExperimentMetrics` - 61 edges
3. `TeamApp` - 44 edges
4. `PendingVictimAlert` - 35 edges
5. `FitnessParameters` - 21 edges
6. `BaGaussMarkovMobility` - 19 edges
7. `TeamLinkState` - 18 edges
8. `collect()` - 17 edges
9. `RepositionFitness` - 15 edges
10. `RepositionController` - 13 edges

## Surprising Connections (you probably didn't know these)
- `tryReposition` --references--> `RepositionFitness`  [INFERRED]
  src/app/DroneApp.h → src/optimization/RepositionFitness.h
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `load_arm()` --calls--> `collect()`  [EXTRACTED]
  analysis/reports/report_main_experiment.py → analysis/core/network_metrics.py
- `parameter_differences()` --calls--> `parse_sca()`  [EXTRACTED]
  analysis/reports/report_main_experiment.py → analysis/core/process_results.py
- `summarize()` --calls--> `ci95()`  [EXTRACTED]
  analysis/reports/report_main_experiment.py → analysis/core/process_results.py

## Import Cycles
- None detected.

## Communities (43 total, 18 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.02
Nodes (88): BatParameters, FitnessParameters, RepositionController, DroneApp, ackTimeout, alertAttemptSentSignal, alertAttemptsSent, alertConfirmedSignal (+80 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (31): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+23 more)

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
Cohesion: 0.15
Nodes (24): Packet, cMessage, Coord, string, UdpSocket, calculatePositionUpdatePdr, detectDegradation, estimateTeamPosition (+16 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (35): AbstractObstacleSensor, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY, areaMinX (+27 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.13
Nodes (18): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, finish (+10 more)

### Community 9 - "network_metrics.py"
Cohesion: 0.10
Nodes (36): aggregate(), collect(), extreme_where(), global_or_legacy(), global_scalar(), main(), mean_where(), pooled_statistic_mean() (+28 more)

### Community 10 - "Cenários científicos da professora"
Cohesion: 0.09
Nodes (19): Cenários científicos da professora, Desenho solicitado, Execução, Gatilho e Bat Algorithm, Hierarquia do escopo, Mobilidade e referência física, Métricas e validade, Pergunta e hipóteses (+11 more)

### Community 13 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 14 - "PendingVictimAlert"
Cohesion: 0.07
Nodes (28): map, simtime_t, string, PendingVictimAlert, ackDeadline, activeRepositionCycleId, alertId, attempts (+20 more)

### Community 15 - "RepositionController"
Cohesion: 0.22
Nodes (5): string, RepositionController, activeAlertId, state, State

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "TeamApp.h"
Cohesion: 0.25
Nodes (5): Indication, ApplicationBase, LifecycleOperation, UdpSocket, set

### Community 18 - "TeamLinkState"
Cohesion: 0.12
Nodes (18): deque, Coord, simtime_t, string, LinkSample, receptionTime, rssiDbm, sequence (+10 more)

### Community 19 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.06
Nodes (59): main(), pcap_metrics(), plot_pcap(), DataFrame, Capture, compare_group(), discover_captures(), main() (+51 more)

### Community 20 - "string"
Cohesion: 0.42
Nodes (4): ApplicationBase, map, string, Coord

### Community 21 - "AlertMetricEvent"
Cohesion: 0.18
Nodes (10): AlertMetricEvent, alertId, category, messageId, referenceTime, secondaryTime, value, cObject (+2 more)

### Community 22 - "ExperimentMetrics.h"
Cohesion: 0.22
Nodes (7): cListener, cSimpleModule, cMessage, cMessage, finish, handleMessage, initialize

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (55): ExperimentMetrics, alertAttemptsSent, attemptDeliveryDelaySum, attemptsByAlert, attemptSentSignal, attemptSentTimes, baActivations, baActivationSignal (+47 more)

### Community 24 - "receiveSignal"
Cohesion: 0.50
Nodes (4): cComponent, cObject, simsignal_t, receiveSignal

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

### Community 36 - "report_main_experiment.py"
Cohesion: 0.20
Nodes (14): load_arm(), main(), pair_runs(), parameter_differences(), DataFrame, Analyze the minimal paired BA Off/On confirmatory experiment. The seed is the…, Reject a study in which the treatment was never actually exercised., Return recorded parameter differences other than the treatment flag. (+6 more)

## Knowledge Gaps
- **284 isolated node(s):** `Organização`, `Análise`, `Pergunta e hipóteses`, `Hierarquia do escopo`, `Desenho solicitado` (+279 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **18 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `DroneApp.cc`, `LifecycleOperation`, `PendingVictimAlert`, `TeamApp.h`, `TeamLinkState`, `string`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.227) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `receiveSignal`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.105) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.h`, `TeamApp.cc`, `UdpSocket`, `LifecycleOperation`?**
  _High betweenness centrality (0.079) - this node is a cross-community bridge._
- **What connects `Organização`, `Análise`, `Pergunta e hipóteses` to the rest of the system?**
  _284 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.02247191011235955 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.0625 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._