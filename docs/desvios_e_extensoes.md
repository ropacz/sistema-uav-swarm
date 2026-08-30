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
implementação **aplica o limite de 30 m** no cenário científico — o alcance
frontal real do sistema visual do DJI Phantom 4 Pro (0,7–30 m), tratado de
forma omnidirecional (ver simplificação de FOV abaixo).

**Decisão revista.** Uma versão anterior desta seção desabilitava o limite
(`maximumRange = -1m`), porque um prédio que obstrui a linha drone–equipe está,
tipicamente, longe do drone, e com 30 m detecção e obstrução eram quase
disjuntas: medido, `obstaclesDetected` permanecia zero em todas as execuções.
A decisão atual prioriza fidelidade ao alcance físico declarado do sensor da
aeronave sobre exercitar o mecanismo de reposicionamento neste cenário — ver a
consequência medida logo abaixo.

**Consequência sobre a detecção:** edifícios que bloqueiam o enlace
drone–equipe ficam tipicamente muito além de 30 m, então `obstaclesDetected`
permanece zero no cenário científico. Enquanto a confirmação visual era
condição de acionamento, isso zerava também `baActivations` e o mecanismo nunca
era exercitado. **D7 desfez esse acoplamento:** a degradação da rede aciona o
BA, e `obstaclesDetected = 0` passou a significar apenas "nenhum obstáculo
estava ao alcance da câmera", sem impedir o reposicionamento. O funil medido
está em D7.

`obstaclesDetected` continua sendo diagnóstico de exposição útil, e deve ser
lido como tal — não como contagem de reposicionamentos justificados.

O §14 já prevê a hipótese alternativa (sem limite) e define como declará-la:
*"Se a avaliação utilizar a geometria completa do cenário para testar posições
candidatas, essa hipótese deve ser declarada como: Conhecimento geométrico
idealizado disponibilizado ao mecanismo de reposicionamento."* Sob o limite de
30 m essa hipótese deixa de se aplicar à etapa de *detecção* (que agora respeita
o alcance físico do sensor); continua valendo para a *geometria* usada dentro
do alcance — a presença de obstáculo dentro de 30 m é constatada por
verificação geométrica idealizada do segmento, não por visão computacional.

O parâmetro continua configurável: valor negativo desabilita o limite (modo
oráculo sem alcance, usado por análises que queiram isolar o mecanismo do
funil de detecção). Os smoke tests fixam 30 m e 20 m, porque o teste
obrigatório §28.4 valida justamente o comportamento de faixa limitada.

O módulo também declara `fieldOfViewHorizontal` (60°), `fieldOfViewVertical`
(54°, ou seja ±27°) e `measurementFrequency` (10 Hz), conforme a especificação
DJI do sistema visual frontal/traseiro
(`docs/references/especificacoes_dji_phantom_4_pro_v2.docx`). São parâmetros
**declarativos**: documentam a câmera real de referência, mas não recortam a
linha de visada avaliada.

**Simplificação adicional, também assumida deliberadamente:** o oráculo trata a
detecção como **omnidirecional**, e não como o cone frontal/traseiro real do
Phantom 4 Pro (60° × 54°, com pontos cegos laterais e inferiores fora da faixa
coberta pelos sensores visuais e infravermelhos). O reconhecimento do
obstáculo em si — o algoritmo de visão computacional/ML que, num drone real,
classificaria o que a câmera captura — é o que fica abstraído; a geometria
da cena dentro do alcance de 30 m permanece disponível ao oráculo. A
justificativa é a mesma do §14 citado acima, estendida ao apontamento do
sensor: o foco do trabalho é conectividade e reposicionamento, não
reconhecimento de imagem, e modelar orientação de câmera, blind spots e
múltiplos sensores direcionais custaria processamento sem contribuir para a
pergunta de pesquisa.

**Impacto em dados já gravados:** os 480 runs da campanha `Scenario1_*` e as
demais execuções já realizadas foram gerados com `maximumRange = -1m`. Sob a
decisão atual (30 m) esses resultados não são mais reproduzíveis a partir deste
`.ini` e a campanha precisa ser regerada antes de qualquer análise que use este
limite. (Os resultados antigos foram apagados de `simulations/results/`.)

