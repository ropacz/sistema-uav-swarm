# Graph Report - sistema  (2026-08-24)

## Corpus Check
- 54 files · ~27,532 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 758 nodes · 1114 edges · 39 communities (26 shown, 13 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 29 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `b23c82cc`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- StaticVictim
- DroneApp.cc
- FitnessParameters
- LifecycleOperation
- BaGaussMarkovMobility
- network_metrics.py
- Protocolo científico
- plot_scenario1_line1.py
- run.sh
- SarScenarioManager
- PendingVictimAlert
- RepositionController
- Repository Guidelines
- UdpSocket
- TeamLinkState
- pcap_batch_to_spreadsheet.py
- string
- AlertMetricEvent
- ExperimentMetrics.h
- ExperimentMetrics
- receiveSignal
- core/__init__.py
- TeamApp.cc
- UdpSocket
- LifecycleOperation
- analysis/__init__.py
- SarMessageSerializers.cc
- pcap/__init__.py
- plots/__init__.py
- reports/__init__.py
- analysis/README.md
- validation/__init__.py
- Contratos e fórmulas das métricas
- Rastreabilidade e validações
- Modelo, funcionamento e premissas

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 118 edges
2. `ExperimentMetrics` - 61 edges
3. `TeamApp` - 44 edges
4. `PendingVictimAlert` - 35 edges
5. `FitnessParameters` - 22 edges
6. `BaGaussMarkovMobility` - 19 edges
7. `TeamLinkState` - 18 edges
8. `collect()` - 17 edges
9. `parse_sca()` - 15 edges
10. `RepositionController` - 14 edges

## Surprising Connections (you probably didn't know these)
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `MultiseedStatisticsTests` --uses--> `Capture`  [INFERRED]
  analysis/tests/test_pcap_multiseed.py → analysis/pcap/pcap_batch_to_spreadsheet.py
- `decode_echosar_payload()` --calls--> `text()`  [INFERRED]
  analysis/pcap/pcap_core.py → analysis/tests/test_pcap_analysis.py
- `DroneApp` --references--> `PendingVictimAlert`  [EXTRACTED]
  src/app/DroneApp.h → src/app/PendingVictimAlert.h
- `DroneApp` --references--> `RepositionController`  [EXTRACTED]
  src/app/DroneApp.h → src/app/RepositionController.h

## Import Cycles
- None detected.

## Communities (39 total, 13 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.02
Nodes (85): DroneApp, ackTimeout, alertAttemptSentSignal, alertAttemptsSent, alertConfirmedSignal, alertExpiredSignal, alertGeneratedSignal, alertsExpired (+77 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (31): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+23 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (31): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+23 more)

### Community 4 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 5 - "DroneApp.cc"
Cohesion: 0.15
Nodes (23): cMessage, Coord, Packet, string, UdpSocket, calculatePositionUpdatePdr, detectDegradation, estimateTeamPosition (+15 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (36): AbstractObstacleSensor, tryReposition, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY (+28 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.13
Nodes (18): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, finish (+10 more)

### Community 9 - "network_metrics.py"
Cohesion: 0.07
Nodes (50): aggregate(), collect(), extreme_where(), global_or_legacy(), global_scalar(), main(), mean_where(), pooled_statistic_mean() (+42 more)

### Community 10 - "Protocolo científico"
Cohesion: 0.05
Nodes (33): Cenários científicos da professora, Desenho solicitado, Execução, Gatilho e Bat Algorithm, Hierarquia do escopo, Mobilidade e referência física, Métricas e validade, Pergunta e hipóteses (+25 more)

### Community 13 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 14 - "PendingVictimAlert"
Cohesion: 0.07
Nodes (28): map, simtime_t, string, PendingVictimAlert, ackDeadline, activeRepositionCycleId, alertId, attempts (+20 more)

### Community 15 - "RepositionController"
Cohesion: 0.22
Nodes (5): string, RepositionController, activeAlertId, state, State

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 18 - "TeamLinkState"
Cohesion: 0.12
Nodes (18): deque, Coord, simtime_t, string, LinkSample, receptionTime, rssiDbm, sequence (+10 more)

### Community 19 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.06
Nodes (59): main(), pcap_metrics(), plot_pcap(), DataFrame, Capture, compare_group(), discover_captures(), main() (+51 more)

### Community 20 - "string"
Cohesion: 0.35
Nodes (5): ApplicationBase, map, string, Coord, set

### Community 21 - "AlertMetricEvent"
Cohesion: 0.18
Nodes (10): AlertMetricEvent, alertId, category, messageId, referenceTime, secondaryTime, value, cObject (+2 more)

### Community 22 - "ExperimentMetrics.h"
Cohesion: 0.22
Nodes (7): cListener, cMessage, cMessage, finish, handleMessage, initialize, cSimpleModule

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (55): ExperimentMetrics, alertAttemptsSent, attemptDeliveryDelaySum, attemptsByAlert, attemptSentSignal, attemptSentTimes, baActivations, baActivationSignal (+47 more)

### Community 24 - "receiveSignal"
Cohesion: 0.50
Nodes (4): cComponent, cObject, simsignal_t, receiveSignal

### Community 26 - "TeamApp.cc"
Cohesion: 0.24
Nodes (9): cMessage, Packet, UdpSocket, finish, handleMessageWhenUp, handleVictimAlert, initialize, sendPositionUpdate (+1 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

### Community 36 - "Contratos e fórmulas das métricas"
Cohesion: 0.08
Nodes (24): 10. Agregação entre execuções, 11. Fonte adequada para cada resultado, 1. Princípios de medição, 2. Alerta, tentativa e ACK, 3.1 PDR de alertas, 3.2 Taxa de perda de alertas, 3.3 Taxa de confirmação, 3.4 Entrega por tentativa (+16 more)

### Community 37 - "Rastreabilidade e validações"
Cohesion: 0.12
Nodes (17): 10. Manutenção da rastreabilidade, 1. Objetivo, 2. Matriz de rastreabilidade funcional, 3. Contrato dos sinais centrais, 4. Configuração e proveniência, 5.1 Validação estática, 5.2 Testes unitários da análise, 5.3 Compilação C++/NED (+9 more)

### Community 38 - "Modelo, funcionamento e premissas"
Cohesion: 0.13
Nodes (15): 10. Mobilidade normal dos UAVs, 11. Premissas e limites de interpretação, 1. Finalidade e fronteira do sistema, 2. Componentes e responsabilidades, 3. Fluxo completo de um alerta, 4. Identidades e deduplicação, 5. Descoberta, seleção e roteamento, 6. Estado do enlace e gatilho de degradação (+7 more)

## Knowledge Gaps
- **343 isolated node(s):** `run.sh script`, `socket`, `maintenanceTimer`, `movementCompleteTimer`, `droneId` (+338 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **13 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `BatParameters`, `DroneApp.cc`, `FitnessParameters`, `LifecycleOperation`, `PendingVictimAlert`, `RepositionController`, `UdpSocket`, `TeamLinkState`, `string`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.236) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `receiveSignal`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.088) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.cc`, `UdpSocket`, `string`, `LifecycleOperation`?**
  _High betweenness centrality (0.066) - this node is a cross-community bridge._
- **What connects `run.sh script`, `socket`, `maintenanceTimer` to the rest of the system?**
  _343 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.023255813953488372 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.0625 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._