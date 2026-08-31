# Graph Report - sistema  (2026-08-31)

## Corpus Check
- 60 files · ~36,434 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 788 nodes · 1171 edges · 41 communities (28 shown, 13 thin omitted)
- Extraction: 96% EXTRACTED · 4% INFERRED · 0% AMBIGUOUS · INFERRED: 47 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `5fd510ef`
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
- sendAttempt
- metricas_e_arquivos_de_resultado.md
- Path
- Repository Guidelines
- Ptr
- Coord
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- ActiveVictim
- ExperimentMetrics
- run_audit.sh
- core/__init__.py
- SarScenarioManager
- cObject
- DroneApp.h
- analysis/__init__.py
- SarMessageSerializers.cc
- DataFrame
- cSimpleModule
- reports/__init__.py
- Auditoria: atendimento e perda via `.elog`, comparado ao `.sca`
- validation/__init__.py
- AlertSheetTests
- AlertRecord
- VictimAlertChunk
- socketDataArrived
- write_manifest.py

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 106 edges
2. `ExperimentMetrics` - 65 edges
3. `TeamApp` - 35 edges
4. `PendingVictimAlert` - 33 edges
5. `collect()` - 28 edges
6. `FitnessParameters` - 22 edges
7. `RepositionFitness` - 19 edges
8. `AlertRecord` - 13 edges
9. `AlertMetricEvent` - 13 edges
10. `optimize` - 13 edges

## Surprising Connections (you probably didn't know these)
- `sampleInDomain()` --calls--> `inDomain`  [INFERRED]
  src/optimization/BatAlgorithm.cc → src/optimization/RepositionFitness.h
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_alert_lifecycle_smoke_test.py → analysis/core/experiment_metrics.py
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_ba_smoke_test.py → analysis/core/experiment_metrics.py
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_multihop_smoke_test.py → analysis/core/experiment_metrics.py

## Import Cycles
- None detected.

## Communities (41 total, 13 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (66): BatParameters, DroneLinkState, FitnessParameters, RepositionController, DroneApp, ackTimeout, activeVictims, alertAttemptSentSignal (+58 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (35): cMessage, Packet, UdpSocket, ApplicationBase, cMessage, Indication, LifecycleOperation, set (+27 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "optimize"
Cohesion: 0.08
Nodes (33): cRNG, DomainFunction, FeasibilityFunction, FitnessFunction, vector, Bat, amplitude, fitness (+25 more)

### Community 4 - "AlertMetricEvent"
Cohesion: 0.15
Nodes (12): AlertMetricEvent, alertId, category, droneId, messageId, referenceTime, teamId, value (+4 more)