**Correções aplicadas ao ativar o modo sensor físico:**

- `inspect()` agora trunca o segmento inspecionado em `maximumRange` (quando
  configurado) em vez de avaliar até a posição da equipe e só então comparar
  a distância. Um obstáculo além do alcance simplesmente não é visto — a
  câmera real não relata "há algo lá fora, fora de alcance", ela não vê nada.
- Zona morta física (`distance < minimumRange`) deixou de anular a
  confirmação. A câmera estereoscópica não estima distância com
  confiabilidade abaixo do alcance mínimo, mas continua constatando que há
  um obstáculo ali — `reason = "obstacleTooCloseToMeasure"` marca essa
  situação, sem impedir a ativação do BA. Um drone encostado numa parede não
  deixa de notar o obstáculo por
  estar perto demais. Ambas as correções só se aplicam no modo sensor físico;
  o oráculo idealizado (`maximumRange < 0`) continua sem zona morta nem
  truncamento, como antes.
- `fieldOfViewHorizontal`/`fieldOfViewVertical` passaram a ser lidos com
  `doubleValueInUnit("deg")`, para não depender da unidade angular usada no
  `.ini`. Continuam declarativos (não recortam a linha de visada): aplicar o
  cone de visão de verdade exigiria modelar a orientação da câmera, que a
  mobilidade Gauss–Markov 3D do drone (§3) não pilota — ver discussão
  registrada como lacuna abaixo.
- Descartado throttling por `measurementFrequency` (10 Hz): rastreado que
  `DroneApp::tryReposition()` chama `inspect()` no máximo uma vez por alerta
  (`repositionDecisionMade`), nunca em rajada para o mesmo alvo — um cache de
  10 Hz não teria o que cachear nesta base de código.

**Revisão seguinte: 30 m virou o padrão do NED, e o resultado distingue
"livre" de "não observado":**

- `maximumRange` agora tem `default(30m)` no próprio `.ned` (antes, o padrão
  era o oráculo sem limite, `-1m`, e o `.ini` do cenário científico é que
  sobrescrevia para 30 m). O modo oráculo continua disponível — basta
  configurar um valor negativo — mas deixou de ser o comportamento implícito
  do módulo.
- `ObstacleObservation` ganhou `fullyObserved` e `observedRange`. Antes, um
  alvo a 200 m com os primeiros 30 m livres retornava `reason =
  "clearLineOfSight"` — indistinguível de uma LOS realmente livre até o alvo.
  Agora: `reason = "clearWithinSensorRange"` (`fullyObserved = false`) nesse
  caso, e `reason = "clearToTarget"` (`fullyObserved = true`) só quando o
  alvo inteiro coube dentro do alcance observado (ou o sensor está em modo
  oráculo). Nenhum consumidor atual (`DroneApp`) distingue os dois `reason`
  ainda — só `confirmed` importa para o gatilho do BA —, mas a informação
  passa a existir corretamente no sensor em vez de ser mascarada.
- `inspect()` renomeado: segundo parâmetro passa de `teamPosition` para
  `inspectionTarget`. O sensor não é conceitualmente restrito à equipe; pode
  inspecionar qualquer direção (candidato do BA, trecho de movimento, etc.),
  ainda que hoje só `DroneApp::tryReposition()` o chame com a posição da
  equipe.

`src/camera/AbstractObstacleSensor.{h,cc,ned}`, `simulations/omnetpp.ini`.

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

### D7. Confirmação visual deixou de ser condição para acionar o BA — §15

O §15 lista a confirmação de obstáculo pelo sensor entre as condições de
acionamento do reposicionamento. A implementação **aciona o BA pela degradação
da rede**, usando a observação visual apenas como informação para a aptidão.

O acionamento continua sendo o do §12 (ausência de ACK acima do limiar
configurável). O que mudou é que `observation.confirmed` deixou de ser um
portão em `DroneApp::tryReposition()`.

