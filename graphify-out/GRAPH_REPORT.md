# Graph Report - sistema  (2026-08-26)

## Corpus Check
- 51 files · ~17,846 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 607 nodes · 880 edges · 34 communities (26 shown, 8 thin omitted)
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 32 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `ed89c422`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- AlertMetricEvent
- alert_sheet.py
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
- TeamApp.cc
- UdpSocket
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- LifecycleOperation
- ExperimentMetrics
- core/__init__.py
- DroneApp.h
- TeamLinkState
- analysis/__init__.py
- SarMessageSerializers.cc
- reports/__init__.py
- Análise
- validation/__init__.py
- AlertRecord
- write_manifest.py

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 89 edges
2. `ExperimentMetrics` - 58 edges
3. `TeamApp` - 33 edges
4. `collect()` - 25 edges
5. `FitnessParameters` - 23 edges
6. `PendingVictimAlert` - 22 edges
7. `RepositionFitness` - 17 edges
8. `AlertMetricEvent` - 13 edges
9. `AlertRecord` - 13 edges
10. `BatParameters` - 13 edges

## Surprising Connections (you probably didn't know these)
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_alert_lifecycle_smoke_test.py → analysis/core/experiment_metrics.py
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_ba_smoke_test.py → analysis/core/experiment_metrics.py
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_multihop_smoke_test.py → analysis/core/experiment_metrics.py
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_no_known_team_smoke_test.py → analysis/core/experiment_metrics.py

## Import Cycles
- None detected.

## Communities (34 total, 8 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.04
Nodes (51): DroneApp, ackTimeout, activeVictims, alertAttemptSentSignal, alertConfirmedSignal, alertExpiredSignal, alertGeneratedSignal, alertInterval (+43 more)

### Community 1 - "TeamApp"
Cohesion: 0.09
Nodes (21): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+13 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (32): cRNG, FeasibilityFunction, FitnessFunction, vector, Bat, amplitude, fitness, frequency (+24 more)

### Community 4 - "AlertMetricEvent"
Cohesion: 0.15
Nodes (12): AlertMetricEvent, alertId, category, droneId, messageId, referenceTime, teamId, value (+4 more)

### Community 5 - "alert_sheet.py"
Cohesion: 0.09
Nodes (23): load(), main(), DataFrame, Path, Lê seed, equipes e política do .sca da mesma execução., Uma linha por célula experimental, com as duas taxas pedidas., run_context(), summarize() (+15 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.06
Nodes (41): AbstractObstacleSensor, tryReposition, AbstractObstacleSensor, Coord, simtime_t, vector, FitnessParameters, areaMaxX (+33 more)

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

### Community 17 - "TeamApp.cc"
Cohesion: 0.27
Nodes (8): cMessage, Packet, UdpSocket, handleMessageWhenUp, handleVictimAlert, initialize, sendTeamUpdate, socketDataArrived

### Community 19 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): Coord, map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts (+9 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.50
Nodes (3): cListener, cMessage, cSimpleModule

### Community 21 - "collect"
Cohesion: 0.07
Nodes (35): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run. (+27 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (47): ExperimentMetrics, alertOrder, alertRecords, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal (+39 more)

### Community 26 - "DroneApp.h"
Cohesion: 0.06
Nodes (33): cModule, ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, Coord (+25 more)

### Community 28 - "TeamLinkState"
Cohesion: 0.22
Nodes (9): Coord, simtime_t, string, TeamLinkState, ipAddress, lastSequence, lastUpdateTime, position (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.12
Nodes (29): b, Chunk, ChunkSerializer, DroneStatusChunk, MemoryInputStream, MemoryOutputStream, Ptr, simtime_t (+21 more)

### Community 34 - "Análise"
Cohesion: 0.50
Nodes (3): Análise, Como gerar, Planilha de atendimento

### Community 37 - "AlertRecord"
Cohesion: 0.17
Nodes (12): AlertRecord, acknowledged, ackTeamId, alertId, attempts, delivered, droneId, generationTime (+4 more)

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **241 isolated node(s):** `run.sh script`, `victimId`, `position`, `nextAlertTime`, `alertSequence` (+236 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **8 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `BatParameters`, `FitnessParameters`, `validateParameters`, `DroneApp.cc`, `socketDataArrived`, `LifecycleOperation`, `RepositionController`, `PendingVictimAlert`, `ExperimentMetrics.h`, `DroneApp.h`, `TeamLinkState`?**
  _High betweenness centrality (0.252) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `BatParameters`, `ExperimentMetrics.cc`, `ExperimentMetrics.h`, `AlertRecord`?**
  _High betweenness centrality (0.118) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.cc`, `DroneApp.h`, `UdpSocket`, `LifecycleOperation`?**
  _High betweenness centrality (0.072) - this node is a cross-community bridge._
- **What connects `run.sh script`, `victimId`, `position` to the rest of the system?**
  _241 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.038461538461538464 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.09090909090909091 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._