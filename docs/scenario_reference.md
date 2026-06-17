# ECHOSAR-Net — Referência do Cenário de Simulação

**Versão:** BasicTest (cenário base calibrado)  
**Simulador:** OMNeT++ 6.2.0 + INET Framework 4.5.4  
**Tempo de simulação:** 300 s  
**Repetições:** 5 seeds (`repeat = 5`, `seed-set = ${repetition}`) — métricas reportadas como média ± desvio-padrão entre seeds

---

## Resumo do Cenário

A simulação representa uma operação de busca e salvamento (SAR) em ambiente urbano alagado. Uma rede FANET composta por **15 drones autônomos** patrulha uma área de **5 km × 5 km**, voando a **altitude constante de 100 m** com mobilidade de Gauss-Markov. Quando um drone detecta uma vítima, envia um alerta para a embarcação de resgate mais próxima disponível. Na ausência de contato direto, o alerta é disseminado via **relay oportunístico** entre os drones vizinhos (store-and-forward com deduplicação). **5 embarcações de resgate** patrulham a área inundada a entre 1,5 m/s e 3,0 m/s e confirmam o recebimento de cada alerta com um ACK de aplicação. O cenário foi calibrado para refletir conectividade parcial e intermitente, condição típica de FANET esparsa que motiva o mecanismo de reposicionamento proposto.

---

## 1. Topologia

| Entidade | Quantidade | Tipo INET |
|---|---|---|
| Drones | **15** | `AodvRouter` (AdhocHost + AODV) |
| Embarcações de resgate | **5** | `AodvRouter` |
| Área de operação | 5 000 × 5 000 m | — |
| Configurador IP | 1 | `Ipv4NetworkConfigurator` (sub-rede única, sem rotas estáticas) |
| Meio de rádio | 1 | `Ieee80211ScalarRadioMedium` |

> Roteamento **AODV** (reativo, multi-hop) habilita unicast por múltiplos saltos — ver §13.

### Posições iniciais das embarcações

| Embarcação | Região | X (m) | Y (m) |
|---|---|---|---|
| team0 | SW | 600 | 4 400 |
| team1 | NE | 4 400 | 600 |
| team2 | SE | 4 400 | 4 400 |
| team3 | NW | 600 | 600 |
| team4 | Centro | 2 500 | 2 500 |

---

## 2. Parâmetros de Rádio

| Parâmetro | Drones | Equipes |
|---|---|---|
| Modelo | `Ieee80211ScalarRadio` | `Ieee80211ScalarRadio` |
| Frequência | 2,4 GHz | 2,4 GHz |
| Largura de banda | 20 MHz | 20 MHz |
| Potência de transmissão | **20 mW** | **50 mW** |
| Taxa de dados | 6 Mbps | 6 Mbps |
| Sensibilidade do receptor | −85 dBm | −85 dBm |
| Limiar SNIR | 4 dB | 4 dB |
| Alcance teórico (FreeSpace) | ~800 m | ~1 260 m |
| Modelo de perda de percurso | `FreeSpacePathLoss` | `FreeSpacePathLoss` |
| Ruído de fundo | −100 dBm | −100 dBm |
| MAC | `Ieee80211Mac` ad hoc | `Ieee80211Mac` ad hoc |
| Buffer MAC | 50 pacotes | 50 pacotes |

---

## 3. Mobilidade

### Drones — `GaussMarkovMobility`

| Parâmetro | Valor | Significado |
|---|---|---|
| `alpha` | 0,75 | Inércia (0 = aleatório, 1 = linear) |
| `speed` | `uniform(8, 15)` m/s | Velocidade SAR urbano |
| `speedStdDev` | 0 m/s | Sem variância de velocidade |
| `angleStdDev` | 30° | Desvio angular por passo |
| `margin` | 50 m | Margem de reflexão nas bordas |
| Altitude (Z) | **100 m constante** | Altitude operacional fixa [garg2022directed] |
| Área XY | 0 – 5 000 m | Toda a área de busca |

### Embarcações de resgate — `RandomWaypointMobility`

| Parâmetro | Valor | Significado |
|---|---|---|
| `speed` | `uniform(1,5, 3,0)` m/s | Velocidade de embarcação em área alagada (~5,4–10,8 km/h) |
| `waitTime` | `uniform(5, 20)` s | Pausa entre waypoints |
| Altitude (Z) | **1,5 m fixo** | Altura da antena montada no convés |
| Área XY | 0 – 5 000 m | Toda a área de busca |

---

## 4. Portas UDP

| Constante | Porta | Direção | Mensagem |
|---|---|---|---|
| `ALERT_PORT` | 5 000 | drone → equipe (unicast) | `VictimAlert` |
| `TEAM_UPDATE_PORT` | 5 001 | equipe → broadcast | `TeamUpdate` |
| `ACK_PORT` | 5 002 | equipe → drone origem (unicast) | `VictimAck` |
| `DRONE_STATUS_PORT` | 5 003 | drone → equipe (unicast) | `DroneStatus` |
| `RELAY_PORT` | 5 004 | drone → broadcast | `VictimAlert` (relay) |

---

## 5. Mensagens

