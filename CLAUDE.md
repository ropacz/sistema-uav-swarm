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
**Roteamento:** AODV (reativo, multi-hop) — nós são `AodvRouter`  
**Ambiente:** gerenciado via `opp_env` (não chamar `omnetpp`/`opp_run` diretamente)

## Build Commands

⚠️ **CRÍTICO:** `run.sh` / `simulations/run` executa o binário **DEBUG** (`src/sistema_dbg`).
O `make` puro gera só o release (`src/sistema`). **Sempre compilar com `MODE=debug`** ou as
mudanças de C++ não aparecem ao rodar (o binário debug fica obsoleto e roda código antigo).

```bash
# Gerar src/Makefile (necessário após adicionar arquivos novos / .msg)
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'make makefiles'

# Build incremental DEBUG (o que run.sh usa) — caminho -C explícito evita "Nothing to be done"
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'make -C /Users/rodrigo/omnetpp-workspace/sistema/src MODE=debug'

# Rebuild completo
opp_env run inet-4.5.4 -w /Users/rodrigo/omnetpp-workspace --no-isolated \
  -c 'make makefiles && make clean && make MODE=debug'
```

Após editar um `.msg`, rode `make makefiles` e em seguida `touch src/app/*.cc` antes do
`make MODE=debug` — garante recompilação dos `.cc` contra os `_m.h` regenerados.

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
├── configurator: Ipv4NetworkConfigurator     ← uma sub-rede 10.0.0.x/24, SEM rotas estáticas
├── radioMedium:  Ieee80211ScalarRadioMedium
├── drone[15]: AodvRouter                ← AdhocHost + AODV (forwarding IP multi-hop)
│   ├── mobility: GaussMarkovMobility   ← altitude constante 100 m, 8–15 m/s
│   ├── aodv:     Aodv                   ← roteamento reativo (RREQ/RREP), activeRouteTimeout=5s
│   └── app[0]:   SimpleDroneApp
└── team[5]: AodvRouter                  ← 5 embarcações de resgate (4 cantos + centro)
    ├── mobility: RandomWaypointMobility ← Z fixo 1.5 m (antena convés), 1.5–3.0 m/s
    ├── aodv:     Aodv
    └── app[0]:   SimpleTeamApp
```

**AODV:** rotas estáticas/subnet/default DESLIGADAS no configurator (o AODV as gerencia sob
demanda). Todos os nós numa única sub-rede `/24` para o RREQ alcançar vizinhos. Habilita
**multi-hop unicast**: o `VictimAck` da equipe volta ao drone origem mesmo a vários saltos
(antes era só 1 salto — alertas que chegavam via relay não eram confirmados). O flooding de
aplicação (`RELAY_PORT`) permanece como mecanismo de DESCOBERTA de equipe para drones distantes.

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
| `TeamUpdate.msg`  | `TeamUpdateChunk`  | teamId, ipAddress, posX, posY, available        | 512 B   |
| `DroneStatus.msg` | `DroneStatusChunk` | droneId, posX/Y/Z, sentAt                      | 512 B   |
| `VictimAlert.msg` | `VictimAlertChunk` | droneId, msgId, **originIp**, posX, posY, sentAt | 1024 B  |
| `VictimAck.msg`   | `VictimAckChunk`   | msgId, teamId, sentAt                           | 64 B    |

> `posX`/`posY` são coordenadas **cartesianas** do playground (m), não geográficas — antes
> chamavam-se `lat`/`lon` (nome enganoso, sempre guardaram X/Y). Renomeadas para clareza.

`originIp` em `VictimAlertChunk` carrega o IP do drone que originou o alerta para que a equipe roteie o `VictimAck` direto ao criador, independente do caminho de relay.  
Os arquivos `*_m.{cc,h}` são gerados automaticamente pelo `opp_msgc` — não editar manualmente.

### Fluxo de comunicação completo (15 passos, sem passo 14)

```
t=5s, 10s, 15s …  [passos 2–5]
  SimpleTeamApp[0..4] ──[TeamUpdate bcast:5001]──▶ SimpleDroneApp[0..14]
  SimpleDroneApp      ──[DroneStatus uni:5003]───▶ SimpleTeamApp

t=Exp(40s) por drone  [passos 7–12]
  SimpleDroneApp detecta vítima → msgId = droneId_N → insere em seenAlerts
    ├─ teamTable não vazia → VictimAlert uni:5000 para TODAS as equipes conhecidas
    │    (independente de available; a que estiver no alcance recebe e confirma)
    └─ teamTable vazia → VictimAlert bcast:5004 → drones vizinhos (relay)
         └─ drone relay: verifica seenAlerts (dedup), repassa com a mesma lógica
  SimpleTeamApp  → dedup em seenAlerts → *** ALERTA *** → calcula busyDuration → available=false
                 → VictimAck uni:5002 → drone origem
  SimpleDroneApp (origem) recebe 1º VictimAck → remove de pendingAlerts
    (ACKs subsequentes de outras equipes são ignorados — pendingAlerts já vazio para esse msgId)

