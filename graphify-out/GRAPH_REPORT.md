# Graph Report - sistema  (2026-08-24)

## Corpus Check
- 54 files · ~20,061 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 609 nodes · 916 edges · 34 communities (27 shown, 7 thin omitted)
- Extraction: 97% EXTRACTED · 3% INFERRED · 0% AMBIGUOUS · INFERRED: 27 edges (avg confidence: 0.79)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `80c82160`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- DroneApp
- TeamApp
- AbstractObstacleSensor
- BatParameters
- StaticVictim
- collect
- FitnessParameters
- 3. Níveis de validação
- BaGaussMarkovMobility
- Simulações
- run.sh
- SarScenarioManager
- PendingVictimAlert
- RepositionController
- Repository Guidelines
- TeamApp.h
- TeamLinkState
- pcap_batch_to_spreadsheet.py
- string
- ExperimentMetrics
- core/__init__.py
- analysis/__init__.py
- SarMessageSerializers.cc
- pcap/__init__.py
- reports/__init__.py
- analysis/README.md
- validation/__init__.py
- Contratos e fórmulas das métricas
- Rastreabilidade e validações
- Modelo, funcionamento e premissas
- Cenários científicos da professora — arquivo histórico
- docs/README.md
- Protocolo científico
- write_manifest.py

## God Nodes (most connected - your core abstractions)
1. `DroneApp` - 62 edges
2. `ExperimentMetrics` - 40 edges
3. `TeamApp` - 33 edges
4. `PendingVictimAlert` - 22 edges
5. `FitnessParameters` - 22 edges
6. `BaGaussMarkovMobility` - 19 edges
7. `collect()` - 17 edges
8. `BatParameters` - 13 edges
9. `RepositionFitness` - 13 edges
10. `optimize` - 12 edges

## Surprising Connections (you probably didn't know these)
- `optimize` --calls--> `feasible`  [INFERRED]
  src/optimization/BatAlgorithm.h → src/optimization/RepositionFitness.h
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_ba_smoke_test.py → analysis/core/experiment_metrics.py
- `main()` --calls--> `collect()`  [EXTRACTED]
  analysis/validation/validate_network_discovery.py → analysis/core/experiment_metrics.py
- `MultiseedStatisticsTests` --uses--> `Capture`  [INFERRED]
  analysis/tests/test_pcap_multiseed.py → analysis/pcap/pcap_batch_to_spreadsheet.py
- `decode_echosar_payload()` --calls--> `text()`  [INFERRED]
  analysis/pcap/pcap_core.py → analysis/tests/test_pcap_analysis.py

## Import Cycles
- None detected.

## Communities (34 total, 7 thin omitted)

### Community 0 - "DroneApp"
Cohesion: 0.05
Nodes (52): cMessage, Packet, string, UdpSocket, DroneApp, ackTimeout, alertAttemptSentSignal, alertConfirmedSignal (+44 more)

### Community 1 - "TeamApp"
Cohesion: 0.05
Nodes (39): cMessage, Packet, UdpSocket, ApplicationBase, cMessage, Indication, LifecycleOperation, simsignal_t (+31 more)

### Community 2 - "AbstractObstacleSensor"
Cohesion: 0.09
Nodes (26): IPhysicalEnvironment, IPhysicalObject, IVisitor, ModuleRefByPar, AbstractObstacleSensor, environment, initialize, inspect (+18 more)

### Community 3 - "BatParameters"
Cohesion: 0.08
Nodes (31): cRNG, FeasibilityFunction, FitnessFunction, Bat, amplitude, fitness, frequency, position (+23 more)

### Community 4 - "StaticVictim"
Cohesion: 0.33
Nodes (3): cMessage, cSimpleModule, StaticVictim

### Community 5 - "collect"
Cohesion: 0.06
Nodes (48): central_scalar(), collect(), DataFrame, ratio(), Read the small, normative ExperimentMetrics contract from one SCA file., Return a ratio without inventing a value when no alert was generated., Read exactly one scalar from the central collector. Missing or duplicate rows…, Return raw counters and the four outcomes for one experimental run. (+40 more)

### Community 6 - "FitnessParameters"
Cohesion: 0.08
Nodes (35): AbstractObstacleSensor, tryReposition, AbstractObstacleSensor, Coord, simtime_t, FitnessParameters, areaMaxX, areaMaxY (+27 more)

### Community 7 - "3. Níveis de validação"
Cohesion: 0.33
Nodes (6): 3. Níveis de validação, Build C++/NED, Descoberta direta, Estática e unitária, Experimento confirmatório, Smoke do mecanismo

### Community 8 - "BaGaussMarkovMobility"
Cohesion: 0.13
Nodes (18): GaussMarkovMobility, rad, BaGaussMarkovMobility, baOverride, elevation, elevationMean, elevationStdDev, finish (+10 more)

### Community 10 - "Simulações"
Cohesion: 0.20
Nodes (8): ECHOSAR-Net — reposicionamento de UAV com Bat Algorithm, Organização, Experimento principal, Extensões opcionais, Resultados, Robustez, Simulações, Validações técnicas

### Community 13 - "SarScenarioManager"
Cohesion: 0.20
Nodes (10): cModule, cMessage, cMessage, cSimpleModule, map, SarScenarioManager, detections, handleMessage (+2 more)

