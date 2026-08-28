# Graph Report - sistema  (2026-08-27)

## Corpus Check
- 59 files · ~29,296 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 729 nodes · 1029 edges · 54 communities (35 shown, 19 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 34 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `deab34e9`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- optimize
- AlertMetricEvent
- alert_sheet.py
- FitnessParameters
- reconstruct
- BaGaussMarkovMobility
- DroneApp.cc
- Desvios e extensões em relação à diretriz normativa
- ExperimentMetrics.cc
- run.sh
- ActiveVictim
- metricas_e_arquivos_de_resultado.md
- RepositionController
- Repository Guidelines
- TeamApp.cc
- DroneApp.h
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- TeamLinkState
- ExperimentMetrics
- run_audit.sh
- core/__init__.py
- SarScenarioManager
- DroneLinkState
- Indication
- analysis/__init__.py
- SarMessageSerializers.cc
- StaticVictim
- scheduleTeamUpdateRelay
- reports/__init__.py
- Auditoria: atendimento e perda via `.elog`, comparado ao `.sca`
- validation/__init__.py
- AlertSheetTests
- AlertRecord
- DataFrame
- Path
- Packet
- TeamUpdateChunk
- write_manifest.py
- simtime_t
- UdpSocket::ICallback
- cObject
- TeamApp.h
- string
- handleAssignment
- UdpSocket
- LifecycleOperation
- DataFrame
- Path
- Series

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 91 edges
2. `ExperimentMetrics` - 55 edges
3. `TeamApp` - 33 edges
4. `collect()` - 25 edges
5. `FitnessParameters` - 22 edges
6. `RepositionFitness` - 19 edges
7. `PendingVictimAlert` - 17 edges
8. `AlertRecord` - 13 edges
9. `AlertMetricEvent` - 13 edges
10. `optimize` - 13 edges

## Surprising Connections (you probably didn't know these)
- `ExperimentMetrics` --defines--> `ExperimentMetrics::finish()`  [EXTRACTED]
  src/metrics/ExperimentMetrics.h → src/metrics/ExperimentMetrics.cc
- `ExperimentMetrics` --defines--> `ExperimentMetrics::initialize()`  [EXTRACTED]
  src/metrics/ExperimentMetrics.h → src/metrics/ExperimentMetrics.cc
- `ExperimentMetrics` --defines--> `ExperimentMetrics::writeAlertRecords()`  [EXTRACTED]
  src/metrics/ExperimentMetrics.h → src/metrics/ExperimentMetrics.cc
- `tryReposition` --references--> `RepositionFitness`  [INFERRED]
  src/app/DroneApp.h → src/optimization/RepositionFitness.h
- `sampleInDomain()` --calls--> `inDomain`  [INFERRED]
  src/optimization/BatAlgorithm.cc → src/optimization/RepositionFitness.h

## Import Cycles
- None detected.

## Communities (54 total, 19 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (58): BatParameters, DroneLinkState, FitnessParameters, RepositionController, simtime_t, DroneApp, ackTimeout, activeVictims (+50 more)

### Community 1 - "TeamApp"
Cohesion: 0.09
Nodes (21): ApplicationBase, cMessage, set, simsignal_t, simtime_t, string, UdpSocket::ICallback, TeamApp (+13 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "optimize"
Cohesion: 0.08
Nodes (33): cRNG, DomainFunction, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency (+25 more)

### Community 4 - "AlertMetricEvent"
Cohesion: 0.15
Nodes (12): AlertMetricEvent, alertId, category, droneId, messageId, referenceTime, teamId, value (+4 more)

### Community 5 - "alert_sheet.py"
Cohesion: 0.11
Nodes (31): build_pairs(), load(), main(), paired_effects(), DataFrame, Path, Uma linha por (cenário, numTeams, seed), com o efeito BA-On − BA-Off. Separada…, Resume diferenças BA-On − BA-Off por seed, com IC bootstrap de 95%. (+23 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.07
Nodes (41): AbstractObstacleSensor, AbstractObstacleSensor, Coord, simtime_t, vector, FitnessParameters, areaMaxX, areaMaxY (+33 more)

### Community 7 - "reconstruct"
Cohesion: 0.20
Nodes (16): diff_run(), load_ground_truth(), main(), Path, blank_record(), build_module_index(), fields(), full_path() (+8 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (13): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, initialize (+5 more)

### Community 9 - "DroneApp.cc"
Cohesion: 0.16
Nodes (23): Packet, cMessage, PendingVictimAlert, UdpSocket, completeAlertCycle, expireDiscoveredEntries, finish, handleDroneStatus (+15 more)

### Community 10 - "Desvios e extensões em relação à diretriz normativa"
Cohesion: 0.05
Nodes (38): Conformidade verificada, Correção: amostragem inicial do Bat Algorithm ignorava o domínio, Correção: `minimumRange` do sensor ativo mesmo no modo oráculo idealizado, D1. Mobilidade das equipes — §3, D2. Dimensões dos obstáculos — §10 e §15, D3. Intervalo entre alertas periódicos — §7.3 e §13, D4. Alcance do sensor de obstáculos — §14, D5. Nomes de escalares e de parâmetros — §17 e §24 (+30 more)

### Community 11 - "ExperimentMetrics.cc"
Cohesion: 0.20
Nodes (9): cComponent, cObject, cMessage, simsignal_t, ExperimentMetrics::finish(), ExperimentMetrics::handleMessage(), ExperimentMetrics::initialize(), ExperimentMetrics::receiveSignal() (+1 more)

### Community 13 - "ActiveVictim"
Cohesion: 0.29
Nodes (7): ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, simtime_t

### Community 14 - "metricas_e_arquivos_de_resultado.md"
Cohesion: 0.11
Nodes (18): 1. Tipos de arquivo gerados, 2. `.sca`: o arquivo das métricas finais, 3. `.vec` e `.vci`: séries temporais, 4. `.elog`: o registro detalhado dos eventos, 5. Comparação entre `.sca` e `.elog`, 6. Cálculo de atendimento, perda e volume de mensagens, 7.1 Consultar o conteúdo de um arquivo, 7.2 Filtrar as métricas de interesse (+10 more)

### Community 15 - "RepositionController"
Cohesion: 0.31
Nodes (3): string, RepositionController, activeAlertId

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "TeamApp.cc"
Cohesion: 0.25
Nodes (9): cMessage, Packet, UdpSocket, hasTypePrefix(), handleMessageWhenUp, handleVictimAlert, initialize, sendTeamUpdate (+1 more)

### Community 18 - "DroneApp.h"
Cohesion: 0.22
Nodes (5): ApplicationBase, LifecycleOperation, map, set, string

### Community 19 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): Coord, map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts (+9 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.40
Nodes (4): cListener, cMessage, cSimpleModule, vector

### Community 21 - "collect"
Cohesion: 0.07
Nodes (38): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run. (+30 more)

### Community 22 - "TeamLinkState"
Cohesion: 0.22
Nodes (9): Coord, simtime_t, string, TeamLinkState, ipAddress, lastSequence, lastUpdateTime, position (+1 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.05
Nodes (44): ExperimentMetrics, alertOrder, alertRecords, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal (+36 more)

### Community 26 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 27 - "DroneLinkState"
Cohesion: 0.33
Nodes (6): DroneLinkState, lastSequence, lastUpdateTime, position, Coord, simtime_t

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.12
Nodes (29): b, Chunk, ChunkSerializer, DroneStatusChunk, MemoryInputStream, MemoryOutputStream, Ptr, simtime_t (+21 more)

### Community 31 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 32 - "scheduleTeamUpdateRelay"
Cohesion: 0.67
Nodes (3): string, scheduleTeamUpdateRelay, TeamUpdateChunk

### Community 34 - "Auditoria: atendimento e perda via `.elog`, comparado ao `.sca`"
Cohesion: 0.07
Nodes (24): 1. Drone → equipe: primeira tentativa, enlace já obstruído, 2. Degradação do enlace, sensor, Bat Algorithm — **invisível ao `.elog`**, 3. Segunda tentativa — na nova posição, 4. Equipe → drone: confirmação, As quatro mensagens do protocolo, Como ler uma linha do `.elog`, Como reproduzir, Exemplos de mensagens capturadas no `.elog` (+16 more)

### Community 36 - "AlertSheetTests"
Cohesion: 0.14
Nodes (8): AlertSheetTests, FigureTests, DataFrame, Path, Segunda via de atendimento/perda: lê o CSV do opp_scavetool, não o .sca., As duas taxas contam alertId únicos, não tentativas nem recebimentos., As duas figuras precisam sair em PDF com desenho de verdade., ScavetoolFiguresTests

### Community 37 - "AlertRecord"
Cohesion: 0.17
Nodes (12): AlertRecord, acknowledged, ackTeamId, alertId, attempts, delivered, droneId, generationTime (+4 more)

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

### Community 46 - "TeamApp.h"
Cohesion: 0.25
Nodes (5): Indication, ApplicationBase, LifecycleOperation, set, UdpSocket

### Community 47 - "string"
Cohesion: 0.36
Nodes (3): Coord, string, map

### Community 48 - "handleAssignment"
Cohesion: 0.40
Nodes (5): ActiveVictim, canStartAlertCycle, handleAssignment, startAlertCycle, VictimAssignment

## Knowledge Gaps
- **277 isolated node(s):** `1. Tipos de arquivo gerados`, `Como um escalar é gerado`, `3. `.vec` e `.vci`: séries temporais`, `4. `.elog`: o registro detalhado dos eventos`, `5. Comparação entre `.sca` e `.elog`` (+272 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **19 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `scheduleTeamUpdateRelay`, `DroneApp.cc`, `TeamApp.h`, `handleAssignment`, `DroneApp.h`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.125) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `ExperimentMetrics.cc`, `ExperimentMetrics.h`, `AlertRecord`?**
  _High betweenness centrality (0.083) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `UdpSocket`, `LifecycleOperation`, `TeamApp.h`, `TeamApp.cc`?**
  _High betweenness centrality (0.052) - this node is a cross-community bridge._
- **What connects `1. Tipos de arquivo gerados`, `Como um escalar é gerado`, `3. `.vec` e `.vci`: séries temporais` to the rest of the system?**
  _277 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.03389830508474576 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.09090909090909091 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._