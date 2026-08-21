# Graph Report - sistema  (2026-08-21)

## Corpus Check
- 34 files · ~16,911 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 530 nodes · 771 edges · 27 communities (18 shown, 9 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 26 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `de1d7afc`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- PendingVictimAlert
- DroneApp.cc
- FitnessParameters
- LifecycleOperation
- BaGaussMarkovMobility
- network_metrics.py
- Experimento piloto do Bat Algorithm
- UdpSocket
- run.sh
- SarScenarioManager
- pcap_batch_to_spreadsheet.py
- RepositionController
- Repository Guidelines
- validateParameters
- Indication
- simsignal_t
- UdpSocket::ICallback
- pcap_core.py
- TeamApp.cc
- UdpSocket
- LifecycleOperation
- SarMessageSerializers.cc
- analysis/README.md

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 101 edges
2. `TeamApp` - 43 edges
3. `PendingVictimAlert` - 30 edges
4. `FitnessParameters` - 20 edges
5. `TeamLinkState` - 16 edges
6. `RepositionFitness` - 14 edges
7. `Experimento piloto do Bat Algorithm` - 13 edges
8. `RepositionController` - 13 edges
9. `BatParameters` - 12 edges
10. `AbstractObstacleSensor` - 11 edges

## Surprising Connections (you probably didn't know these)
- `discover_captures()` --calls--> `load_capture()`  [INFERRED]
  analysis/pcap_batch_to_spreadsheet.py → analysis/pcap_core.py
- `compare_group()` --calls--> `is_group_destination()`  [INFERRED]
  analysis/pcap_batch_to_spreadsheet.py → analysis/pcap_core.py
- `compare_group()` --calls--> `packet_identity_method()`  [INFERRED]
  analysis/pcap_batch_to_spreadsheet.py → analysis/pcap_core.py
- `compare_group()` --calls--> `packet_key()`  [INFERRED]
  analysis/pcap_batch_to_spreadsheet.py → analysis/pcap_core.py
- `main()` --calls--> `format_workbook()`  [INFERRED]
  analysis/pcap_batch_to_spreadsheet.py → analysis/pcap_core.py

## Import Cycles
- None detected.

## Communities (27 total, 9 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (72): BatParameters, RepositionController, simsignal_t, DroneApp, ackTimeout, alertAttemptsSent, alertsExpired, alertTtl (+64 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (30): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+22 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): cObject, IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (31): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+23 more)

### Community 4 - "PendingVictimAlert"
Cohesion: 0.05
Nodes (43): ApplicationBase, deque, map, set, Coord, simtime_t, string, LinkSample (+35 more)

### Community 5 - "DroneApp.cc"
Cohesion: 0.18
Nodes (20): cMessage, Coord, Packet, string, UdpSocket, detectDegradation, estimateTeamPosition, finish (+12 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (35): AbstractObstacleSensor, AbstractObstacleSensor, Coord, FitnessParameters, simtime_t, FitnessParameters, areaMaxX, areaMaxY (+27 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (9): GaussMarkovMobility, BaGaussMarkovMobility, baOverride, holding, moveTo, resumeNormal, setTargetPosition, waypointId (+1 more)

### Community 9 - "network_metrics.py"
Cohesion: 0.14
Nodes (24): aggregate(), collect(), extreme_where(), main(), mean_where(), DataFrame, ratio(), Extract INET network-layer metrics per run and aggregate them across seeds.… (+16 more)

### Community 10 - "Experimento piloto do Bat Algorithm"
Cohesion: 0.06
Nodes (30): Contrato das métricas do piloto, Evidência e agregação, Métrica primária, Métricas do mecanismo, Métricas secundárias, Componentes, Estados do reposicionamento, Modelo e premissas do piloto (+22 more)

### Community 13 - "SarScenarioManager"
Cohesion: 0.13
Nodes (14): cModule, ApplicationBase, LifecycleOperation, map, set, cMessage, cMessage, cSimpleModule (+6 more)

### Community 14 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.14
Nodes (22): Capture, compare_group(), discover_captures(), main(), parse_capture_name(), DataFrame, Path, Compara todas as transmissões de uma configuração/execução. (+14 more)

### Community 15 - "RepositionController"
Cohesion: 0.22
Nodes (5): string, RepositionController, activeAlertId, state, State

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "validateParameters"
Cohesion: 0.67
Nodes (3): initialize, validateParameters, require()

### Community 21 - "pcap_core.py"
Cohesion: 0.09
Nodes (33): compare_direction(), decode_echosar_payload(), find_ipv4_udp(), format_workbook(), is_group_destination(), iter_pcapng_packets(), load_capture(), packet_identity_method() (+25 more)

### Community 26 - "TeamApp.cc"
Cohesion: 0.20
Nodes (9): cMessage, Packet, UdpSocket, TeamApp::finish(), TeamApp::handleMessageWhenUp(), TeamApp::handleVictimAlert(), TeamApp::initialize(), TeamApp::sendPositionUpdate() (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

## Knowledge Gaps
- **211 isolated node(s):** `Project Structure & Module Organization`, `Build, Test, and Development Commands`, `Coding Style & Naming Conventions`, `Testing Guidelines`, `Commit & Pull Request Guidelines` (+206 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **9 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `PendingVictimAlert`, `DroneApp.cc`, `LifecycleOperation`, `UdpSocket`, `validateParameters`?**
  _High betweenness centrality (0.232) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.cc`, `UdpSocket`, `LifecycleOperation`, `SarScenarioManager`?**
  _High betweenness centrality (0.121) - this node is a cross-community bridge._
- **Why does `PendingVictimAlert` connect `PendingVictimAlert` to `DroneApp`, `DroneApp.cc`?**
  _High betweenness centrality (0.082) - this node is a cross-community bridge._
- **What connects `Project Structure & Module Organization`, `Build, Test, and Development Commands`, `Coding Style & Naming Conventions` to the rest of the system?**
  _211 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.0273972602739726 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06451612903225806 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._