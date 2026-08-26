# Desvios e extensões em relação à diretriz normativa

Este documento registra **apenas o que difere** de
[`Diretrizes_para_implementao_do_projeto_no_OMNeT_e_INET.pdf`](Diretrizes_para_implementao_do_projeto_no_OMNeT_e_INET.pdf).
Tudo o que não aparece aqui foi implementado conforme a diretriz.

Cada item indica a seção do PDF, o que a implementação faz de diferente, por quê,
e onde isso vive no repositório. Os valores citados são apenas nomes de chave; os
valores em si pertencem aos arquivos em `simulations/`.

Três categorias:

- **Desvio** — a implementação contraria ou substitui o que a diretriz pede.
- **Extensão** — a implementação acrescenta algo que a diretriz não previu.
- **Lacuna** — a diretriz pede algo que ainda não está implementado.

---

## Desvios

### D1. Mobilidade das equipes — §3

A diretriz especifica **Random Walk**. A implementação usa
`RandomWaypointMobility` do INET 4.5.4.

Decisão de orientação posterior à reunião que originou a tabela. O INET não
oferece um Random Walk equivalente para nós de solo com pausa entre trechos, e o
Random Waypoint produz o mesmo caráter — deslocamento errático, sem rota
planejada — com parâmetros de velocidade e pausa explicitamente configuráveis.

`simulations/experiment.ini`, `**.team[*].mobility.typename`.

### D2. Dimensões dos obstáculos — §10 e §15

A diretriz apresenta dois valores conflitantes entre si: §10 indica
5 × 3 × 2 m e §15 revisa para 15 × 20 × 10 m. A implementação usa **dois
edifícios ocos com envelope de 120 × 120 × 18 m**, cada um formado por quatro
paredes e uma cobertura de 0,30 m de concreto.

O §15 autoriza explicitamente o ajuste: *"As dimensões podem ser ajustadas após
testes preliminares, desde que as alterações sejam justificadas e mantidas
consistentes entre os cenários com e sem reposicionamento."* A justificativa é
geométrica, e foi medida:

- Um bloco de 20 × 20 m ocupa 0,04% de uma área de 1 km². A probabilidade de a
  linha drone–equipe cruzá-lo é desprezível, e o mecanismo avaliado nunca é
  exercitado. Verificado experimentalmente: com blocos desse tamanho,
  `obstaclesDetected` permaneceu zero.
- O envelope de 120 m corresponde a uma quadra urbana, coerente com a
  recomendação do próprio §15 de *"representar obstáculos compatíveis com
  edificações presentes em áreas urbanas"*.
- A construção **oca** é necessária por causa do modelo de propagação. O
  `DielectricObstacleLoss` atenua proporcionalmente ao comprimento do percurso
  dentro do dielétrico. Um cuboide maciço de 120 m faria o INET interpretar até
  120 m contínuos de concreto, o que não representa nenhuma edificação real.
  Com paredes de 0,30 m, o percurso atravessa no máximo duas paredes.
- A altura de 18 m fica acima da altitude inicial de voo, sem o que o enlace
  passaria sistematicamente por cima do obstáculo (a coerência geométrica que o
  §11 exige verificar).

Os obstáculos são idênticos nos dois braços pareados. Cada edifício é **um
obstáculo lógico**, embora composto por cinco objetos geométricos — a contagem
de "2 obstáculos" da §3 refere-se aos edifícios.

`simulations/professor-scenario-obstacles.xml`.

### D3. Intervalo entre alertas periódicos — §7.3 e §13

A diretriz é inconsistente consigo mesma: §7.3 define o intervalo entre alertas
como 30 s, e a tabela do §13 indica 10 s. A implementação adota **30 s**.

É o único valor compatível com a própria restrição do §18: *"A validade do
alerta deve ser inferior ao intervalo entre alertas periódicos quando houver
apenas um alerta pendente por vítima."* Com validade de 25 s (§13), um intervalo
de 10 s violaria essa condição e produziria alertas concorrentes para a mesma
vítima — que o §7.3 proíbe. A restrição é verificada em tempo de execução
(`alertInterval must exceed alertTtl to prevent overlapping alerts`).

