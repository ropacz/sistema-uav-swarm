# Plano de Implementação — Reposicionamento com o Algoritmo do Morcego (BA)

> Mapeia o Capítulo "Abordagem Proposta" da dissertação para o código OMNeT++/INET
> existente (`echosar`). Cada fase é incremental, compilável e testável isoladamente.
>
> **Status:** PLANEJAMENTO — nenhum código deste plano foi escrito ainda.
> Referência cruzada: `docs/scenario_reference.md`, `CLAUDE.md`.

---

## 1. Objetivo e escopo

Implementar, na camada de aplicação do drone, o mecanismo reativo de quatro etapas
descrito no capítulo:

1. **Identificação da degradação do enlace** — janela deslizante de RSSI (sentido
   equipe→drone) + ARR (sentido drone→equipe), limiares, indicador binário `Γ_ij(t)`.
2. **Obtenção da posição do obstáculo** `p_obs` — camada sensorial abstraída, acionada
   sob demanda quando `Γ=1`, simulada por *ray-cast* contra o `PhysicalEnvironment`.
3. **Reposicionamento com o BA** — metaheurística que minimiza
   `F(p) = w1·ρ(p) + w2·d_des(p)` e devolve `p*`.
4. **Validação** — pacote de sondagem (*probe*) + reavaliação de `Γ`; memória de tabu
   espacial para obstáculos compostos; descarte após N tentativas.

O foco da contribuição permanece na **camada de aplicação + atuação de mobilidade**.
O processamento bruto de sensores (segmentação geométrica) é abstraído, conforme o
capítulo (Subseção "Obtenção da Posição do Obstáculo").

### Fora de escopo
- Coordenação do enxame / planejamento global de trajetória.
- Decisão de qual equipe atende a vítima (já tratada na literatura citada).
- Modelo físico de bateria — energia será aproximada pela **distância de deslocamento**
  acumulada (proxy), salvo decisão contrária (ver §9).

---

## 2. Estado atual × alvo (gap analysis)

| Capacidade exigida pelo capítulo | Existe hoje? | Lacuna |
|---|---|---|
| Medição de RSSI por enlace (equipe→drone) | ❌ | App não lê potência de recepção |
| ACK periódico drone→equipe para ARR | ⚠️ parcial | `DroneStatus` é enviado, mas a equipe **não** o confirma |
| Janela deslizante / monitor por equipe | ❌ | `TeamEntry` guarda só posição/disponibilidade |
| Limiares `θ_ARR`, `θ_RSSI` (log-distância) | ❌ | — |
| Indicador `Γ_ij(t)` | ❌ | — |
| Posição do obstáculo `p_obs` (sensor) | ❌ | obstáculos existem em `obstacles.xml`, mas não são consultados pela app |
| Algoritmo do Morcego | ❌ | — |
| Atuação: mover o drone para `p*` / *hover* | ❌ | `GaussMarkovMobility` é autônoma, sem comando externo |
| Probe + validação + tabu | ❌ | — |
| Obstáculos no rádio (`DielectricObstacleLoss`) | ✅ | já em `Cenario_ComObstaculos` |
| `PhysicalEnvironment` no `.ned` | ✅ | já importado/instanciado em `BasicNetwork.ned` |

**Dois itens de maior risco de engenharia** (detalhados em §9):
- **A. Extração de RSSI na app** — depende da propagação do tag `SignalPowerInd` até a aplicação.
- **B. Atuação de mobilidade** — `GaussMarkovMobility` não aceita destino comandado; exige
  uma mobilidade customizada que suporte `hover()` e `moveTo(p*)`.

---

## 3. Mapeamento capítulo → artefatos de código

| Etapa do capítulo | Artefato (novo/✎ modificado) |
|---|---|
| Janela + RSSI + ARR + `Γ` | ✎ `SimpleDroneApp` + novo `LinkMonitor` (struct/classe por equipe) |
| ACK periódico p/ ARR | ✎ `SimpleTeamApp` (confirma `DroneStatus`) **ou** nova msg `Probe`/`ProbeAck` |
| `p_obs` (sensor abstraído) | novo `ObstacleSensor` (helper que consulta `IPhysicalEnvironment`) |
| Bat Algorithm | novo `BatOptimizer` (classe C++ pura, sem dependência de módulo) |
| Atuação `hover`/`moveTo` | novo módulo de mobilidade `BatControlledMobility` (NED + C++) |
| Validação + probe + tabu | ✎ `SimpleDroneApp` + `BatOptimizer` (termo de penalidade tabu) |
| Métricas novas | ✎ `finish()` de `SimpleDroneApp` |
| Configs experimentais | ✎ `simulations/omnetpp.ini` |

