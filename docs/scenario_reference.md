# ECHOSAR-Net — Referência do Cenário de Simulação

**Versão:** BasicTest (cenário base calibrado)  
**Simulador:** OMNeT++ 6.2.0 + INET Framework 4.5.4  
**Tempo de simulação:** 300 s

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

## 8.1. Correção de Roteamento — Beco sem Saída em Equipe Ocupada

**Bug identificado:** `forwardAlertOnce()` só selecionava equipe **disponível** (`e.available == true`) na `teamTable` para envio unicast. Quando todas as equipes conhecidas pelo drone estavam ocupadas (`available = false`), a função caía sempre no relay broadcast (`RELAY_PORT = 5004`) — porta que **nenhuma equipe escuta** (`SimpleTeamApp` só faz bind em `ALERT_PORT`, `DRONE_STATUS_PORT`, `TEAM_UPDATE_PORT`). Resultado: todo alerta gerado depois que as equipes conhecidas ficavam ocupadas caía num beco sem saída e expirava garantidamente, independente de `maxRetries`, `retryInterval` ou densidade de drones.

**Evidência:** com `busyDuration` típico de 330–2 340 s (muito maior que os 300 s de simulação), bastaram ~135 s para 3 das 5 equipes ficarem ocupadas permanentemente. A partir daí, qualquer drone cuja tabela só continha essas 3 equipes nunca mais conseguia entregar — confirmado pelos logs (`team0`, `team3`, `team4` ficam ocupados em t≈14s, 21s, 135s e nunca voltam a ficar livres dentro da janela de 300 s).

**Correção aplicada** (`SimpleDroneApp.cc`, `forwardAlertOnce`): mantém a prioridade por equipe disponível, mas adiciona um *fallback* para qualquer equipe conhecida (mesmo ocupada) antes de recorrer ao relay broadcast. O lado da equipe já confirmava o alerta independente do status `available` (`SimpleTeamApp.cc`), então a entrega passou a funcionar sem nenhuma mudança naquele lado.

**Impacto medido** (mesma seed, mesmos parâmetros, antes vs. depois):

| Métrica | Antes do fix | Depois do fix |
|---|---|---|
| PDR | 2,7% | **29,6%** |
| Confirmados | 3 | **32** |
| Expirados | 93 | 65 |
| Retries/entrega | 172,7 | **11,6** |
| Overhead | 215,5 msgs/alerta | **15,3** msgs/alerta |
| Taxa AppACK | 3,1% | **33,0%** |

Esse fix também invalida a hipótese de que `maxRetries` baixo (5) fosse a causa do PDR ruim — testes anteriores mostraram que **aumentar** `maxRetries` para 15 piorava o PDR (mais broadcasts de relay competindo no MAC, sem destino possível). O gargalo nunca foi o TTL; era o roteamento.

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

---

## 10. Métricas Computadas — `process_results.py`

| Métrica | Fórmula | Unidade |
|---|---|---|
| **PDR** (taxa de entrega) | `alertsAcked / alertsGenerated × 100` | % |
| **Atraso E2E médio** | média de `meanE2EDelay` por drone com ACK | s |
| **Retransmissões por entrega** | `totalRetries / alertsAcked` | tentativas |
| **Overhead de comunicação** | `(alertsSentDirect + alertsSentRelay) / alertsReceived` | msgs/alerta |
| **Taxa de sucesso AppACK** | `alertsAcked / (alertsAcked + alertsExpired) × 100` | % |

---

## 11. Resultado da Última Rodada (seed 0, pós-correção §8.1)

| Métrica | Valor |
|---|---|
| PDR | **29,6%** |
| Alertas gerados | 108 |
| Alertas confirmados (ACK) | 32 |
| Alertas expirados | 65 |
| Alertas pendentes ao final | 11 |
| Atraso E2E médio | 3,87 s |
| Retransmissões por entrega | 11,6 |
| Overhead | 15,3 msgs/alerta |
| Taxa de sucesso AppACK | 33,0% |

> **Interpretação:** com o roteamento corrigido, o PDR de ~30% reflete agora a limitação real de cobertura (alcance ~800 m em área de 25 km²) — não mais um defeito de protocolo. Esse é o **baseline sem reposicionamento e sem obstáculos** coerente para comparação posterior com o cenário com obstáculos urbanos e com o reposicionamento via Bat Algorithm.

---

## 12. Parâmetros Pendentes de Validação

Os itens abaixo ainda precisam de revisão ou decisão antes da defesa:

- [ ] `victimInterval = 40 s` — intervalo médio de detecção adequado para o cenário SAR proposto?
- [ ] `teamTimeout = 30 s` — tempo máximo aceitável sem `TeamUpdate` antes de considerar link perdido?
- [ ] `serviceTime = 120 s` — tempo mínimo de triagem/estabilização no local, adequado para o cenário de inundação?
- [ ] `teamSpeed = 0,9 m/s` — média adequada para equipe em área severamente alagada?
- [ ] Potência dos drones (`20 mW → ~800 m`) e equipes (`50 mW → ~1 260 m`) — calibradas com as referências usadas na dissertação?
- [ ] `numDrones = 15` e `numTeams = 5` — densidade coerente com a área 5 km × 5 km da dissertação?
- [ ] Tempo de simulação `300 s` — suficiente para cobrir os cenários comparativos, agora que o fix do §8.1 desbloqueou o roteamento?

### Resolvidos nesta sessão

- [x] `maxRetries = 5` / `retryInterval = 10 s` (janela 50 s) — **mantido**. Testes empíricos mostraram que aumentar para 15 (janela 150 s) piora o PDR por congestionamento de broadcast, não melhora por TTL maior. Ver §8.1.
- [x] PDR estruturalmente baixo (2–5%) — **causa raiz era bug de roteamento**, não calibração de parâmetros. Corrigido em §8.1; PDR baseline agora em ~30%.

### Próximo passo planejado

- [ ] Modelar obstáculos urbanos (prédios) via `IdealObstacleLoss`/`DielectricObstacleLoss` + `PhysicalEnvironment` do INET, para então acionar o gatilho Γ e o reposicionamento via Bat Algorithm (passo 14, ainda não implementado).