Todos os demais valores da tabela do §13 seguem a diretriz.

### D4. Alcance do sensor de obstáculos — §14

A diretriz sugere `obstacleSensorRange` configurável e cita 30 m como exemplo. A
implementação **não aplica limite superior** no cenário científico.

Os 30 m correspondem à faixa frontal do sensor de prevenção de colisões do DJI
Phantom 4 Pro. Essa faixa não determina a capacidade de constatar que um
edifício bloqueia um enlace de centenas de metros: um prédio que obstrui a linha
drone–equipe está, tipicamente, longe do drone. Medido: com o limite de 30 m,
detecção e obstrução eram quase disjuntas e `obstaclesDetected` permanecia zero
em todas as execuções do cenário científico.

O §14 já prevê essa situação e define como declará-la: *"Se a avaliação utilizar
a geometria completa do cenário para testar posições candidatas, essa hipótese
deve ser declarada como: Conhecimento geométrico idealizado disponibilizado ao
mecanismo de reposicionamento."* A implementação estende a mesma hipótese à
etapa de detecção.

**Formulação para a dissertação:** a presença de obstáculo é determinada por uma
verificação geométrica idealizada do segmento entre o drone e a última posição
conhecida da equipe. A obtenção real dessa geometria por câmera, LiDAR ou mapa
cooperativo não faz parte do escopo.

O parâmetro continua existindo: valor negativo desabilita o limite. Os smoke
tests fixam 30 m e 20 m, porque o teste obrigatório §28.4 valida justamente o
comportamento de faixa limitada.

`src/sensing/AbstractObstacleSensor.{h,cc,ned}`, `simulations/omnetpp.ini`.

### D5. Nomes de escalares e de parâmetros — §17 e §24

A diretriz lista nomes de exemplo que a implementação não segue literalmente:

| Diretriz | Implementação |
| --- | --- |
| `generatedAlerts`, `deliveredAlerts`, `confirmedAlerts` | `alertsGenerated`, `alertsDelivered`, `alertsConfirmed` |
| `transmissionAttempts`, `receivedAttempts` | `alertAttemptsSent`, `attemptsReceived` |
| `minimumFlightAltitude`, `maximumFlightAltitude` | `minimumAltitude`, `maximumAltitude` |
| `minimumObstacleClearance` | `obstacleSafetyMargin` |
| `maximumFlightTime` | `flightTimeLimit` |

O §24 apresenta os nomes de escalares como *"Exemplos"*, não como contrato. A
semântica é idêntica em todos os casos. Renomear agora invalidaria os resultados
já gravados sem ganho de conteúdo.

### D6. Vetores não são gravados — §24

A diretriz permite registrar atrasos e reposicionamentos como vetores
(`deliveryDelay`, `confirmationDelay`, `hopCount`, `repositionDistance`,
`repositionDuration`) *"quando necessário"*. A implementação mantém
`**.vector-recording = false` e grava soma e contagem como escalares.

A unidade experimental é a execução completa (§22), e as médias por execução são
reconstruídas a partir de soma e contagem sem perda. A consequência é que a
**distribuição** intra-execução dos atrasos não fica disponível para análise
posterior; apenas a média. Reativar é uma linha de configuração, ao custo de
volume de dados.

---

## Extensões

### E1. Propagação de `TeamUpdate` pela FANET

A diretriz descreve `TeamUpdate` como broadcast da equipe (§7.1) e atribui ao
AODV a descoberta da rota (§8). Isso é insuficiente na prática: o AODV encontra
rota multissalto, mas o drone precisa **primeiro conhecer o endereço de
destino**. Com broadcast de um salto, só drones diretamente ao alcance da equipe
a descobrem, e os demais drones do enxame ficam sem função.

Medido antes da mudança: as falhas por equipe jamais conhecida dominavam o
cenário. Depois: zero em todas as execuções, e entregas multissalto passaram a
ocorrer.