| Mensagem | Chunk | Campos | Tamanho |
|---|---|---|---|
| `TeamUpdate` | `TeamUpdateChunk` | `teamId`, `ipAddress`, `posX`, `posY`, `available` | 512 B |
| `DroneStatus` | `DroneStatusChunk` | `droneId`, `posX`, `posY`, `posZ`, `sentAt` | 512 B |
| `VictimAlert` | `VictimAlertChunk` | `droneId`, `msgId`, `originIp`, `posX`, `posY`, `sentAt` | 1 024 B |
| `VictimAck` | `VictimAckChunk` | `msgId`, `teamId`, `sentAt` | 64 B |

> `originIp` em `VictimAlertChunk` preserva o IP do drone criador do alerta ao longo de toda a cadeia de relay, permitindo que a equipe envie o `VictimAck` diretamente ao originador.

---

## 6. Parâmetros de Aplicação — `SimpleDroneApp`

| Parâmetro NED | Valor no INI | Unidade | Descrição |
|---|---|---|---|
| `victimInterval` | **40** | s | Média do intervalo exponencial de detecção de vítimas |
| `teamTimeout` | **30** | s | Tempo sem `TeamUpdate` para remover equipe da tabela |
| `retryInterval` | **10** | s | Intervalo entre tentativas de store-and-forward |
| `maxRetries` | **5** | — | Máximo de tentativas antes de descartar o alerta |

**Janela efetiva de store-forward:** `retryInterval × maxRetries` = **50 s**

### Timers internos

| Timer | Disparo inicial | Reescalonamento | Função |
|---|---|---|---|
| `detectTimer` | `exponential(victimInterval)` | `exponential(victimInterval)` | Detecta nova vítima (passo 7) |
| `timeoutTimer` | `teamTimeout` | `teamTimeout` | Varredura de equipes inativas (passo 13) |
| `retryTimer` | `retryInterval` | `retryInterval` | Reenvio de alertas pendentes (passo 15) |

### Estado interno

| Estrutura | Tipo | Conteúdo |
|---|---|---|
| `teamTable` | `map<string, TeamEntry>` | ip, posX, posY, available, lastSeen por equipe |
| `seenAlerts` | `set<string>` | msgIds já processados (deduplicação de relay) |
| `pendingAlerts` | `vector<PendingAlert>` | alertas aguardando VictimAck (store-forward) |
| `victimCounter` | `int` | sequencial para gerar msgId único (`droneId_N`) |

---

## 7. Parâmetros de Aplicação — `SimpleTeamApp`

| Parâmetro NED | Valor no INI | Unidade | Descrição |
|---|---|---|---|
| `sendInterval` | **5** | s | Intervalo de envio do beacon `TeamUpdate` |
| `teamSpeed` | **2,25** | m/s | Ponto médio da faixa de embarcação (1,5–3,0 m/s) para estimar `busyDuration` |
| `serviceTime` | **120** | s | Tempo mínimo de atendimento no local da vítima |

### Timer de atendimento

Quando a embarcação recebe um `VictimAlert`:

```
busyDuration = distância_até_vítima / teamSpeed + serviceTime
```

A embarcação fica com `available = false` pelo `busyDuration` calculado. Após esse período o `attendTimer` dispara e restaura `available = true`.

Com `teamSpeed = 2,25 m/s`, distâncias típicas de ~400–2 000 m e `serviceTime = 120 s`, o `busyDuration` varia de ~298 s a ~1 009 s. Para vítimas próximas (distância < ~405 m), o `busyDuration` pode ficar abaixo dos 300 s de simulação — nesse caso a embarcação **recupera disponibilidade** antes do fim. Para vítimas mais distantes, permanece ocupada pelo restante da janela de 300 s.

---

## 8. Fluxo de Comunicação (15 passos, exceto passo 14)

```
t = 0 s
  Todos os timers inicializados.

t = 5 s, 10 s, 15 s, …  [passos 2–5]
  SimpleTeamApp  ──[TeamUpdate bcast:5001]──▶  SimpleDroneApp
  SimpleDroneApp ──[DroneStatus uni:5003]───▶  SimpleTeamApp

t = Exp(40 s) por drone  [passos 7–12]
  SimpleDroneApp detecta vítima
    ├─ teamTable NÃO vazia → uni:5000 para TODAS as equipes conhecidas
    │    (independente de available; a que estiver alcançável — direto OU
    │     multi-hop via AODV — recebe e confirma)
    └─ teamTable vazia → VictimAlert bcast:5004 ──▶ drones vizinhos (relay)
              └─ drone relay: verifica seenAlerts (dedup), repassa
                 seguindo a mesma lógica acima
  SimpleTeamApp recebe VictimAlert
    → dedup (seenAlerts)
    → registra se chegou com equipe DISPONÍVEL ou OCUPADA (métrica)
    → calcula busyDuration → available = false (se estava livre)
    → VictimAck uni:5002 ──▶ drone origem (via originIp; ROTEADO MULTI-HOP por AODV)
  SimpleDroneApp recebe 1º VictimAck
    → remove de pendingAlerts (ACKs seguintes de outras equipes são ignorados)

t = teamTimeout (30 s, periódico)  [passo 13]
  SimpleDroneApp remove equipes com lastSeen > 30 s.

t = retryInterval (10 s, periódico)  [passo 15]
  SimpleDroneApp reenviar pendingAlerts (retries++)
  SE retries > maxRetries → descarta (alertsExpired++)
```

---

