# Graph Report - sistema  (2026-08-17)

## Corpus Check
- 32 files · ~20,116 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 430 nodes · 541 edges · 28 communities (21 shown, 7 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 16 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `2fe2b821`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- ObstacleObservation
- BatParameters
- PendingVictimAlert
- DroneApp.cc
- Guia de implementação e validação do ECHOSAR-Net
- ECHOSAR-Net
- BaGaussMarkovMobility
- process_results.py
- validate_results.py
- StaticVictim
- run.sh
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
- SarScenarioManager
- TeamApp.cc
- UdpSocket
- LifecycleOperation

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 100 edges
2. `TeamApp` - 44 edges
3. `PendingVictimAlert` - 27 edges
4. `BatParameters` - 13 edges
5. `ObstacleObservation` - 13 edges
6. `Guia de implementação e validação do ECHOSAR-Net` - 12 edges
7. `TeamLinkState` - 12 edges
8. `What You Must Do When Invoked` - 12 edges
9. `optimize` - 11 edges
10. `AbstractObstacleSensor` - 11 edges

## Surprising Connections (you probably didn't know these)
- `DroneApp` --references--> `BatParameters`  [EXTRACTED]
  src/app/DroneApp.h → src/optimization/BatAlgorithm.h
- `tryReposition` --calls--> `optimize`  [EXTRACTED]
  src/app/DroneApp.h → src/optimization/BatAlgorithm.h
- `optimize` --calls--> `fitness`  [EXTRACTED]
  src/optimization/BatAlgorithm.h → src/optimization/BatAlgorithm.cc
- `optimize` --calls--> `randomInSphere()`  [EXTRACTED]
  src/optimization/BatAlgorithm.h → src/optimization/BatAlgorithm.cc

## Import Cycles
- None detected.

## Communities (28 total, 7 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (71): RepositionState, DroneApp, ackTimeout, activeRepositionAlertId, alertAttemptsSent, alertsExpired, alertTtl, applicationIpTtl (+63 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (31): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+23 more)

### Community 2 - "ObstacleObservation"
Cohesion: 0.07
Nodes (31): cObject, IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize (+23 more)

### Community 3 - "BatParameters"
Cohesion: 0.07
Nodes (33): cRNG, FeasibilityFunction, FitnessFunction, Coord, Bat, amplitude, fitness, frequency (+25 more)

### Community 4 - "PendingVictimAlert"
Cohesion: 0.06
Nodes (36): deque, map, ApplicationBase, Indication, map, set, simtime_t, string (+28 more)

### Community 5 - "DroneApp.cc"
Cohesion: 0.17
Nodes (21): cMessage, Coord, Packet, string, UdpSocket, computeFitness, detectDegradation, finish (+13 more)

### Community 6 - "Guia de implementação e validação do ECHOSAR-Net"
Cohesion: 0.11
Nodes (19): 10. Experimentos principais, 11. Limitações, 1. Finalidade, 2. Tecnologias e modelos, 3. Organização do projeto, 4.1 SarScenarioManager, 4.2 DroneApp, 4.3 TeamApp (+11 more)

### Community 7 - "ECHOSAR-Net"
Cohesion: 0.07
Nodes (22): Convenções experimentais, ECHOSAR-Net — arquitetura experimental, Fluxo, Limitações, Parâmetros iniciais, Controle experimental, Engenharia científica, Engenharia de software (+14 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (9): GaussMarkovMobility, BaGaussMarkovMobility, baOverride, holding, moveTo, resumeNormal, setTargetPosition, waypointId (+1 more)

### Community 9 - "process_results.py"
Cohesion: 0.29
Nodes (14): aggregate(), ci95(), file_sha256(), load_runs(), main(), paired(), parse_sca(), plot() (+6 more)

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

### Community 25 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

## Knowledge Gaps
- **232 isolated node(s):** `Arquitetura`, `Estrutura`, `Compilação`, `Validações determinísticas`, `Experimentos` (+227 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **7 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `LifecycleOperation`, `BatParameters`, `PendingVictimAlert`, `DroneApp.cc`?**
  _High betweenness centrality (0.266) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.cc`, `UdpSocket`, `PendingVictimAlert`, `LifecycleOperation`?**
  _High betweenness centrality (0.121) - this node is a cross-community bridge._
- **Why does `PendingVictimAlert` connect `PendingVictimAlert` to `DroneApp`, `BatParameters`, `DroneApp.cc`?**
  _High betweenness centrality (0.065) - this node is a cross-community bridge._
- **What connects `Arquitetura`, `Estrutura`, `Compilação` to the rest of the system?**
  _232 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.027777777777777776 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.0625 - nodes in this community are weakly interconnected._
- **Should `ObstacleObservation` be split into smaller, more focused modules?**
  _Cohesion score 0.07142857142857142 - nodes in this community are weakly interconnected._