A implementação acrescenta encaminhamento controlado:

- cada drone repassa uma atualização **no máximo uma vez** por combinação de
  `teamId` e `sequenceNumber` — a mesma condição que aceita uma sequência mais
  recente é a que autoriza o repasse, de modo que deduplicação e repasse único
  decorrem da mesma regra;
- limite de saltos (`teamUpdateMaxHops`);
- jitter curto antes de repassar (`teamUpdateForwardJitter`), para dispersar
  retransmissões simultâneas;
- entradas continuam expirando normalmente.

Dois campos novos na mensagem, além dos mínimos do §7.1: `hopCount` e
`teamAddress`. O endereço viaja dentro da mensagem porque, sem ele, um drone
distante aprenderia o endereço do **relay** em vez do da equipe e endereçaria o
alerta ao host errado. No primeiro salto o endereço é lido do pacote recebido,
como o §8 exige; os repasses apenas o preservam.

O AODV permanece sem modificação: ele continua sendo o responsável por encontrar
a rota unicast até o endereço descoberto.

`src/messages/TeamUpdate.msg`, `src/app/DroneApp.cc`.

### E2. Retenção da última posição conhecida da equipe

O §8 exige que registros antigos expirem, e o §15 condiciona o acionamento do BA
a *"a equipe continua conhecida"*. A implementação separa dois prazos:

- `teamEntryLifetime` encerra a validade **operacional**: a entrada deixa de ser
  elegível em `selectTargetTeam()` e não endereça nova tentativa;
- `lastKnownTeamRetention` mantém a última posição **marcada como
  desatualizada**, disponível somente ao mecanismo de recuperação.

Sem isso, a degradação do enlace — exatamente o caso que o reposicionamento
existe para tratar — apagava a informação necessária para reposicionar. O BA
continua **nunca** sendo acionado por uma equipe que jamais foi conhecida.

**Limitação a declarar:** a posição utilizada durante a recuperação pode estar
desatualizada devido à mobilidade da equipe. Não há predição de posição nem
extrapolação de velocidade.

`src/app/DroneApp.cc`, `src/app/TeamLinkState.h`.

### E3. Falha operacional separada em três diagnósticos

O §8 pede registrar a ausência de equipe conhecida como falha operacional. Um
contador único misturava situações distintas e escondia qual delas dominava o
cenário. A implementação registra:

| Contador | Significado |
| --- | --- |
| `neverKnownTeamSelectionEvents` | o drone nunca recebeu informação de equipe alguma |
| `expiredKnownTeamSelectionEvents` | conhecia uma equipe, mas a informação expirou |
| `knownTeamNoAckTimeoutEvents` | havia equipe conhecida, o alerta foi transmitido, e expirou sem confirmação |

**A unidade é evento, não alerta.** Um mesmo alerta pode falhar a seleção de
destino em várias oportunidades de envio, então estes contadores **podem exceder
o número de alertas gerados** — ler "65 eventos" como "65 alertas com falha"
seria incorreto. São diagnósticos de exposição, não desfechos por alerta.

`knownTeamNoAckTimeoutEvents` deixou de ser gravado como escalar (por hora, a
análise só mantém atendimento e perda — ver L1). O sinal `"knownTeamNoAck"`
continua sendo emitido por `DroneApp` e aceito por `ExperimentMetrics`, só não
é mais contabilizado; a contagem é candidata a voltar, comentada no topo de
`ExperimentMetrics.cc` junto com as demais.

`src/metrics/ExperimentMetrics.{h,cc}`.

### E4. Restrições adicionais na viabilidade de candidatos

O §17 lista as restrições obrigatórias, todas implementadas. A implementação
acrescenta duas condições de linha de visada em `RepositionFitness::feasible()`:

- o trajeto entre a posição atual e a candidata deve estar livre;
- a posição candidata deve ter linha de visada até a posição estimada da equipe.