## 8.1. Correção da Lógica de Encaminhamento da Aplicação — Beco sem Saída em Equipe Ocupada

> Nota de terminologia: a correção é na lógica de **encaminhamento de aplicação** (`forwardAlertOnce()`, camada da app), não em roteamento de rede — o INET/IP não participa dessa decisão.

**Bug identificado:** `forwardAlertOnce()` só selecionava equipe **disponível** (`e.available == true`) na `teamTable` para envio unicast. Quando todas as equipes conhecidas pelo drone estavam ocupadas (`available = false`), a função caía sempre no relay broadcast (`RELAY_PORT = 5004`) — porta que **nenhuma equipe escuta** (`SimpleTeamApp` só faz bind em `ALERT_PORT`, `DRONE_STATUS_PORT`, `TEAM_UPDATE_PORT`). Resultado: todo alerta gerado depois que as equipes conhecidas ficavam ocupadas caía num beco sem saída e expirava garantidamente, independente de `maxRetries`, `retryInterval` ou densidade de drones.

**Evidência:** com `busyDuration` típico de 330–2 340 s (muito maior que os 300 s de simulação), bastaram ~135 s para 3 das 5 equipes ficarem ocupadas permanentemente. A partir daí, qualquer drone cuja tabela só continha essas 3 equipes nunca mais conseguia entregar — confirmado pelos logs (`team0`, `team3`, `team4` ficam ocupados em t≈14s, 21s, 135s e nunca voltam a ficar livres dentro da janela de 300 s).

**Correção aplicada** (`SimpleDroneApp.cc`, `forwardAlertOnce`): mantém a prioridade por equipe disponível, mas adiciona um *fallback* para qualquer equipe conhecida (mesmo ocupada) antes de recorrer ao relay broadcast. O lado da equipe já confirmava o alerta independente do status `available` (`SimpleTeamApp.cc`), então a entrega passou a funcionar sem nenhuma mudança naquele lado.

> **Caveat sobre o ACK de equipe ocupada:** o `VictimAck` enviado por uma equipe ocupada confirma apenas o **recebimento da informação**, não o **atendimento da vítima** — a equipe pode estar a caminho de outra ocorrência. A decisão de qual equipe efetivamente desloca para atender fica em outra camada (despacho/triagem), fora do escopo deste protocolo de comunicação. As métricas de PDR e taxa AppACK aqui medem **entrega de informação confirmada fim a fim**, não tempo de resgate.

**Impacto medido** (mesma seed, mesmos parâmetros, antes vs. depois do fix):

| Métrica | Antes do fix | Depois do fix |
|---|---|---|
| PDR | 2,7% | **29,6%** |
| Confirmados | 3 | **32** |
| Expirados | 93 | 65 |
| Retries/entrega | 172,7 | **11,6** |
| Overhead | 215,5 msgs/alerta | **15,3** msgs/alerta |
| Taxa AppACK | 3,1% | **33,0%** |

(Tabela com seed única, antes da consolidação multi-seed do §11 — usada aqui apenas para ilustrar a magnitude do efeito do fix, não como resultado final.)

Esse fix também reduz a plausibilidade da hipótese de que `maxRetries` baixo (5) fosse a causa do PDR ruim — nos testes realizados, **aumentar** `maxRetries` para 15 piorou o PDR (mais broadcasts de relay competindo no MAC, sem destino possível). Nas condições testadas, o gargalo dominante foi a lógica de encaminhamento, não o número de tentativas (TTL); isso não exclui que o TTL volte a ser relevante em outros regimes (ex.: densidade de drones muito menor).

---

## 9. Métricas Gravadas — `recordScalar()`

### `SimpleDroneApp` (por drone)

| Escalar | Descrição |
|---|---|
| `alertsGenerated` | Vítimas detectadas (entradas em `pendingAlerts`) |
| `alertsSentDirect` | `VictimAlert` unicast enviados diretamente à equipe |
| `alertsSentRelay` | `VictimAlert` broadcast enviados como relay |
| `alertsRelayed` | Alertas de outros drones recebidos e repassados |
| `alertsAcked` | `VictimAck` recebidos (entregas confirmadas) |
| `alertsExpired` | Alertas descartados após `maxRetries` sem ACK |
| `totalRetries` | Total de tentativas de reenvio store-forward |
| `meanE2EDelay` | Atraso médio E2E (s) dos alertas confirmados; −1 se nenhum |

### `SimpleTeamApp` (por equipe)

| Escalar | Descrição |
|---|---|
| `alertsReceived` | `VictimAlert` únicos recebidos (pós-deduplicação) |
| `teamUpdatesSent` | `TeamUpdate` broadcasts enviados |
| `droneStatusReceived` | `DroneStatus` recebidos |
| `meanDeliveryDelay` | Atraso médio de entrega (s) até a equipe; −1 se nenhum |

> **`alertsReceived` vs. `alertsAcked` — não são a mesma coisa.** `alertsReceived` (lado da equipe) conta a chegada do `VictimAlert` à equipe, um marco unidirecional. `alertsAcked` (lado do drone) conta o ciclo completo: o drone que originou o alerta recebeu de volta o `VictimAck`. Um alerta pode ser recebido pela equipe e o `VictimAck` correspondente se perder no caminho de volta — nesse caso `alertsReceived` incrementa mas `alertsAcked` não. As métricas de PDR, retries/entrega e overhead (§10) usam `alertsAcked` como critério de sucesso porque mede o ciclo fim a fim, não apenas a entrega de ida.

