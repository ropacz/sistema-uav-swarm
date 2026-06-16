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

O `UserGuide.txt` é o guia oficial do OMNeT++ IDE 6.2.0 (9 231 linhas, não versionado).
Para parâmetros do INET (visualizadores, mobilidade, rádio), consulte diretamente os `.ned` em:
`/Users/rodrigo/omnetpp-workspace/inet-4.5.4/src/inet/`

---

## Project Context

Simulação OMNeT++ de enxame de drones para busca e resgate (SAR) — dissertação de mestrado PPGCAP/UDESC.  
Repositório: `github.com/ropacz/sistema-uav-swarm`

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
O flag `-I.` é crítico: coloca `src/` no include path para que `#include "messages/Foo_m.h"` resolva.

## Run Commands

```bash
./run.sh              # Cmdenv, log INFO, BasicTest run 0
./run.sh --gui        # Qtenv (janela gráfica)
./run.sh --warn       # log WARN (mais rápido)
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
├── visualizer: IntegratedCanvasVisualizer   ← exibe alcance de rádio (mediumVisualizer)
├── configurator: Ipv4NetworkConfigurator
├── radioMedium:  Ieee80211ScalarRadioMedium
├── drone[10]: WirelessHost
│   ├── mobility: GaussMarkovMobility   ← voo 3D, Z: 100–150 m, 11–31 m/s
│   └── app[0]:   SimpleDroneApp
└── team[3]: WirelessHost               ← 3 equipes terrestres (SW/NE/SE da área)
    ├── mobility: StationaryMobility
    └── app[0]:   SimpleTeamApp
```

### Portas UDP (`src/app/ports.h`)

| Constante           | Porta | Direção                               |
|---------------------|-------|---------------------------------------|
| `ALERT_PORT`        | 5000  | VictimAlert  drone → equipe (unicast) |
| `TEAM_UPDATE_PORT`  | 5001  | TeamUpdate   equipe → broadcast       |
| `ACK_PORT`          | 5002  | VictimAck    equipe → drone origem    |
| `DRONE_STATUS_PORT` | 5003  | DroneStatus  drone → equipe           |
| `RELAY_PORT`        | 5004  | VictimAlert  drone → drone (relay)    |

### Mensagens (`src/messages/`)

| Arquivo           | Chunk              | Campos                                          | Tamanho |
|-------------------|--------------------|-------------------------------------------------|---------|
| `TeamUpdate.msg`  | `TeamUpdateChunk`  | teamId, ipAddress, lat, lon, available          | 512 B   |
| `DroneStatus.msg` | `DroneStatusChunk` | droneId, posX/Y/Z, sentAt                      | 512 B   |
| `VictimAlert.msg` | `VictimAlertChunk` | droneId, msgId, **originIp**, lat, lon, sentAt  | 1024 B  |
| `VictimAck.msg`   | `VictimAckChunk`   | msgId, teamId, sentAt                           | 64 B    |

`originIp` em `VictimAlertChunk` carrega o IP do drone que originou o alerta para que a equipe roteie o `VictimAck` direto ao criador, independente do caminho de relay.  
Os arquivos `*_m.{cc,h}` são gerados automaticamente pelo `opp_msgc` — não editar manualmente.

### Fluxo de comunicação completo (15 passos, sem passo 14)

```
t=5s, 10s, 15s …  [passos 2–5]
  SimpleTeamApp[0..2] ──[TeamUpdate bcast:5001]──▶ SimpleDroneApp[0..9]
  SimpleDroneApp      ──[DroneStatus uni:5003]───▶ SimpleTeamApp

t=Exp(20s) por drone  [passos 7–12]
  SimpleDroneApp detecta vítima → msgId = droneId_N → insere em seenAlerts
    ├─ SE tem equipe na teamTable →  VictimAlert uni:5000  → SimpleTeamApp
    └─ SE não tem               →  VictimAlert bcast:5004 → drones vizinhos (relay)
         └─ drone relay: verifica seenAlerts (dedup), repassa até chegar a equipe
  SimpleTeamApp  → dedup em seenAlerts → *** ALERTA *** → VictimAck uni:5002 → drone origem
  SimpleDroneApp (origem) recebe VictimAck → remove de pendingAlerts

Timeout: drone remove equipe da teamTable após 30s sem TeamUpdate  [passo 13]
Store-forward: retry a cada retryInterval=10s, até maxRetries=5  [passo 15]
```

### SimpleDroneApp — estado interno relevante

- `teamTable: map<string, TeamEntry>` — ip, lat, lon, available, lastSeen por equipe
- `seenAlerts: set<string>` — msgIds já vistos (deduplicação de relay)
- `pendingAlerts: vector<PendingAlert>` — alertas pendentes de confirmação (store-forward)
- `victimCounter` — gera msgId único (`droneId_N`)
- Timers: `detectTimer` (Exp 20s), `timeoutTimer` (30s), `retryTimer` (10s)
- Sockets: `teamSocket` (bind 5001), `ackSocket` (sem bind), `alertSocket` (sem bind),
  `relaySocket` (bind 5004), `fwdSocket` (sem bind), `ackRxSocket` (bind 5002)

### SimpleTeamApp — estado interno relevante

- Descobre próprio IP via `L3AddressResolver` → `IInterfaceTable` no `initialize()`
- `seenAlerts: set<string>` — deduplicação de VictimAlerts recebidos via relay
- Sockets: `sendSocket` (broadcast), `statusSocket` (bind 5003), `alertSocket` (bind 5000), `ackTxSocket` (sem bind)

### Parâmetros-chave do BasicTest

| Parâmetro | Valor | Fonte |
|-----------|-------|-------|
| Área | 5000 × 5000 m | [FEA-2024] |
| Altitude drones | 100–150 m | [SCI-2019] |
| Velocidade drones | uniform(11, 31) m/s | [FEA-2024] |
| Potência drone | 20 mW → ~800 m | [OPP-MAN] |
| Potência equipe | 50 mW → ~1260 m | proporcional |
| MAC buffer | 50 pacotes | [SCI-2019] |
| sim-time-limit | 300 s | [FEA-2024] |

Rastreabilidade completa em `docs/params_reference.md`.

### Visualização (`mediumVisualizer`)

Círculos de alcance são do **`mediumVisualizer`**, não do `radioVisualizer`:
```ini
*.visualizer.mediumVisualizer.displayCommunicationRanges = true
```

## Adicionando mensagens novas

1. Criar `src/messages/Foo.msg` com `class FooChunk extends inet::FieldsChunk { ... }`
2. Rodar `make makefiles && make` — gera `Foo_m.h` e `Foo_m.cc` automaticamente
3. Incluir `"messages/Foo_m.h"` no `.h` da app
4. Usar `makeShared<FooChunk>()` e `peekAtFront<FooChunk>()`

## Análise de resultados

```bash
python3 analysis/process_results.py   # gera analysis/figures/comparison.pdf
```

Espera escalares: `alertSent:sum`, `alertReceived:sum`, `alertDeliveryDelay:mean`, `residualEnergyCapacity:last`.  
As configs `Baseline_FixedPower`, `Baseline_NoCooperation`, `ECHOSAR` ainda não estão no `.ini`.

## Namespaces

- `src/package.ned` → `package echosar`
- `simulations/package.ned` → `package echosar.simulations`
- Todo C++ em `namespace echosar { ... }`
