# Graph Report - sistema  (2026-08-21)

## Corpus Check
- 32 files · ~15,229 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 515 nodes · 779 edges · 31 communities (21 shown, 10 thin omitted)
- Extraction: 95% EXTRACTED · 5% INFERRED · 0% AMBIGUOUS · INFERRED: 41 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `c3201b29`
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
- Cenários científicos da professora
- handleAssignment
- run.sh
- SarScenarioManager
- TeamLinkState
- RepositionController
- Repository Guidelines
- DroneApp.h
- StaticVictim
- pcap_batch_to_spreadsheet.py
- UdpSocket
- pcap_core.py
- DataFrame
- Path
- Coord
- cMessage
- TeamApp.cc
- UdpSocket
- LifecycleOperation
- SarMessageSerializers.cc
- analysis/README.md

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 101 edges
2. `TeamApp` - 43 edges
3. `PendingVictimAlert` - 30 edges
4. `FitnessParameters` - 22 edges
5. `BaGaussMarkovMobility` - 19 edges
6. `TeamLinkState` - 16 edges
7. `RepositionController` - 14 edges
8. `BatParameters` - 13 edges
9. `RepositionFitness` - 13 edges
10. `optimize` - 12 edges

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

## Communities (31 total, 10 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (69): DroneApp, ackTimeout, alertAttemptsSent, alertsExpired, alertTtl, applicationIpTtl, appPort, baActivations (+61 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (30): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+22 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): cObject, IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (32): cRNG, FeasibilityFunction, FitnessFunction, Coord, Bat, amplitude, fitness, frequency (+24 more)

### Community 4 - "PendingVictimAlert"
Cohesion: 0.10
Nodes (20): PendingVictimAlert, ackDeadline, alertId, attempts, attemptSentTimes, baCycles, degradationEvaluated, generationTime (+12 more)

### Community 5 - "DroneApp.cc"
Cohesion: 0.19
Nodes (18): cMessage, Packet, string, UdpSocket, detectDegradation, finish, handleMessageWhenUp, handlePositionUpdate (+10 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.08
Nodes (35): AbstractObstacleSensor, tryReposition, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY (+27 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.13
Nodes (18): Coord, GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev (+10 more)

### Community 9 - "network_metrics.py"
Cohesion: 0.12
Nodes (27): aggregate(), collect(), extreme_where(), main(), mean_where(), DataFrame, ratio(), Extract INET network-layer metrics per run and aggregate them across seeds.… (+19 more)

### Community 10 - "Cenários científicos da professora"
Cohesion: 0.14
Nodes (11): Cenários científicos da professora, Desenho solicitado, Execução, Gatilho e Bat Algorithm, Mobilidade e referência física, Métricas e validade, Pergunta e hipóteses, Rádio, obstáculo e FSPL (+3 more)

### Community 11 - "handleAssignment"
Cohesion: 0.50
Nodes (4): Coord, estimateTeamPosition, handleAssignment, VictimAssignment

### Community 13 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cMessage, cModule, cMessage, cSimpleModule, map, SarScenarioManager, detections, SarScenarioManager::handleMessage() (+2 more)

### Community 14 - "TeamLinkState"
Cohesion: 0.15
Nodes (14): simtime_t, LinkSample, receptionTime, rssiDbm, sequence, TeamLinkState, ipAddress, lastSeen (+6 more)

### Community 15 - "RepositionController"
Cohesion: 0.22
Nodes (5): string, RepositionController, activeAlertId, state, State

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "DroneApp.h"
Cohesion: 0.33
Nodes (5): deque, ApplicationBase, map, set, string

### Community 18 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 19 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.14
Nodes (22): Capture, compare_group(), discover_captures(), main(), parse_capture_name(), Compara todas as transmissões de uma configuração/execução., Calcula métricas por configuração, execução, tipo e enlace., Calcula métricas agregadas por configuração, execução e mensagem. (+14 more)

### Community 21 - "pcap_core.py"
Cohesion: 0.09
Nodes (33): compare_direction(), decode_echosar_payload(), find_ipv4_udp(), format_workbook(), is_group_destination(), iter_pcapng_packets(), load_capture(), packet_identity_method() (+25 more)

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

## Knowledge Gaps
- **199 isolated node(s):** `ECHOSAR-Net — cenários da professora`, `Análise`, `Documentação`, `Pergunta e hipóteses`, `Desenho solicitado` (+194 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **10 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `BatParameters`, `PendingVictimAlert`, `DroneApp.cc`, `FitnessParameters`, `LifecycleOperation`, `handleAssignment`, `TeamLinkState`, `RepositionController`, `DroneApp.h`, `UdpSocket`?**
  _High betweenness centrality (0.314) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `DroneApp.h`, `TeamApp.cc`, `UdpSocket`, `LifecycleOperation`?**
  _High betweenness centrality (0.109) - this node is a cross-community bridge._
- **Why does `FitnessParameters` connect `FitnessParameters` to `DroneApp`?**
  _High betweenness centrality (0.070) - this node is a cross-community bridge._
- **What connects `ECHOSAR-Net — cenários da professora`, `Análise`, `Documentação` to the rest of the system?**
  _199 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.02857142857142857 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06451612903225806 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._