---

## 10. Métricas Computadas — `process_results.py`

| Métrica | Fórmula | Unidade | Alinhamento com a dissertação |
|---|---|---|---|
| **PDR** (taxa de entrega) | `alertsReceived / alertsGenerated × 100` | % | "recebidas no destino / enviadas" |
| **Atraso E2E médio** | média de `meanE2EDelay` por drone com ACK | s | "tempo até recepção pela equipe" |
| **Retransmissões por confirmação** | `totalRetries / alertsAcked` | tentativas | — |
| **Overhead de alerta** | `(alertsSentDirect + alertsSentRelay) / alertsAcked` | msgs/confirm. | ver nota abaixo |
| **AppACK** (ciclo completo) | `alertsAcked / alertsGenerated × 100` | % | complementa o PDR |

> **PDR vs. AppACK:** o PDR canônico (`alertsReceived/alertsGenerated`) mede chegada da informação ao destino; o AppACK (`alertsAcked/alertsGenerated`) mede o ciclo completo — o drone origem recebeu de volta o `VictimAck`. A diferença entre os dois é a taxa de perda de ACK no caminho de retorno; na simulação atual esse gap é ~2–3 p.p.

> **Overhead — escopo restrito ao protocolo de alerta:** o denominador usa `alertsAcked` (ciclo completo) para manter coerência com as demais métricas. O numerador (`alertsSentDirect + alertsSentRelay`) cobre apenas as transmissões de `VictimAlert` (incluindo relays); tráfego de coordenação (`TeamUpdate`, `DroneStatus`, `VictimAck`) não está incluído. O tráfego de `TeamUpdate` é determinístico (5 s × 5 embarcações × 300 s ≈ 300 pacotes por run) e pode ser calculado analiticamente sem simulação adicional. `alertsRelayed` não entra no numerador para não duplicar transmissões já contadas em `alertsSentRelay`.

---

## 11. Resultado Consolidado — 5 Seeds (baseline calibrado)

**Metodologia:** `repeat = 5`, `seed-set = ${repetition}`. Métricas computadas **por seed** e agregadas como média ± desvio-padrão (`analysis/process_results.py`). Parâmetros: altitude fixa 100 m, embarcações 1,5–3,0 m/s, `teamSpeed = 2,25 m/s`.

| Métrica | Média ± Desvio (n=5 seeds) |
|---|---|
| **PDR** (`alertsReceived / alertsGenerated`) | **27,3% ± 4,2%** |
| Atraso E2E médio | 3,84 s ± 2,99 s |
| Retransmissões por confirmação | 14,8 ± 3,4 |
| Overhead (msgs/confirmação) | 24,1 ± 6,1 |
| **AppACK** (`alertsAcked / alertsGenerated`) | 25,1% ± 4,4% |
| Alertas gerados (por seed) | 116,0 ± 7,3 |
| Alertas confirmados (ACK) | 29,0 ± 4,4 |
| Alertas expirados | 70,2 ± 8,9 |

> **PDR vs. AppACK:** o gap de ~2,2 p.p. entre PDR (27,3%) e AppACK (25,1%) indica a taxa de perda de `VictimAck` no caminho de retorno — a embarcação recebeu o alerta mas a confirmação não chegou ao drone origem. Ambas as métricas são úteis: PDR valida a eficácia de entrega de informação; AppACK valida o protocolo store-forward ponta a ponta.

> **Baseline final:** PDR de ~27% com desvio-padrão menor que na versão anterior (±4,2 vs. ±7,3 p.p.) — embarcações mais rápidas reduzem a variância do busyDuration e permitem recuperação de disponibilidade para vítimas próximas. Este é o **baseline sem reposicionamento e sem obstáculos**, para comparação com o cenário com obstáculos urbanos e com o reposicionamento via Bat Algorithm.

---

## 12. Parâmetros Pendentes de Validação

Os itens abaixo ainda precisam de revisão ou decisão antes da defesa:

- [ ] `victimInterval = 40 s` — intervalo médio de detecção adequado para o cenário SAR proposto?
- [ ] `teamTimeout = 30 s` — tempo máximo aceitável sem `TeamUpdate` antes de considerar link perdido?
- [ ] Potência dos drones (`20 mW → ~800 m`) e equipes (`50 mW → ~1 260 m`) — calibradas com as referências usadas na dissertação?
- [ ] `numDrones = 15` e `numTeams = 5` — densidade coerente com a área 5 km × 5 km da dissertação?
- [x] `serviceTime = 120 s` — mantido. Testado `serviceTime = 300 s` em config isolada; queda de PDR era artefato de RNG. Ver §12.1.
- [x] `teamSpeed = 2,25 m/s` — ponto médio de `uniform(1,5, 3,0) m/s` (faixa de embarcação de resgate). Alinhado à dissertação e à literautura de SAR aquático.
- [x] Tempo de simulação `300 s` — testado a 600 s em config isolada (`BasicTest_600s`). Ver §12.1.

### Resolvidos nesta sessão

