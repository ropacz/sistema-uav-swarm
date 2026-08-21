# Graph Report - sistema  (2026-08-20)

## Corpus Check
- 33 files · ~20,606 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 550 nodes · 833 edges · 26 communities (22 shown, 4 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 22 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `2e7cab66`
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
- Contrato das métricas
- BaGaussMarkovMobility
- process_results.py
- validate_results.py
- Protocolo científico do ECHOSAR-Net
- run.sh
- SarScenarioManager
- LifecycleOperation
- RepositionController
- Repository Guidelines
- StaticVictim
- DroneApp.h
- pcap_batch_to_spreadsheet.py
- TeamApp.cc
- UdpSocket
- LifecycleOperation
- TeamLinkState
- SarMessageSerializers.cc
- Ferramentas de análise

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 93 edges
2. `TeamApp` - 43 edges
3. `PendingVictimAlert` - 27 edges
4. `FitnessParameters` - 22 edges
5. `RepositionController` - 14 edges
6. `BatParameters` - 13 edges
7. `RepositionFitness` - 13 edges
8. `optimize` - 12 edges
9. `TeamLinkState` - 11 edges
10. `AbstractObstacleSensor` - 11 edges

## Surprising Connections (you probably didn't know these)
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `MultiseedStatisticsTests` --uses--> `Capture`  [INFERRED]
  analysis/tests/test_pcap_multiseed.py → analysis/pcap_batch_to_spreadsheet.py
- `decode_echosar_payload()` --calls--> `text()`  [INFERRED]
  analysis/pcap_core.py → analysis/tests/test_pcap_analysis.py
- `DroneApp` --references--> `RepositionController`  [EXTRACTED]
  src/app/DroneApp.h → src/app/RepositionController.h
- `DroneApp` --references--> `BatParameters`  [EXTRACTED]
  src/app/DroneApp.h → src/optimization/BatAlgorithm.h

## Import Cycles
- None detected.

## Communities (26 total, 4 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (63): DroneApp, ackTimeout, alertAttemptsSent, alertsExpired, alertTtl, applicationIpTtl, appPort, baActivations (+55 more)

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
Cohesion: 0.11
Nodes (18): PendingVictimAlert, ackDeadline, alertId, attempts, attemptSentTimes, baCycles, degradationEvaluated, generationTime (+10 more)

### Community 5 - "DroneApp.cc"
Cohesion: 0.17
Nodes (19): cMessage, Packet, string, UdpSocket, detectDegradation, finish, handleAssignment, handleMessageWhenUp (+11 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.08
Nodes (35): AbstractObstacleSensor, tryReposition, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY (+27 more)

### Community 7 - "Contrato das métricas"
Cohesion: 0.05
Nodes (36): 1. Métrica primária, 2. Métricas secundárias, 3. Métricas diagnósticas, 4. Métricas de rede a partir dos escalares do INET, 5. Métricas derivadas de capturas de rede, 6. Fontes de evidência, 7. Saídas da análise, 8. Proveniência (+28 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (9): GaussMarkovMobility, BaGaussMarkovMobility, baOverride, holding, moveTo, resumeNormal, setTargetPosition, waypointId (+1 more)

### Community 9 - "process_results.py"
Cohesion: 0.10
Nodes (39): aggregate(), collect(), extreme_where(), main(), mean_where(), DataFrame, ratio(), Extract INET network-layer metrics per run and aggregate them across seeds.… (+31 more)

### Community 10 - "validate_results.py"
Cohesion: 0.57
Nodes (6): main(), Fail-fast checks for the deterministic dissertation validation scenarios., require(), require_failure_decomposition(), scalars(), total()

### Community 11 - "Protocolo científico do ECHOSAR-Net"
Cohesion: 0.11
Nodes (18): 10. Limites de validade, 1. Pergunta, 2. Hipóteses, 3. Unidade experimental, 4. Variável independente, 5. Fatores controlados, 6. Desenho, 7. Separação entre verificação e evidência (+10 more)

### Community 13 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 15 - "RepositionController"
Cohesion: 0.22
Nodes (5): string, RepositionController, activeAlertId, state, State

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 18 - "DroneApp.h"
Cohesion: 0.24
Nodes (6): ApplicationBase, Indication, map, set, string, UdpSocket

### Community 21 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.06
Nodes (55): Capture, compare_group(), discover_captures(), main(), parse_capture_name(), DataFrame, Path, Compara todas as transmissões de uma configuração/execução. (+47 more)

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

### Community 29 - "TeamLinkState"
Cohesion: 0.20
Nodes (11): deque, simtime_t, LinkSample, receptionTime, rssiDbm, sequence, TeamLinkState, ipAddress (+3 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

### Community 34 - "Ferramentas de análise"
Cohesion: 0.50
Nodes (3): Ferramentas de análise, Localização dos artefatos, Portões de integridade

## Knowledge Gaps
- **216 isolated node(s):** `run.sh script`, `sequence`, `receptionTime`, `rssiDbm`, `ipAddress` (+211 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **4 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `BatParameters`, `PendingVictimAlert`, `DroneApp.cc`, `FitnessParameters`, `LifecycleOperation`, `RepositionController`, `DroneApp.h`, `TeamLinkState`?**
  _High betweenness centrality (0.250) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.cc`, `DroneApp.h`, `UdpSocket`, `LifecycleOperation`?**
  _High betweenness centrality (0.092) - this node is a cross-community bridge._
- **Why does `FitnessParameters` connect `FitnessParameters` to `DroneApp`?**
  _High betweenness centrality (0.059) - this node is a cross-community bridge._
- **What connects `run.sh script`, `sequence`, `receptionTime` to the rest of the system?**
  _216 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.03125 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06451612903225806 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._