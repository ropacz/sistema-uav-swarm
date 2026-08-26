# Graph Report - sistema  (2026-08-25)

## Corpus Check
- 51 files · ~13,660 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 559 nodes · 844 edges · 34 communities (26 shown, 8 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 29 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `53305d45`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- AlertMetricEvent
- report_main_experiment.py
- FitnessParameters
- experiment_metrics.py
- BaGaussMarkovMobility
- DroneApp.cc
- Simulações
- ExperimentMetrics.cc
- run.sh
- socketDataArrived
- LifecycleOperation
- RepositionController
- Repository Guidelines
- test_experiment_metrics.py
- string
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- report_robustness.py
- ExperimentMetrics
- UdpSocket
- core/__init__.py
- SarScenarioManager
- central_scalar
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
8. `BatParameters` - 13 edges
9. `BaGaussMarkovMobility` - 12 edges
10. `optimize` - 12 edges

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

## Communities (34 total, 8 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.04
Nodes (46): DroneApp, ackTimeout, activeVictims, alertAttemptSentSignal, alertConfirmedSignal, alertExpiredSignal, alertGeneratedSignal, alertInterval (+38 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (33): ApplicationBase, cMessage, Packet, UdpSocket, ApplicationBase, cMessage, Indication, LifecycleOperation (+25 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (32): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+24 more)

### Community 4 - "AlertMetricEvent"
Cohesion: 0.20
Nodes (9): AlertMetricEvent, alertId, category, messageId, referenceTime, value, cObject, simtime_t (+1 more)

### Community 5 - "report_main_experiment.py"
Cohesion: 0.20
Nodes (14): load_arm(), main(), pair_runs(), parameter_differences(), DataFrame, Path, Analyze the minimal paired BA Off/On confirmatory experiment. The seed is the…, Describe treatment exposure without excluding post-treatment runs. (+6 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (40): AbstractObstacleSensor, tryReposition, AbstractObstacleSensor, Coord, simtime_t, vector, FitnessParameters, areaMaxX (+32 more)

### Community 7 - "experiment_metrics.py"
Cohesion: 0.24
Nodes (9): ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., parse_sca(), DataFrame, Return attributes, scalar rows and recorded parameters from one SCA., main(), Validate DroneStatus discovery and BA neighbor preservation. (+1 more)

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

### Community 17 - "test_experiment_metrics.py"
Cohesion: 0.46
Nodes (4): ExperimentMetricsReaderTests, Path, Contract tests for the normative, central scalar reader., write_sca()

### Community 18 - "string"
Cohesion: 0.07
Nodes (27): ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, Coord, simtime_t (+19 more)

### Community 19 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): Coord, map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts (+9 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.50
Nodes (3): cListener, cMessage, cSimpleModule

### Community 21 - "collect"
Cohesion: 0.11
Nodes (16): collect(), Return raw counters and auditable outcomes for one experimental run., main(), Validate sequential periodic alert cycles for one active victim., main(), Validate the complete, minimal BA control chain with central scalars., main(), Validate hop accounting from the IPv4 HopLimitInd received by TeamApp. (+8 more)

### Community 22 - "report_robustness.py"
Cohesion: 0.16
Nodes (17): ci95(), Student-t approximate 95% confidence-interval half-width for a mean., configured_teams(), main(), pair_runs(), DataFrame, Paired report for the one/two-victim robustness matrix., Describe complete executions, never individual packets. (+9 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (45): ExperimentMetrics, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal, completedRepositionAlertIds, confirmationDelayCount (+37 more)

### Community 26 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 27 - "central_scalar"
Cohesion: 0.67
Nodes (3): central_scalar(), DataFrame, Read exactly one scalar from the central collector. Missing or duplicate rows…

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.12
Nodes (29): b, Chunk, ChunkSerializer, DroneStatusChunk, MemoryInputStream, MemoryOutputStream, Ptr, simtime_t (+21 more)

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **203 isolated node(s):** `run.sh script`, `victimId`, `position`, `nextAlertTime`, `alertSequence` (+198 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **8 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `TeamApp`, `BatParameters`, `FitnessParameters`, `DroneApp.cc`, `socketDataArrived`, `LifecycleOperation`, `RepositionController`, `string`, `PendingVictimAlert`, `ExperimentMetrics.h`, `UdpSocket`?**
  _High betweenness centrality (0.277) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `ExperimentMetrics.cc`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.116) - this node is a cross-community bridge._
- **What connects `run.sh script`, `victimId`, `position` to the rest of the system?**
  _203 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.0425531914893617 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.0627177700348432 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._
- **Should `BatParameters` be split into smaller, more focused modules?**
  _Cohesion score 0.0766488413547237 - nodes in this community are weakly interconnected._