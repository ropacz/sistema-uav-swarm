# Graph Report - sistema  (2026-08-29)

## Corpus Check
- 60 files · ~34,165 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 765 nodes · 1070 edges · 60 communities (28 shown, 32 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 34 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `fcc54fe5`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- AlertMetricEvent
- figures.py
- FitnessParameters
- reconstruct
- BaGaussMarkovMobility
- DroneApp.cc
- Desvios e extensões em relação à diretriz normativa
- ExperimentMetrics.cc
- run.sh
- alert_sheet.py
- metricas_e_arquivos_de_resultado.md
- DataFrame
- Repository Guidelines
- BatAlgorithm::optimize
- DroneApp.h
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- ActiveVictim
- ExperimentMetrics
- run_audit.sh
- core/__init__.py
- SarScenarioManager
- cSimpleModule
- Indication
- analysis/__init__.py
- SarMessageSerializers.cc
- Coord
- map
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
- set
- UdpSocket::ICallback
- cObject
- simtime_t
- BatParameters
- FitnessParameters
- Path
- Series
- PendingVictimAlert
- PendingVictimAlert
- simsignal_t
- cObject
- Coord
- cMessage
- Coord
- cSimpleModule
- string

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 91 edges
2. `ExperimentMetrics` - 58 edges
3. `TeamApp` - 33 edges
4. `collect()` - 23 edges
5. `PendingVictimAlert` - 23 edges
6. `FitnessParameters` - 23 edges
7. `RepositionFitness` - 19 edges
8. `AlertRecord` - 13 edges
9. `AlertMetricEvent` - 13 edges
10. `BatAlgorithm::optimize()` - 13 edges

## Surprising Connections (you probably didn't know these)
- `BatAlgorithm::optimize()` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.cc → src/optimization/RepositionFitness.h
- `sampleInDomain()` --calls--> `inDomain`  [INFERRED]
  src/optimization/BatAlgorithm.cc → src/optimization/RepositionFitness.h
- `collect()` --calls--> `parse_sca()`  [EXTRACTED]
  analysis/core/experiment_metrics.py → analysis/core/process_results.py
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_alert_lifecycle_smoke_test.py → analysis/core/experiment_metrics.py
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_ba_smoke_test.py → analysis/core/experiment_metrics.py

## Import Cycles
- None detected.

## Communities (60 total, 32 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (57): DroneLinkState, RepositionController, simsignal_t, DroneApp, ackTimeout, activeVictims, alertAttemptSentSignal, alertConfirmedSignal (+49 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (33): cMessage, Packet, UdpSocket, ApplicationBase, cMessage, Indication, LifecycleOperation, set (+25 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): cObject, Coord, IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.11
Nodes (18): vector, BatAlgorithm, BatParameters, amplitudeDecay, frequencyMax, frequencyMin, initialAmplitude, initializationAttempts (+10 more)

### Community 4 - "AlertMetricEvent"
Cohesion: 0.15
Nodes (12): AlertMetricEvent, alertId, category, droneId, messageId, referenceTime, teamId, value (+4 more)

### Community 5 - "figures.py"
Cohesion: 0.15
Nodes (24): main(), attendance_figures(), configure_style(), dispersion_figure(), dispersion_figures(), effect_figure(), effect_figures(), DataFrame (+16 more)

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
Cohesion: 0.12
Nodes (30): Packet, ActiveVictim, cMessage, string, UdpSocket, canStartAlertCycle, completeAlertCycle, expireDiscoveredEntries (+22 more)

### Community 10 - "Desvios e extensões em relação à diretriz normativa"
Cohesion: 0.05
Nodes (40): Conformidade verificada, Correção: amostragem inicial do Bat Algorithm ignorava o domínio, Correção: `minimumRange` do sensor ativo mesmo no modo oráculo idealizado, D1. Mobilidade das equipes — §3, D2. Dimensões dos obstáculos — §10 e §15, D3. Intervalo entre alertas periódicos — §7.3 e §13, D4. Alcance do sensor de obstáculos — §14, D5. Nomes de escalares e de parâmetros — §17 e §24 (+32 more)

### Community 11 - "ExperimentMetrics.cc"
Cohesion: 0.25
Nodes (8): cComponent, cMessage, simsignal_t, finish, handleMessage, initialize, receiveSignal, writeAlertRecords

### Community 13 - "alert_sheet.py"
Cohesion: 0.09
Nodes (34): parse_sca(), DataFrame, Return attributes, scalar rows and recorded parameters from one SCA., build_pairs(), dispersion_summary(), load(), paired_effects(), per_seed_rates() (+26 more)

### Community 14 - "metricas_e_arquivos_de_resultado.md"
Cohesion: 0.11
Nodes (18): 1. Tipos de arquivo gerados, 2. `.sca`: o arquivo das métricas finais, 3. `.vec` e `.vci`: séries temporais, 4. `.elog`: o registro detalhado dos eventos, 5. Comparação entre `.sca` e `.elog`, 6. Cálculo de atendimento, perda e volume de mensagens, 7.1 Consultar o conteúdo de um arquivo, 7.2 Filtrar as métricas de interesse (+10 more)

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "BatAlgorithm::optimize"
Cohesion: 0.18
Nodes (18): BatResult, cRNG, DomainFunction, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness (+10 more)

### Community 18 - "DroneApp.h"
Cohesion: 0.18
Nodes (7): ApplicationBase, Indication, LifecycleOperation, map, string, UdpSocket, Coord

### Community 19 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts, attemptTeamAddresses (+9 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.40
Nodes (4): cListener, map, set, vector

### Community 21 - "collect"
Cohesion: 0.09
Nodes (26): central_scalar(), collect(), ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run., ExperimentMetricsReaderTests (+18 more)

### Community 22 - "ActiveVictim"
Cohesion: 0.05
Nodes (30): ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, Coord, simtime_t (+22 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (46): ExperimentMetrics, alertOrder, alertRecords, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal (+38 more)

### Community 26 - "SarScenarioManager"
Cohesion: 0.14
Nodes (13): cModule, ApplicationBase, LifecycleOperation, set, cMessage, cMessage, cSimpleModule, map (+5 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.12
Nodes (29): b, Chunk, ChunkSerializer, DroneStatusChunk, MemoryInputStream, MemoryOutputStream, Ptr, simtime_t (+21 more)

### Community 34 - "Auditoria: atendimento e perda via `.elog`, comparado ao `.sca`"
Cohesion: 0.07
Nodes (24): 1. Drone → equipe: primeira tentativa, enlace já obstruído, 2. Degradação do enlace, sensor, Bat Algorithm — **invisível ao `.elog`**, 3. Segunda tentativa — na nova posição, 4. Equipe → drone: confirmação, As quatro mensagens do protocolo, Como ler uma linha do `.elog`, Como reproduzir, Exemplos de mensagens capturadas no `.elog` (+16 more)

### Community 36 - "AlertSheetTests"
Cohesion: 0.14
Nodes (8): AlertSheetTests, FigureTests, DataFrame, Path, Segunda via de atendimento/perda: lê o CSV do opp_scavetool, não o .sca., As duas taxas contam alertId únicos, não tentativas nem recebimentos., As duas figuras precisam sair em PDF com desenho de verdade., ScavetoolFiguresTests

### Community 37 - "AlertRecord"
Cohesion: 0.17
Nodes (12): simtime_t, AlertRecord, acknowledged, ackTeamId, alertId, attempts, delivered, droneId (+4 more)

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **283 isolated node(s):** `D1. Mobilidade das equipes — §3`, `D2. Dimensões dos obstáculos — §10 e §15`, `D3. Intervalo entre alertas periódicos — §7.3 e §13`, `D4. Alcance do sensor de obstáculos — §14`, `D5. Nomes de escalares e de parâmetros — §17 e §24` (+278 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **32 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `FitnessParameters`, `DroneApp.cc`, `DroneApp.h`, `PendingVictimAlert`, `ExperimentMetrics.h`?**
  _High betweenness centrality (0.174) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `ExperimentMetrics.cc`, `ExperimentMetrics.h`, `AlertRecord`?**
  _High betweenness centrality (0.086) - this node is a cross-community bridge._
- **Why does `FitnessParameters` connect `FitnessParameters` to `DroneApp`?**
  _High betweenness centrality (0.048) - this node is a cross-community bridge._
- **What connects `D1. Mobilidade das equipes — §3`, `D2. Dimensões dos obstáculos — §10 e §15`, `D3. Intervalo entre alertas periódicos — §7.3 e §13` to the rest of the system?**
  _283 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.034482758620689655 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.06341463414634146 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._