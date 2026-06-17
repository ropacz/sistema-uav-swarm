# ECHOSAR-Net — Referência do Cenário de Simulação

**Versão:** BasicTest (cenário base calibrado)  
**Simulador:** OMNeT++ 6.2.0 + INET Framework 4.5.4  
**Tempo de simulação:** 300 s  
**Repetições:** 5 seeds (`repeat = 5`, `seed-set = ${repetition}`) — métricas reportadas como média ± desvio-padrão entre seeds

---

## Resumo do Cenário

A simulação representa uma operação de busca e salvamento (SAR) em ambiente urbano alagado. Uma rede FANET composta por **15 drones autônomos** patrulha uma área de **5 km × 5 km**, voando em altitudes entre 100 m e 150 m com mobilidade de Gauss-Markov. Quando um drone detecta uma vítima, envia um alerta para a equipe terrestre mais próxima disponível. Na ausência de contato direto, o alerta é disseminado via **relay oportunístico** entre os drones vizinhos (store-and-forward com deduplicação). **5 equipes de resgate terrestres** patrulham o terreno a pé em velocidade reduzida, característica de áreas severamente alagadas, e confirmam o recebimento de cada alerta com um ACK de aplicação. O cenário foi calibrado para refletir conectividade parcial e intermitente, condição típica de FANET esparsa que motiva o mecanismo de reposicionamento proposto.

---

## 1. Topologia

| Entidade | Quantidade | Tipo INET |
|---|---|---|
| Drones | **15** | `WirelessHost` |
| Equipes terrestres | **5** | `WirelessHost` |
| Área de operação | 5 000 × 5 000 m | — |
| Configurador IP | 1 | `Ipv4NetworkConfigurator` |
| Meio de rádio | 1 | `Ieee80211ScalarRadioMedium` |

### Posições iniciais das equipes

| Equipe | Região | X (m) | Y (m) |
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
| Altitude (Z) | 100 – 150 m | Faixa operacional |
| Área XY | 0 – 5 000 m | Toda a área de busca |

### Equipes — `RandomWaypointMobility`

| Parâmetro | Valor | Significado |
|---|---|---|
| `speed` | `uniform(0,45, 1,4)` m/s | Velocidade em área alagada |
| `waitTime` | `uniform(5, 20)` s | Pausa entre waypoints |
| Altitude (Z) | **1,5 m fixo** | Altura da antena do terminal |
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
| `TeamUpdate` | `TeamUpdateChunk` | `teamId`, `ipAddress`, `lat`, `lon`, `available` | 512 B |
| `DroneStatus` | `DroneStatusChunk` | `droneId`, `posX`, `posY`, `posZ`, `sentAt` | 512 B |
| `VictimAlert` | `VictimAlertChunk` | `droneId`, `msgId`, `originIp`, `lat`, `lon`, `sentAt` | 1 024 B |
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
| `teamTable` | `map<string, TeamEntry>` | ip, lat, lon, available, lastSeen por equipe |
| `seenAlerts` | `set<string>` | msgIds já processados (deduplicação de relay) |
| `pendingAlerts` | `vector<PendingAlert>` | alertas aguardando VictimAck (store-forward) |
| `victimCounter` | `int` | sequencial para gerar msgId único (`droneId_N`) |

---

## 7. Parâmetros de Aplicação — `SimpleTeamApp`

| Parâmetro NED | Valor no INI | Unidade | Descrição |
|---|---|---|---|
| `sendInterval` | **5** | s | Intervalo de envio do beacon `TeamUpdate` |
| `teamSpeed` | **0,9** | m/s | Velocidade estimada para cálculo de `busyDuration` |
| `serviceTime` | **120** | s | Tempo mínimo de atendimento no local da vítima |

### Timer de atendimento

Quando a equipe recebe um `VictimAlert`:

```
busyDuration = distância_até_vítima / teamSpeed + serviceTime
```

A equipe fica com `available = false` pelo `busyDuration` calculado. Após esse período o `attendTimer` dispara e restaura `available = true`.

Com velocidade 0,9 m/s, distâncias típicas de ~400–2 000 m e `serviceTime = 120 s`, o `busyDuration` varia de ~560 s a ~2 340 s — excedendo o tempo total de simulação (300 s) na maioria dos casos. **Na prática, uma equipe que recebe o primeiro alerta permanece ocupada pelo restante da simulação.**

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
    ├─ 1ª prioridade: equipe DISPONÍVEL na teamTable → uni:5000 (direto)
    ├─ 2ª prioridade: teamTable não vazia mas todas ocupadas
    │    → uni:5000 para qualquer equipe conhecida (equipe ocupada
    │      ainda recebe e confirma — decisão de atendimento é de outra
    │      camada, fora do escopo deste sistema)
    └─ 3ª prioridade: teamTable vazia (nenhuma equipe conhecida)
         → VictimAlert bcast:5004 ──▶ drones vizinhos (relay)
              └─ drone relay: verifica seenAlerts (dedup), repassa
                 seguindo a mesma prioridade acima
  SimpleTeamApp recebe VictimAlert
    → dedup (seenAlerts)
    → calcula busyDuration → available = false
    → VictimAck uni:5002 ──▶ drone origem (via originIp)
  SimpleDroneApp recebe VictimAck
    → remove de pendingAlerts

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