**Motivo.** Com o sensor limitado a 30 m (D4), exigir confirmação visual
tornava o mecanismo inalcançável: um edifício que bloqueia um enlace de
centenas de metros está tipicamente muito além do alcance da câmera. Medido no
cenário científico, com a confirmação obrigatória:

| | confirmação obrigatória | acionamento pela rede |
| --- | --- | --- |
| `repositionTriggers` | 5 | 5 |
| `obstaclesDetected` | 0 | 0 |
| `baActivations` | **0** | **5** |
| `repositionsCompleted` | **0** | **5** |
| distância total | 0 m | 43,2 m |

(`MainExperiment_BaOn`, seed 0.) O mecanismo avaliado pela dissertação
simplesmente não era exercitado.

**Justificativa conceitual.** Não observar obstáculo significa apenas *nenhum
obstáculo foi observado dentro do alcance e da direção inspecionada*. Não
significa *a comunicação não está degradada*. A degradação pode vir de
bloqueador além dos 30 m, afastamento da equipe, interferência,
desvanecimento, posição desatualizada da equipe ou perda de rota. Condicionar
a resposta à identificação visual da causa exigiria que o bloqueador estivesse
a menos de 30 m, iluminado, texturizado e na direção certa — restrições que
não decorrem da pergunta de pesquisa.

**Formulação para a dissertação:** *após o número configurado de tentativas sem
confirmação, a aplicação inicia a avaliação de reposicionamento,
independentemente da detecção visual de obstáculos. O modelo visual, limitado a
30 m, fornece opcionalmente o primeiro ponto de interseção observado, utilizado
no termo de proximidade da aptidão. A viabilidade dos candidatos é verificada
por um avaliador geométrico idealizado do simulador, sem limite de alcance, que
exige trajetória livre e linha de visada até a última posição conhecida da
equipe. Consequentemente, a detecção visual não condiciona a ativação do BA,
mas a geometria idealizada ainda condiciona a ocorrência de deslocamento
efetivo.*

**Ativação não implica deslocamento — não afirmar o contrário.** Toda
degradação aciona a *avaliação*, mas nem toda avaliação produz movimento. Se a
degradação ocorrer com a linha de visada geometricamente livre, a posição atual
continua viável, e como a aptidão favorece permanecer parado (ver a aritmética
em E4) o resultado pode ser um deslocamento próximo de zero — ou nenhum, se o
candidato coincidir com a posição atual (`distance <= 1e-6` em
`DroneApp::tryReposition()`). O deslocamento efetivo ocorre quando o avaliador
geométrico identifica que a configuração atual não atende às restrições de
trajetória e de linha de visada.

Por isso o funil é medido em etapas separadas, e `effectiveRepositions`
(deslocamento acima de `effectiveRepositionThreshold`, 1 m por padrão) existe
para não ler um deslocamento de poucos centímetros como reposicionamento
operacionalmente relevante. A distância bruta continua sendo gravada.

**O que a câmera continua fazendo.** `victimSensorEvaluated` segue sendo
emitido nos **dois braços**, agora como diagnóstico de exposição e não como
portão. O funil passou a registrar `sensorEvaluations` (quantas vezes o modelo
visual foi consultado) separadamente de `obstaclesDetected` (quantas vezes
havia obstáculo dentro de 30 m): sem essa separação, "consultou e não viu nada"
seria indistinguível de "nunca consultou", que agora são situações diferentes.
A comparação pareada da exposição é preservada. Quando há detecção, o ponto observado entra no termo
`C_obstaculo`; quando não há, esse termo é zero para todos os candidatos e não
influencia a ordenação (`obstaclePoint` é `std::optional`).

**O que faz o drone se mover.** Não é a aptidão — são as restrições de linha
de visada em `feasible()` (E4), que tornam inviável permanecer numa posição
obstruída. Isso é essencial: sem elas, a aptidão linear atual torna qualquer
deslocamento desfavorável, e o BA escolheria ficar parado (a demonstração está
em E4). Por isso as duas mudanças não podem ser separadas.

Invariantes de §23 não foram afetados: existe `repositionsStarted >
baActivations` e `obstaclesDetected > repositionTriggers`, mas nunca houve
`baActivations ≤ obstaclesDetected`.