### Arquivos novos previstos
```
src/control/BatOptimizer.h / .cc        # Algoritmo 1 (puro, testável isolado)
src/control/LinkMonitor.h               # janela deslizante + Γ por equipe (header-only)
src/control/ObstacleSensor.h / .cc      # ray-cast LoS → p_obs (sensor simulado)
src/mobility/BatControlledMobility.h / .cc / .ned   # hover + moveTo
src/messages/Probe.msg                  # (se optar pelo probe dedicado p/ ARR e validação)
```

---

## 4. Detalhes técnicos por etapa

### 4.1 RSSI (sentido equipe → drone) — Eq. RSSI_avg

- A equipe já transmite `TeamUpdate` em *broadcast* a cada `sendInterval=1s`
  (`SimpleTeamApp::sendUpdate`). Estes quadros são as "mensagens periódicas" da janela.
- No `SimpleDroneApp::handleTeamUpdate(pkt)`, ler a potência de recepção:
  ```cpp
  #include "inet/physicallayer/wireless/common/contract/packetlevel/SignalTag_m.h"
  using namespace inet::physicallayer;
  if (auto *t = pkt->findTag<SignalPowerInd>()) {
      double rssi_dBm = inet::math::mW2dBmW(t->getPower().get() * 1000.0); // W → dBm
      teamMonitor[teamId].pushRssi(rssi_dBm);   // alimenta a janela
  }
  ```
  > `SignalPowerInd` é anexado em `ReceiverBase.cc:80`. **Verificar** que o tag
  > sobrevive até a app através do UDP (ver §9-A; há fallback por *subscription* de sinal).
- `RSSI_k` em dBm → `RSSI_avg` = média aritmética dos dBm na janela (não a potência média),
  exatamente como na Eq. RSSI_avg.

### 4.2 ARR (sentido drone → equipe) — Eq. ARR

Precisamos de um ciclo *ida-e-volta* confirmado por janela. **Decisão de projeto** (§9-C):

- **Opção 1 (reaproveitar `DroneStatus`)** — o drone já envia `DroneStatus` unicast à
  equipe ao receber cada `TeamUpdate`. Adicionar à `SimpleTeamApp` um ACK de aplicação
  (`DroneStatusAck`, msg de 64 B) de volta ao drone. Cada `DroneStatus` enviado conta
  como "mensagem na janela"; cada ACK recebido incrementa `N_ACK`.
- **Opção 2 (probe dedicado)** — `Probe`/`ProbeAck` periódico independente do tráfego de
  missão. Mais limpo conceitualmente e reutilizável na **validação** (§4.6), ao custo de
  uma mensagem nova. **Recomendado**, pois o capítulo já prevê o *probe* na validação.
- `ARR = N_ACK / N_W`, com `N_W` = número de mensagens **enviadas** na janela completa.

### 4.3 `LinkMonitor` — janela deslizante por equipe

`src/control/LinkMonitor.h` (header-only), uma instância por `teamId` (em `map<string,LinkMonitor>`):

```cpp
struct LinkMonitor {
    std::deque<double> rssiWindow;     // últimos RSSI (dBm) recebidos
    std::deque<bool>   ackWindow;      // últimas N_W tentativas: confirmada?
    int    N_W;                        // tamanho da janela
    void   pushRssi(double dBm);       // mantém |window| ≤ N_W
    void   pushAttempt(bool acked);    // mantém |ackWindow| == N_W
    bool   windowComplete() const;     // ackWindow cheia
    double rssiAvg() const;            // média dBm; NaN se n==0
    int    rssiCount() const;          // n
    double arr() const;                // N_ACK / N_W
};
```