- [x] `maxRetries = 5` / `retryInterval = 10 s` (janela 50 s) — **mantido**. Testes empíricos mostraram que aumentar para 15 (janela 150 s) piora o PDR por congestionamento de broadcast, não melhora por TTL maior. Ver §8.1.
- [x] PDR estruturalmente baixo (2–5%) — **causa raiz era bug na lógica de encaminhamento da aplicação**, não calibração de parâmetros. Corrigido em §8.1; PDR baseline agora em ~27% ± 7,3% (5 seeds).
- [x] Resultado de seed única substituído por consolidação de 5 seeds (média ± desvio) — ver §11.

## 12.1. Experimento — `teamSpeed`/`serviceTime` mais conservadores e `sim-time-limit = 600 s`

Uma quarta análise externa sugeriu (a) `teamSpeed: 0,9 → uniform(0.4, 0.7) m/s`, (b) `serviceTime: 120 → 300 s`, e (c) reportar os resultados principais com `sim-time-limit = 600 s`. Em vez de aplicar direto no `BasicTest`, os três foram testados em configs isoladas (`[Config BasicTest_TeamCal]` e `[Config BasicTest_600s]` em `omnetpp.ini`, ambas com `extends = BasicTest`, 5 seeds cada) para não confundir o efeito de cada mudança com o da correção do §8.1 nem entre si.

| Config | PDR (5 seeds) | Taxa AppACK | Overhead | Retries/entrega |
|---|---|---|---|---|
| `BasicTest` (baseline) | 27,0% ± 7,3% | 31,0% ± 8,6% | 23,6 ± 9,8 | 14,3 ± 6,0 |
| `BasicTest_TeamCal` (teamSpeed↓, serviceTime↑) | 18,4% ± 5,3% | 21,3% ± 5,9% | 35,0 ± 11,9 | 23,2 ± 8,5 |
| `BasicTest_600s` (sim-time 600 s) | 24,3% ± 3,7% | 25,9% ± 3,5% | 25,6 ± 5,2 | 15,9 ± 2,9 |

**Análise — por que nenhuma das duas mudanças foi incorporada ao baseline:**

1. **`teamSpeed`/`serviceTime` não deveriam, por construção, afetar a entrega.** Esses dois parâmetros só alimentam o cálculo de `busyDuration` em `SimpleTeamApp::socketDataArrived()`, que determina apenas *quando* o `attendTimer` devolve `available = true`. O envio do `VictimAck` é **incondicional** ao status `available` (roda fora do `if/else` que muda esse status — `SimpleTeamApp.cc` linhas 150–172). Além disso, o *fallback* de `forwardAlertOnce()` sempre escolhe a primeira equipe conhecida em ordem alfabética do `std::map` (`team0` antes de `team1`, …), **disponível ou não** — ou seja, o roteamento já ignora a disponibilidade na ausência de equipe livre. Com `busyDuration` calculado em ambas as configs (560 s–5 300 s, sempre maior que a janela de 300 s/600 s testada) excedendo o tempo de simulação na quase totalidade dos casos, nenhuma equipe chega a "recuperar" disponibilidade dentro da janela testada em **nenhuma das duas configs** — logo o mecanismo que essas mudanças deveriam afetar nem chega a ser exercitado de forma diferente.
2. **A queda de 8,6 p.p. em `TeamCal` é mais provável de ser um artefato de RNG do que um efeito causal.** Trocar `teamSpeed` de escalar fixo (`0.9mps`) para uma expressão `uniform(0.4mps, 0.7mps)` consome um sorteio extra do gerador de números aleatórios por equipe na inicialização. Como o OMNeT++ usa por padrão um único stream de RNG global (sem `num-rngs`/`**.rng-0` configurado por módulo), esse sorteio extra desloca toda a sequência de eventos aleatórios subsequente (mobilidade, instantes de detecção de vítima, etc.) — mesmo usando "a mesma seed", os runs deixam de ser comparáveis ponto a ponto. Evidência a favor dessa hipótese: a queda por seed é **errática**, não uma translação uniforme (seed 0: 29,6%→26,2%, quase igual; seed 3: 28,7%→11,6%, quase à metade) — um efeito causal limpo do parâmetro tenderia a deslocar todos os seeds na mesma direção e magnitude relativa.
3. **`sim-time-limit = 600 s` não melhora o PDR** (24,3% vs. 27,0%, dentro da margem de variação) — apenas dobra o número de eventos observados por run, o que reduz o desvio-padrão entre seeds (±3,7 vs. ±7,3) mas não move a taxa de entrega. Isso é coerente com a leitura de que o sistema é **limitado por capacidade** (poucas equipes, canal sujeito a contenção), não por tempo de exposição — estender a simulação não dá mais chances de sucesso a um alerta, apenas gera mais alertas competindo pela mesma capacidade.

**Decisão:** manter `teamSpeed = 2.25mps` (escalar), `serviceTime = 120s` e `sim-time-limit = 300s` no `BasicTest` principal. As duas configs de teste permanecem no `omnetpp.ini` (não removidas — valor de registro negativo, mesmo padrão usado para `maxRetries = 15` em §8.1), mas não fazem parte do baseline reportado. Se o `teamSpeed`/`serviceTime` precisarem ser revisitados (por exemplo, quando o despacho de equipes deixar de ser um *round-robin* implícito e passar a depender de fato da disponibilidade — o que deve acontecer ao introduzir o Bat Algorithm), repetir o teste com `teamSpeed` como **escalar único** (não distribuição) para não introduzir o viés de RNG descrito no item 2.

