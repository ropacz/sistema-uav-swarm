# Guia de leitura — `analysis/examples/simulacao_completa.log`

Log gerado com:

```bash
./run.sh --info -c SmokeTest_Beacons -r 0 > analysis/examples/simulacao_completa.log
```

Cenário `SmokeTest_Beacons`: 1 drone parado + 2 equipes, 15s de simulação.
`--info` liga o nível mais verboso — **todas** as camadas (aplicação, IP,
ARP, AODV, MAC, PHY) imprimem, não só `[DRONE]`/`[TEAM]`. Resultado: 16.918
linhas para 15s simulados. Este guia percorre o arquivo em blocos, na ordem
em que aparecem, explicando o que cada tipo de linha significa.

> Nota sobre os códigos estranhos tipo `[1m`, `[2m`, `[3m` que aparecem em
> algumas linhas: são sequências ANSI de formatação (negrito/cor) que o
> OMNeT++ imprime para deixar bonito num terminal colorido. Como o log foi
> redirecionado para arquivo (`>`), elas ficam gravadas como texto cru em
> vez de virar cor — pode ignorar, fazem parte do nome do campo seguinte.

---

## Bloco 0 — Cabeçalho de execução (linhas 1–18)

```
>>> Config=SmokeTest_Beacons  seeds=0  UI=Cmdenv  log=INFO
INFO Using specified projects [inet-4.5.4] with effective projects [inet-4.5.4, omnetpp-6.2.0] in workspace ...
...
OMNeT++ Discrete Event Simulation  (C) 1992-2025 Andras Varga, OpenSim Ltd.
Version: 6.2.0, build: 250714-83e173e93a, edition: Academic Public License -- NOT FOR COMMERCIAL USE
...
Loading NED files from .:  2
Loading NED files from ../src:  3
Loading NED files from /Users/rodrigo/omnetpp-workspace/inet-4.5.4/src:  1187
...
Preparing for running configuration SmokeTest_Beacons, run #0...
Assigned runID=SmokeTest_Beacons-0-20260811-21:30:02-70286
Setting up network "echosar.simulations.BasicNetwork"...
```

Não é log da simulação em si — é o `opp_env` entrando no ambiente Nix,
depois o motor OMNeT++ carregando os `.ned` (do projeto, do INET) e
montando a rede `BasicNetwork` a partir do `omnetpp.ini`. O `runID` é o
identificador único dessa execução (usado nos arquivos `.sca`/`.vec`).

---

## Bloco 1 — Inicialização do cenário (linhas 19–4813)

A maior parte "morta" do arquivo — quase 4.800 linhas antes do primeiro
evento de verdade. Duas categorias:

```
Initializing channel BasicNetwork.drone[0].radioIn[0].channel, stage 0
Initializing channel BasicNetwork.drone[0].wlan[0].radioIn.channel, stage 0
...
Initializing module BasicNetwork.visualizer.mediumVisualizer, stage 0
Initializing module BasicNetwork.radioMedium, stage 0
Initializing module BasicNetwork.configurator, stage 0
```

**`Initializing channel ...`**: cada conexão interna entre submódulos (fio
"ideal" dentro de um nó, tipo app→udp→ip) é inicializada uma vez. Puro
overhead de setup do INET, sempre igual entre execuções.

**`Initializing module ..., stage N`**: o OMNeT++ inicializa módulos em
**múltiplos estágios** (`stage 0`, `stage 1`, ...) porque alguns módulos
dependem de outros já estarem prontos (ex: a interface IP precisa existir
antes do roteador configurar rotas). Isso se repete para cada um dos ~20
nós × dezenas de submódulos (rádio, MAC, IP, ARP, AODV, mobilidade...).

**Não precisa ler linha por linha.** Se for procurar algo aqui, é só para
conferir que um parâmetro específico foi aplicado (ex: `grep "power ="` pra
ver a potência de transmissão configurada).

---

## Bloco 2 — O motor de eventos discretos (linha 4814 em diante)

```
** Event #1  t=0  0% completed  (0% total)   BasicNetwork.drone[0].udp (Udp, id=51)
```

A partir daqui o log muda de forma: cada linha `** Event #N` é um
**cabeçalho de evento** do simulador — marca que o motor está processando o
evento número `N`, no instante de simulação `t=...` segundos, no módulo
indicado no fim da linha. Todo `[INFO]`/`[WARN]` que aparece **depois** desse
cabeçalho e **antes do próximo** pertence a esse evento/módulo/instante.