### 4.4 Limiares e indicador `Γ_ij(t)` — Eqs. limiar_arr, limiar_rssi, gamma

Parâmetros NED novos em `SimpleDroneApp.ned`:

| Símbolo | Param NED | Default sugerido (calibrar) |
|---|---|---|
| `N_W` | `windowSize` | 20 |
| `L_max` | `maxLosses` | 2  → `θ_ARR = 1 − 2/20 = 0,90` |
| `RSSI(d0)` | `rssiRef` | derivar do `FreeSpacePathLoss` a `d0=1m`, 2,4 GHz |
| `d0` | `refDistance` | 1 m |
| `η` | `pathLossExp` | 2,0 (espaço livre; calibrar) |
| `Δ` | `rssiMargin` | 6 dB |

- `θ_ARR = 1 − L_max/N_W` (constante).
- `θ_RSSI(d_ij) = RSSI(d0) − 10·η·log10(d_ij/d0) − Δ`, com `d_ij` da Eq. distancia
  (norma euclidiana entre `p_di` e a `posX/posY` da equipe na `teamTable`).
- `Γ_ij` (Eq. gamma): avaliar **só** com janela completa.
  - `n≥1`: `Γ=1` ⟺ `RSSI_avg < θ_RSSI` **E** `ARR < θ_ARR`.
  - `n=0`: `Γ=1` ⟺ `ARR < θ_ARR` (RSSI indefinido).
- Função `bool evaluateGamma(teamId)` chamada ao **fechar** cada janela (a cada `N_W`
  beacons de uma equipe) — não a cada pacote.

### 4.5 Obtenção de `p_obs` — sensor simulado (`ObstacleSensor`)

Aciona **somente** quando `Γ=1` (custo sob demanda, conforme o capítulo).

`src/control/ObstacleSensor.h/.cc`: dado `p_di`, `p_ej` (última posição conhecida da
equipe) e o ponteiro para o `PhysicalEnvironment`, faz *ray-cast* na LoS:

```cpp
// IPhysicalEnvironment: getNumObjects(), getObject(i) → PhysicalObject
//   .getPosition(), .getOrientation(), .getShape() (ShapeBase)
// ShapeBase::computeIntersection(LineSegment, i1,i2,n1,n2) → bool   (coords locais!)
// Transformar o segmento p_di→p_ej para o referencial local do objeto
// (subtrair position, aplicar inverse(orientation)) — ver TracingObstacleLoss como modelo.
std::optional<Coord> senseObstacleOnLoS(p_di, p_ej, physEnv);
```
- Se algum objeto intercepta a LoS → `p_obs = objeto.getPosition()` (centro da barreira,
  Eq. p_obs). Retorna o **mais próximo** do drone se houver vários.
- Se **nenhum** objeto intercepta → retorna vazio: degradação atribuída à mobilidade da
  equipe (saiu do alcance) → **aborta** o reposicionamento, drone retoma a trajetória
  (conforme o capítulo). Importante para não reposicionar à toa.
- Esta é a abstração honesta do "sensor": consulta a *ground truth* do ambiente, sem
  modelar LiDAR/câmera. Documentar a abstração em `scenario_reference.md`.

### 4.6 Bat Algorithm (`BatOptimizer`) — Algoritmo 1, Eqs. fitness–ba_rate

Classe C++ pura, **sem** herdar de `cModule` (testável em unidade), recebendo um
`cRNG*`/`cModule*` para sorteios reprodutíveis por seed:

```cpp
struct BatParams { int Nb=40, Imax=30, G; double w1=0.6,w2=0.4,
                   sigmaRho, dMax, zMin, zMax, fMin, fMax, alpha=0.9, gamma=0.9, r0; };
class BatOptimizer {
  public:
    Coord optimize(const Coord& p_di, const Coord& p_obs,
                   const std::vector<Coord>& tabu);   // tabu: §4.7
  private:
    double fitness(const Coord& p, ...);  // F = w1·ρ + w2·d_des (+ penalidade tabu)
};
```

- **Sorteios via RNG do OMNeT** (`module->uniform(0,1)`/`intuniform`), **nunca** `std::rand`,
  para respeitar `seed-set = ${repetition}` e garantir reprodutibilidade.