A segunda decorre da finalidade do reposicionamento: uma posição que permanece
obstruída não cumpre o objetivo e não deveria competir apenas por penalidade de
custo. Pelo mesmo motivo, `C_obstaculo` recebe **custo máximo** quando o
candidato continua obstruído, em vez de apenas o decaimento exponencial do
§16.2.

O efeito é um espaço de busca mais restrito que o da diretriz. Medido: 86% das
ativações do BA resultam em movimento iniciado, então a restrição não está
inviabilizando a busca.

`src/optimization/RepositionFitness.cc`.

### E5. Potência de transmissão calibrada

A diretriz não especifica potência de transmissão. Ela foi escolhida em teste
preliminar **antes** da campanha, por uma regra fixada previamente: manter pelo
menos 80% dos alertas entregues no cenário sem obstáculos com o BA desligado.

| Potência | Alcance FSPL nominal | PDR sem obstáculos |
| --- | --- | --- |
| 2,88 mW | 300 m | 29,6% |
| 5,12 mW | 400 m | 50,4% |
| **10 mW** | **559 m** | **80,8%** |
| 20 mW | 790 m | 99,2% |

10 mW é a menor potência que atende o critério. Potência menor preserva mais
falhas de enlace, que são a exposição do mecanismo avaliado; 20 mW entrega
praticamente tudo e quase não gera gatilho.

O critério foi fixado antes de observar o desempenho do BA e **não foi
relaxado** depois, precisamente para não configurar calibração dirigida ao
resultado desejado.