| Métrica | Fórmula | Unidade |
|---|---|---|
| **PDR** (taxa de entrega) | `alertsAcked / alertsGenerated × 100` | % |
| **Atraso E2E médio** | média de `meanE2EDelay` por drone com ACK | s |
| **Retransmissões por entrega** | `totalRetries / alertsAcked` | tentativas |
| **Overhead de comunicação** | `(alertsSentDirect + alertsSentRelay) / alertsAcked` | msgs/entrega |
| **Taxa de sucesso AppACK** | `alertsAcked / (alertsAcked + alertsExpired) × 100` | % |

> Denominador do overhead alinhado com PDR e retries/entrega: usa `alertsAcked` (entrega confirmada fim a fim) em vez de `alertsReceived` (chegada à equipe), para manter as três métricas coerentes com o mesmo critério de sucesso. `alertsSentDirect + alertsSentRelay` já soma toda transmissão de `VictimAlert` (originada ou repassada) — `alertsRelayed` não entra na conta para não duplicar.

---

## 11. Resultado Consolidado — 5 Seeds (pós-correção §8.1)

**Metodologia:** `repeat = 5`, `seed-set = ${repetition}` no `omnetpp.ini`. Cada seed é uma repetição independente da simulação completa (300 s); as 5 métricas são calculadas **por seed** e depois agregadas como média ± desvio-padrão entre seeds (`analysis/process_results.py`) — não somadas num único pool, para não mascarar a variabilidade entre execuções.

| Métrica | Média ± Desvio (n=5 seeds) |
|---|---|
| PDR | **27,0% ± 7,3%** |
| Atraso E2E médio | 5,09 s ± 3,64 s |
| Retransmissões por entrega | 14,3 ± 6,0 |
| Overhead | 23,6 ± 9,8 msgs/entrega |
| Taxa de sucesso AppACK | 31,0% ± 8,6% |
| Alertas gerados (total/seed) | 114,0 ± 11,3 |
| Alertas confirmados (ACK) | 31,2 ± 11,0 |
| Alertas expirados | 67,8 ± 4,4 |

> **Interpretação:** com o encaminhamento corrigido, o PDR médio de ~27% (variando 17,3%–36,4% entre as 5 seeds) reflete a limitação real de cobertura (alcance ~800 m em área de 25 km²) e a aleatoriedade de mobilidade/detecção entre execuções — não mais um defeito de protocolo. O desvio-padrão relativamente alto (±7,3 p.p. de PDR) é esperado num cenário com poucas equipes (5) competindo por cobertura de 15 drones móveis; é um indicativo de que comparações futuras (ex.: com vs. sem Bat Algorithm) devem reportar significância estatística entre médias, não apenas comparar pontos únicos. Este é o **baseline sem reposicionamento e sem obstáculos**, consolidado com múltiplas seeds, para comparação posterior com o cenário com obstáculos urbanos e com o reposicionamento via Bat Algorithm.

---

## 12. Parâmetros Pendentes de Validação

Os itens abaixo ainda precisam de revisão ou decisão antes da defesa:

- [ ] `victimInterval = 40 s` — intervalo médio de detecção adequado para o cenário SAR proposto?
- [ ] `teamTimeout = 30 s` — tempo máximo aceitável sem `TeamUpdate` antes de considerar link perdido?
- [ ] `serviceTime = 120 s` — tempo mínimo de triagem/estabilização no local, adequado para o cenário de inundação?
- [ ] `teamSpeed = 0,9 m/s` — média adequada para equipe em área severamente alagada?
- [ ] Potência dos drones (`20 mW → ~800 m`) e equipes (`50 mW → ~1 260 m`) — calibradas com as referências usadas na dissertação?
- [ ] `numDrones = 15` e `numTeams = 5` — densidade coerente com a área 5 km × 5 km da dissertação?
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

**Decisão:** manter `teamSpeed = 0.9mps`, `serviceTime = 120s` e `sim-time-limit = 300s` no `BasicTest` principal. As duas configs de teste permanecem no `omnetpp.ini` (não removidas — valor de registro negativo, mesmo padrão usado para `maxRetries = 15` em §8.1), mas não fazem parte do baseline reportado. Se o `teamSpeed`/`serviceTime` precisarem ser revisitados (por exemplo, quando o despacho de equipes deixar de ser um *round-robin* implícito e passar a depender de fato da disponibilidade — o que deve acontecer ao introduzir o Bat Algorithm), repetir o teste com `teamSpeed` como **escalar único** (não distribuição) para não introduzir o viés de RNG descrito no item 2.

### Próximo passo planejado

- [ ] Modelar obstáculos urbanos (prédios) via `IdealObstacleLoss`/`DielectricObstacleLoss` + `PhysicalEnvironment` do INET, para então acionar o gatilho Γ e o reposicionamento via Bat Algorithm (passo 14, ainda não implementado).