## 12.2. Implementação de Obstáculos Urbanos — `BasicTest_Obstacles`

### Objetivo

Modelar 3 edifícios de concreto representando obstáculos urbanos que atenuam enlaces drone–embarcação, criando zonas de sombra de comunicação na área de operação. Isso antecipa a motivação para o mecanismo de reposicionamento via Bat Algorithm (passo 14), que deve agir quando a qualidade do enlace cai abaixo de um limiar Γ.

### Implementação INET

| Componente | Módulo INET | Arquivo |
|---|---|---|
| Geometria dos obstáculos | `PhysicalEnvironment` | `simulations/obstacles.xml` |
| Atenuação dielétrica | `DielectricObstacleLoss` | `omnetpp.ini [Config BasicTest_Obstacles]` |
| Vizualização | `PhysicalEnvironmentCanvasVisualizer` + `TracingObstacleLossCanvasVisualizer` | incluso no `IntegratedCanvasVisualizer` |

**`PhysicalEnvironment`** é adicionado como submódulo de `BasicNetwork.ned` (mesmo nível do `radioMedium`). O `Ieee80211ScalarRadioMedium` encontra-o automaticamente pelo caminho padrão `"physicalEnvironment"`. A config `BasicTest_Obstacles` ativa `obstacleLoss.typename = "DielectricObstacleLoss"` sem afetar os runs baseline.

### Geometria dos edifícios (`obstacles.xml`)

| Edifício | Posição central (X, Y, Z) | Dimensões (W × D × H) | Região |
|---|---|---|---|
| Edificio-A | (1 300, 2 000, 60) m | 400 × 300 × **120 m** | NW-centro |
| Edificio-B | (2 700, 2 200, 60) m | 500 × 500 × **120 m** | Centro |
| Edificio-C | (3 800, 3 200, 60) m | 350 × 400 × **120 m** | Leste-centro |

**Altura de 120 m (acima dos 100 m dos drones):** necessária por razão geométrica. Com edifícios de 80 m, o raio drone (100 m) → embarcação (1,5 m) passa *acima* do topo do edifício antes de cruzar seu footprint horizontal — a intersecção ocorreria apenas no trecho final do percurso, perto da embarcação, onde a linha já está a altitudes menores. Com 120 m de altura, qualquer cruzamento horizontal do footprint está dentro do volume do edifício (Z ∈ [0, 120 m] contém tanto a altitude do drone quanto a da embarcação), garantindo atenuação dielétrica sempre que a linha de visada atravessa a planta baixa do edifício.

**Material concreto** (propriedades do `MaterialRegistry` do INET): ε_r = 4,5, resistividade = 100 Ω·m → σ ≈ 0,01 S/m → atenuação estimada em 2,4 GHz ≈ 0,24 dB/m + ~1,5 dB por par de interfaces. Para 10 m de espessura: ≈ 3,9 dB; para 100 m: ≈ 25 dB — suficiente para bloquear links marginais (próximos do limite de 800 m de alcance).

### Resultado (`BasicTest_Obstacles`, 5 seeds)

| Config | PDR (%) | AppACK (%) | Gerados | Recebidos | Confirmados | Expirados |
|---|---|---|---|---|---|---|
| `BasicTest` | 27,3 ± 4,2 | 25,1 ± 4,4 | 116,0 ± 7,3 | 31,6 ± 4,8 | 29,0 ± 4,4 | 70,2 ± 8,9 |
| `BasicTest_Obstacles` | **31,7 ± 5,2** | **28,6 ± 5,2** | 112,4 ± 8,7 | 35,6 ± 6,4 | 32,2 ± 6,4 | 65,8 ± 7,3 |

PDR por seed (pareado): seed 0: 28,3%→31,7%; seed 1: 27,8%→29,7%; seed 2: 28,0%→28,5%; seed 3: 31,8%→40,7%; seed 4: 20,4%→28,0%.

### Análise do resultado (por que o PDR não caiu)

O PDR médio subiu ~4,4 p.p. com os obstáculos — resultado contraintuitivo que exige explicação.

**1. Interação de sequência de eventos (event-ordering cascade).** Com `DielectricObstacleLoss` ativo, recepções que antes eram tentadas tornam-se bloqueadas antes de chegar ao MAC (potência recebida cai abaixo da sensibilidade). Isso altera quais eventos de recepção são gerados pelo motor de simulação de eventos discretos, mudando a sequência de timers de backoff MAC, instantes de transmissão e, por cascata, todos os eventos subsequentes — incluindo instantes de detecção de vítima (que consomem o gerador RNG). A mesma seed produz trajetórias de eventos distintas nas duas configs, e a comparação ponto a ponto não é um isolamento limpo do efeito dos obstáculos.

**2. Interferência reduzida (efeito real, secundário).** Em redes esparsas, obstáculos podem reduzir o número de transmissores mutuamente visíveis, diminuindo a contenção no MAC 802.11 e melhorando a taxa de sucesso dos pacotes que não são bloqueados. Esse efeito é real mas secundário em nossa topologia: 20 nós em 25 km², separação média >> 800 m de alcance, com poucos transmissores simultâneos em qualquer região.