Configuração `Calibration_NoObstacles`, prevista pelo §25.3 (*"Cenários sem
obstáculos podem ser executados separadamente como referência"*).

### E6. Configurações auxiliares

Duas configurações que não constam do §25 e **não são braços do experimento**:

- `Calibration_NoObstacles` — referência preliminar para escolher a potência.
- `Calibration_Exposure` — verifica em seeds reservadas (deslocadas em 100 para
  não coincidir com as da campanha) se o funil gatilho → obstáculo → BA →
  movimento é alcançável. Não compara braços.

Existe ainda `MainExperiment_*`, um contraste pareado mínimo com uma vítima e
uma equipe. É um subconjunto do §25, não uma configuração adicional: fixar
`numTeams = 1` elimina a seleção de destino como fator de confusão. A campanha
completa do §25 corresponde às configurações `Scenario1_*`.

### E7. Injeção determinística de falha nos testes

`TeamApp` possui `ackStartTime`: antes desse instante a equipe recebe alertas e
não responde. Serve exclusivamente para tornar determinísticos os testes
obrigatórios §28.5 e §28.7. O valor padrão é zero, sem efeito nos cenários
científicos.

Os testes funcionais fixam também a própria potência e os próprios limites de
sensor, para não dependerem da calibração do cenário científico:
**as configurações dos testes funcionais não representam os parâmetros do
experimento principal.**

`src/app/TeamApp.ned`, `simulations/validation/smoke-tests.ini`.

---

## Lacunas

### L1. Teste estatístico formal — §27

O §27 pede média, mediana quando pertinente, desvio padrão, intervalo de
confiança, número de repetições e a diferença entre braços; e um teste
compatível com a métrica (pareado, não paramétrico ou de permutação, e análise
específica para resultados binários com poucos eventos por execução).

A análise hoje produz apenas atendimento e perda por braço (proporção de
`alertId` únicos com ACK e sem entrega). Um relatório anterior calculava média
por braço, diferença pareada, desvio padrão, intervalo de confiança de 95% e o
número de pares — removido temporariamente para simplificar a entrega em
andamento; recuperável do histórico do Git
(`analysis/reports/report_main_experiment.py`, função `summarize()`).

Faltam, em qualquer versão: **mediana** e um **teste formal com estatística e
p-valor**. Pendência a resolver antes da versão final da dissertação.

---

## Conformidade verificada

Implementado conforme a diretriz, sem desvio:

- **§3** área, duração, drones, vítimas, equipes, altitude, velocidade,
  mobilidade dos drones (Gauss–Markov 3D), vítimas estáticas, 802.11 DSSS
  1 Mbit/s, UDP, AODV sem modificações, repetições ≥ 30, 900 s operacionais.
- **§5** TTL de aplicação permite múltiplos saltos; alertas não são limitados a
  um salto.
- **§6** todos os componentes com os nomes sugeridos: `SarScenarioManager`,
  `StaticVictim`, `TeamApp`, `DroneApp`, `AbstractObstacleSensor`,
  `BatAlgorithm`, `RepositionFitness`, `BaGaussMarkovMobility`,
  `ExperimentMetrics`.
- **§7** campos mínimos das quatro mensagens.
- **§8** descoberta apenas por mensagens recebidas, duplicatas não renovam,
  fora de ordem ignoradas, expiração, endereço obtido do pacote, seleção da
  equipe conhecida mais próxima, sem gerenciador global.
- **§9** detecção da vítima como evento abstrato.
- **§11** coerência geométrica verificada em cenários controlados (§28.3).
- **§12** gatilho por ausência de confirmação, limiar configurável, sem RSSI nem
  PDR em janela.
- **§13** todos os valores da tabela, exceto D3; `N_limiar < N_máximo`.
- **§15** as seis condições de acionamento do BA, com a ressalva de E2.
- **§16** formulação da aptidão e os três pesos, somando 1 (verificado em tempo
  de execução); restrição de conectividade do §16.4.
- **§17** todas as restrições de candidatos.
- **§18** deslocamento gradual, sem teletransporte, um reposicionamento por vez,
  no máximo um por alerta, retransmissão na chegada, retomada da mobilidade.
- **§20** `victimId` estável, novo `alertId` por alerta periódico, novo
  `messageId` por tentativa.
- **§21** entrega e confirmação distintas; validade do ACK pelos cinco campos e
  coerência do endereço com a tentativa original.
- **§22** todas as métricas, incluindo as sete de reposicionamento do §22.11.
- **§23** as seis relações de consistência verificadas como invariantes em tempo
  de execução; razões com denominador zero resultam em indefinido, nunca zero.
- **§25** a matriz completa de 480 execuções.
- **§26** mesma seed, mesmo cenário, fluxos de RNG separados por nó para que o
  tratamento não consuma sorteios que o controle não consome.
- **§28** os nove testes obrigatórios, com rastreabilidade em
  [`simulations/README.md`](../simulations/README.md#rastreabilidade-dos-testes-obrigatórios).
- **§29** limitações declaradas em [`README.md`](../README.md), incluindo as
  hipóteses de D4 (geometria idealizada) e E2 (posição possivelmente
  desatualizada na recuperação).

---

## Validação metodológica comparativa

Nenhum item aqui é desvio, extensão ou lacuna frente à diretriz — é registro de
por que duas decisões de rigor do projeto não são acidentais, contrastadas com
a prática observada em outro projeto FANET/OMNeT++ de código aberto (drones +
estação terrestre, INET, aplicação UDP) usado como termo de comparação durante
a revisão dos `.ini`.

### Modelo de rádio: 802.11 realista, não rádio ideal

O projeto comparado usa `IdealWirelessNic`/`IdealRadioMedium`: conectividade
binária dentro de um raio fixo, sem perda por propagação, sem modelo de
interferência (`ignoreInterference = true`). É escolha razoável quando o
objeto de estudo é outra camada — no caso comparado, alocação de recursos por
teoria dos jogos, não a rede em si.

Aqui o objeto de estudo **é** a rede: a comparação BA ligado/desligado só tem
sentido se existir degradação de enlace real para o mecanismo reagir a ela.
Rádio ideal eliminaria exatamente essa degradação — não haveria o que o BA
corrigisse. `Ieee80211ScalarRadioMedium` + `FreeSpacePathLoss` + sensibilidade
e SNIR calibrados (E5) preserva perda por distância, colisão e retransmissão;
a comparação confirma que essa era a escolha correta para o objetivo do
trabalho, não apenas uma entre opções equivalentes.

### Fluxos de RNG dedicados, não `rand()` da libc

O projeto comparado sorteia parâmetros (valores de armazenamento simulado) com
`rand()` da biblioteca padrão C, fora do controle de semente do OMNeT++.
Trocar a seed configurada no `.ini` não afeta esses sorteios: são uma fonte de
não determinismo que a própria configuração de reprodutibilidade do
experimento não alcança.

Este projeto já havia sido corrigido de um problema da mesma classe: o
`BatAlgorithm` sorteava de `getRNG(0)`, o fluxo global também consumido por
mobilidade, backoff do 802.11, AODV e jitter das aplicações — só o braço
tratado consumia esses sorteios extras, o que divergia trajetórias e filas
entre os braços mesmo com a mesma seed (36 escalares divergentes, medido na
época). A correção — `num-rngs = 25`, um fluxo dedicado por drone, por equipe
e por aplicação de drone (`omnetpp.ini`, linhas 3–29) — é exatamente a
disciplina que evita a classe de erro observada no projeto comparado. A
comparação não motivou a correção (já existia), mas confirma que o cuidado
era necessário e não excesso de engenharia: é o tipo de falha real que ocorre
quando esse cuidado não é tomado.

## Verificação empírica: `ackTimeout` não é responsável pela perda

Revisão externa do `.ini` levantou uma dúvida legítima: `ackTimeout = 2s`
poderia estar provocando retransmissão prematura, competindo com a latência
de descoberta de rota do AODV (`activeRouteTimeout = 5s`) em vez de refletir
degradação real de enlace. A hipótese é testável com escalares que já são
gravados — resolvida com dados de duas seeds reais de `MainExperiment_BaOff`,
não por inspeção do `.ini`.

Seed 0 e seed 5 divergem fortemente em resultado (mesmo cenário, só a seed
muda):

| | seed 0 | seed 5 |
| --- | --- | --- |
| alertas gerados | 25 | 25 |
| alertas confirmados | 19 | 24 |
| tentativas enviadas | 41 | 28 |
| tentativas recebidas | 21 | 24 |
| retransmissões | 17 | 3 |
| `confirmationDelay` médio | ≈1,93 s | ≈0,10 s |

Se o `ackTimeout` fosse curto demais para o AODV, a assinatura esperada seria
fila de MAC crescendo (mensagens acumulando enquanto a rota ainda não existe)
e `packetDropQueueOverflow` não-nulo. Os escalares de diagnóstico de MAC do
`.sca` da seed 0 (a mais castigada) não mostram isso:

- `packetDropQueueOverflow:count = 0` em drone0, drone1, drone2, drone3 e
  team0 — nenhuma fila jamais encheu.
- `queueLength:max` entre 1 e 3, muito abaixo da capacidade configurada
  (`**.wlan[*].mac.queue.packetCapacity = 50`).
- `packetDropIncorrectlyReceived:count` = 17 / 20 / 12 / 23 / 45 (drone0,
  drone1, drone2, drone3, team0) — este é o mecanismo de perda dominante, não
  fila.

`packetDropIncorrectlyReceived` é descarte de PHY (SNIR abaixo do limiar na
recepção), não backlog de fila. A leitura consistente com os dois seeds e os
dois diagnósticos: a variação de resultado entre seeds vem de quão perto os
nós ficam do alcance nominal de 559 m (§E5, `communicationRange`) durante
cada realização de mobilidade — não de uma corrida entre `ackTimeout` e o
tempo de descoberta de rota do AODV. `ackTimeout = 2s` está retransmitindo em
resposta a perda de enlace real, que é exatamente a exposição que o
mecanismo avaliado (BA) precisa para ser testado — não um artefato de
temporização apertada demais.

## Correção: amostragem inicial do Bat Algorithm ignorava o domínio

Revisão externa do funil de exposição (ver seção anterior) levantou uma
segunda dúvida ao investigar por que uma ativação do BA (seed 4,
`MainExperiment_BaOn`) não produzia reposicionamento: seria falta de solução
viável, ou desperdício de orçamento de amostragem?

Instrumentação temporária em `RepositionFitness::feasible()` (contador por
motivo de rejeição, removida após uso) confirmou a segunda hipótese como
causa dominante. `BatAlgorithm::optimize()` sorteava candidatos num raio de
`maximumRepositionDistance` (120 m) nos três eixos, sem considerar que a
faixa de altitude válida (`minimumAltitude`–`maximumAltitude`, 6–20 m) tem só
14 m de largura, ou que o drone podia estar perto da borda do cenário
(999,6 m de 1000 m no caso investigado). Resultado medido, 2021 avaliações
de uma ativação sem solução:

| motivo da rejeição | contagem | % |
| --- | --- | --- |
| fora da área (0–1000 m) | 967 | 47,8% |
| fora da altitude (6–20 m) | 984 | 48,7% |
| linha de visada obstruída até a equipe | 70 | 3,5% |
| **passou tudo** | **0** | 0% |

96,5% das avaliações eram rejeitadas antes mesmo de testar a restrição
cientificamente relevante (obstrução geométrica). Apenas 70 candidatos
chegavam ao teste de linha de visada, e todos permaneciam obstruídos —
tornando ambíguo se a ausência de solução refletia geometria genuinamente
inviável ou amostragem malsucedida.

Correção: `BatAlgorithm::optimize()` ganhou um parâmetro `DomainFunction`
(`RepositionFitness::inDomain()`, só área e altitude, sem raycasting) usado
para reamostrar por rejeição — até 500 tentativas geométricas baratas — antes
de cada avaliação cara de `feasible()`/`cost()`. Sem `std::clamp()`: reamostrar
preserva a distribuição condicionada ao domínio; recortar concentraria
candidatos artificialmente nas bordas/teto/piso (ver `BatAlgorithm.h`/`.cc`).

Reexecutando a mesma ativação (seed 4) com a correção, mesmas seeds de RNG:

| | pré-fix | pós-fix |
| --- | --- | --- |
| avaliações rejeitadas por domínio | 1951 (96,5%) | 0 (0%) |
| avaliações que testaram linha de visada | 70 | 2021 (100%) |
| candidatos viáveis encontrados | 0 | 0 |

A conclusão qualitativa não mudou — essa ativação específica genuinamente não
tem solução viável (equipe a 458 m, nenhum ponto dentro de 120 m livre de
obstrução) — mas agora é uma afirmação sustentada por 100% do orçamento
testando a restrição real, não por 3,5%. Uma segunda ativação da mesma seed
(que já encontrava solução antes da correção) passou a encontrá-la com 930
avaliações em vez de 2126 — mesmo resultado qualitativo, menos desperdício.

`Calibration_Exposure` (10 seeds reservadas, `seed-set + 100`) não teve
nenhum escalar de funil alterado pela correção — nenhuma das ativações
daquele lote era um caso-limite que a ineficiência de amostragem decidia. A
correção melhora a validade do diagnóstico "sem solução viável" e a
eficiência computacional; não há evidência, nas seeds testadas, de que altere
a taxa de reposicionamento da campanha. Verificado com a suíte completa dos 8
smoke tests obrigatórios (8/8 OK) antes do commit.

## Correção: `minimumRange` do sensor ativo mesmo no modo oráculo idealizado

Revisão externa notou uma inconsistência lógica em `AbstractObstacleSensor`:
`maximumRange = -1m` (mecanismo científico, D4) desativa o limite de alcance
físico do sensor, mas `minimumRange = 0.7m` (zona morta da câmera do Phantom
4 Pro) continuava sendo aplicado incondicionalmente. Um drone geometricamente
encostado num obstáculo (distância < 0,7 m) deixaria de confirmar a
obstrução — o oposto do que um oráculo geométrico idealizado deveria fazer.

Corrigido em `AbstractObstacleSensor::inspect()`: `minimumRange` e
`maximumRange` agora só se aplicam juntos, quando `maximumRange >= 0` (modo
sensor físico, usado pelos smoke tests). No modo oráculo (`maximumRange` < 0,
usado pelo mecanismo científico), nenhum dos dois limites se aplica.