- `ρ(p)` = Eq. rho (decaimento exponencial, `σ_ρ`); `d_des(p)` = Eq. ddes (normalizada por
  `d_max`). Ambas em `[0,1]` → ponderação `w1/w2` comparável.
- Inicialização, frequência (ba_freq), velocidade/posição (ba_vel/ba_pos), busca local
  (ba_local), aceitação + atualização de `A`/`r` (ba_loud/ba_rate), projeção nos limites
  `d_max` e `[z_min,z_max]`, e **parada antecipada** por estagnação `G` — exatamente o
  Algoritmo 1.
- Valores iniciais (calibrar depois): `Nb=40, Imax=30` [LONG2024]; `α=γ=0,9` [HAMEED2020];
  `f_min/f_max` **recalibrados** para a escala física `d_max` (NÃO usar 0..100 abstratos).

### 4.7 Validação e memória de tabu — Subseção "Validação"

- Após `p*`, comandar `moveTo(p*)` (§4.8). Ao chegar, enviar **probe curto sem payload**
  (reuso da msg `Probe` da Opção 2, §4.2). ACK imediato ⇒ candidato a sucesso.
- Reavaliar `Γ_ij` pelo mesmo monitor (§4.4): `Γ→0` **e** probe respondido ⇒ enlace
  recuperado → drone envia a mensagem de socorro original e sai do modo de reposicionamento.
- Probe sem resposta **ou** `Γ` ainda em degradação severa ⇒ obstáculo composto:
  - adicionar `p*` à lista de **tabu** (`std::vector<Coord> tabuPoints`);
  - `BatOptimizer::fitness` ganha termo de penalidade de repulsão para cada ponto tabu
    (mesma forma de `ρ`, com `σ` próprio) → força nova topologia geométrica;
  - reexecutar BA. Após `maxRepositionTries` (parametrizável, ligado a "bateria"), **descartar**
    a mensagem para preservar a FANET.

### 4.8 Atuação de mobilidade (`BatControlledMobility`) — item de risco B

`GaussMarkovMobility` não aceita destino comandado. Criar `src/mobility/BatControlledMobility`
(NED `like IMobility`, C++ estendendo `MovingMobilityBase`):

- **Modo CRUISE (default):** replica o movimento Gauss-Markov atual (altitude 80–120 m,
  8–15 m/s) — preserva o baseline.
- **Modo HOVER:** posição congelada (estabiliza para a leitura sensorial, §4.5).
- **Modo MOVE_TO(target):** desloca-se em linha reta até `p*` à velocidade de cruzeiro;
  ao chegar, dispara *callback*/sinal para a app prosseguir à validação; volta a CRUISE.
- API chamada pela app (que já obtém o submódulo via `getSubmodule("mobility")`):
  `hover()`, `moveTo(Coord)`, `resumeCruise()`, `bool arrived()`.
- Trocar no `.ini` (apenas nos configs com BA):
  `**.drone[*].mobility.typename = "echosar.mobility.BatControlledMobility"`.

> Alternativa mais barata (se B se mostrar custoso): manter `GaussMarkovMobility` e modelar
> o reposicionamento **logicamente** (teletransporte instantâneo + atraso fixo), medindo só
> o efeito na comunicação. Perde realismo de trajetória mas destrava a avaliação de rede.
> Decidir em §9-B.

### 4.9 Máquina de estados da app do drone

Adicionar ao `SimpleDroneApp` um estado por enlace monitorado:
```
CRUISE ──Γ=1──▶ SENSING ──p_obs?──▶ (sim) OPTIMIZING ──p*──▶ REPOSITIONING
   ▲                │ (não: mobilidade)        │                    │
   │                └──────── abort ───────────┘            chegou → VALIDATING
   │                                                                  │
   └──────────── Γ=0 & probe ok ──────────────────────────────────────┤
                                                                        │
                              tabu + retry (< maxTries) ◀── falhou ─────┘
                                          │
                                    descarta msg (≥ maxTries)
```

---

## 5. Mensagens e parâmetros novos