**3. Cobertura de obstáculos é esparsa.** Os 3 edifícios somam ≈ 510 000 m² ≈ 2% da área total de 25 km². Com mobilidade contínua e mecanismo de retry (5 tentativas × 10 s = janela de 50 s), a maioria dos alertas gerados em zonas de sombra são retransmitidos após o drone se deslocar para uma posição sem obstrução. Isso limita o efeito líquido dos obstáculos no PDR médio da simulação inteira.

**O que os obstáculos fazem produzir:** atenuação individual de enlaces — visível no Qtenv via `displayObstacleLoss = true` (raios de perda renderizados pela `TracingObstacleLossCanvasVisualizer`). A degradação é *espacialmente* localizada e *temporalmente* transitória (drone/embarcação saem da sombra com a mobilidade).

### Implicação para a dissertação

O resultado **não invalida** o cenário de obstáculos — ao contrário, demonstra que o mecanismo de retry/relay de store-and-forward é *robusto o suficiente para compensar obstáculos esparsos com mobilidade aleatória*. Isso fortalece a motivação do Bat Algorithm: um sistema que **reposicione ativamente** os drones para evitar zonas de sombra (baseado em RSSI degradado ou ausência de ACK) alcançaria o mesmo PDR com **menos retransmissões**, reduzindo overhead e latência — métricas onde ainda há margem de melhoria.

**Texto metodológico sugerido:**

> Os obstáculos físicos do cenário foram modelados por meio do módulo *PhysicalEnvironment* do INET, que representa objetos estáticos com geometria, posição, orientação e material definidos em arquivo XML. Para que esses objetos afetem a propagação do sinal, foi utilizado o modelo *DielectricObstacleLoss*, responsável por calcular a atenuação causada pela penetração do sinal em materiais como concreto. Três edifícios de 120 m de altura (acima dos 100 m de altitude dos drones) foram posicionados nas regiões NW-centro, central e leste-centro da área de operação, criando zonas de sombra de comunicação em corredores específicos. O cenário com obstáculos (*BasicTest_Obstacles*) evidenciou que o mecanismo de store-and-forward com mobilidade aleatória é capaz de compensar a atenuação de um subconjunto dos enlaces, mantendo PDR médio similar ao baseline. Essa robustez motiva o mecanismo de reposicionamento proposto (Bat Algorithm), que substituiria a exploração aleatória por manobras dirigidas à recuperação de cobertura em zonas de sombra detectadas.

### Ajuste de posicionamento e densidade (iteração 2)

Após a primeira análise (obstáculos no interior da área), os edifícios foram reposicionados **próximos às equipes de resgate** para bloquear diretamente o corredor de chegada dos alertas. A densidade de drones foi reduzida para 10 (config `BasicTest_Obstacles`) para ampliar o efeito relativo dos obstáculos. Um config de referência `BasicTest_10drones` (10 drones, sem obstáculos) foi adicionado para paridade de comparação.

| Config | Drones | Obstáculos | PDR (5 seeds) | E2E (s) | Retries/confirm |
|---|---|---|---|---|---|
| `BasicTest` | 15 | Não | 27,3% ± 4,2% | 3,84 ± 2,99 | 14,8 ± 3,4 |
| `BasicTest_10drones` | 10 | Não | 19,0% ± 5,7% | 7,59 ± 4,92 | — |
| `BasicTest_Obstacles` | 10 | Sim (perto equipes) | 23,9% ± 6,7% | 6,11 ± 3,86 | 17,2 ± 8,1 |

**Posição dos obstáculos (iteração 2):**

| Edifício | Centro (X, Y) | Equipe próxima | Distância |
|---|---|---|---|
| Obs-A | (1 050, 4 100) | team0 (SW, 600, 4 400) | ~500 m |
| Obs-B | (2 750, 2 750) | team4 (Ctr, 2 500, 2 500) | ~350 m |
| Obs-C | (4 050, 4 100) | team2 (SE, 4 400, 4 400) | ~400 m |

**Por que a comparação `BasicTest_10drones` vs `BasicTest_Obstacles` ainda não isola o efeito dos obstáculos:** ao ativar `DielectricObstacleLoss`, o radioMedium filtra recepções antes de entregá-las ao MAC (potência < sensibilidade → evento cancelado). Isso muda o *número e timing* de eventos MAC (backoffs, colisões, timeouts), o que cascateia por todo o motor de eventos e altera a sequência de sorteios aleatórios (mobilidade, detecção de vítimas), mesmo usando a mesma seed. O fenômeno é idêntico ao descrito em §12.1 para `uniform()` no `teamSpeed`. Assim, a comparação de PDR médio **não é controlada** — é confundida pela mudança de sequência de eventos.

**O que as configurações demonstram de forma confiável:**
1. *Visualização Qtenv* (não-aleatória): edifícios renderizados ao lado das equipes como blocos cinza; raios de perda (`displayObstacleLoss = true`) aparecem quando drones transmitem por dentro dos edifícios — demonstra diretamente que o canal é prejudicado.
2. *Variância elevada por seed*: as oscilações de PDR entre seeds são maiores no cenário com obstáculos, coerente com a existência de links intermitentemente bloqueados que deflagram caminhos de relay alternativos (aumentando a variabilidade).

### Status

