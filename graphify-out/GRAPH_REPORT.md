# Graph Report - sistema  (2026-08-26)

## Corpus Check
- 51 files · ~14,589 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 591 nodes · 865 edges · 44 communities (26 shown, 18 thin omitted)
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 32 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `ba1ac5b9`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- AlertMetricEvent
- Packet
- FitnessParameters
- Indication
- BaGaussMarkovMobility
- DroneApp.cc
- Simulações
- ExperimentMetrics.cc
- run.sh
- socketDataArrived
- LifecycleOperation
- RepositionController
- Repository Guidelines
- UdpSocket::ICallback
- DroneApp.h
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- report_main_experiment.py
- ExperimentMetrics
- Coord
- core/__init__.py
- SarScenarioManager
- ActiveVictim
- TeamLinkState
- analysis/__init__.py
- SarMessageSerializers.cc
- UdpSocket
- DroneLinkState
- reports/__init__.py
- analysis/README.md
- validation/__init__.py
- StaticVictim
- cObject
- cSimpleModule
- cObject
- cMessage
- write_manifest.py
- cSimpleModule
- string

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 89 edges
2. `ExperimentMetrics` - 53 edges
3. `TeamApp` - 33 edges
4. `collect()` - 29 edges
5. `FitnessParameters` - 22 edges
6. `RepositionFitness` - 18 edges
7. `PendingVictimAlert` - 17 edges
8. `BatParameters` - 12 edges
9. `BaGaussMarkovMobility` - 12 edges
10. `AbstractObstacleSensor` - 11 edges

## Surprising Connections (you probably didn't know these)
- `tryReposition` --references--> `RepositionFitness`  [INFERRED]
  src/app/DroneApp.h → src/optimization/RepositionFitness.h
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `DroneApp` --references--> `TeamLinkState`  [EXTRACTED]
  src/app/DroneApp.h → src/app/TeamLinkState.h
- `tryReposition` --references--> `BatAlgorithm`  [EXTRACTED]
  src/app/DroneApp.h → src/optimization/BatAlgorithm.h
- `load_arm()` --calls--> `collect()`  [EXTRACTED]
  analysis/reports/report_main_experiment.py → analysis/core/experiment_metrics.py

## Import Cycles
- None detected.

## Communities (44 total, 18 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.04
Nodes (56): BatParameters, DroneLinkState, FitnessParameters, RepositionController, DroneApp, ackTimeout, activeVictims, alertAttemptSentSignal (+48 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (35): ApplicationBase, LifecycleOperation, set, cMessage, Packet, UdpSocket, ApplicationBase, cMessage (+27 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (25): cMessage, cObject, IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment (+17 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (31): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+23 more)

### Community 4 - "AlertMetricEvent"
Cohesion: 0.20
Nodes (9): AlertMetricEvent, alertId, category, messageId, referenceTime, value, cObject, simtime_t (+1 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (40): AbstractObstacleSensor, AbstractObstacleSensor, Coord, simtime_t, vector, FitnessParameters, areaMaxX, areaMaxY (+32 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (13): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, initialize (+5 more)

### Community 9 - "DroneApp.cc"
Cohesion: 0.18
Nodes (20): ActiveVictim, cMessage, PendingVictimAlert, completeAlertCycle, expireDiscoveredEntries, finish, handleAssignment, handleMessageWhenUp (+12 more)

### Community 10 - "Simulações"
Cohesion: 0.13
Nodes (13): Documentação técnica e científica, Fonte única dos parâmetros, Material complementar, ECHOSAR-Net — reposicionamento de UAV com Bat Algorithm, Limitações do modelo, Organização, Ambiente físico, Experimento principal (+5 more)

### Community 11 - "ExperimentMetrics.cc"
Cohesion: 0.25
Nodes (7): cComponent, cMessage, simsignal_t, finish, handleMessage, initialize, receiveSignal

### Community 13 - "socketDataArrived"
Cohesion: 0.31
Nodes (9): Packet, string, TeamUpdateChunk, UdpSocket, handleDroneStatus, handleTeamUpdate, handleVictimAck, scheduleTeamUpdateRelay (+1 more)

### Community 15 - "RepositionController"
Cohesion: 0.31
Nodes (3): string, RepositionController, activeAlertId

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 18 - "DroneApp.h"
Cohesion: 0.36
Nodes (4): ApplicationBase, Coord, string, map

### Community 19 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): Coord, map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts (+9 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.50
Nodes (3): cListener, cSimpleModule, cMessage

### Community 21 - "collect"
Cohesion: 0.07
Nodes (35): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run. (+27 more)

### Community 22 - "report_main_experiment.py"
Cohesion: 0.10
Nodes (30): load_arm(), main(), pair_runs(), parameter_differences(), DataFrame, Path, Analyze the minimal paired BA Off/On confirmatory experiment. The seed is the…, Pair arms by seed and calculate neutral treatment-minus-control effects. (+22 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (47): ExperimentMetrics, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal, completedRepositionAlertIds, confirmationDelayCount (+39 more)

### Community 26 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 27 - "ActiveVictim"
Cohesion: 0.29
Nodes (7): ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, simtime_t

### Community 28 - "TeamLinkState"
Cohesion: 0.20
Nodes (10): Coord, string, simtime_t, string, TeamLinkState, ipAddress, lastSequence, lastUpdateTime (+2 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.12
Nodes (29): b, Chunk, ChunkSerializer, DroneStatusChunk, MemoryInputStream, MemoryOutputStream, Ptr, simtime_t (+21 more)

### Community 32 - "DroneLinkState"
Cohesion: 0.33
Nodes (6): DroneLinkState, lastSequence, lastUpdateTime, position, Coord, simtime_t

### Community 36 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **211 isolated node(s):** `socket`, `maintenanceTimer`, `movementCompleteTimer`, `droneStatusTimer`, `droneId` (+206 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **18 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `TeamApp`, `DroneApp.cc`, `socketDataArrived`, `LifecycleOperation`, `DroneApp.h`, `ExperimentMetrics.h`, `TeamLinkState`, `UdpSocket`?**
  _High betweenness centrality (0.237) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `ExperimentMetrics.cc`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.114) - this node is a cross-community bridge._
- **Why does `tryReposition` connect `DroneApp.cc` to `DroneApp`, `BatParameters`, `FitnessParameters`?**
  _High betweenness centrality (0.090) - this node is a cross-community bridge._
- **What connects `socket`, `maintenanceTimer`, `movementCompleteTimer` to the rest of the system?**
  _211 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.03508771929824561 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.05813953488372093 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.09195402298850575 - nodes in this community are weakly interconnected._