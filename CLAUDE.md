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
├── drone[15]: WirelessHost
│   ├── mobility: GaussMarkovMobility   ← voo 3D, Z: 100–150 m, 8–15 m/s
│   └── app[0]:   SimpleDroneApp
└── team[5]: WirelessHost               ← 5 equipes terrestres (4 cantos + centro)
    ├── mobility: RandomWaypointMobility ← patrulha a pé, Z fixo 1.5 m, 0.45–1.4 m/s (cenário inundação)
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
  SimpleTeamApp[0..4] ──[TeamUpdate bcast:5001]──▶ SimpleDroneApp[0..14]
  SimpleDroneApp      ──[DroneStatus uni:5003]───▶ SimpleTeamApp

t=Exp(40s) por drone  [passos 7–12]
  SimpleDroneApp detecta vítima → msgId = droneId_N → insere em seenAlerts
    ├─ 1ª prioridade: equipe DISPONÍVEL na teamTable → VictimAlert uni:5000 → SimpleTeamApp
    ├─ 2ª prioridade: teamTable não vazia mas todas ocupadas →
    │    VictimAlert uni:5000 → qualquer equipe conhecida (equipe ocupada ainda recebe
    │    e confirma; decisão de qual equipe atende é de outra camada, fora de escopo)
    └─ 3ª prioridade: teamTable vazia (nenhuma equipe conhecida) →
         VictimAlert bcast:5004 → drones vizinhos (relay)
         └─ drone relay: verifica seenAlerts (dedup), repassa seguindo a mesma prioridade
  SimpleTeamApp  → dedup em seenAlerts → *** ALERTA *** → calcula busyDuration → available=false
                 → VictimAck uni:5002 → drone origem
  SimpleDroneApp (origem) recebe VictimAck → remove de pendingAlerts

Timeout: drone remove equipe da teamTable após 30s sem TeamUpdate  [passo 13]
Store-forward: retry a cada retryInterval=10s, até maxRetries=5  [passo 15]
```

> **Nota de correção:** `forwardAlertOnce()` originalmente só considerava equipes **disponíveis**
> (2ª prioridade ausente). Quando todas as equipes conhecidas ficavam ocupadas, todo alerta
> caía no relay broadcast — porta que nenhuma equipe escuta — e expirava garantidamente.
> Corrigido adicionando o fallback para "qualquer equipe conhecida" antes do relay.
> PDR do baseline saltou de ~3% para ~30% com essa correção. Detalhes em
> `docs/scenario_reference.md` §8.1.

### SimpleDroneApp — estado interno relevante

- `teamTable: map<string, TeamEntry>` — ip, lat, lon, available, lastSeen por equipe
- `seenAlerts: set<string>` — msgIds já vistos (deduplicação de relay)
- `pendingAlerts: vector<PendingAlert>` — alertas pendentes de confirmação (store-forward)
- `victimCounter` — gera msgId único (`droneId_N`)
- Timers: `detectTimer` (Exp 40s), `timeoutTimer` (30s), `retryTimer` (10s)
- Sockets: `teamSocket` (bind 5001), `ackSocket` (sem bind), `alertSocket` (sem bind),
  `relaySocket` (bind 5004), `fwdSocket` (sem bind), `ackRxSocket` (bind 5002)
- Contadores de métrica: `alertsGenerated`, `alertsSentDirect`, `alertsSentRelay`, `alertsRelayed`,
  `alertsAcked`, `alertsExpired`, `totalRetries`, `totalE2EDelay` — gravados via `recordScalar()` no `finish()`

### SimpleTeamApp — estado interno relevante

- Descobre próprio IP via `L3AddressResolver` → `IInterfaceTable` no `initialize()`
- `seenAlerts: set<string>` — deduplicação de VictimAlerts recebidos via relay
- `available: bool` — false enquanto atende vítima; `attendTimer` restaura para true após
  `busyDuration = distância/teamSpeed + serviceTime` (parâmetros `teamSpeed`, `serviceTime` no NED)
- Sockets: `sendSocket` (broadcast), `statusSocket` (bind 5003), `alertSocket` (bind 5000), `ackTxSocket` (sem bind)
- Contadores de métrica: `alertsReceived`, `teamUpdatesSent`, `droneStatusReceived`, `totalDeliveryDelay`

### Parâmetros-chave do BasicTest

| Parâmetro | Valor | Fonte |
|-----------|-------|-------|
| Área | 5000 × 5000 m | [FEA-2024] |
| numDrones / numTeams | 15 / 5 | calibração de densidade (ver scenario_reference.md) |
| Altitude drones | 100–150 m | [SCI-2019] |
| Velocidade drones | uniform(8, 15) m/s | cenário SAR urbano |
| Velocidade equipes | uniform(0.45, 1.4) m/s | patrulha a pé em área alagada |
| Potência drone | 20 mW → ~800 m | [OPP-MAN] |
| Potência equipe | 50 mW → ~1260 m | proporcional |
| MAC buffer | 50 pacotes | [SCI-2019] |
| victimInterval | 40 s (exponencial) | cenário moderado |
| maxRetries / retryInterval | 5 / 10 s (janela 50 s) | validado empiricamente — aumentar piora o PDR (congestiona MAC) |
| serviceTime / teamSpeed | 120 s / 0.9 m/s | timer de atendimento da equipe |
| sim-time-limit | 300 s | [FEA-2024] |

Rastreabilidade completa em `docs/params_reference.md`.  
Referência consolidada do cenário (topologia, métricas, histórico de correções) em `docs/scenario_reference.md`.

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
python3 analysis/process_results.py   # gera analysis/figures/metrics.{pdf,png}
```

Parser próprio em regex (sem dependência de `omnetpp.scave`) lê os `.sca` em `simulations/results/`.  
Calcula 5 métricas por config: PDR, atraso E2E médio, retransmissões/entrega, overhead de comunicação,
taxa de sucesso AppACK — a partir dos escalares gravados por `SimpleDroneApp`/`SimpleTeamApp` (ver
`docs/scenario_reference.md` §9–10 para a lista completa de escalares e fórmulas).  
As configs `Baseline_FixedPower`, `Baseline_NoCooperation`, `ECHOSAR` ainda não estão no `.ini`.

## Namespaces

- `src/package.ned` → `package echosar`
- `simulations/package.ned` → `package echosar.simulations`
- Todo C++ em `namespace echosar { ... }`
