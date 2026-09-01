# Graph Report - sistema  (2026-09-01)

## Corpus Check
- 61 files · ~40,275 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 847 nodes · 1199 edges · 57 communities (27 shown, 30 thin omitted)
- Extraction: 99% EXTRACTED · 1% INFERRED · 0% AMBIGUOUS · INFERRED: 12 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `6cd9587a`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- optimize
- TeamLinkState
- alert_sheet.py
- FitnessParameters
- reconstruct
- BaGaussMarkovMobility
- DroneApp.cc
- Desvios e extensões em relação à diretriz normativa
- ExperimentMetrics.cc
- run.sh
- string
- metricas_e_arquivos_de_resultado.md
- Path
- Repository Guidelines
- DroneApp::startAlertCycle
- Coord
- PendingVictimAlert
- ExperimentMetrics.h
- collect
- AlertMetricEvent
- ExperimentMetrics
- run_audit.sh
- core/__init__.py
- DroneApp::handleMessageWhenUp
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
- cSimpleModule
- ActiveVictim
- Packet
- Packet
- write_manifest.py
- Ptr
- TeamUpdateChunk
- vector
- VictimAlertChunk
- Indication
- simsignal_t
- UdpSocket::ICallback
- cMessage
- string
- simsignal_t
- Coord
- PendingVictimAlert
- string
- UdpSocket

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 125 edges
2. `ExperimentMetrics` - 65 edges
3. `TeamApp` - 35 edges
4. `collect()` - 27 edges
5. `PendingVictimAlert` - 24 edges
6. `FitnessParameters` - 23 edges
7. `RepositionFitness` - 19 edges
8. `TeamLinkState` - 18 edges
9. `AlertMetricEvent` - 13 edges
10. `AlertRecord` - 13 edges

## Surprising Connections (you probably didn't know these)
- `DroneApp` --defines--> `DroneApp::initialize()`  [EXTRACTED]
  src/app/DroneApp.h → src/app/DroneApp.cc
- `DroneApp::socketDataArrived()` --calls--> `handleRecoveryProbe`  [INFERRED]
  src/app/DroneApp.cc → src/app/TeamApp.h
- `DroneApp` --defines--> `DroneApp::canStartAlertCycle()`  [EXTRACTED]
  src/app/DroneApp.h → src/app/DroneApp.cc
- `DroneApp` --defines--> `DroneApp::expectedRssiDbm()`  [EXTRACTED]
  src/app/DroneApp.h → src/app/DroneApp.cc
- `DroneApp` --defines--> `DroneApp::sendDroneStatus()`  [EXTRACTED]
  src/app/DroneApp.h → src/app/DroneApp.cc

## Import Cycles
- None detected.

## Communities (57 total, 30 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.03
Nodes (79): BatParameters, DroneLinkState, FitnessParameters, RepositionController, DroneApp, ackTimeout, activeVictims, alertAttemptSentSignal (+71 more)

