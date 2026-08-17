# Graph Report - sistema  (2026-08-17)

## Corpus Check
- 33 files · ~18,977 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 409 nodes · 515 edges · 27 communities (19 shown, 8 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 16 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `eee78bf8`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- ObstacleObservation
- BatParameters
- PendingVictimAlert
- DroneApp.cc
- Arquitetura e validação básica do ECHOSAR-Net
- SarScenarioManager
- BaGaussMarkovMobility
- process_results.py
- validate_results.py
- StaticVictim
- run.sh
- run
- What You Must Do When Invoked
- graphify reference: extra exports and benchmark
- Repository Guidelines
- graphify reference: query, path, explain
- graphify reference: add a URL and watch a folder
- graphify reference: commit hook and native CLAUDE.md integration
- graphify reference: incremental update and cluster-only
- LifecycleOperation
- graphify reference: GitHub clone and cross-repo merge
- graphify reference: transcribe video and audio
- extraction-spec.md
- cMessage
- cSimpleModule

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 98 edges
2. `TeamApp` - 41 edges
3. `PendingVictimAlert` - 28 edges
4. `ObstacleObservation` - 13 edges
5. `TeamLinkState` - 13 edges
6. `What You Must Do When Invoked` - 12 edges
7. `Arquitetura e validação básica do ECHOSAR-Net` - 12 edges
8. `BatParameters` - 12 edges
9. `AbstractObstacleSensor` - 11 edges
10. `optimize` - 11 edges

## Surprising Connections (you probably didn't know these)
- `DroneApp` --references--> `BatParameters`  [EXTRACTED]
  src/app/DroneApp.h → src/optimization/BatAlgorithm.h
- `optimize` --calls--> `fitness`  [EXTRACTED]
  src/optimization/BatAlgorithm.h → src/optimization/BatAlgorithm.cc
- `tryReposition` --calls--> `optimize`  [EXTRACTED]
  src/app/DroneApp.h → src/optimization/BatAlgorithm.h
- `optimize` --calls--> `randomInSphere()`  [EXTRACTED]
  src/optimization/BatAlgorithm.h → src/optimization/BatAlgorithm.cc

## Import Cycles
- None detected.

## Communities (27 total, 8 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (69): RepositionState, DroneApp, ackTimeout, activeRepositionAlertId, alertAttemptsSent, alertsExpired, alertTtl, applicationIpTtl (+61 more)

### Community 1 - "TeamApp"
Cohesion: 0.05
Nodes (40): cMessage, Packet, UdpSocket, ApplicationBase, cMessage, Indication, LifecycleOperation, set (+32 more)

### Community 2 - "ObstacleObservation"
Cohesion: 0.07
Nodes (31): cObject, IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize (+23 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (32): cRNG, FeasibilityFunction, FitnessFunction, Coord, Bat, amplitude, fitness, frequency (+24 more)

### Community 4 - "PendingVictimAlert"
Cohesion: 0.06
Nodes (37): deque, ApplicationBase, Indication, map, set, simtime_t, string, UdpSocket (+29 more)

### Community 5 - "DroneApp.cc"
Cohesion: 0.17
Nodes (21): cMessage, Coord, Packet, string, UdpSocket, computeFitness, detectDegradation, finish (+13 more)

### Community 6 - "Arquitetura e validação básica do ECHOSAR-Net"
Cohesion: 0.07
Nodes (26): 10. Experimentos principais, 11. Limitações, 1. Finalidade, 2. Tecnologias e modelos, 3. Organização do projeto, 4.1 SarScenarioManager, 4.2 DroneApp, 4.3 TeamApp (+18 more)

### Community 7 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (9): GaussMarkovMobility, BaGaussMarkovMobility, baOverride, holding, moveTo, resumeNormal, setTargetPosition, waypointId (+1 more)

### Community 9 - "process_results.py"
Cohesion: 0.36
Nodes (11): aggregate(), ci95(), load_runs(), main(), paired(), parse_sca(), plot(), Aggregate ECHOSAR-Net OMNeT++ scalars and compare paired BA runs. Usage:… (+3 more)

### Community 10 - "validate_results.py"
Cohesion: 0.53
Nodes (5): main(), Fail-fast checks for the deterministic dissertation validation scenarios., require(), scalars(), total()

### Community 11 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 14 - "What You Must Do When Invoked"
Cohesion: 0.08
Nodes (24): For /graphify add and --watch, For /graphify query, For the commit hook and native CLAUDE.md integration, For --update and --cluster-only, /graphify, Honesty Rules, Interpreter guard for subcommands, Part A - Structural extraction for code files (+16 more)

### Community 15 - "graphify reference: extra exports and benchmark"
Cohesion: 0.22
Nodes (8): graphify reference: extra exports and benchmark, Step 6b - Wiki (only if --wiki flag), Step 7 - Neo4j export (only if --neo4j or --neo4j-push flag), Step 7a - FalkorDB export (only if --falkordb or --falkordb-push flag), Step 7b - SVG export (only if --svg flag), Step 7c - GraphML export (only if --graphml flag), Step 7d - MCP server (only if --mcp flag), Step 8 - Token reduction benchmark (only if total_words > 5000)

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "graphify reference: query, path, explain"
Cohesion: 0.33
Nodes (5): For /graphify explain, For /graphify path, graphify reference: query, path, explain, Step 0 — Constrained query expansion (REQUIRED before traversal), Step 1 — Traversal

### Community 18 - "graphify reference: add a URL and watch a folder"
Cohesion: 0.50
Nodes (3): For /graphify add, For --watch, graphify reference: add a URL and watch a folder

### Community 19 - "graphify reference: commit hook and native CLAUDE.md integration"
Cohesion: 0.50
Nodes (3): For git commit hook, For native CLAUDE.md integration, graphify reference: commit hook and native CLAUDE.md integration

### Community 20 - "graphify reference: incremental update and cluster-only"
Cohesion: 0.50
Nodes (3): For --cluster-only, For --update (incremental re-extraction), graphify reference: incremental update and cluster-only

## Knowledge Gaps
- **216 isolated node(s):** `Usage`, `What graphify is for`, `Step 0 - GitHub repos and multi-path merge (only if a URL or several paths)`, `Step 1 - Ensure graphify is installed`, `Step 2 - Detect files` (+211 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **8 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `LifecycleOperation`, `BatParameters`, `PendingVictimAlert`, `DroneApp.cc`?**
  _High betweenness centrality (0.274) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `PendingVictimAlert`?**
  _High betweenness centrality (0.121) - this node is a cross-community bridge._
- **Why does `PendingVictimAlert` connect `PendingVictimAlert` to `DroneApp`, `BatParameters`, `DroneApp.cc`?**
  _High betweenness centrality (0.071) - this node is a cross-community bridge._
- **What connects `Usage`, `What graphify is for`, `Step 0 - GitHub repos and multi-path merge (only if a URL or several paths)` to the rest of the system?**
  _216 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.02857142857142857 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.05230496453900709 - nodes in this community are weakly interconnected._
- **Should `ObstacleObservation` be split into smaller, more focused modules?**
  _Cohesion score 0.07142857142857142 - nodes in this community are weakly interconnected._