`src/app/DroneApp.cc`, `analysis/validation/validate_sensor_range_smoke_test.py`.

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
custo.

O efeito é um espaço de busca mais restrito que o da diretriz. Sua viabilidade
deve ser avaliada pelo funil de ativações, soluções e movimentos da campanha
corrente, sem reutilizar percentuais obtidos antes de mudanças nos RNGs.

**O modelo é híbrido, e isso precisa ser declarado.** Duas abstrações
diferentes convivem, e confundi-las seria incorreto:

1. **Detecção** — o que ativa o reposicionamento passa pelo sensor visual
   limitado a 30 m (`AbstractObstacleSensor::inspect()`, D4). Respeita alcance
   mínimo e máximo do Phantom 4 Pro.
2. **Avaliação de trajetórias candidatas** — usa um predicado geométrico
   idealizado do simulador
   (`AbstractObstacleSensor::intersectsAnyObstacleGroundTruth()`), sem limite
   de alcance. É precisamente o uso que o §14 autoriza: *"Se a avaliação
   utilizar a geometria completa do cenário para testar posições candidatas,
   essa hipótese deve ser declarada como: Conhecimento geométrico idealizado
   disponibilizado ao mecanismo de reposicionamento."*

Não há mapa persistente em nenhum dos dois casos. A finalidade da segunda
abstração é isolar a análise da **política de reposicionamento** das
incertezas do planejamento de trajetória, que não são objeto deste trabalho.

**Formulação para a dissertação:** *a detecção inicial respeita o alcance
nominal do sensor visual; após a ativação do reposicionamento, a viabilidade
das posições candidatas é avaliada por um predicado geométrico idealizado do
simulador, utilizado para isolar a análise da política de reposicionamento das
incertezas do planejamento de trajetória.*

**Medição que justifica manter a restrição.** Uma revisão propôs remover as
duas condições de linha de visada, para que o BA não tivesse conhecimento além
do alcance da câmera. Implementada e medida, a remoção **inutilizou o
mecanismo**: no `BA_SmokeTest` o deslocamento caiu de 29,60 m para 0,17 m, e o
`RepositionInterrupted_SmokeTest` passou a falhar (o movimento terminava em
0,013 s, antes de poder ser interrompido).

A causa é aritmética, não de implementação. Com os pesos do §16 e
`linkNormalizationDistance` = 1000 m, aproximar-se 25 m da equipe melhora
`C_enlace` em `0,60 × 25/1000` = 0,015, enquanto o mesmo movimento custa
`0,15 × 25/25` = 0,15 em `C_movimento` — dez vezes mais do que rende. A
repulsão do obstáculo é da mesma ordem (`0,25 × e^{-20/10}` ≈ 0,034). Sem a
restrição de viabilidade, **permanecer parado é o ótimo da função de
aptidão**: as condições de linha de visada são o que efetivamente obriga o
drone a se deslocar.

A alternativa seria retunar os pesos até o movimento compensar — calibração
dirigida ao resultado observado, exatamente o que E5 registra que este
projeto evita. A restrição foi mantida e a hipótese, declarada.

`C_obstaculo` **não** repete o teste de obstrução: `cost()` só é avaliada em
candidatos que `feasible()` já aprovou, e aquela exige linha de visada livre
até a equipe. O termo seria constante e custaria um raycasting por avaliação.
Fica apenas o decaimento exponencial do §16.2, como repulsão local em torno da
superfície observada. `obstaclePoint` é opcional (`std::optional`): sem
observação, o termo é zero para todos os candidatos e não influencia a
ordenação — valores absolutos de aptidão não são comparáveis entre uma
ativação com observação e outra sem.

`src/optimization/RepositionFitness.cc`, `src/camera/AbstractObstacleSensor.h`.

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

### L2. Campo de visão e orientação real da câmera — D4

D4 declara `fieldOfViewHorizontal`/`fieldOfViewVertical` (60°/54°, Phantom 4
Pro) mas não os aplica: a observação continua omnidirecional. Recortar pela
direção real do sensor exigiria saber para onde a câmera aponta, e a
mobilidade do drone (Gauss–Markov 3D, §3) não pilota heading para mirar a
equipe.

