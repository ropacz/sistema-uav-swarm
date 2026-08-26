# Graph Report - sistema  (2026-08-25)

## Corpus Check
- 52 files · ~13,710 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 566 nodes · 842 edges · 32 communities (23 shown, 9 thin omitted)
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 30 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `c7ca9a16`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- AlertMetricEvent
- AbstractObstacleSensor
- FitnessParameters
- Q: instale skills, plugins e coisas necessárias para esse projeto
- BaGaussMarkovMobility
- DroneApp.cc
- Simulações
- ExperimentMetrics.cc
- run.sh
- socketDataArrived
- LifecycleOperation
- RepositionController
- Repository Guidelines
- cSimpleModule
- string
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- report_main_experiment.py
- ExperimentMetrics
- core/__init__.py
- SarScenarioManager
- analysis/__init__.py
- SarMessageSerializers.cc
- reports/__init__.py
- analysis/README.md
- validation/__init__.py
- write_manifest.py

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 81 edges
2. `ExperimentMetrics` - 51 edges
3. `TeamApp` - 33 edges
4. `collect()` - 29 edges
5. `FitnessParameters` - 23 edges
6. `PendingVictimAlert` - 22 edges
7. `RepositionFitness` - 17 edges
8. `BatParameters` - 12 edges
9. `BaGaussMarkovMobility` - 12 edges
10. `ActiveVictim` - 11 edges

## Surprising Connections (you probably didn't know these)
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `collect()` --calls--> `parse_sca()`  [EXTRACTED]
  analysis/core/experiment_metrics.py → analysis/core/process_results.py
- `load_arm()` --calls--> `collect()`  [EXTRACTED]
  analysis/reports/report_main_experiment.py → analysis/core/experiment_metrics.py
- `run_record()` --calls--> `collect()`  [EXTRACTED]
  analysis/reports/report_robustness.py → analysis/core/experiment_metrics.py
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_alert_lifecycle_smoke_test.py → analysis/core/experiment_metrics.py

## Import Cycles
- None detected.

## Communities (32 total, 9 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.04
Nodes (47): BatParameters, DroneApp, ackTimeout, activeVictims, alertAttemptSentSignal, alertConfirmedSignal, alertExpiredSignal, alertGeneratedSignal (+39 more)

### Community 1 - "TeamApp"
Cohesion: 0.07
Nodes (31): cMessage, Packet, UdpSocket, ApplicationBase, cMessage, Indication, LifecycleOperation, simsignal_t (+23 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (31): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+23 more)

### Community 4 - "AlertMetricEvent"
Cohesion: 0.20
Nodes (9): AlertMetricEvent, alertId, category, messageId, referenceTime, value, cObject, simtime_t (+1 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (40): tryReposition, AbstractObstacleSensor, Coord, simtime_t, vector, FitnessParameters, areaMaxX, areaMaxY (+32 more)

### Community 7 - "Q: instale skills, plugins e coisas necessárias para esse projeto"
Cohesion: 0.40
Nodes (4): Answer, Outcome, Q: instale skills, plugins e coisas necessárias para esse projeto, Source Nodes

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (13): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, initialize (+5 more)

### Community 9 - "DroneApp.cc"
Cohesion: 0.19
Nodes (18): cMessage, string, completeAlertCycle, finish, handleAssignment, handleMessageWhenUp, handleVictimAck, initialize (+10 more)

### Community 10 - "Simulações"
Cohesion: 0.13
Nodes (13): Documentação técnica e científica, Fonte única dos parâmetros, Material complementar, ECHOSAR-Net — reposicionamento de UAV com Bat Algorithm, Limitações do modelo, Organização, Ambiente físico, Experimento principal (+5 more)

### Community 11 - "ExperimentMetrics.cc"
Cohesion: 0.22
Nodes (8): cComponent, cMessage, cObject, simsignal_t, finish, handleMessage, initialize, receiveSignal

### Community 13 - "socketDataArrived"
Cohesion: 0.60
Nodes (5): Packet, UdpSocket, handleDroneStatus, handleTeamUpdate, socketDataArrived

### Community 15 - "RepositionController"
Cohesion: 0.36
Nodes (3): string, RepositionController, activeAlertId

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 18 - "string"
Cohesion: 0.06
Nodes (30): ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, Coord, simtime_t (+22 more)

### Community 19 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): Coord, map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts (+9 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.50
Nodes (3): cListener, cSimpleModule, cMessage

### Community 21 - "collect"
Cohesion: 0.08
Nodes (29): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run. (+21 more)

### Community 22 - "report_main_experiment.py"
Cohesion: 0.08
Nodes (34): ci95(), parse_sca(), DataFrame, Return attributes, scalar rows and recorded parameters from one SCA., Student-t approximate 95% confidence-interval half-width for a mean., load_arm(), main(), pair_runs() (+26 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (45): ExperimentMetrics, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal, completedRepositionAlertIds, confirmationDelayCount (+37 more)

### Community 26 - "SarScenarioManager"
Cohesion: 0.18
Nodes (11): cModule, set, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections (+3 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.12
Nodes (29): b, Chunk, ChunkSerializer, DroneStatusChunk, MemoryInputStream, MemoryOutputStream, Ptr, simtime_t (+21 more)

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **206 isolated node(s):** `Organização`, `Limitações do modelo`, `Fonte única dos parâmetros`, `Material complementar`, `Experimento principal` (+201 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **9 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `FitnessParameters`, `DroneApp.cc`, `socketDataArrived`, `LifecycleOperation`, `RepositionController`, `string`, `PendingVictimAlert`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.241) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `ExperimentMetrics.cc`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.113) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `string`, `SarScenarioManager`?**
  _High betweenness centrality (0.077) - this node is a cross-community bridge._
- **What connects `Organização`, `Limitações do modelo`, `Fonte única dos parâmetros` to the rest of the system?**
  _206 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.041666666666666664 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06612685560053981 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._