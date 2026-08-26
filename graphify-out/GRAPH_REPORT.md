# Graph Report - sistema  (2026-08-26)

## Corpus Check
- 55 files · ~20,777 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 637 nodes · 927 edges · 37 communities (30 shown, 7 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 32 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `6604b657`
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
- reconstruct
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
- TeamApp.h
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- TeamLinkState
- ExperimentMetrics
- run_audit.sh
- core/__init__.py
- SarScenarioManager
- ActiveVictim
- DroneLinkState
- analysis/__init__.py
- SarMessageSerializers.cc
- StaticVictim
- reports/__init__.py
- Auditoria: atendimento e perda via `.elog`, comparado ao `.sca`
- validation/__init__.py
- AlertRecord
- write_manifest.py

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 89 edges
2. `ExperimentMetrics` - 55 edges
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

## Communities (37 total, 7 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.04
Nodes (51): DroneApp, ackTimeout, activeVictims, alertAttemptSentSignal, alertConfirmedSignal, alertExpiredSignal, alertGeneratedSignal, alertInterval (+43 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (33): cMessage, Packet, UdpSocket, ApplicationBase, cMessage, Indication, LifecycleOperation, set (+25 more)

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

### Community 7 - "reconstruct"
Cohesion: 0.20
Nodes (16): diff_run(), load_ground_truth(), main(), Path, blank_record(), build_module_index(), fields(), full_path() (+8 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (13): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, initialize (+5 more)

### Community 9 - "DroneApp.cc"
Cohesion: 0.19
Nodes (18): cMessage, completeAlertCycle, expireDiscoveredEntries, finish, handleAssignment, handleMessageWhenUp, handleVictimAck, initialize (+10 more)

### Community 10 - "Extensões"
Cohesion: 0.06
Nodes (32): Conformidade verificada, D1. Mobilidade das equipes — §3, D2. Dimensões dos obstáculos — §10 e §15, D3. Intervalo entre alertas periódicos — §7.3 e §13, D4. Alcance do sensor de obstáculos — §14, D5. Nomes de escalares e de parâmetros — §17 e §24, D6. Vetores não são gravados — §24, Desvios (+24 more)

### Community 11 - "ExperimentMetrics.cc"
Cohesion: 0.22
Nodes (9): cComponent, cMessage, cObject, simsignal_t, finish, handleMessage, initialize, receiveSignal (+1 more)

### Community 13 - "socketDataArrived"
Cohesion: 0.28
Nodes (9): Packet, string, TeamUpdateChunk, UdpSocket, handleDroneStatus, handleTeamUpdate, scheduleTeamUpdateRelay, socketDataArrived (+1 more)

### Community 15 - "RepositionController"
Cohesion: 0.36
Nodes (3): string, RepositionController, activeAlertId

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "DroneApp.h"
Cohesion: 0.42
Nodes (3): Coord, string, map

### Community 18 - "TeamApp.h"
Cohesion: 0.29
Nodes (4): ApplicationBase, Indication, set, UdpSocket

### Community 19 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): Coord, map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts (+9 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.50
Nodes (3): cListener, cMessage, cSimpleModule

### Community 21 - "collect"
Cohesion: 0.07
Nodes (35): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run. (+27 more)

### Community 22 - "TeamLinkState"
Cohesion: 0.22
Nodes (9): Coord, simtime_t, string, TeamLinkState, ipAddress, lastSequence, lastUpdateTime, position (+1 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.05
Nodes (44): ExperimentMetrics, alertOrder, alertRecords, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal (+36 more)

### Community 26 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 27 - "ActiveVictim"
Cohesion: 0.29
Nodes (7): ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, simtime_t

### Community 28 - "DroneLinkState"
Cohesion: 0.33
Nodes (6): DroneLinkState, lastSequence, lastUpdateTime, position, Coord, simtime_t

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.12
Nodes (29): b, Chunk, ChunkSerializer, DroneStatusChunk, MemoryInputStream, MemoryOutputStream, Ptr, simtime_t (+21 more)

### Community 31 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 34 - "Auditoria: atendimento e perda via `.elog`, comparado ao `.sca`"
Cohesion: 0.14
Nodes (12): Auditoria: atendimento e perda via `.elog`, comparado ao `.sca`, Como o `alertId` chegou no `.elog` sem tocar no protocolo, Como reproduzir, Execuções auditadas, Leitura, Metodologia da reconstrução (`eventlog_metrics.py`), Quais logs foram usados, e por quê, Resultado (+4 more)

### Community 37 - "AlertRecord"
Cohesion: 0.17
Nodes (12): AlertRecord, acknowledged, ackTeamId, alertId, attempts, delivered, droneId, generationTime (+4 more)

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **246 isolated node(s):** `run_audit.sh script`, `run.sh script`, `victimId`, `position`, `nextAlertTime` (+241 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **7 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `BatParameters`, `FitnessParameters`, `DroneApp.cc`, `socketDataArrived`, `LifecycleOperation`, `RepositionController`, `DroneApp.h`, `TeamApp.h`, `PendingVictimAlert`, `ExperimentMetrics.h`, `TeamLinkState`, `ActiveVictim`, `DroneLinkState`?**
  _High betweenness centrality (0.228) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `BatParameters`, `ExperimentMetrics.cc`, `ExperimentMetrics.h`, `AlertRecord`?**
  _High betweenness centrality (0.101) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.h`?**
  _High betweenness centrality (0.066) - this node is a cross-community bridge._
- **What connects `run_audit.sh script`, `run.sh script`, `victimId` to the rest of the system?**
  _246 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.038461538461538464 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06341463414634146 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._