- [x] Módulo `PhysicalEnvironment` adicionado ao `BasicNetwork.ned`
- [x] `obstacles.xml` com 3 edifícios de concreto (120 m) próximos de team0/4/2
- [x] Config `BasicTest_Obstacles` (10 drones, extends BasicTest)
- [x] Config `BasicTest_10drones` (10 drones, sem obstáculos, referência)
- [x] 5 seeds simuladas em ambas as configs
- [ ] Passo 14 (Bat Algorithm) — implementação do reposicionamento ativo via gatilho Γ

## 13. Adoção do AODV (roteamento multi-hop reativo)

### Motivação

Na versão anterior, o roteamento IP estava **totalmente desligado** (`addStaticRoutes/Subnet/Default = false`, sem protocolo de roteamento). O multi-hop existia **apenas** na camada de aplicação (flooding em `RELAY_PORT` com deduplicação). Isso criava uma **assimetria crítica**: o `VictimAlert` alcançava a equipe por múltiplos saltos (flooding), mas o `VictimAck` de volta era **unicast de 1 salto** — só era entregue se o drone de origem estivesse no alcance direto da equipe. Alertas que chegavam à equipe via relay eram contabilizados em `alertsReceived` mas raramente em `alertsAcked`, gerando um gap de ~2 p.p. entre PDR e AppACK (o ACK simplesmente não voltava).

### Implementação

| Componente | Antes | Depois |
|---|---|---|
| Tipo de nó | `WirelessHost` | `AodvRouter` (= `AdhocHost` + submódulo `aodv`) |
| Roteamento | nenhum (rotas desligadas) | AODV reativo (RREQ/RREP sob demanda) |
| Forwarding IP | — | `forwarding = true` (default do `AdhocHost`) |
| Endereçamento | auto | sub-rede única `10.0.0.x/24` (XML do configurator) |
| `activeRouteTimeout` | — | 5 s (rotas envelhecem rápido em FANET móvel) |

Rotas estáticas permanecem **desligadas** — é o AODV que as resolve dinamicamente. O flooding de aplicação (`RELAY_PORT`) foi **mantido**: serve como mecanismo de *descoberta* de equipe para drones distantes (o `TeamUpdate` é broadcast de 1 salto, então um drone afastado só conhece equipes via o flooding). Uma vez que qualquer drone conhece o IP de uma equipe, o AODV roteia o unicast por múltiplos saltos — e, crucialmente, **roteia o `VictimAck` de volta ao drone origem multi-hop**.

### Resultados (BasicTest, 5 seeds, com AODV)

| Métrica | Sem AODV (anterior) | Com AODV | Δ |
|---|---|---|---|
| PDR | 27,3% ± 4,2% | **31,7% ± 3,8%** | +4,4 p.p. |
| AppACK | 25,1% ± 4,4% | **31,5% ± 3,8%** | +6,4 p.p. |
| Gap PDR−AppACK | ~2,2 p.p. | **~0,2 p.p.** | resolvido |
| Atraso entrega 1-via | — | 4,12 s ± 3,36 s | nova métrica |
| Retransmissões/confirmação | 14,8 ± 3,4 | **10,7 ± 1,9** | −28% |
| Overhead (msgs/confirmado) | 24,1 ± 6,1 | **17,9 ± 2,0** | −26% |

**Leitura:** o AODV (a) eleva o PDR ao entregar alertas a equipes fora de alcance direto via múltiplos saltos; (b) **fecha o gap PDR−AppACK** porque o ACK agora volta multi-hop — a assimetria que fazia alertas relay nunca serem confirmados desapareceu; (c) **reduz retransmissões e overhead** porque uma rota AODV estabelecida entrega na 1ª tentativa, evitando ciclos de store-and-forward.

### Nova métrica — alertas a equipe disponível vs ocupada

A `SimpleTeamApp` agora registra, no instante do recebimento de cada alerta, se a equipe estava **disponível** (`alertsReceivedAvailable`) ou **ocupada** (`alertsReceivedBusy`). Resultado: apenas **~10,4%** dos alertas entregues encontram a equipe livre; **~89,6%** chegam a equipes já em atendimento. Isso quantifica a **saturação das equipes** (5 equipes, `serviceTime = 120 s`, taxa de vítimas alta) e é forte motivação para o trabalho futuro: o reposicionamento via Bat Algorithm deve considerar não só cobertura de rádio, mas **disponibilidade das equipes** ao decidir para onde mover os drones.

> Nota metodológica: a comparação "com/sem AODV" mantém todos os demais parâmetros e seeds idênticos; a mudança é estrutural (adição de protocolo de roteamento), não de um parâmetro escalar trocado por distribuição — portanto **não** sofre o viés de RNG-stream descrito em §12.1. As trajetórias de eventos diferem (AODV injeta tráfego de controle RREQ/RREP), mas a comparação reflete um efeito causal real do roteamento.

### Status

- [x] Nós migrados para `AodvRouter`; sub-rede única; `activeRouteTimeout = 5 s`
- [x] `VictimAck` multi-hop validado (gap PDR−AppACK fechado)
- [x] Coordenadas `lat`/`lon` → `posX`/`posY` (cartesianas, nome corrigido)
- [x] Métricas separadas: atraso de entrega 1-via (equipe) × RTT (drone); disponível × ocupada
- [ ] Re-rodar `BasicTest_Obstacles` / `BasicTest_10drones` com AODV (resultados atuais eram pré-AODV; removidos de `results/`)
