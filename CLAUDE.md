# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Ferramenta de consulta ao UserGuide

Antes de responder dúvidas sobre configuração OMNeT++ (Qtenv, INI editor, NED editor, launch, etc.),
consulte o guia com:

```bash
python3 search_guide.py "sua dúvida aqui"          # top-3 seções relevantes
python3 search_guide.py -n 5 "sua dúvida aqui"     # top-5
python3 search_guide.py --list-sections             # índice completo
```

O `UserGuide.txt` é o guia oficial do OMNeT++ IDE 6.2.0 (9 231 linhas).
O script usa BM25 (sem dependências externas) e retorna as seções mais relevantes com número de linha.

---

## Project Context

Simulação OMNeT++ de enxame de drones para busca e resgate (SAR) — dissertação de mestrado PPGCAP/UDESC.

**Stack:** OMNeT++ 6.2.0 + INET Framework 4.5.4  
**Ambiente:** gerenciado via `opp_env` (não chamar `omnetpp`/`opp_run` diretamente)

## Build Commands

```bash
# Gerar src/Makefile (necessário após adicionar arquivos novos)
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'make makefiles'

# Build incremental
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'make'

# Rebuild completo
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'make makefiles && make clean && make'
```

O Makefile usa `-I. -I$INET/src -L$INET/src -lINET_dbg` via `opp_makemake --deep`.  
O flag `-I.` é crítico: coloca `src/` no include path para que `#include "messages/Foo_m.h"` resolva corretamente.

## Run Commands

```bash
# Cmdenv (terminal) — BasicTest
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'cd simulations && ./run -u Cmdenv -c BasicTest -r 0 --cmdenv-express-mode=false'

# Qtenv (GUI)
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'cd simulations && ./run -u Qtenv -c BasicTest'

# Atalho via script
./run.sh              # Cmdenv, log INFO, BasicTest run 0
./run.sh --gui        # Qtenv (janela gráfica)
./run.sh --warn       # Cmdenv com log WARN (mais rápido)
./run.sh --build      # compila antes de rodar
./run.sh -c OutraConf # config diferente
./run.sh -r 2         # seed/run 2
```

O script `simulations/run` invoca `../src/sistema_dbg` (binário debug com INET linkado).  
Usar sempre esse script em vez de `opp_run` — `opp_run` não tem INET linkado e falha.

## Architecture

### Hierarquia de módulos

```
BasicNetwork (BasicNetwork.ned)
├── visualizer: IntegratedCanvasVisualizer   ← exibe alcance de rádio no canvas
├── configurator: Ipv4NetworkConfigurator
├── radioMedium:  Ieee80211ScalarRadioMedium
├── drone[10]: WirelessHost
│   ├── mobility: GaussMarkovMobility   ← voo 3D, Z: 50–150 m
│   └── app[0]:   SimpleDroneApp        ← recebe TeamUpdate, envia DroneStatus e VictimAlert
└── team: WirelessHost
    ├── mobility: StationaryMobility    ← terrestre, em (500, 900, 1.5) m
    └── app[0]:   SimpleTeamApp         ← broadcast TeamUpdate, recebe DroneStatus e VictimAlert
```

### Portas UDP (`src/app/ports.h`)

| Constante          | Porta | Uso                          |
|--------------------|-------|------------------------------|
| `ALERT_PORT`       | 5000  | VictimAlert (drone → equipe) |
| `TEAM_UPDATE_PORT` | 5001  | TeamUpdate (equipe → broadcast) |
| `DRONE_STATUS_PORT`| 5003  | DroneStatus / ACK (drone → equipe) |

### Mensagens (`src/messages/`)

| Arquivo            | Chunk               | Direção              | Campos                                      |
|--------------------|---------------------|----------------------|---------------------------------------------|
| `TeamUpdate.msg`   | `TeamUpdateChunk`   | equipe → broadcast   | teamId, ipAddress, lat, lon, available      |
| `DroneStatus.msg`  | `DroneStatusChunk`  | drone → unicast → equipe | droneId, posX/Y/Z, sentAt               |
| `VictimAlert.msg`  | `VictimAlertChunk`  | drone → unicast → equipe | droneId, msgId, lat, lon, sentAt        |