### Mensagens (`src/messages/`)
- `Probe.msg` → `ProbeChunk` (msgId, originIp, seq; ~32 B, sem payload).
- `ProbeAck.msg` → `ProbeAckChunk` (msgId, seq; 32 B). *(ou reaproveitar `DroneStatus`+ack — §4.2)*

### Parâmetros NED (`SimpleDroneApp.ned`)
```
bool   enableBatReposition = default(false);  // liga/desliga o mecanismo (p/ baseline A/B)
int    windowSize          = default(20);     // N_W
int    maxLosses           = default(2);      // L_max
double refDistance @unit(m)= default(1m);     // d0
double rssiRef @unit(dBm)  = default(...);    // RSSI(d0)
double pathLossExp         = default(2.0);    // η
double rssiMargin @unit(dB)= default(6dB);    // Δ
double probeInterval @unit(s) = default(2s);
int    batPop              = default(40);     // Nb
int    batIters            = default(30);     // Imax
int    batStagnation       = default(8);      // G
double wRepulsion          = default(0.6);    // w1
double wDisplacement       = default(0.4);    // w2  (w1+w2=1)
double sigmaRho @unit(m)   = default(150m);   // σ_ρ
double dMaxReposition @unit(m) = default(300m); // d_max
double zMinReposition @unit(m) = default(80m);
double zMaxReposition @unit(m) = default(120m);
double batAlpha            = default(0.9);
double batGamma            = default(0.9);
int    maxRepositionTries  = default(3);
```

`enableBatReposition=false` deve reproduzir **exatamente** o comportamento atual
(regressão — ver §7).

---

## 6. Métricas e configurações experimentais

### Novos scalars em `SimpleDroneApp::finish()`
- `gammaTriggers` — quantas vezes `Γ=1` disparou.
- `repositionsAttempted` / `repositionsSucceeded`.
- `sensorAbortsNoObstacle` — `Γ=1` mas sem obstáculo (mobilidade).
- `totalRecoveryTime` / `meanRecoveryTime` — de `Γ=1` até `Γ=0` confirmado.
- `totalRepositionDistance` — proxy de energia (Σ‖p* − p_di‖).
- `tabuActivations` — obstáculos compostos detectados.

### Configs no `omnetpp.ini` (comparação A/B controlada)
Reaproveitar a infraestrutura existente (`Cenario_ComObstaculos` já tem
`DielectricObstacleLoss` + `obstacles.xml`). Adicionar:

```ini
[Config Cenario_ComObstaculos_BA]      # ECHOSAR completo
extends = Cenario_ComObstaculos
**.drone[*].app[0].enableBatReposition = true
**.drone[*].mobility.typename = "echosar.mobility.BatControlledMobility"
```
A comparação principal passa a ser **três vias**, mesma seed/rede/mobilidade/carga:
`Cenario_SemObstaculos` (teto) × `Cenario_ComObstaculos` (sem BA, piso) ×
`Cenario_ComObstaculos_BA` (com BA). Diferença atribuível só ao mecanismo BA.

> `obstacles.xml` ainda não existe no repo (referenciado pelo `.ini`) — criar com os
> 3 blocos `300×300×120 m` citados em `omnetpp.ini` antes de rodar os configs com obstáculo.

---

## 7. Estratégia de testes / validação

1. **Teste de unidade do `BatOptimizer`** (standalone, fora do OMNeT ou em config minúsculo):
   obstáculo fixo, verificar que `p*` se afasta de `p_obs`, respeita `d_max`/`[z_min,z_max]`,
   e converge (estagnação). Determinístico por seed.
2. **`ObstacleSensor`**: cenário com 1 bloco entre drone e equipe estáticos → retorna o
   centro correto; sem bloco → retorna vazio. (estender o `SmokeTest_Beacons`).
3. **Regressão `enableBatReposition=false`**: scalars de `BasicTest` idênticos ao baseline
   atual (o mecanismo desligado não pode alterar nada).
4. **Integração** `SmokeTest` novo: 1 drone + 1 equipe + 1 obstáculo posicionado para
   bloquear a LoS → observar a sequência `Γ=1 → sensor → BA → moveTo → Γ=0` no log INFO.
5. **A/B** nos 3 configs (5 seeds) → comparar PDR/AppACK/atraso/recuperação via
   `analysis/process_results.py` (estender o parser com as novas métricas).