### Community 5 - "alert_sheet.py"
Cohesion: 0.08
Nodes (46): build_pairs(), dispersion_summary(), load(), main(), paired_effects(), per_seed_rates(), DataFrame, Path (+38 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.06
Nodes (43): AbstractObstacleSensor, AbstractObstacleSensor, Coord, optional, simtime_t, vector, FitnessParameters, areaMaxX (+35 more)

### Community 7 - "reconstruct"
Cohesion: 0.20
Nodes (16): diff_run(), load_ground_truth(), main(), Path, blank_record(), build_module_index(), fields(), full_path() (+8 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (13): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, initialize (+5 more)

### Community 9 - "DroneApp.cc"
Cohesion: 0.16
Nodes (20): ActiveVictim, cMessage, canStartAlertCycle, completeAlertCycle, expireDiscoveredEntries, finish, handleAssignment, handleMessageWhenUp (+12 more)

### Community 10 - "Desvios e extensões em relação à diretriz normativa"
Cohesion: 0.05
Nodes (40): Conformidade verificada, Correção: amostragem inicial do Bat Algorithm ignorava o domínio, Correção: `minimumRange` do sensor ativo mesmo no modo oráculo idealizado, D1. Mobilidade das equipes — §3, D2. Dimensões dos obstáculos — §10 e §15, D3. Intervalo entre alertas periódicos — §7.3 e §13, D4. Alcance do sensor de obstáculos — §14, D5. Nomes de escalares e de parâmetros — §17 e §24 (+32 more)

### Community 11 - "ExperimentMetrics.cc"
Cohesion: 0.22
Nodes (9): cComponent, cObject, cMessage, simsignal_t, finish, handleMessage, initialize, receiveSignal (+1 more)

### Community 13 - "sendAttempt"
Cohesion: 0.28
Nodes (9): Ptr, string, vector, VictimAlertChunk, attachVictimPhoto, handleRecoveryProbe, selectTargetTeam, sendAttempt (+1 more)

### Community 14 - "metricas_e_arquivos_de_resultado.md"
Cohesion: 0.11
Nodes (18): 1. Tipos de arquivo gerados, 2. `.sca`: o arquivo das métricas finais, 3. `.vec` e `.vci`: séries temporais, 4. `.elog`: o registro detalhado dos eventos, 5. Comparação entre `.sca` e `.elog`, 6. Cálculo de atendimento, perda e volume de mensagens, 7.1 Consultar o conteúdo de um arquivo, 7.2 Filtrar as métricas de interesse (+10 more)

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 19 - "PendingVictimAlert"
Cohesion: 0.08
Nodes (27): expireRecoveryProbes, finishRecoveryProbe, sendRecoveryProbe, startRecoveryProbe, map, simtime_t, string, PendingVictimAlert (+19 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.50
Nodes (3): cListener, cSimpleModule, cMessage

### Community 21 - "collect"
Cohesion: 0.06
Nodes (39): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run. (+31 more)

### Community 22 - "ActiveVictim"
Cohesion: 0.05
Nodes (30): ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, Coord, simtime_t (+22 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (54): ExperimentMetrics, alertOrder, alertRecords, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal (+46 more)

### Community 26 - "SarScenarioManager"
Cohesion: 0.19
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 28 - "DroneApp.h"
Cohesion: 0.18
Nodes (8): Coord, ApplicationBase, Indication, LifecycleOperation, map, set, string, UdpSocket

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.10
Nodes (35): b, Chunk, ChunkSerializer, DroneStatusChunk, MemoryInputStream, MemoryOutputStream, RecoveryProbeChunk, Ptr (+27 more)

### Community 34 - "Auditoria: atendimento e perda via `.elog`, comparado ao `.sca`"
Cohesion: 0.07
Nodes (24): 1. Drone → equipe: primeira tentativa, enlace já obstruído, 2. Degradação do enlace, sensor, Bat Algorithm — **invisível ao `.elog`**, 3. Segunda tentativa — na nova posição, 4. Equipe → drone: confirmação, As quatro mensagens do protocolo, Como ler uma linha do `.elog`, Como reproduzir, Exemplos de mensagens capturadas no `.elog` (+16 more)

### Community 36 - "AlertSheetTests"
Cohesion: 0.14
Nodes (8): AlertSheetTests, FigureTests, Path, Segunda via de atendimento/perda: lê o CSV do opp_scavetool, não o .sca., As duas taxas contam alertId únicos, não tentativas nem recebimentos., As duas figuras precisam sair em PDF com desenho de verdade., ScavetoolFiguresTests, DataFrame

### Community 37 - "AlertRecord"
Cohesion: 0.17
Nodes (12): AlertRecord, acknowledged, ackTeamId, alertId, attempts, delivered, droneId, generationTime (+4 more)

### Community 40 - "socketDataArrived"
Cohesion: 0.32
Nodes (8): Packet, TeamUpdateChunk, UdpSocket, handleDroneStatus, handleTeamUpdate, scheduleTeamUpdateRelay, socketDataArrived, hasTypePrefix()

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **307 isolated node(s):** `socket`, `maintenanceTimer`, `movementCompleteTimer`, `droneStatusTimer`, `droneId` (+302 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **13 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `socketDataArrived`, `DroneApp.cc`, `sendAttempt`, `PendingVictimAlert`, `ExperimentMetrics.h`, `DroneApp.h`?**
  _High betweenness centrality (0.133) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `optimize`, `ExperimentMetrics.cc`, `ExperimentMetrics.h`, `AlertRecord`?**
  _High betweenness centrality (0.092) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `DroneApp.h`?**
  _High betweenness centrality (0.047) - this node is a cross-community bridge._
- **What connects `socket`, `maintenanceTimer`, `movementCompleteTimer` to the rest of the system?**
  _307 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.029850746268656716 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06201550387596899 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._