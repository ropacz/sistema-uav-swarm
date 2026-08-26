# Exemplos de mensagens capturadas no `.elog`

Este documento mostra, com **linhas reais** extraídas de um `.elog` gerado por
este projeto, o que `eventlog_metrics.py` lê para reconstruir atendimento e
perda, e narra um ciclo completo — alerta, degradação do enlace,
reposicionamento pelo Algoritmo do Morcego, nova tentativa, confirmação.

Fonte: `analysis/audit/raw/BA_SmokeTest-0.elog`, gerado com
`--record-eventlog=true` a partir da configuração `BA_SmokeTest`
(`simulations/validation/smoke-tests.ini`) — o cenário determinístico que a
diretriz normativa usa para validar o mecanismo (§28.4, §28.5). Como reproduzir
está no fim deste arquivo.

## Como ler uma linha do `.elog`

Cada linha começa com um código de duas letras e segue em pares
`chave valor`, em ordem não fixa:

| Código | Significa |
| --- | --- |
| `E #` | Início de um evento do simulador; `t` é o instante de simulação |
| `MC` | Criação de módulo — `id`, `c` (classe C++), `n` (nome), `pid` (módulo pai) |
| `CM` | Criação de mensagem — `n` é o nome do pacote |
| `DM` | Descarte de mensagem — `m` é o módulo que a processou e deletou |

## As quatro mensagens do protocolo

| Mensagem | Sentido | Exemplo real (`CM`) |
| --- | --- | --- |
| `TeamUpdate` | equipe → drones (broadcast) | `CM id 88 tid 88 eid 88 etid 88 c omnetpp::cMessage n TeamUpdate pe -1` |
| `DroneStatus` | drone → drones (broadcast) | `CM id 114 tid 114 eid 114 etid 114 c omnetpp::cMessage n DroneStatus pe -1` |
| `VictimAlert` | drone → equipe (unicast) | `CM id 22929 tid 22929 eid 22929 etid 22929 c omnetpp::cMessage n VictimAlert:drone0-smoke-victim-alert-1 pe -1` |
| `VictimAck` | equipe → drone (unicast) | `CM id 23250 tid 23250 eid 23250 etid 23250 c omnetpp::cMessage n VictimAck:drone0-smoke-victim-alert-1 pe -1` |

`TeamUpdate` e `DroneStatus` não carregam identidade no nome — o `.elog` só
precisa distinguir o *tipo* delas. `VictimAlert`/`VictimAck` carregam
`:<alertId>` porque é disso que `eventlog_metrics.py` precisa para juntar
geração, entrega e confirmação do mesmo alerta lógico (ver
`analysis/audit/README.md`, seção "Como o alertId chegou no `.elog`").

Chegada, do lado de quem recebe (`DM`, com o módulo consumidor):

```
DM id 100 tid 88 eid 100 etid 88 c omnetpp::cMessage n TeamUpdate m 56 pe 32
```

`m 56` é `drone0.app[0]` — confirmado pelo mapa de módulos do próprio arquivo:

```
MC id 56  c echosar::DroneApp t echosar.app.DroneApp pid 8  n app[0]
MC id 123 c echosar::TeamApp  t echosar.app.TeamApp   pid 9  n app[0]
```

---

## Fluxo completo: alerta, degradação, reposicionamento, confirmação

Cenário `BA_SmokeTest`: um drone parado, uma equipe que começa visível e
cruza atrás de uma parede de concreto — comentário do próprio `.ini`:
*"A equipe começa visível, cruza a parede em t=23 s e continua atrás dela"*.
A vítima é detectada 5 s depois (`detectionTime = 28s`), já com a equipe
obstruída.

Parâmetros usados abaixo, lidos de `[Config BA_SmokeTest]` e do `[General]`
compartilhado — nenhum valor deste parágrafo foi inventado:

| Parâmetro | Valor |
| --- | --- |
| `ackTimeout` | 2 s |
| `retryInterval` | 10 s |
| `repositionAfterUnackedAttempts` | 1 |
| `team[0].app[0].ackStartTime` | 30 s (a equipe não confirma nada antes disso) |
| `victim[0].detectionTime` | 28 s |

### 1. Drone → equipe: primeira tentativa, enlace já obstruído

```
t=28        CM  VictimAlert:drone0-smoke-victim-alert-1        (criado por drone0.app)
t=28.0103   DM  VictimAlert:drone0-smoke-victim-alert-1  m=team0.app  (consumido)
```

O pacote **chega** — a equipe ainda está ao alcance de rádio — mas
`ackStartTime = 30s` faz `TeamApp` recebê-lo sem confirmar. Do lado do drone,
o prazo `ackDeadline = 28 + ackTimeout = 30s` corre sem resposta.

### 2. Degradação do enlace, sensor, Bat Algorithm — **invisível ao `.elog`**