---

## 8. Ordem de implementação (incremental, cada passo compila e roda)

- [ ] **F0 — Preparação:** criar `obstacles.xml`; confirmar que `Cenario_ComObstaculos`
      roda e que a obstrução degrada o PDR (sanity do efeito a mitigar).
- [ ] **F1 — Telemetria de RSSI:** ler `SignalPowerInd` em `handleTeamUpdate`, gravar
      `RSSI_avg` por equipe como scalar. Valida o item de risco A cedo. *(sem comportamento novo)*
- [ ] **F2 — ARR + janela:** `LinkMonitor`; probe/ack (ou DroneStatus-ack); gravar `ARR`.
- [ ] **F3 — Limiares + `Γ`:** Eqs. limiar_*/gamma; logar disparos `Γ=1` (ainda sem ação).
- [ ] **F4 — Sensor `p_obs`:** `ObstacleSensor` (ray-cast); logar `p_obs`/aborto.
- [ ] **F5 — `BatOptimizer`:** Algoritmo 1 puro + teste de unidade; logar `p*` (sem mover).
- [ ] **F6 — Atuação:** `BatControlledMobility` (hover/moveTo); fechar o laço movimento.
- [ ] **F7 — Validação + tabu:** probe pós-movimento, reavaliação de `Γ`, tabu, descarte.
- [ ] **F8 — Métricas + configs + análise:** scalars novos, `Cenario_ComObstaculos_BA`,
      parser e figuras.

Cada fase: editar → `make makefiles` (se novo arquivo/.msg) → `make MODE=debug`
(ver `CLAUDE.md`, build DEBUG obrigatório) → `./run.sh -c <Config>`.

---

## 9. Riscos e decisões em aberto (precisam de validação/escolha)

- **A. RSSI até a app.** `SignalPowerInd` é anexado no rádio (`ReceiverBase.cc:80`). Risco:
  o tag pode não sobreviver à subida pela pilha UDP. **Plano B:** subscrever, no nível do
  host, o sinal de recepção do rádio (`receptionEndedSignal`/`SnirInd`) e correlacionar com
  o remetente pelo endereço L2/L3. Resolver em **F1**, antes de qualquer outra coisa.
- **B. Atuação de mobilidade.** `BatControlledMobility` (§4.8) é a maior peça de engenharia.
  Decisão: mobilidade comandada real **vs.** reposicionamento lógico (teletransporte+atraso).
  Recomendo a real para sustentar as métricas de "tempo de recuperação" e "energia".
- **C. ARR: probe dedicado vs. DroneStatus-ack.** Recomendo **probe dedicado** (Opção 2):
  reutilizável na validação (§4.7) e conceitualmente alinhado ao capítulo.
- **D. Energia.** Proxy por distância de deslocamento, ou integrar o framework de energia do
  INET (`power`/`energyStorage`)? Proxy é suficiente para a tese; decidir em F8.
- **E. Acoplamento `Γ` × AODV.** O AODV pode mascarar a degradação roteando multi-hop antes
  de `Γ` disparar. Verificar se o sinal de degradação do **enlace direto** equipe–drone
  ainda é observável; se necessário, restringir o monitor ao enlace de 1 salto.
- **F. Múltiplas equipes simultâneas.** `Γ_ij` é por par drone–equipe. Definir política
  quando vários enlaces degradam ao mesmo tempo (reposicionar pela equipe mais relevante /
  alerta pendente). Default sugerido: a equipe do alerta pendente mais antigo.

---

## 10. Definição de "pronto"

- Os 3 configs A/B rodam 5 seeds sem crash.
- Com BA ligado e obstáculos, **PDR/AppACK sobem** em relação ao `Cenario_ComObstaculos`
  sem BA, com `meanRecoveryTime` e `totalRepositionDistance` reportados.
- `enableBatReposition=false` é bit-idêntico ao baseline (regressão F3-passo 3).
- `analysis/process_results.py` gera figuras incluindo as métricas novas.
- `docs/scenario_reference.md` atualizado com o mecanismo, a abstração do sensor e as
  fórmulas; `docs/params_reference.md` com os novos parâmetros e suas fontes.