Timeout: drone remove equipe da teamTable após 30s sem TeamUpdate  [passo 13]
Store-forward: retry a cada retryInterval=10s, até maxRetries=5  [passo 15]
```

> **Nota de correção (histórico):** `forwardAlertOnce()` originalmente selecionava uma única
> equipe por ordem de prioridade (disponível → qualquer → relay). Quando todas as equipes
> ficavam ocupadas, o alerta caía no relay broadcast — porta que nenhuma equipe escuta — e
> expirava. PDR saltou de ~3% para ~30% ao adicionar o fallback "qualquer equipe". Depois,
> a seleção foi trocada para "todas as equipes da tabela" (independente de disponibilidade),
> eliminando a dependência da ordem lexicográfica do map. Em rede esparsa (300 m de alcance
> em 4 km²), apenas ~1 equipe está tipicamente ao alcance por drone — o efeito prático é
> equivalente, mas a lógica é mais correta. Detalhes em `docs/scenario_reference.md` §8.1.

### SimpleDroneApp — estado interno relevante

- `teamTable: map<string, TeamEntry>` — ip, posX, posY, available, lastSeen por equipe
- `seenAlerts: set<string>` — msgIds já vistos (deduplicação de relay)
- `pendingAlerts: vector<PendingAlert>` — alertas pendentes de confirmação (store-forward)
- `victimCounter` — gera msgId único (`droneId_N`)
- Timers: `detectTimer` (Exp 40s), `timeoutTimer` (30s), `retryTimer` (10s)
- Sockets: `teamSocket` (bind 5001), `ackSocket` (sem bind), `alertSocket` (sem bind),
  `relaySocket` (bind 5004), `fwdSocket` (sem bind), `ackRxSocket` (bind 5002)
- Contadores de métrica: `alertsGenerated`, `alertsSentDirect`, `alertsSentRelay`, `alertsRelayed`,
  `alertsAcked`, `alertsExpired`, `totalRetries`, `totalRTT`/`meanRTT` (ciclo completo alerta→ACK,
  lado do drone — **não** é o atraso de entrega 1-via) — via `recordScalar()` no `finish()`

### SimpleTeamApp — estado interno relevante

- Descobre próprio IP via `L3AddressResolver` → `IInterfaceTable` no `initialize()`
- `seenAlerts: set<string>` — deduplicação de VictimAlerts recebidos via relay
- `available: bool` — false enquanto atende vítima; `attendTimer` restaura para true após
  `busyDuration = distância/teamSpeed + serviceTime` (parâmetros `teamSpeed`, `serviceTime` no NED)
- Sockets: `sendSocket` (broadcast), `statusSocket` (bind 5003), `alertSocket` (bind 5000), `ackTxSocket` (sem bind)
- Contadores de métrica: `alertsReceived`, `alertsReceivedAvailable`/`alertsReceivedBusy` (alerta chegou
  com equipe livre vs ocupada), `teamUpdatesSent`, `droneStatusReceived`, `totalDeliveryDelay`/`meanDeliveryDelay`
  (atraso de entrega **1-via**, drone→equipe — esta é a métrica de "atraso fim-a-fim" reportada)

### Parâmetros-chave do BasicTest

| Parâmetro | Valor | Fonte |
|-----------|-------|-------|
| Área | 2000 × 2000 m | cenário SAR realista (k̄≈2,0 para r=300 m em 4 km²) |
| numDrones / numTeams | 30 / 10 | k̄≈2,0 para r=300 m em 4 km² — [bettstetter2002minimum] |
| Altitude drones | **100 m constante** | [garg2022directed] |
| Velocidade drones | uniform(8, 15) m/s | cenário SAR urbano |
| Velocidade embarcações | uniform(1.5, 3.0) m/s | embarcações de resgate em área alagada |
| Potência (todos) | 2,9 mW → ~300 m @ 2,4 GHz (IEEE 802.11n/s) | [panda2019design] |
| MAC buffer | 50 pacotes | [SCI-2019] |
| victimInterval | 40 s (exponencial) | cenário moderado |
| maxRetries / retryInterval | 5 / 10 s (janela 50 s) | validado empiricamente — aumentar piora o PDR (congestiona MAC) |
| serviceTime / teamSpeed | 120 s / 2.25 m/s | ponto médio da faixa de embarcação |
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
Calcula **6 métricas** por config (média ± desvio entre seeds):
1. **PDR** canônico (`alertsReceived/alertsGenerated`) — chegada ao destino;
2. **Atraso de entrega 1-via** (drone→equipe), ponderado (`Σ totalDeliveryDelay / Σ alertsReceived`);
3. Retransmissões por confirmação; 4. Overhead de alerta;
5. **AppACK** (`alertsAcked/alertsGenerated`) — ciclo completo confirmado;
6. **Alertas a equipe DISPONÍVEL** (`alertsReceivedAvailable/alertsReceived`) — fração que chegou com equipe livre.

PDR e AppACK são distintos: PDR mede chegada ao destino, AppACK mede ciclo completo. **Com AODV
os dois praticamente coincidem** (gap < 0,5 p.p.), pois o `VictimAck` volta multi-hop — antes o
gap era ~2 p.p. por ACK single-hop. Ver `docs/scenario_reference.md` §10/§13 para fórmulas e análise.  
As configs `Baseline_FixedPower`, `Baseline_NoCooperation`, `ECHOSAR` ainda não estão no `.ini`.

## Namespaces

- `src/package.ned` → `package echosar`
- `simulations/package.ned` → `package echosar.simulations`
- Todo C++ em `namespace echosar { ... }`