Desde D7 isso deixou de ser bloqueante para o mecanismo: como o acionamento do
BA não depende mais da câmera, aplicar o FOV reduziria a frequência com que
`obstaclePoint` está disponível para a aptidão, mas não impediria o
reposicionamento. Continua pendente por exigir um modelo de orientação que a
mobilidade atual não fornece.

Também não implementado: validação de trajetória do BA em trechos limitados
pelo alcance do sensor (hoje `RepositionFitness::feasible()` avalia o
deslocamento inteiro de uma vez, pelo predicado ground truth, até
`maximumRepositionDistance`) e uma camada de desvio reativo de colisão
independente do BA (presente nos dois braços, tipo TapFly). Ambos mudariam o
mecanismo de reposicionamento e a comparação BaOn/BaOff além do escopo de D4 —
pendências para decisão futura. A execução incremental é também o único
consumidor plausível de um throttling real de `measurementFrequency` (ver D4).

**Resolvido, registrado em D7:** o gatilho do BA foi desacoplado da confirmação
da câmera. A degradação da rede aciona o reposicionamento; a observação visual
alimenta a aptidão quando existe.

**Pendente da mesma proposta:** remover também as restrições de linha de visada
de `feasible()`, deixando o BA sem qualquer conhecimento além do observado.
Isso exige **antes** reformular a aptidão, porque a formulação linear atual
torna qualquer deslocamento desfavorável (demonstração em E4) — o mecanismo
ficaria inerte. Escolher `linkNormalizationDistance` menor que 480 m só para
produzir movimento seria indistinguível de calibração dirigida ao resultado; e
normalizar pelo alcance de comunicação (559 m) **não resolve**, porque
`0,60/559 = 0,001073 < 0,15/120 = 0,00125`. Alternativas a especificar e
validar numa etapa separada: custo de enlace não linear em torno do limiar
operacional do enlace, por exemplo `C_link(d) = 1/(1 + e^{-k(d - d_0)})`, ou
custo de movimento quadrático, que reduz a derivada perto da origem.

Já **resolvido** e registrado em E4: a proposta paralela de retirar do BA o
acesso à geometria além de 30 m foi implementada, medida e parcialmente
revertida — as duas condições de linha de visada em `feasible()` permanecem
(sem elas o mecanismo fica inerte), agora nomeadas com honestidade
(`intersectsAnyObstacleGroundTruth()`) e declaradas como hipótese do §14. O
que se manteve daquela revisão: `cost()` não repete o raycasting (redundante
com `feasible()`), `obstaclePoint` virou `std::optional`, e a duplicação de
`feasible()` na inicialização do `BatAlgorithm` foi eliminada.

**Sobre a definição de degradação (§12):** verificado que código e diretriz já
concordam — o gatilho é ausência de ACK após um limiar configurável de
tentativas, sem PDR em janela e sem RSSI. Nenhuma mudança necessária; o texto
da dissertação não deve afirmar que PDR ou RSSI participam do acionamento.

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
- **§15** as condições de acionamento do BA, com a ressalva de E2 e o desvio
  de D7 (a confirmação visual de obstáculo deixou de ser condição).
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

O `BatAlgorithm` usa um fluxo próprio de cada aplicação de drone. O jitter
operacional dos repasses de `TeamUpdate` permanece em outro fluxo local, além
dos fluxos dedicados à mobilidade. Assim, os sorteios exclusivos do braço
tratado não deslocam nem a mobilidade nem a temporização aleatória do tráfego
de controle. O mapeamento normativo está em `simulations/omnetpp.ini`.

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

Em uma medição histórica anterior à separação dos RNGs, a mesma ativação foi
reexecutada após a correção:

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

Esses números documentam o defeito corrigido, mas não são resultado da
campanha corrente. Após mudanças no mapeamento dos RNGs ou nas janelas de
alerta, o funil deve ser regenerado com `Calibration_Exposure`. A correção
melhora a validade do diagnóstico "sem solução viável" e a eficiência
computacional, sem antecipar o efeito confirmatório.

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
