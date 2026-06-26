# CLAUDE.md

Guia para o Claude Code (claude.ai/code) trabalhar neste repositório.
Simulação OMNeT++/INET de enxame de drones para busca e resgate (SAR) — dissertação de
mestrado PPGCAP/UDESC. Repo: `github.com/ropacz/sistema-uav-swarm`.

---

## 🗺️ Mapa rápido

**Onde está cada coisa neste arquivo:**
[Ferramentas de consulta](#ferramentas-de-consulta) · [Stack](#stack) ·
[Build](#build) · [Run](#run) · [Arquitetura](#arquitetura) ·
[Parâmetros-chave](#parâmetros-chave-config-basictest) · [Tarefas comuns](#tarefas-comuns) ·
[Namespaces](#namespaces) · [Notas históricas](#notas-históricas)

**Arquivos-chave do código:**

| Caminho | O que é |
|---|---|
| `src/app/SimpleDroneApp.{h,cc,ned}` | App do drone (detecção, alerta, relay, retry, métricas) |
| `src/app/SimpleTeamApp.{h,cc,ned}` | App da equipe (beacon, ACK, ocupação, métricas) |
| `src/app/ports.h` | Constantes de portas UDP |
| `src/messages/*.msg` | Definições de chunks (geram `*_m.{h,cc}`) |
| `simulations/BasicNetwork.ned` | Topologia da rede |
| `simulations/omnetpp.ini` | Todos os configs (parâmetros, cenários) |
| `simulations/obstacles.xml` | Blocos físicos (configs com obstáculo) |
| `analysis/process_results.py` | Parser dos `.sca` → figuras de métricas |
| `run.sh` / `simulations/run` | Lançam a simulação (binário **debug**) |
| `search_guide.py` | Busca no UserGuide do OMNeT++ |

**Documentação de apoio (`docs/`):**

| Doc | Conteúdo |
|---|---|
| `docs/scenario_reference.md` | Referência consolidada: topologia, métricas, fórmulas, histórico de correções |
| `docs/bat_algorithm_plan.md` | Plano de implementação do reposicionamento por Algoritmo do Morcego (BA) — **ainda não implementado** |

> `docs/params_reference.md` é citado em discussões antigas mas **não existe** — não buscar.

**Configs disponíveis no `omnetpp.ini`** (use com `./run.sh -c <Config>`):

| Config | Para quê |
|---|---|
| `BasicTest` | Cenário principal — 15 UAVs, 5 equipes, 5 km², 900 s |
| `BasicTest_Piloto` | Igual ao BasicTest mas 300 s (validação rápida, não é resultado final) |
| `Cenario_SemObstaculos` | Baseline FANET sem obstáculos |
| `Cenario_ComObstaculos` | Com obstáculos urbanos (`DielectricObstacleLoss` + `obstacles.xml`) |
| `Cenario_Favoravel` | Teto de desempenho — 20 UAVs, 8 equipes, alertInterval 80 s |
| `Cenario_Intermediario` | ComObstaculos com alertInterval 40 s |
| `Cenario_Degradado` | Rede fragmentada — 10 UAVs, 3 equipes, alertInterval 20 s |
| `SmokeTest_Beacons` | Regressão: 1 UAV + 2 equipes estáticas, verifica recepção de beacon |
| `BasicTest_Visual` | Visualização Qtenv, 60 s, sem círculos de alcance |

---

## Ferramentas de consulta

**Dúvidas de configuração OMNeT++** (Qtenv, INI/NED editor, launch): consulte o UserGuide.

```bash
python3 search_guide.py "sua dúvida aqui"        # top-3 seções relevantes
python3 search_guide.py -n 5 "sua dúvida aqui"   # top-5
python3 search_guide.py --list-sections          # índice completo
```

`UserGuide.txt` = guia oficial do OMNeT++ IDE 6.2.0 (9 231 linhas, não versionado).

**Parâmetros do INET** (visualizadores, mobilidade, rádio, ambiente físico): consulte os
`.ned`/`.h` diretamente em `/Users/rodrigo/omnetpp-workspace/inet-4.5.4/src/inet/`.

---

## Stack

- **OMNeT++ 6.2.0 + INET Framework 4.5.4**
- **Roteamento:** AODV (reativo, multi-hop) — todos os nós são `AodvRouter`
- **Ambiente:** gerenciado via `opp_env` — **nunca** chamar `omnetpp`/`opp_run` direto

---

## Build

> ⚠️ **CRÍTICO:** `run.sh` / `simulations/run` executa o binário **DEBUG** (`src/sistema_dbg`).
> `make` puro gera só o release (`src/sistema`). **Sempre compile com `MODE=debug`**, senão
> mudanças de C++ não aparecem ao rodar (o binário debug fica obsoleto e roda código antigo).

```bash
# Gerar src/Makefile (após adicionar arquivos novos / .msg)
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'make makefiles'

# Build incremental DEBUG (o que run.sh usa) — -C explícito evita "Nothing to be done"
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'make -C /Users/rodrigo/omnetpp-workspace/sistema/src MODE=debug'

# Rebuild completo
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'make makefiles && make clean && make MODE=debug'
```

- Após editar um `.msg`: `make makefiles`, depois `touch src/app/*.cc`, depois `make MODE=debug`
  — garante recompilação dos `.cc` contra os `_m.h` regenerados.
- O Makefile usa `-I. -I$INET/src -L$INET/src -lINET_dbg` via `opp_makemake --deep`.
  O flag `-I.` é crítico: põe `src/` no include path para `#include "messages/Foo_m.h"` resolver.

---

## Run

```bash
./run.sh              # Cmdenv, log INFO, BasicTest run 0
./run.sh --gui        # Qtenv (janela gráfica)
./run.sh --warn       # log WARN (mais rápido)
./run.sh --build      # compila antes de rodar
./run.sh -c OutraConf # config diferente (ver tabela de Configs no Mapa rápido)
./run.sh -r 2         # seed/run 2
```

`simulations/run` invoca `../src/sistema_dbg` (binário debug com INET linkado).
Use sempre esse script — `opp_run` não tem INET linkado e falha.

---

## Arquitetura

### Hierarquia de módulos

```
BasicNetwork (BasicNetwork.ned)
├── visualizer: IntegratedCanvasVisualizer   ← exibe alcance de rádio (mediumVisualizer)
├── physicalEnvironment: PhysicalEnvironment  ← obstáculos físicos (configs ComObstaculos)
├── configurator: Ipv4NetworkConfigurator     ← uma sub-rede 10.0.0.x/24, SEM rotas estáticas
├── radioMedium:  Ieee80211ScalarRadioMedium
├── drone[15]: AodvRouter                ← AdhocHost + AODV (forwarding IP multi-hop)
│   ├── mobility: GaussMarkovMobility   ← altitude 80–120 m, velocidade 8–15 m/s
│   ├── aodv:     Aodv                   ← roteamento reativo (RREQ/RREP), activeRouteTimeout=5s
│   └── app[0]:   SimpleDroneApp
└── team[5]: AodvRouter                  ← 5 embarcações de resgate (4 cantos + centro)
    ├── mobility: RandomWaypointMobility ← Z fixo 1.5 m (antena convés), 1.5–3.0 m/s
    ├── aodv:     Aodv
    └── app[0]:   SimpleTeamApp
```

> Contagens (`drone[15]`, `team[5]`) e mobilidade refletem o `Config BasicTest`. Outros
> configs sobrescrevem `numDrones`/`numTeams`/velocidades (ver tabela de Configs).

**AODV:** rotas estáticas/subnet/default DESLIGADAS no configurator (o AODV as gerencia sob
demanda). Todos os nós numa única sub-rede `/24` para o RREQ alcançar vizinhos. Habilita
**multi-hop unicast**: o `VictimAck` da equipe volta ao drone origem mesmo a vários saltos.
O flooding de aplicação (`RELAY_PORT`) permanece como mecanismo de DESCOBERTA de equipe
para drones distantes.

### Portas UDP (`src/app/ports.h`)

| Constante           | Porta | Direção                               |
|---------------------|-------|---------------------------------------|
| `ALERT_PORT`        | 5000  | VictimAlert  drone → equipe (unicast) |
| `TEAM_UPDATE_PORT`  | 5001  | TeamUpdate   equipe → broadcast       |
| `ACK_PORT`          | 5002  | VictimAck    equipe → drone origem    |
| `DRONE_STATUS_PORT` | 5003  | DroneStatus  drone → equipe           |
| `RELAY_PORT`        | 5004  | VictimAlert  drone → drone (relay)    |

> Sockets de **envio** usam portas dinâmicas por nó (drones 9000–9059, equipes 9100–9115)
> para o MessageDispatcher do INET rotear ICMPs de volta ao socket certo.

### Mensagens (`src/messages/`)

| Arquivo           | Chunk              | Campos                                          | Tamanho |
|-------------------|--------------------|-------------------------------------------------|---------|
| `TeamUpdate.msg`  | `TeamUpdateChunk`  | teamId, ipAddress, posX, posY, available        | 512 B   |
| `DroneStatus.msg` | `DroneStatusChunk` | droneId, posX/Y/Z, sentAt                       | 512 B   |
| `VictimAlert.msg` | `VictimAlertChunk` | droneId, msgId, **originIp**, posX, posY, sentAt | 1024 B |
| `VictimAck.msg`   | `VictimAckChunk`   | msgId, teamId, sentAt                           | 64 B    |

- `posX`/`posY` = coordenadas **cartesianas** do playground (m), não geográficas (antes
  `lat`/`lon`, nome enganoso — sempre guardaram X/Y).
- `originIp` em `VictimAlertChunk` = IP do drone que originou o alerta, para a equipe rotear
  o `VictimAck` direto ao criador, independente do caminho de relay.
- Os `*_m.{cc,h}` são gerados pelo `opp_msgc` — **não editar à mão**.

### Fluxo de comunicação (15 passos, sem passo 14)

```
periódico (sendInterval=1s, com jitter)  [passos 2–5]
  SimpleTeamApp[0..4] ──[TeamUpdate bcast:5001]──▶ SimpleDroneApp[0..14]
  SimpleDroneApp      ──[DroneStatus uni:5003]───▶ SimpleTeamApp

t = Exp(40s) por drone  [passos 7–12]
  SimpleDroneApp detecta vítima → msgId = droneId_N → insere em seenAlerts
    ├─ teamTable não vazia → VictimAlert uni:5000 p/ a melhor equipe conhecida
    │    (disponível+próxima → qualquer+próxima; exclui equipes já tentadas no retry)
    └─ teamTable vazia → VictimAlert bcast:5004 → drones vizinhos (relay)
         └─ drone relay: dedup em seenAlerts, repassa com a mesma lógica
  SimpleTeamApp  → dedup → *** ALERTA *** → calcula busyDuration → available=false
                 → VictimAck uni:5002 → drone origem (via originIp, multi-hop)
  SimpleDroneApp (origem) recebe 1º VictimAck → remove de pendingAlerts (cancela retries)

Timeout: drone remove equipe da teamTable após 30s sem TeamUpdate  [passo 13]
Store-forward: retry a cada retryInterval=10s, até maxRetries=5  [passo 15]
```

### SimpleDroneApp — estado interno

- `teamTable: map<string, TeamEntry>` — ip, posX, posY, available, lastSeen por equipe
- `seenAlerts: set<string>` — msgIds já vistos (dedup de relay)
- `pendingAlerts: vector<PendingAlert>` — alertas aguardando confirmação (store-forward)
- `victimCounter` — gera msgId único (`droneId_N`)
- Timers: `detectTimer` (Exp 40s), `timeoutTimer` (30s), `retryTimer` (10s)
- Sockets: `teamSocket` (bind 5001), `ackSocket`, `alertSocket`, `relaySocket` (bind 5004),
  `fwdSocket`, `ackRxSocket` (bind 5002)
- Métricas (`recordScalar` no `finish()`): `alertsGenerated`, `alertsSentDirect`,
  `alertsSentRelay`, `alertsRelayed`, `alertsAcked`, `alertsExpired`, `totalRetries`,
  `totalRTT`/`meanRTT` (ciclo completo alerta→ACK, lado do drone — **não** é o atraso 1-via)

### SimpleTeamApp — estado interno

- Descobre próprio IP via `L3AddressResolver` → `IInterfaceTable` no `initialize()`
- `seenAlerts: set<string>` — dedup de VictimAlerts recebidos via relay (ACK idempotente)
- `available: bool` — false enquanto atende; `attendTimer` restaura após
  `busyDuration = distância/teamSpeed + serviceTime`
- Sockets: `sendSocket` (broadcast), `statusSocket` (bind 5003), `alertSocket` (bind 5000),
  `ackTxSocket`
- Métricas: `alertsReceived`, `alertsReceivedAvailable`/`alertsReceivedBusy`,
  `teamUpdatesSent`, `droneStatusReceived`, `totalDeliveryDelay`/`meanDeliveryDelay`
  (atraso de entrega **1-via** drone→equipe — é a métrica de "atraso fim-a-fim" reportada)

---

## Parâmetros-chave (`Config BasicTest`)

> Fonte de verdade = `simulations/omnetpp.ini`. Esta tabela é um resumo; em caso de
> divergência, o `.ini` vence.

| Parâmetro | Valor (BasicTest) | Fonte/nota |
|-----------|-------------------|------------|
| Área | 5000 × 5000 m (25 km²) | `bgb=5000,5000` |
| numDrones / numTeams | 15 / 5 | k̄ ≈ 1,1 (r≈788 m, 25 km²) — rede esparsa |
| Altitude drones | 80–120 m (GaussMarkov) | `constraintAreaMinZ/MaxZ` |
| Velocidade drones | uniform(8, 15) m/s | cenário SAR |
| Velocidade equipes | uniform(1.5, 3.0) m/s | embarcações em área alagada |
| Potência | drone 20 mW / equipe 50 mW | [panda2019uavwifi] |
| Rádio | 2,4 GHz, 20 MHz, 6 Mbps, sens. −85 dBm | IEEE 802.11 ad hoc |
| Path loss | FreeSpacePathLoss | sem obstáculo; `DielectricObstacleLoss` nos configs ComObstaculos |
| MAC buffer | 50 pacotes | [SCI-2019] |
| victimInterval | 40 s (exponencial) | cenário moderado |
| maxRetries / retryInterval | 5 / 10 s | validado empíricamente (aumentar piora PDR — congestiona MAC) |
| serviceTime / teamSpeed | 120 s / 2.25 m/s | ponto médio da faixa de embarcação |
| sim-time-limit | **900 s** (Piloto: 300 s) | avaliação principal |

Topologia, métricas e histórico em `docs/scenario_reference.md`.

---

## Tarefas comuns

### Adicionar uma mensagem nova
1. Criar `src/messages/Foo.msg` com `class FooChunk extends inet::FieldsChunk { ... }`
2. `make makefiles && make MODE=debug` — gera `Foo_m.{h,cc}` automaticamente
3. Incluir `"messages/Foo_m.h"` no `.h` da app
4. Usar `makeShared<FooChunk>()` e `peekAtFront<FooChunk>()`

### Analisar resultados
```bash
python3 analysis/process_results.py   # gera analysis/figures/metrics.{pdf,png}
```
Parser próprio em regex (sem `omnetpp.scave`) lê os `.sca` em `simulations/results/`.
Calcula **6 métricas** por config (média ± desvio entre seeds):
1. **PDR** canônico (`alertsReceived/alertsGenerated`) — chegada ao destino
2. **Atraso de entrega 1-via** (drone→equipe), ponderado (`Σ totalDeliveryDelay / Σ alertsReceived`)
3. Retransmissões por confirmação
4. Overhead de alerta
5. **AppACK** (`alertsAcked/alertsGenerated`) — ciclo completo confirmado
6. **Alertas a equipe DISPONÍVEL** (`alertsReceivedAvailable/alertsReceived`)

PDR e AppACK são distintos (chegada × ciclo completo), mas com AODV praticamente coincidem
(gap < 0,5 p.p.), pois o `VictimAck` volta multi-hop. Fórmulas em `docs/scenario_reference.md`.

### Visualização — círculos de alcance
São do **`mediumVisualizer`**, não do `radioVisualizer`:
```ini
*.visualizer.mediumVisualizer.displayCommunicationRanges = true
```

---

## Namespaces

- `src/package.ned` → `package echosar`
- `simulations/package.ned` → `package echosar.simulations`
- Todo C++ em `namespace echosar { ... }`

---

## Notas históricas

> Correções já aplicadas — contexto para não regredir. Detalhes em `docs/scenario_reference.md`.

- **Seleção de equipe em `forwardAlertOnce()`:** originalmente escolhia uma equipe por
  prioridade (disponível → qualquer → relay). Com todas ocupadas, o alerta caía no relay
  broadcast (porta que nenhuma equipe escuta) e expirava. Adicionar o fallback "qualquer
  equipe" subiu o PDR de ~3% para ~30%. Em rede esparsa, ~1 equipe ao alcance por drone, então
  o efeito é equivalente a "todas as equipes", mas a lógica atual (melhor equipe, com exclusão
  de já-tentadas no retry) é mais correta. Ver `docs/scenario_reference.md` §8.1.
- **`beaconJitter=0`** causa colisões MAC (todas as equipes transmitem em `t=sendInterval`
  simultaneamente → PDR ~3%). `SimpleTeamApp` aborta com `cRuntimeError` se for 0.
- **`originIp` + AODV multi-hop** fechou o gap PDR×AppACK: antes o `VictimAck` era single-hop
  e alertas que chegavam via relay não eram confirmados.
