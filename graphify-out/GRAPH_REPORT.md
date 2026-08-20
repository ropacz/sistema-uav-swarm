# Graph Report - sistema  (2026-08-20)

## Corpus Check
- 39 files · ~28,182 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 563 nodes · 802 edges · 32 communities (26 shown, 6 thin omitted)
- Extraction: 98% EXTRACTED · 2% INFERRED · 0% AMBIGUOUS · INFERRED: 18 edges (avg confidence: 0.78)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `8a980df0`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- ObstacleObservation
- BatParameters
- PendingVictimAlert
- DroneApp.cc
- TeamLinkState
- Modelo e premissas
- BaGaussMarkovMobility
- process_results.py
- validate_results.py
- Protocolo científico do ECHOSAR-Net
- run.sh
- What You Must Do When Invoked
- graphify reference: extra exports and benchmark
- Repository Guidelines
- graphify reference: query, path, explain
- graphify reference: add a URL and watch a folder
- graphify reference: commit hook and native CLAUDE.md integration
- graphify reference: incremental update and cluster-only
- pcap_batch_to_spreadsheet.py
- graphify reference: GitHub clone and cross-repo merge
- graphify reference: transcribe video and audio
- extraction-spec.md
- SarScenarioManager
- TeamApp.cc
- UdpSocket
- LifecycleOperation
- SarMessageSerializers.cc
- DroneApp.h
- Ferramentas de análise
- StaticVictim

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 105 edges
2. `TeamApp` - 44 edges
3. `PendingVictimAlert` - 27 edges
4. `BatParameters` - 13 edges
5. `ObstacleObservation` - 13 edges
6. `TeamLinkState` - 12 edges
7. `What You Must Do When Invoked` - 12 edges
8. `optimize` - 11 edges
9. `AbstractObstacleSensor` - 11 edges
10. `Protocolo científico do ECHOSAR-Net` - 11 edges

## Surprising Connections (you probably didn't know these)
- `MultiseedStatisticsTests` --uses--> `Capture`  [INFERRED]
  analysis/tests/test_pcap_multiseed.py → analysis/pcap_batch_to_spreadsheet.py
- `decode_echosar_payload()` --calls--> `text()`  [INFERRED]
  analysis/pcap_core.py → analysis/tests/test_pcap_analysis.py
- `DroneApp` --references--> `BatParameters`  [EXTRACTED]
  src/app/DroneApp.h → src/optimization/BatAlgorithm.h
- `tryReposition` --calls--> `optimize`  [EXTRACTED]
  src/app/DroneApp.h → src/optimization/BatAlgorithm.h
- `optimize` --calls--> `fitness`  [EXTRACTED]
  src/optimization/BatAlgorithm.h → src/optimization/BatAlgorithm.cc

## Import Cycles
- None detected.

## Communities (32 total, 6 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (76): RepositionState, DroneApp, ackTimeout, activeRepositionAlertId, alertAttemptsSent, alertsExpired, alertTtl, applicationIpTtl (+68 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (31): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+23 more)

### Community 2 - "ObstacleObservation"
Cohesion: 0.07
Nodes (31): cObject, IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize (+23 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (32): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+24 more)

### Community 4 - "PendingVictimAlert"
Cohesion: 0.11
Nodes (18): PendingVictimAlert, ackDeadline, alertId, attempts, attemptSentTimes, baCycles, degradationEvaluated, generationTime (+10 more)

### Community 5 - "DroneApp.cc"
Cohesion: 0.17
Nodes (21): cMessage, Coord, Packet, string, UdpSocket, computeFitness, detectDegradation, finish (+13 more)

### Community 6 - "TeamLinkState"
Cohesion: 0.18
Nodes (12): deque, Coord, simtime_t, LinkSample, receptionTime, rssiDbm, sequence, TeamLinkState (+4 more)

### Community 7 - "Modelo e premissas"
Cohesion: 0.05
Nodes (35): 1. Métrica primária, 2. Métricas secundárias, 3. Métricas diagnósticas, 4. Métricas derivadas de capturas de rede, 5. Fontes de evidência, 6. Saídas da análise, 7. Proveniência, AppACK (+27 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (9): GaussMarkovMobility, BaGaussMarkovMobility, baOverride, holding, moveTo, resumeNormal, setTargetPosition, waypointId (+1 more)

### Community 9 - "process_results.py"
Cohesion: 0.13
Nodes (29): aggregate(), ci95(), data_quality(), file_sha256(), IntegrityError, load_runs(), main(), paired() (+21 more)

### Community 10 - "validate_results.py"
Cohesion: 0.57
Nodes (6): main(), Fail-fast checks for the deterministic dissertation validation scenarios., require(), require_failure_decomposition(), scalars(), total()

### Community 11 - "Protocolo científico do ECHOSAR-Net"
Cohesion: 0.17
Nodes (12): 10. Limites de validade, 1. Pergunta, 2. Hipóteses, 3. Unidade experimental, 4. Variável independente, 5. Fatores controlados, 6. Desenho, 7. Separação entre verificação e evidência (+4 more)

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

### Community 21 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.06
Nodes (55): Capture, compare_group(), discover_captures(), main(), parse_capture_name(), DataFrame, Path, Compara todas as transmissões de uma configuração/execução. (+47 more)

### Community 25 - "SarScenarioManager"
Cohesion: 0.18
Nodes (11): cModule, map, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections (+3 more)

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

### Community 32 - "DroneApp.h"
Cohesion: 0.21
Nodes (6): ApplicationBase, Indication, LifecycleOperation, set, string, UdpSocket

### Community 34 - "Ferramentas de análise"
Cohesion: 0.50
Nodes (3): Ferramentas de análise, Localização dos artefatos, Portões de integridade

### Community 37 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

## Knowledge Gaps
- **245 isolated node(s):** `run.sh script`, `sequence`, `receptionTime`, `rssiDbm`, `ipAddress` (+240 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **6 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `DroneApp.h`, `BatParameters`, `PendingVictimAlert`, `DroneApp.cc`, `TeamLinkState`, `SarScenarioManager`?**
  _High betweenness centrality (0.191) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `DroneApp.h`, `TeamApp.cc`, `UdpSocket`, `LifecycleOperation`?**
  _High betweenness centrality (0.081) - this node is a cross-community bridge._
- **Why does `PendingVictimAlert` connect `PendingVictimAlert` to `DroneApp`, `DroneApp.h`, `DroneApp.cc`, `TeamLinkState`, `SarScenarioManager`?**
  _High betweenness centrality (0.044) - this node is a cross-community bridge._
- **What connects `run.sh script`, `sequence`, `receptionTime` to the rest of the system?**
  _245 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.025974025974025976 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.0625 - nodes in this community are weakly interconnected._
- **Should `ObstacleObservation` be split into smaller, more focused modules?**
  _Cohesion score 0.07142857142857142 - nodes in this community are weakly interconnected._