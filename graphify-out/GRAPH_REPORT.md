# Graph Report - sistema  (2026-08-26)

## Corpus Check
- 56 files · ~21,601 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 682 nodes · 1043 edges · 39 communities (33 shown, 6 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 32 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `28967447`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- AlertMetricEvent
- figures.py
- FitnessParameters
- validateParameters
- BaGaussMarkovMobility
- DroneApp.cc
- Extensões
- ExperimentMetrics.cc
- run.sh
- socketDataArrived
- LifecycleOperation
- RepositionController
- Repository Guidelines
- DroneApp.h
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- report_main_experiment.py
- ExperimentMetrics
- core/__init__.py
- SarScenarioManager
- ActiveVictim
- TeamLinkState
- analysis/__init__.py
- SarMessageSerializers.cc
- TeamApp.h
- DroneLinkState
- reports/__init__.py
- Análise
- validation/__init__.py
- StaticVictim
- AlertRecord
- write_manifest.py
- FigureTests
- verification_sheet.py

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 89 edges
2. `ExperimentMetrics` - 58 edges
3. `TeamApp` - 33 edges
4. `collect()` - 29 edges
5. `FitnessParameters` - 23 edges
6. `PendingVictimAlert` - 22 edges
7. `RepositionFitness` - 17 edges
8. `parse_sca()` - 16 edges
9. `AlertMetricEvent` - 13 edges
10. `AlertRecord` - 13 edges

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

## Communities (39 total, 6 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.04
Nodes (51): DroneApp, ackTimeout, activeVictims, alertAttemptSentSignal, alertConfirmedSignal, alertExpiredSignal, alertGeneratedSignal, alertInterval (+43 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (32): cMessage, Packet, UdpSocket, ApplicationBase, cMessage, Indication, LifecycleOperation, set (+24 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (31): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+23 more)

### Community 4 - "AlertMetricEvent"
Cohesion: 0.15
Nodes (12): AlertMetricEvent, alertId, category, droneId, messageId, referenceTime, teamId, value (+4 more)

### Community 5 - "figures.py"
Cohesion: 0.24
Nodes (17): main(), attendance_figures(), configure_style(), exposure_funnel_figure(), main(), paired_effect_figure(), DataFrame, Path (+9 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.06
Nodes (42): AbstractObstacleSensor, tryReposition, vector, AbstractObstacleSensor, Coord, simtime_t, vector, FitnessParameters (+34 more)

### Community 7 - "validateParameters"
Cohesion: 0.67
Nodes (3): initialize, validateParameters, require()

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (13): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, initialize (+5 more)

### Community 9 - "DroneApp.cc"
Cohesion: 0.24
Nodes (15): cMessage, completeAlertCycle, expireDiscoveredEntries, finish, handleAssignment, handleMessageWhenUp, handleVictimAck, performMaintenance (+7 more)

### Community 10 - "Extensões"
Cohesion: 0.06
Nodes (32): Conformidade verificada, D1. Mobilidade das equipes — §3, D2. Dimensões dos obstáculos — §10 e §15, D3. Intervalo entre alertas periódicos — §7.3 e §13, D4. Alcance do sensor de obstáculos — §14, D5. Nomes de escalares e de parâmetros — §17 e §24, D6. Vetores não são gravados — §24, Desvios (+24 more)

### Community 11 - "ExperimentMetrics.cc"
Cohesion: 0.22
Nodes (9): cComponent, cMessage, cObject, simsignal_t, finish, handleMessage, initialize, receiveSignal (+1 more)

### Community 13 - "socketDataArrived"
Cohesion: 0.32
Nodes (8): Packet, string, TeamUpdateChunk, UdpSocket, handleDroneStatus, handleTeamUpdate, scheduleTeamUpdateRelay, socketDataArrived

### Community 15 - "RepositionController"
Cohesion: 0.36
Nodes (3): string, RepositionController, activeAlertId

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 18 - "DroneApp.h"
Cohesion: 0.42
Nodes (3): Coord, string, map

### Community 19 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): Coord, map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts (+9 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.50
Nodes (3): cListener, cMessage, cSimpleModule

### Community 21 - "collect"
Cohesion: 0.08
Nodes (29): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run. (+21 more)

### Community 22 - "report_main_experiment.py"
Cohesion: 0.07
Nodes (43): ci95(), parse_sca(), DataFrame, Return attributes, scalar rows and recorded parameters from one SCA., Student-t approximate 95% confidence-interval half-width for a mean., load(), DataFrame, Path (+35 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (47): ExperimentMetrics, alertOrder, alertRecords, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal (+39 more)

### Community 26 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 27 - "ActiveVictim"
Cohesion: 0.29
Nodes (7): ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, simtime_t

### Community 28 - "TeamLinkState"
Cohesion: 0.22
Nodes (9): Coord, simtime_t, string, TeamLinkState, ipAddress, lastSequence, lastUpdateTime, position (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.12
Nodes (29): b, Chunk, ChunkSerializer, DroneStatusChunk, MemoryInputStream, MemoryOutputStream, Ptr, simtime_t (+21 more)

### Community 31 - "TeamApp.h"
Cohesion: 0.29
Nodes (4): ApplicationBase, Indication, set, UdpSocket

### Community 32 - "DroneLinkState"
Cohesion: 0.33
Nodes (6): DroneLinkState, lastSequence, lastUpdateTime, position, Coord, simtime_t

### Community 34 - "Análise"
Cohesion: 0.50
Nodes (3): Análise, Figuras e planilha, Planilha de atendimento

### Community 36 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 37 - "AlertRecord"
Cohesion: 0.17
Nodes (12): AlertRecord, acknowledged, ackTeamId, alertId, attempts, delivered, droneId, generationTime (+4 more)

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

### Community 45 - "FigureTests"
Cohesion: 0.10
Nodes (14): AlertSheetTests, exposure_frames(), FigureTests, main_summary(), DataFrame, Path, Contrato entre relatórios e figuras, verificado nos dois sentidos., As duas taxas contam alertId únicos, não tentativas nem recebimentos. (+6 more)

### Community 46 - "verification_sheet.py"
Cohesion: 0.24
Nodes (12): build(), classify(), clean(), lookup(), main(), numbers(), DataFrame, Path (+4 more)

## Knowledge Gaps
- **241 isolated node(s):** `run.sh script`, `victimId`, `position`, `nextAlertTime`, `alertSequence` (+236 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **6 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `DroneLinkState`, `BatParameters`, `FitnessParameters`, `validateParameters`, `DroneApp.cc`, `socketDataArrived`, `LifecycleOperation`, `RepositionController`, `DroneApp.h`, `PendingVictimAlert`, `ExperimentMetrics.h`, `ActiveVictim`, `TeamLinkState`, `TeamApp.h`?**
  _High betweenness centrality (0.199) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `ExperimentMetrics.cc`, `ExperimentMetrics.h`, `AlertRecord`, `FitnessParameters`?**
  _High betweenness centrality (0.093) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.h`?**
  _High betweenness centrality (0.057) - this node is a cross-community bridge._
- **What connects `run.sh script`, `victimId`, `position` to the rest of the system?**
  _241 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.038461538461538464 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.0641025641025641 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._