### Community 1 - "TeamApp"
Cohesion: 0.06
Nodes (38): ApplicationBase, LifecycleOperation, string, cMessage, Packet, UdpSocket, ApplicationBase, cMessage (+30 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (27): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, clearCorridorGroundTruth, environment, initialize (+19 more)

### Community 3 - "optimize"
Cohesion: 0.08
Nodes (34): cRNG, DomainFunction, FeasibilityFunction, FitnessFunction, vector, Bat, amplitude, fitness (+26 more)

### Community 4 - "TeamLinkState"
Cohesion: 0.11
Nodes (20): deque, Coord, simtime_t, string, RssiSample, distanceMeters, receptionTime, rssiDbm (+12 more)

### Community 5 - "alert_sheet.py"
Cohesion: 0.08
Nodes (46): build_pairs(), dispersion_summary(), load(), main(), paired_effects(), per_seed_rates(), DataFrame, Path (+38 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.06
Nodes (44): AbstractObstacleSensor, AbstractObstacleSensor, Coord, optional, simtime_t, vector, FitnessParameters, areaMaxX (+36 more)

### Community 7 - "reconstruct"
Cohesion: 0.20
Nodes (16): diff_run(), load_ground_truth(), main(), Path, blank_record(), build_module_index(), fields(), full_path() (+8 more)

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.21
Nodes (13): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, initialize (+5 more)

### Community 9 - "DroneApp.cc"
Cohesion: 0.10
Nodes (27): PendingVictimAlert, Ptr, DroneApp::attachVictimPhoto(), DroneApp::canStartAlertCycle(), DroneApp::completeAlertCycle(), DroneApp::expectedRssiDbm(), DroneApp::expireDiscoveredEntries(), DroneApp::expireRecoveryProbes() (+19 more)

### Community 10 - "Desvios e extensões em relação à diretriz normativa"
Cohesion: 0.05
Nodes (41): Conformidade verificada, Correção: amostragem inicial do Bat Algorithm ignorava o domínio, Correção: `minimumRange` do sensor ativo mesmo no modo oráculo idealizado, D1. Mobilidade das equipes — §3, D2. Dimensões dos obstáculos — §10 e §15, D3. Intervalo entre alertas periódicos — §7.3 e §13, D4. Alcance do sensor de obstáculos — §14, D5. Nomes de escalares e de parâmetros — §17 e §24 (+33 more)

### Community 11 - "ExperimentMetrics.cc"
Cohesion: 0.20
Nodes (9): cComponent, simsignal_t, cMessage, cObject, ExperimentMetrics::finish(), ExperimentMetrics::handleMessage(), ExperimentMetrics::initialize(), ExperimentMetrics::receiveSignal() (+1 more)

### Community 13 - "string"
Cohesion: 0.40
Nodes (5): DroneApp::handleRecoveryProbe(), DroneApp::scheduleTeamUpdateRelay(), DroneApp::selectTargetTeam(), string, TeamUpdateChunk

### Community 14 - "metricas_e_arquivos_de_resultado.md"
Cohesion: 0.11
Nodes (18): 1. Tipos de arquivo gerados, 2. `.sca`: o arquivo das métricas finais, 3. `.vec` e `.vci`: séries temporais, 4. `.elog`: o registro detalhado dos eventos, 5. Comparação entre `.sca` e `.elog`, 6. Cálculo de atendimento, perda e volume de mensagens, 7.1 Consultar o conteúdo de um arquivo, 7.2 Filtrar as métricas de interesse (+10 more)

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 19 - "PendingVictimAlert"
Cohesion: 0.06
Nodes (35): cModule, map, set, map, simtime_t, string, PendingVictimAlert, ackDeadline (+27 more)

### Community 20 - "ExperimentMetrics.h"
Cohesion: 0.50
Nodes (3): cListener, cSimpleModule, cMessage

### Community 21 - "collect"
Cohesion: 0.06
Nodes (43): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and auditable outcomes for one experimental run. (+35 more)

### Community 22 - "AlertMetricEvent"
Cohesion: 0.05
Nodes (33): ActiveVictim, alertSequence, nextAlertTime, pendingAlertId, position, victimId, Coord, simtime_t (+25 more)

### Community 23 - "ExperimentMetrics"
Cohesion: 0.04
Nodes (54): ExperimentMetrics, alertOrder, alertRecords, alertsWithoutKnownTeam, attemptsByAlert, attemptSentSignal, baActivations, baActivationSignal (+46 more)

### Community 28 - "DroneApp.h"
Cohesion: 0.14
Nodes (8): ApplicationBase, Indication, LifecycleOperation, map, optional, set, DroneApp::computeExcessAttenuation(), UdpSocket

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

### Community 40 - "Packet"
Cohesion: 0.20
Nodes (11): Coord, Packet, DroneApp::discardExpiredRssiSamples(), DroneApp::evaluatePossibleObstruction(), DroneApp::handleAssignment(), DroneApp::handleDroneStatus(), DroneApp::handleTeamUpdate(), DroneApp::handleVictimAck() (+3 more)

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **334 isolated node(s):** `SarCodec`, `ackTimeout`, `activeVictims`, `alertAttemptSentSignal`, `alertConfirmedSignal` (+329 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **30 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `TeamApp`, `TeamLinkState`, `Packet`, `DroneApp.cc`, `ExperimentMetrics.cc`, `string`, `DroneApp::startAlertCycle`, `ExperimentMetrics.h`, `DroneApp::handleMessageWhenUp`, `DroneApp.h`?**
  _High betweenness centrality (0.175) - this node is a cross-community bridge._
- **Why does `ExperimentMetrics` connect `ExperimentMetrics` to `optimize`, `ExperimentMetrics.cc`, `ExperimentMetrics.h`, `AlertRecord`?**
  _High betweenness centrality (0.088) - this node is a cross-community bridge._
- **What connects `SarCodec`, `ackTimeout`, `activeVictims` to the rest of the system?**
  _334 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.025 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.05735430157261795 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08870967741935484 - nodes in this community are weakly interconnected._
- **Should `optimize` be split into smaller, more focused modules?**
  _Cohesion score 0.07936507936507936 - nodes in this community are weakly interconnected._