> Isso é a base do parser `analysis/parse_message_log.py`: como o simulador
> é **single-threaded** (um evento de cada vez, em ordem), basta guardar o
> último `t=` visto e aplicá-lo às linhas `[INFO]` seguintes — sem
> ambiguidade possível.

`0% completed (0% total)` é o progresso da simulação (tempo simulado
decorrido / `sim-time-limit`), atualizado a cada evento.

---

## Bloco 3 — Aplicação: `TeamUpdate` (o beacon da equipe)

```
[INFO]	[TEAM team0] TeamUpdate broadcast (ip=10.0.0.2)
...
[INFO]	[DRONE drone[0]] tabela: team0 ip=10.0.0.2
```

A equipe transmite periodicamente sua posição/IP em broadcast — é assim
que um drone **descobre** que uma equipe existe, sem nenhuma configuração
estática. `tabela: team0 ip=10.0.0.2` é o drone confirmando que atualizou
sua `teamTable` interna.

---

## Bloco 4 — Rádio (PHY): como uma transmissão aparece

```
[INFO]	Transmission started: (inet::physicallayer::WirelessSignal)TeamUpdate (94 us 207 B)
	(inet::Packet)TeamUpdate (207 B) ... as Ieee80211ScalarTransmission,
	mode = { Ieee80211ErpOfdmMode }, power = 50 mW, bitrate = 24 Mbps,
	centerFrequency = 2.412 GHz, bandwidth = 20 MHz,
	startPosition = (2550, 2500, 1.5) m, endPosition = (2550, 2500, 1.5) m
```

Toda vez que um quadro sai pelo ar, o rádio simulado imprime a transmissão
com física completa: potência (`power`), taxa (`bitrate`), banda
(`bandwidth`), duração e **posição exata** de quem transmite (usada para
calcular perda de percurso). Do lado de quem recebe:

```
[INFO]	Reception started: attempting (inet::physicallayer::WirelessSignal)TeamUpdate ...
	power = ... pW, transmissionId = 0, receiverId = 1, ...
[INFO]	bpsk snr=... ber=...
[INFO]	16-Qam snr=... ber=...
[INFO]	Reception ended: successfully for (...) ...
[INFO]	Sending up (inet::Packet)TeamUpdate (192 B) ...
```

`snr`/`ber` são calculados **para cada modulação suportada** (BPSK,
16-QAM...) — a relação sinal-ruído e a taxa de erro de bit que aquele link
teria com aquela modulação, usadas para decidir se o quadro é decodificado
com sucesso. `Reception ended: successfully` = passou; se a SNR fosse baixa
demais, apareceria `Reception ended: unsuccessfully` (ou nada — quadro nem
detectado).

---

## Bloco 5 — Enlace (MAC): handshake DATA/ACK 802.11

```
[INFO]	Received frame from PHY: (inet::Packet)TeamUpdate (192 B) ...
[INFO]	Processing lower frame: TeamUpdate
...
[INFO]	Frame sequence finished.
[INFO]	Channel released.
```

O IEEE 802.11 exige confirmação em nível de enlace para **todo** quadro
unicast: quem envia espera um `WlanAck` do MAC do destinatário antes de
liberar o canal. Isso é **diferente** do `VictimAck` da aplicação — este
ACK é interno ao rádio, some em microssegundos:

```
[INFO]	Transmission started: (...)WlanAck (34 us 27 B) ... power = 50 mW, bitrate = 24 Mbps
[INFO]	Reception started: attempting (...)WlanAck ...
[INFO]	Reception ended: successfully for (...)WlanAck ...
```

Acontece até para um simples `TeamUpdate` — o `Frame sequence finished` +
`Channel released` marcam o fim desse handshake e a liberação do canal
rádio para o próximo nó transmitir.

---

## Bloco 6 — Roteamento (AODV): descoberta de rota sob demanda

Trecho real (drone `10.0.0.1` descobrindo rota para a equipe `10.0.0.2`
pela primeira vez, para poder responder um `DroneStatus`):

```
[INFO]	Finding route for source <unspec> with destination 10.0.0.2
[INFO]	Missing route for destination 10.0.0.2
[INFO]	Starting route discovery with originator 10.0.0.1 and destination 10.0.0.2
[INFO]	Sending a Route Request with target 10.0.0.2 and TTL= 2
   ...
[INFO]	AODV Route Request arrived with source addr: 10.0.0.1 originator addr: 10.0.0.1 destination addr: 10.0.0.2
[INFO]	add route ??? 10.0.0.1/32 gw:10.0.0.1 metric:1 if:wlan0 isActive = 1, ... lifetime = 5.596...
[INFO]	I am the destination node for which the route was requested
[INFO]	Sending Route Reply to 10.0.0.1
   ...
[INFO]	Routing (inet::Packet)aodv::Rrep (48 B) ... with destination = 10.0.0.1, output interface = wlan0, next hop address = 10.0.0.1
```