Em t≈30s a manutenção do `DroneApp` encontra o prazo vencido. Como
`repositionAfterUnackedAttempts = 1`, a primeira tentativa sem ACK já basta
para consultar o sensor. A sequência real do código
(`DroneApp::tryReposition`, `AbstractObstacleSensor::inspect`,
`BatAlgorithm::optimize`, `BaGaussMarkovMobility::moveTo`) é:

```
prazo de ACK vencido
  → consulta ao sensor geométrico (obstáculo confirmado, a parede está no caminho)
  → Bat Algorithm calcula uma posição candidata
  → BaGaussMarkovMobility::moveTo() desloca o drone gradualmente
```

**Nenhuma dessas quatro etapas aparece no `.elog`.** Confirmado neste mesmo
arquivo: a única ocorrência da palavra "sensor" no arquivo inteiro (5 MB) é a
declaração do módulo (`MC id 59 c echosar::AbstractObstacleSensor ...`), não
uma atividade. `moveTo()`/`resumeNormal()` usam `Enter_Method_Silent()` — a
variante *silenciosa* do OMNeT++ que deliberadamente não deixa marca no log de
eventos. Não é falha do parser: essas decisões não geram mensagem nem
movimentação de rede, então não existe linha de `.elog` para capturar. Só o
sinal da aplicação (`victimSensorEvaluated`, `victimBaActivated`,
`victimRepositionEvent`) — o que alimenta o `.sca` — enxerga essa etapa.

A única evidência que o `.elog` oferece dessa etapa é **indireta**: o
intervalo até a próxima tentativa.

### 3. Segunda tentativa — na nova posição

```
t=32.1565   CM  VictimAlert:drone0-smoke-victim-alert-1        (2ª tentativa)
t=32.1598   DM  VictimAlert:drone0-smoke-victim-alert-1  m=team0.app  (consumido)
```

O intervalo entre as duas tentativas é **~4,16 s** — bem abaixo do
`retryInterval = 10s` configurado. Isso por si só já é indício, só pela
cronometria, de que a segunda tentativa não veio do temporizador normal de
retry: veio do reenvio imediato que `DroneApp::handleMessageWhenUp` dispara
assim que o movimento termina (`"Envia ainda na posição escolhida"`, comentário
no código-fonte). Mas é inferência sobre o tempo, não observação direta da
decisão — a diferença entre as duas coisas é exatamente o ponto deste
documento.

### 4. Equipe → drone: confirmação

```
t=32.1598   CM  VictimAck:drone0-smoke-victim-alert-1          (criado por team0.app)
t=32.1621   DM  VictimAck:drone0-smoke-victim-alert-1  m=drone0.app  (consumido)
```

Agora `ackStartTime = 30s` já passou, e a nova posição tem linha de visada —
`TeamApp` confirma na hora. O ciclo fecha: `delivered=1`, `acknowledged=1`,
`retryCount=1` (duas tentativas, uma retransmissão), exatamente como
`analysis/audit/raw/BA_SmokeTest-0-alerts.csv` registra:

```
alertId,victimId,droneId,generationTime,delivered,receivingTeamId,acknowledged,ackTeamId,retryCount
drone0-smoke-victim-alert-1,smoke-victim,drone0,28,1,team0,1,team0,1
```

---

## O que fica provado, e o que não fica

**Provado pelo `.elog`**: os quatro tipos de mensagem existem e trocam de mão
na ordem certa; o pacote chega antes de `ackStartTime`, então a ausência de
ACK não é perda de pacote — é a equipe programada para não responder; a
segunda tentativa chega numa janela de tempo incompatível com o retry normal.

**Não provado pelo `.elog`, só pelo sinal de aplicação (`.sca`)**: que o
sensor de fato confirmou o obstáculo; que o Bat Algorithm foi quem escolheu a
posição; que o deslocamento físico aconteceu (em vez de, por exemplo, um bug
que pulasse a etapa toda). Essa é a mesma lacuna documentada em
`analysis/audit/README.md` para entrega/confirmação — aqui ela aparece de novo,
uma camada acima: o log de rede vê pacotes, não decisões internas do C++.

## Como reproduzir

```bash
opp_env run inet-4.5.4 -w <workspace> --no-isolated -c "
  BIN=<repo>/out/clang-debug/src/sistema_dbg
  NED=<repo>/simulations:<repo>/src:<workspace>/inet-4.5.4/src
  cd <repo>/simulations
  \$BIN -n \$NED -u Cmdenv -c BA_SmokeTest -r 0 --cmdenv-express-mode=true \
    --record-eventlog=true --eventlog-file=<repo>/analysis/audit/raw/BA_SmokeTest-0.elog \
    --output-scalar-file=<repo>/analysis/audit/raw/BA_SmokeTest-0.sca \
    --**.experimentMetrics.alertRecordDirectory=\"<repo>/analysis/audit/raw\"
"
```

Ou abra o resultado no Sequence Chart da IDE do OMNeT++ e filtre por
`alertId=drone0-smoke-victim-alert-1` para ver o mesmo fluxo desenhado.