### Community 14 - "PendingVictimAlert"
Cohesion: 0.12
Nodes (17): map, simtime_t, string, PendingVictimAlert, ackDeadline, alertId, attempts, attemptTeamAddresses (+9 more)

### Community 15 - "RepositionController"
Cohesion: 0.33
Nodes (3): string, RepositionController, activeAlertId

### Community 16 - "Repository Guidelines"
Cohesion: 0.25
Nodes (7): Build, Test, and Development Commands, Coding Style & Naming Conventions, Commit & Pull Request Guidelines, graphify, Project Structure & Module Organization, Repository Guidelines, Testing Guidelines

### Community 17 - "TeamApp.h"
Cohesion: 0.29
Nodes (4): ApplicationBase, Indication, UdpSocket, set

### Community 18 - "TeamLinkState"
Cohesion: 0.25
Nodes (8): Coord, simtime_t, string, TeamLinkState, ipAddress, lastSeen, lastSequence, position

### Community 19 - "pcap_batch_to_spreadsheet.py"
Cohesion: 0.06
Nodes (55): Capture, compare_group(), discover_captures(), main(), parse_capture_name(), DataFrame, Path, Compara todas as transmissões de uma configuração/execução. (+47 more)

### Community 20 - "string"
Cohesion: 0.50
Nodes (3): map, string, Coord

### Community 23 - "ExperimentMetrics"
Cohesion: 0.05
Nodes (45): cComponent, cListener, cMessage, cMessage, cObject, simsignal_t, ExperimentMetrics, alertAttemptsSent (+37 more)

### Community 30 - "SarMessageSerializers.cc"
Cohesion: 0.13
Nodes (26): b, Chunk, ChunkSerializer, MemoryInputStream, MemoryOutputStream, PositionUpdateChunk, Ptr, simtime_t (+18 more)

### Community 36 - "Contratos e fórmulas das métricas"
Cohesion: 0.22
Nodes (9): 1. Unidade de contagem, 2. Contadores centrais, 3. Desfecho primário, 4. Desfechos secundários, 5. Diagnóstico de exposição, 6. Invariantes fim a fim, 7. Agregação entre execuções, 8. Diagnósticos opcionais (+1 more)

### Community 37 - "Rastreabilidade e validações"
Cohesion: 0.29
Nodes (6): 1. Matriz funcional, 2. Contrato dos sinais, 4. Artefatos e proveniência, 5. Checklist de publicação, 6. Limitações conhecidas, Rastreabilidade e validações

### Community 38 - "Modelo, funcionamento e premissas"
Cohesion: 0.25
Nodes (8): 1. Fronteira do modelo, 2. Componentes, 3. Fluxo do alerta, 4. Identidades, ACK e estado mínimo, 5. Gatilho e sensor, 6. Bat Algorithm e aptidão, 7. Premissas e limitações, Modelo, funcionamento e premissas

### Community 39 - "Cenários científicos da professora — arquivo histórico"
Cohesion: 0.20
Nodes (10): Cenários científicos da professora — arquivo histórico, Desenho solicitado, Execução, Gatilho e Bat Algorithm, Hierarquia do escopo, Mobilidade e referência física, Métricas e validade, Pergunta e hipóteses (+2 more)

### Community 40 - "docs/README.md"
Cohesion: 0.25
Nodes (3): Documentação técnica e científica, Fonte única dos parâmetros, Material complementar

### Community 41 - "Protocolo científico"
Cohesion: 0.29
Nodes (7): 1. Pergunta e objetivo, 2. Desenho experimental, 3. Métricas, 4. Procedimento, 5. Análise estatística, 6. Interpretação e limitações, Protocolo científico

### Community 42 - "write_manifest.py"
Cohesion: 0.27
Nodes (8): command_output(), main(), omnetpp_version(), Path, Write reproducibility metadata beside generated simulation results., sha256(), ManifestTests, patch

## Knowledge Gaps
- **210 isolated node(s):** `run.sh script`, `socket`, `maintenanceTimer`, `movementCompleteTimer`, `droneId` (+205 more)
  These have ≤1 connection - possible missing edges or undocumented components.
- **7 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `DroneApp` connect `DroneApp` to `BatParameters`, `FitnessParameters`, `PendingVictimAlert`, `RepositionController`, `TeamApp.h`, `TeamLinkState`, `string`, `ExperimentMetrics`?**
  _High betweenness centrality (0.189) - this node is a cross-community bridge._
- **Why does `TeamApp` connect `TeamApp` to `TeamApp.h`?**
  _High betweenness centrality (0.057) - this node is a cross-community bridge._
- **What connects `run.sh script`, `socket`, `maintenanceTimer` to the rest of the system?**
  _210 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `DroneApp` be split into smaller, more focused modules?**
  _Cohesion score 0.05384150030248034 - nodes in this community are weakly interconnected._
- **Should `TeamApp` be split into smaller, more focused modules?**
  _Cohesion score 0.05102040816326531 - nodes in this community are weakly interconnected._
- **Should `AbstractObstacleSensor` be split into smaller, more focused modules?**
  _Cohesion score 0.08602150537634409 - nodes in this community are weakly interconnected._
- **Should `BatParameters` be split into smaller, more focused modules?**
  _Cohesion score 0.07954545454545454 - nodes in this community are weakly interconnected._