Leitura linha a linha:

1. **`Missing route`** — o nó não tem entrada na tabela de roteamento para
   aquele IP, dispara descoberta.
2. **RREQ** (*Route Request*) sai em broadcast com TTL limitado.
3. Quem recebe o RREQ e **é** o destino grava a rota reversa
   (`add route ...`) e responde direto com **RREP** (*Route Reply*); quem
   não é, encaminharia adiante (`Forwarding the Route Request message`).
4. O RREP volta, o originador grava a rota e agora pode rotear o pacote de
   dados de verdade, com `next hop` definido.

Isso só acontece **uma vez** por par origem/destino, enquanto a rota
estiver ativa (`activeRouteTimeout=5s` no `omnetpp.ini`). Depois disso, os
pacotes seguintes só aparecem como `Active route found`, sem handshake de
novo.

---

## Bloco 7 — ARP: resolução de endereço MAC

**Camada separada do AODV** — ter rota AODV ativa não basta para
transmitir; o quadro 802.11 precisa de um endereço MAC de destino:

```
[INFO]	Sending (inet::Packet)arpREQ (28 B) (inet::ArpPacket) ARP req: 10.0.0.1=? (s=10.0.0.2(0A-AA-00-00-00-02)) to network protocol.
...
[INFO]	Received (inet::Packet)arpREQ (28 B) ... from network protocol.
[INFO]	Sending (inet::Packet)arpREPLY (28 B) (inet::ArpPacket) ARP reply: 10.0.0.1=0A-AA-00-00-00-01 ...
...
[INFO]	ARP resolution completed for 10.0.0.1. Sending 1 waiting packets from the queue
```

**ARP request/reply**: quando um nó precisa mandar algo a um IP sem MAC
resolvido, transmite um *ARP request* em broadcast (“quem tem esse IP?”);
o dono responde com *ARP reply* unicast contendo o MAC. Enquanto isso, os
pacotes de dados para aquele destino ficam **enfileirados** — assim que a
resposta chega, a fila inteira é liberada de uma vez
(`Sending N waiting packets from the queue`).

> Se o ARP reply não chegar depois de 3 tentativas, aparece `ARP timeout,
> max retry count 3 reached` seguido de `ARP resolution failed for X,
> dropping N packets` — **toda a fila daquele destino é descartada**,
> mesmo que o AODV achasse a rota perfeitamente boa. Ver
> `docs/guia_logs.pdf` (Seção "Estudo de caso") para uma investigação real
> desse cenário exato.

---

## Bloco 8 — Aplicação: `DroneStatus` (resposta ao beacon)

```
[INFO]	[TEAM team0] DroneStatus de drone[0] pos=(2500,2500,100) RTT=0.004653659192s
```

Resposta automática do drone a todo `TeamUpdate` recebido — funciona como
um "ping" de posição/atividade. O `RTT` é calculado pela equipe:
`simTime() - sentAt` do pacote. **Não** é usado em nenhuma métrica de
entrega de alerta, é só diagnóstico de conectividade.

---

## Bloco 9 — Aplicação: `VictimAlert` / `VictimAck` (o ciclo de alerta)

```
[INFO]	Dispatching packet to service, ... packet = (Packet)VictimAlert (256 B) VictimAlertChunk,
	droneId = drone[0], msgId = drone[0]_1, originIp = 10.0.0.1, posX = 2500, posY = 2500, sentAt = 1.591749009132s.
[INFO]	[DRONE drone[0]] VictimAlert drone[0]_1 → team0 (10.0.0.2)
[INFO]	Sending app packet VictimAlert over ipv4.
...
[INFO]	Routing (inet::Packet)VictimAlert (284 B) ... with destination = 10.0.0.2, output interface = wlan0, next hop address = 10.0.0.2
```

A linha `[DRONE drone[0]] VictimAlert drone[0]_1 → team0 (10.0.0.2)` é a
que também aparece no modo `--msglog` (é o que o parser
`parse_message_log.py` extrai) — as linhas `Dispatching packet to
service...` ao redor são o **mesmo pacote** atravessando cada camada
interna do nó (app → udp → ip), com todos os campos do `VictimAlertChunk`
expostos por completo. Do lado da equipe, o ciclo fecha com:

