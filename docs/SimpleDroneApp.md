# SimpleDroneApp — guia didático

`src/app/SimpleDroneApp.{h,cc}` implementa a aplicação que roda em cada
`drone[i]` (módulo `AodvRouter`). É ela quem gera alertas sintéticos de
vítima, descobre equipes de resgate na vizinhança, decide para quem enviar
cada alerta e garante a entrega via retransmissão (store-and-forward).

Este documento explica o código função por função. Para o desenho geral do
protocolo (portas, mensagens, os 15 passos do fluxo de comunicação), veja o
`CLAUDE.md` na raiz do projeto — aqui o foco é *como o código implementa*
esse fluxo.

## Papel do drone no protocolo

O drone tem três responsabilidades independentes, cada uma dirigida por um
timer:

1. **Ouvir a equipe** — recebe `TeamUpdate` (posição da equipe) e responde
   com `DroneStatus` (sua própria posição). Isso mantém uma tabela local de
   equipes conhecidas.
2. **Gerar alertas** — periodicamente (evento sintético de "vítima
   encontrada"), decide como notificar a equipe mais próxima, direto ou via
   relay por outro drone.
3. **Garantir entrega** — um alerta enviado não é uma tarefa concluída até
   chegar um `VictimAck`. Sem ACK, o drone reenvia periodicamente até um
   limite de tentativas.

## Estado interno (`SimpleDroneApp.h`)

```cpp
struct TeamEntry {
    std::string ip;
    double posX = 0, posY = 0;
    omnetpp::simtime_t lastSeen;
};

struct PendingAlert {
    std::string msgId;
    std::string droneId;
    std::string originIp;
    double posX, posY;
    omnetpp::simtime_t sentAt;
    int retries = 0;
    std::set<std::string> triedTeams;
};
```

- **`teamTable`** (`map<teamId, TeamEntry>`) — cache local de "onde estão as
  equipes que já ouvi falar". Atualizada a cada `TeamUpdate` recebido;
  entradas somem se a equipe ficar `teamTimeout` (30 s) sem se anunciar
  (`checkTimeouts()`).
- **`seenAlerts`** (`set<msgId>`) — deduplicação. Cada alerta (próprio ou
  relayado) tem um `msgId` único (`droneId_contador`). Evita reprocessar o
  mesmo alerta várias vezes quando ele chega por caminhos diferentes do
  flooding de relay.
- **`pendingAlerts`** (`vector<PendingAlert>`) — a fila de "alertas que eu
  originei e ainda não foram confirmados". É o coração do mecanismo de
  store-and-forward: cada entrada carrega quantas vezes já tentou
  (`retries`) e quais equipes já tentou (`triedTeams`), pra não repetir a
  mesma equipe em retries sucessivos.
- **Três timers**: `alertTimer` (dispara `generateAlert()`, intervalo
  exponencial `alertInterval`), `timeoutTimer` (dispara `checkTimeouts()`,
  a cada `teamTimeout`), `retryTimer` (dispara `retryPending()`, a cada
  `retryInterval`).
- **Seis sockets UDP**, cada um com um papel único — ver tabela abaixo.

### Sockets e por que existem seis

| Socket | Porta (bind) | Direção | Por quê separado |
|---|---|---|---|
| `teamSocket` | `TEAM_UPDATE_PORT` 5001 (bind) | recebe `TeamUpdate` | é o único que escuta broadcast da equipe |
| `ackSocket` | `9000+idx*3` (envio) | envia `DroneStatus` unicast | porta própria por drone — necessária pro `MessageDispatcher` do INET rotear ICMP de erro de volta ao socket certo |
| `alertSocket` | `9001+idx*3` (envio) | envia `VictimAlert` unicast p/ equipe | idem |
| `relaySocket` | `RELAY_PORT` 5004 (bind) | recebe `VictimAlert` de outros drones | broadcast, distinto do `alertSocket` que é unicast |
| `fwdSocket` | `9002+idx*3` (envio) | envia `VictimAlert` relay broadcast | idem porta própria |
| `ackRxSocket` | `ACK_PORT` 5002 (bind) | recebe `VictimAck` da equipe | dedicado, não compartilha com `alertSocket` |

A regra geral: **um socket bind por porta de recepção**, e **um socket de
envio dedicado por tipo de mensagem enviada** (mesmo sem bind explícito em
porta fixa, cada um pega uma porta única `9000+idx*3` etc. pra não colidir
entre os 15 drones).

## Ciclo de vida — `initialize()`

Roda em dois estágios (`INITSTAGE_LOCAL` e `INITSTAGE_APPLICATION_LAYER`),
padrão do INET pra garantir que a pilha de rede já exista antes de abrir
sockets:

- **`INITSTAGE_LOCAL`**: lê os parâmetros do `.ned` (`myDroneId`,
  `alertInterval`, `teamTimeout`, `retryInterval`, `maxRetries`).
- **`INITSTAGE_APPLICATION_LAYER`**: descobre o próprio IP (via
  `L3AddressResolver`/`IInterfaceTable` — precisa pra preencher `originIp`
  no `VictimAlert`, usado pela equipe pra rotear o `VictimAck` de volta),
  abre os seis sockets, e agenda os três timers com o primeiro disparo.

## `handleMessageWhenUp()` — o roteador de eventos

Função central de despacho. Todo evento que chega no módulo passa por
aqui:

```cpp
if (msg->isSelfMessage()) {
    // é um dos três timers → chama a lógica correspondente
} else if (teamSocket.belongsToSocket(msg)) {
    teamSocket.processMessage(msg);   // acaba em socketDataArrived()
} else if (relaySocket.belongsToSocket(msg)) {
    ...
}
```

Mensagens de timer chamam a lógica diretamente e reagendam o próprio timer
(`scheduleAt(simTime() + intervalo, timer)`) — padrão clássico de timer
periódico em OMNeT++. Mensagens de socket são delegadas de volta pro
`UdpSocket`, que invoca o callback `socketDataArrived()`.

## `socketDataArrived()` — despacho por tipo de pacote

```cpp
if (socket == &teamSocket && pkt->hasAtFront<TeamUpdateChunk>())
    handleTeamUpdate(pkt);
else if (socket == &relaySocket && pkt->hasAtFront<VictimAlertChunk>())
    handleVictimAlertRelay(pkt);
else if (socket == &ackRxSocket && pkt->hasAtFront<VictimAckChunk>())
    handleVictimAck(pkt);
```

Cada socket só recebe um tipo de chunk esperado (porque cada um está
ligado a uma porta dedicada a uma única mensagem) — o `hasAtFront<T>()` é
mais uma checagem defensiva do que uma necessidade real de desambiguação.

## `handleTeamUpdate()` — passos 3/4/5

Quando chega um `TeamUpdate`:

1. Atualiza (ou cria) a entrada da equipe em `teamTable` — IP, posição,
   `lastSeen = simTime()`.
2. Responde imediatamente com `DroneStatus` (posição atual do drone) —
   unicast de volta pro IP de origem (`srcAddr`, extraído da tag
   `L3AddressInd` do pacote recebido).

Esse par TeamUpdate→DroneStatus é o "handshake de presença": a equipe sabe
quais drones estão por perto, e os drones sabem quais equipes existem e
onde estão.

## `generateAlert()` — passo 7, o evento central

Disparado por `alertTimer` (intervalo `Exponential(alertInterval)`,
simulando chegadas Poisson de eventos de "vítima encontrada"):

1. Lê a posição atual via `IMobility` do módulo pai.
2. Gera um `msgId` único: `"<myDroneId>_<contador incremental>"`.
3. Marca esse `msgId` como já visto (`seenAlerts`) — importante: assim,
   se esse mesmo alerta voltar por relay de outro drone, é descartado como
   duplicata em vez de reprocessado.
4. Monta um `PendingAlert` (ainda sem confirmação) e chama
   `forwardAlertOnce()` pra fazer a primeira tentativa de envio.
5. Guarda o `PendingAlert` em `pendingAlerts` — a partir daqui, é
   responsabilidade do `retryTimer` continuar tentando até `VictimAck` ou
   `maxRetries`.

## `forwardAlertOnce()` — a decisão de roteamento de aplicação

Essa é a função mais importante do arquivo. Ela decide, para um dado
alerta, **uma única equipe destino** (nunca broadcast pra todas):

```cpp
for (auto& [id, e] : teamTable) {
    if (e.ip.empty() || exclude.count(id)) continue;
    double d2 = (e.posX-posX)² + (e.posY-posY)²;
    if (!best || d2 < bestDist2) { best = &e; bestId = id; bestDist2 = d2; }
}
```

- Percorre `teamTable`, ignora equipes já tentadas para esse alerta
  específico (`exclude`, que é o `triedTeams` do `PendingAlert`).
- Escolhe a **mais próxima** por distância euclidiana ao quadrado (evita
  `sqrt`, já que só a ordenação importa, não o valor).
- **Se achou uma equipe elegível**: `VictimAlert` unicast direto pra ela
  (`alertSocket`, porta `ALERT_PORT`). Conta em `alertsSentDirect`.
- **Se não achou nenhuma** (teamTable vazia, ou todas já tentadas neste
  ciclo de retry): broadcast em `RELAY_PORT` pros drones vizinhos
  (`fwdSocket`). Conta em `alertsSentRelay`. Isso é o mecanismo de
  *descoberta*: um drone sem equipe à vista pede ajuda a quem estiver por
  perto.

Retorna o `teamId` escolhido (ou `""` se foi relay), usado pelo chamador
pra atualizar `triedTeams`.

**Por que só uma equipe por vez, não fan-out pra todas?** Comentário no
código explica: broadcast simultâneo pra múltiplas equipes saturaria o
MAC e disparia múltiplas descobertas de rota AODV ao mesmo tempo. Uma
equipe por tentativa, com fallback sequencial nos retries, é mais estável.

## `handleVictimAlertRelay()` — passos 8/9, o drone como repetidor

Quando outro drone manda um `VictimAlert` em broadcast (porque não achou
equipe por perto):

1. **Dedup** (`seenAlerts.count(msgId)`): se esse `msgId` já foi visto —
   seja porque este drone já relayou antes, seja porque foi ele quem
   originou — descarta sem reprocessar.
2. Caso contrário, marca como visto, conta em `alertsRelayed`, e chama
   `forwardAlertOnce()` de novo — agora do ponto de vista *deste* drone
   (que pode ter uma equipe na `teamTable` dele que o drone original não
   tinha). Sem exclusão de equipes (`noExclude`), porque este drone nunca
   tentou entregar esse alerta antes.

Importante: o relay **preserva os metadados originais** do alerta
(`droneId`, `originIp`, posição, `sentAt`) — o drone relay não se torna
"dono" do alerta, só repassa. Isso garante que o `VictimAck` eventualmente
volte pro `originIp` certo, não pro relay.

## `handleVictimAck()` — passo 12, fechando o ciclo

Ao receber `VictimAck`:

1. Procura o `msgId` correspondente em `pendingAlerts`.
2. Se achou: acumula `simTime() - p.sentAt` em `totalE2EDelay` (RTT do
   ciclo completo, do ponto de vista do drone) e incrementa `alertsAcked`.
3. Remove esse `PendingAlert` de `pendingAlerts` — a partir daqui o
   `retryTimer` não vai mais tentar reenviar esse alerta.

Nota (já documentada no `.cc`): esse RTT é diferente do
`meanDeliveryDelay` medido em `SimpleTeamApp` — lá é o atraso de entrega
**1-via** (drone→equipe), aqui é o **ciclo completo** (alerta→ACK, ida e
volta), do lado do drone.

## `retryPending()` — passo 15, store-and-forward

Disparado a cada `retryInterval` (10 s). Para cada `PendingAlert` ainda
pendente:

1. Incrementa `retries`. Se passou de `maxRetries` (5): desiste, conta em
   `alertsExpired`, remove da lista (não entra em `next`).
2. Senão, verifica se **todas** as equipes conhecidas já foram tentadas
   (`triedTeams` cobre toda `teamTable`) — se sim, **limpa `triedTeams`**
   e recomeça o ciclo de exclusão (permite tentar de novo uma equipe que
   já falhou antes, útil se a `teamTable` mudou ou a equipe ficou
   disponível de novo).
3. Chama `forwardAlertOnce()` de novo, agora excluindo as equipes já
   tentadas — força variar o destino a cada retry em vez de martelar
   sempre a mesma equipe.
4. Reconstrói `pendingAlerts` só com os que sobreviveram
   (`next`/`std::move`).

## `checkTimeouts()` — passo 13, limpeza da tabela

A cada `teamTimeout` (30 s), remove de `teamTable` qualquer equipe cujo
`lastSeen` esteja mais velho que `teamTimeout`. Sem isso, o drone
continuaria tentando enviar alertas pra uma equipe que já saiu de alcance
ou parou de responder.

## `finish()` — métricas gravadas por seed

```cpp
recordScalar("alertsGenerated",  alertsGenerated);
recordScalar("alertsSentDirect", alertsSentDirect);
recordScalar("alertsSentRelay",  alertsSentRelay);
recordScalar("alertsRelayed",    alertsRelayed);
recordScalar("alertsAcked",      alertsAcked);
recordScalar("alertsExpired",    alertsExpired);
recordScalar("totalRetries",     totalRetries);
recordScalar("totalRTT",         totalE2EDelay.dbl());
recordScalar("meanRTT", alertsAcked > 0 ? totalE2EDelay.dbl()/alertsAcked : -1.0);
```

Essas escalares alimentam `analysis/process_results.py`: `alertsAcked /
alertsGenerated` vira a métrica primária **AppACK** (m5), `totalRetries`
vira a métrica de retransmissões por confirmação. Note que
`alertsGenerated` não é necessariamente igual a `alertsAcked +
alertsExpired` — a diferença é o que sobrou em `pendingAlerts` ainda em
retry no instante em que a simulação terminou (não é gravado como scalar
hoje).

## Resumo do fluxo, em uma frase por função

| Função | Faz o quê |
|---|---|
| `initialize()` | lê parâmetros, abre 6 sockets, agenda 3 timers |
| `handleMessageWhenUp()` | roteia timer vs. mensagem de socket |
| `socketDataArrived()` | roteia por tipo de chunk recebido |
| `handleTeamUpdate()` | atualiza `teamTable`, responde `DroneStatus` |
| `generateAlert()` | cria alerta sintético, chama `forwardAlertOnce()`, guarda em `pendingAlerts` |
| `forwardAlertOnce()` | escolhe 1 equipe mais próxima (unicast) ou faz relay (broadcast) |
| `handleVictimAlertRelay()` | dedup + repassa alerta de outro drone |
| `handleVictimAck()` | confirma entrega, remove de `pendingAlerts` |
| `retryPending()` | reenvia pendentes, expira após `maxRetries` |
| `checkTimeouts()` | remove equipes inativas de `teamTable` |
| `finish()` | grava contadores agregados como scalars |