### Fluxo de comunicação

```
t=5s, 10s, 15s …
  SimpleTeamApp  ──[TeamUpdate bcast:5001]──▶  SimpleDroneApp (×10)
  SimpleDroneApp ──[DroneStatus uni:5003]───▶  SimpleTeamApp
  SimpleTeamApp  loga: RTT e posição do drone

t=aleatório (Exp(20s) por drone)
  SimpleDroneApp  detecta vítima → consulta teamTable (equipe disponível)
  SimpleDroneApp ──[VictimAlert uni:5000]───▶  SimpleTeamApp
  SimpleTeamApp  loga: droneId, lat/lon da vítima, delay

Timeout: drone remove equipe da teamTable se sem TeamUpdate por >15s
```

### Apps (`src/app/`)

**SimpleDroneApp** (`SimpleDroneApp.h/.cc/.ned`)

Parâmetros NED: `myDroneId` (default: nome do módulo), `victimInterval` (default 20s), `teamTimeout` (default 15s)

Sockets:
- `teamSocket` — bound a `TEAM_UPDATE_PORT` (5001), recebe TeamUpdate, setBroadcast(true)
- `ackSocket`  — sem bind, envia DroneStatus unicast para IP de origem do TeamUpdate
- `alertSocket`— sem bind, envia VictimAlert unicast para IP da equipe disponível

Estado interno:
- `teamTable: map<string, TeamEntry>` — ip, lat, lon, available, lastSeen por equipe
- `victimCounter` — sequência para gerar msgId único (`droneId_N`)
- `detectTimer` — dispara `detectVictim()` com intervalo Exp(`victimInterval`)
- `timeoutTimer` — dispara `checkTimeouts()` a cada `teamTimeout`

**SimpleTeamApp** (`SimpleTeamApp.h/.cc/.ned`)

Parâmetros NED: `myTeamId` (default: nome do módulo), `sendInterval` (default 5s)

Sockets:
- `sendSocket`   — sem bind, setBroadcast(true), envia TeamUpdate
- `statusSocket` — bound a `DRONE_STATUS_PORT` (5003), recebe DroneStatus
- `alertSocket`  — bound a `ALERT_PORT` (5000), recebe VictimAlert

Descobre o próprio IP via `L3AddressResolver` → `IInterfaceTable` na inicialização.

### Configuração (`simulations/omnetpp.ini`)

| Config      | Descrição                                              |
|-------------|--------------------------------------------------------|
| `BasicTest` | 10 drones + 1 equipe, FreeSpacePathLoss, 60s           |

Parâmetros chave de `BasicTest`:
- `GaussMarkovMobility`: α=0.75, 10 m/s ±2, angleStdDev=30°, área 0–1000×0–1000×50–150 m
- `StationaryMobility` da equipe em (500, 900, 1.5) m
- Wi-Fi 2.4 GHz, 5 mW, modo ad-hoc, sem roteamento (limitedBroadcast)
- `displayCommunicationRanges = true` no visualizador

## Análise de resultados (`analysis/`)

```bash
# Requer omnetpp.scave (disponível dentro do opp_env) ou CSVs exportados pelo IDE
python3 analysis/process_results.py
# Saída: analysis/figures/comparison.pdf
```

O script `process_results.py` compara três cenários (ainda não implementados no `.ini`):
- `Baseline_FixedPower`, `Baseline_NoCooperation`, `ECHOSAR`

Métricas geradas: PDR (%), latência de entrega (s), energia residual (J).

## Adicionando mensagens novas

1. Criar `src/messages/Foo.msg` com `class FooChunk extends inet::FieldsChunk { ... }`
2. Rodar `make makefiles && make` — o compilador gera `Foo_m.h` e `Foo_m.cc` automaticamente
3. Incluir `"messages/Foo_m.h"` no `.h` da app
4. Usar `makeShared<FooChunk>()` e `peekAtFront<FooChunk>()`

## Namespaces NED

- `src/package.ned` → `package echosar`
- `simulations/package.ned` → `package echosar.simulations`
- Todas as classes C++ ficam em `namespace echosar { ... }`