```
[INFO]	[TEAM team0] *** ALERTA de drone[0] msgId=drone[0]_1 vitima em (2500,2500) delay=0.000134368468s
[INFO]	[TEAM team0] VictimAck drone[0]_1 → drone origem 10.0.0.1
[INFO]	[DRONE drone[0]] VictimAck recebido para drone[0]_1 de team0
```

`*** ALERTA` é o marcador visual de entrega bem-sucedida (só aparece uma
vez por `msgId`, graças à deduplicação em `seenAlerts`). `delay` é o
atraso 1-via (drone→equipe) — a métrica de "atraso fim-a-fim" reportada na
dissertação.

---

## Bloco 10 — Por que tem tanta linha "Dispatching packet to..."?

Padrão que se repete centenas de vezes no arquivo:

```
[INFO]	Dispatching packet to protocol, protocol = ipv4(38), servicePrimitive = 2,
	inGate = (omnetpp::cGate)in[3] <-- ip.transportOut, outGate = (omnetpp::cGate)out[0] --> up.in[2],
	packet = (Packet)VictimAlert (264 B) [...]
```

Cada nó (`drone[0]`, `team[0]`...) é internamente uma **cadeia de
submódulos** (app → udp → ip → wlan → mac → radio, e o caminho inverso na
recepção). Toda vez que um pacote atravessa a "borda" entre dois desses
submódulos, o framework de mensagens do INET imprime de onde veio
(`inGate`) e para onde vai (`outGate`), com o pacote **inteiro** anexado
(todos os cabeçalhos empilhados: PHY, MAC, IP, UDP, chunk da aplicação).
É verboso mas didático — dá pra ver a pilha de protocolos completa de
qualquer pacote específico só copiando a linha.

Se isso atrapalha mais que ajuda, é exatamente o motivo de existir o
`./run.sh --msglog` (Bloco 3/8/9 sem o Bloco 4/5/6/7/10) — ver
`docs/guia_logs.pdf`.

---

## Bloco 11 — Fim da simulação: `finish()` e estatísticas

```
** Event #2531  t=15  100% completed  (100% total)

<!> Simulation time limit reached -- at t=15s, event #2531

Calling finish() at end of Run #0...
[INFO]	Radio signal arrival computation count = 240
[INFO]	Transmission count = 120
[INFO]	Reception cache hit = 85.7143 %
...
[INFO]	BasicNetwork.drone[0].tcp: finishing with 0 connections open.

End.
```

Quando `t` atinge o `sim-time-limit` do cenário, o motor para e chama
`finish()` em cada módulo — é aqui que os contadores (`alertsGenerated`,
`alertsAcked`, `meanDeliveryDelay`...) são gravados no arquivo `.sca`. As
linhas de `Radio signal arrival computation count`/cache hit são
estatísticas internas do `radioMedium` sobre reaproveitamento de cálculos
de propagação (não tem relação com a aplicação). `TCP finishing with 0
connections` é ruído do INET — o cenário não usa TCP, mas todo `AdhocHost`
carrega o módulo mesmo sem uso.

---

## Referência rápida

| Prefixo/padrão | Camada | O que procurar |
|---|---|---|
| `[DRONE X]` / `[TEAM Y]` | Aplicação | `SimpleDroneApp.cc` / `SimpleTeamApp.cc` |
| `Transmission started` / `Reception ...` / `snr=`/`ber=` | PHY (rádio) | física do link, sucesso/falha de decodificação |
| `Received frame from PHY` / `Frame sequence finished` / `Channel released` | MAC (802.11) | handshake DATA/ACK, contenção de canal |
| `AODV Route Request/Reply` / `add route` / `Active route found` | Roteamento (AODV) | descoberta e uso de rota multi-hop |
| `arpREQ`/`arpREPLY` / `ARP resolution ...` | ARP | resolução IP→MAC, pode falhar mesmo com rota AODV boa |
| `Dispatching packet to ...` | Interno (INET) | pacote atravessando submódulos dentro de um nó |
| `** Event #N t=...` | Motor de simulação | timestamp de tudo que vem depois, até o próximo cabeçalho |

## Ver também

- `docs/guia_logs.pdf` / `.tex` — versão ilustrada, com diagramas e o
  estudo de caso completo de uma falha real por ARP.
- `analysis/parse_message_log.py` + `analysis/export_messages_parquet.sh`
  — extrai só os Blocos 3/8/9 (mensagens de aplicação) numa tabela
  consultável via SQL, sem precisar ler o log bruto.
- `CLAUDE.md` — arquitetura do projeto, portas, mensagens, parâmetros.
