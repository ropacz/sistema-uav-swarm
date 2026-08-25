# Graph Report - sistema  (2026-08-25)

## Corpus Check
- 45 files · ~13,233 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 504 nodes · 720 edges · 23 communities (17 shown, 6 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 25 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `25642bbe`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- collect
- FitnessParameters
- BaGaussMarkovMobility
- Contratos e fórmulas das métricas
- run.sh
- SarScenarioManager
- PendingVictimAlert
- RepositionController
- Repository Guidelines
- string
- ExperimentMetrics
- core/__init__.py
- analysis/__init__.py
- SarMessageSerializers.cc
- reports/__init__.py
- analysis/README.md
- validation/__init__.py
- Rastreabilidade e validações
- write_manifest.py

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 61 edges
2. `ExperimentMetrics` - 40 edges
3. `TeamApp` - 32 edges
4. `PendingVictimAlert` - 22 edges
5. `FitnessParameters` - 22 edges
6. `collect()` - 15 edges
7. `BatParameters` - 13 edges
8. `RepositionFitness` - 13 edges
9. `BaGaussMarkovMobility` - 12 edges
10. `optimize` - 12 edges

## Surprising Connections (you probably didn't know these)
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_ba_smoke_test.py → analysis/core/experiment_metrics.py
- `DroneApp` --references--> `PendingVictimAlert`  [EXTRACTED]
  src/app/DroneApp.h → src/app/PendingVictimAlert.h
- `DroneApp` --references--> `RepositionController`  [EXTRACTED]
  src/app/DroneApp.h → src/app/RepositionController.h
- `DroneApp` --references--> `TeamLinkState`  [EXTRACTED]
  src/app/DroneApp.h → src/app/TeamLinkState.h

## Import Cycles
- None detected.

## Communities (23 total, 6 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.05
Nodes (51): cMessage, Packet, string, UdpSocket, DroneApp, ackTimeout, alertAttemptSentSignal, alertConfirmedSignal (+43 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (32): ApplicationBase, cMessage, Packet, UdpSocket, ApplicationBase, cMessage, Indication, LifecycleOperation (+24 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (31): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+23 more)

### Community 5 - "collect"
Cohesion: 0.08
Nodes (38): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run. (+30 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.08
Nodes (35): AbstractObstacleSensor, tryReposition, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY (+27 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (13): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, initialize (+5 more)

### Community 10 - "Contratos e fórmulas das métricas"
Cohesion: 0.05
Nodes (35): 1. Unidade de contagem, 2. Contadores centrais, 3. Desfecho primário, 4. Desfechos secundários, 5. Diagnóstico de exposição, 6. Invariantes fim a fim, 7. Agregação entre execuções, 8. Registro dos resultados (+27 more)

### Community 13 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 14 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts, attemptTeamAddresses (+9 more)

### Community 15 - "RepositionController"
Cohesion: 0.33
Nodes (3): string, RepositionController, activeAlertId

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 18 - "string"
Cohesion: 0.07
Nodes (24): Indication, map, string, UdpSocket, Coord, Coord, simtime_t, string (+16 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.05
Nodes (45): cComponent, cListener, cMessage, cMessage, cObject, simsignal_t, ExperimentMetrics, alertAttemptsSent (+37 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.12
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

### Community 37 - "Rastreabilidade e validações"
Cohesion: 0.18
Nodes (11): 1. Matriz funcional, 2. Contrato dos sinais, 3. Níveis de validação, 4. Artefatos e proveniência, 5. Checklist de publicação, 6. Limitações conhecidas, Build C++/NED, Estática e unitária (+3 more)

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **195 isolated node(s):** `run.sh script`, `socket`, `maintenanceTimer`, `movementCompleteTimer`, `droneId` (+190 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **6 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `TeamApp`, `BatParameters`, `FitnessParameters`, `PendingVictimAlert`, `RepositionController`, `string`, `ExperimentMetrics`?**
  _High betweenness centrality (0.272) - this node is a cross-community bridge._
- **What connects `run.sh script`, `socket`, `maintenanceTimer` to the rest of the system?**
  _195 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.05451127819548872 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06463414634146342 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._
- **Should `BatParameters` be split into smaller, more focused modules?**
  _Cohesion score 0.07954545454545454 - nodes in this community are weakly interconnected._
- **Should `collect` be split into smaller, more focused modules?**
  _Cohesion score 0.07591836734693877 - nodes in this community are weakly interconnected._