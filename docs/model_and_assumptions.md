# Modelo e premissas

O que o ECHOSAR-Net representa, como cada componente funciona e o que ele
deliberadamente não modela. A pergunta e o desenho estão em
[`scientific_protocol.md`](scientific_protocol.md); os contratos de medida, em
[`metrics.md`](metrics.md).

## 1. Escopo

Modelado: comunicação IEEE 802.11b ad hoc, UDP, AODV, mobilidade de drones e
equipes, degradação por obstáculos, detecção geométrica abstrata, Bat
Algorithm, reposicionamento e confirmação do alerta por `VictimAck`.

Fora do escopo: visão computacional, processamento de imagens, câmera, FOV,
textura ou iluminação, controle de voo de baixo nível, vento e aerodinâmica,
modelo eletroquímico de bateria, OcuSync ou fidelidade a uma plataforma
comercial, e a operação completa de uma missão de resgate.

IEEE 802.11 ad hoc, AODV e o Bat Algorithm são abstrações de pesquisa, não
recursos comerciais nativos de um DJI Phantom 4 Pro V2.0.

## 2. Modelos do INET utilizados

| Camada | Modelo |
|---|---|
| Rádio | `Ieee80211ScalarRadio` e `Ieee80211ScalarRadioMedium`, 2,4 GHz |
| Propagação | `FreeSpacePathLoss` |
| Obstáculos | `DielectricObstacleLoss` sobre `PhysicalEnvironment` |
| MAC | `Ieee80211Mac` com `Ieee80211MgmtAdhoc` |
| Rede | IPv4 com AODV (`AodvRouter`) |
| Mobilidade dos drones | `BaGaussMarkovMobility`, extensão do `GaussMarkovMobility` |
| Mobilidade das equipes | `MassMobility` |

O INET 4.5.4 não possui um módulo `RandomWalkMobility`. `MassMobility` é o
passeio aleatório bidimensional nativo mais próximo e por isso é usado
explicitamente.

`MassMobility` não consegue refletir dentro de um intervalo Z de espessura
zero; a faixa mínima configurada para as equipes evita a asserção de fronteira
e não introduz movimento vertical.

## 3. Responsabilidades

| Componente | Responsabilidade | O que não conclui sozinho |
|---|---|---|
| `SarScenarioManager` | Agenda a detecção abstrata e associa vítima ao drone ativo mais próximo | — |
| `DroneApp` | Executa o protocolo do alerta e coordena a decisão de reposicionamento | que a perda foi causada por obstáculo |
| `TeamApp` | Difunde posição, deduplica alertas e emite `VictimAck` | — |
| `AbstractObstacleSensor` | Confirma geometria e alcance do obstáculo | que o enlace será recuperado |
| `BatAlgorithm` | Otimiza uma posição candidata, sem depender da pilha de rede | que aptidão baixa garante AppACK |
| `BaGaussMarkovMobility` | Executa o movimento gradual e retoma a mobilidade normal | — |
| `analysis/` | Interpreta resultados | — |

O fluxo é cenário → aplicação → INET → sinais e arquivos → análise. Código de
análise nunca influencia o comportamento do simulador.

## 4. Ciclo do alerta

1. `SarScenarioManager` agenda a detecção de cada vítima e associa o drone
   ativo mais próximo; empates usam o menor identificador lexicográfico. A
   associação usa `sendDirect`, que representa uma decisão interna do
   simulador, não uma transmissão de rádio.
2. As equipes difundem `PositionUpdate` em broadcast de um salto. O drone mede
   RSSI pelo `SignalPowerInd`, estima PDR por lacunas de sequência e registra o
   tempo desde a última amostra.
3. O originador envia uma tentativa `VictimAlert` por UDP/AODV. `alertId`
   identifica a vítima; `messageId` identifica a tentativa.
4. A equipe deduplica o atendimento por `alertId`, mas responde a toda tentativa
   com `VictimAck`. Somente esse ACK encerra o alerta no originador.
5. Sem ACK e com degradação indicada, o sensor geométrico consulta a linha de
   visada. A confirmação autoriza o BA; a rejeição é registrada sem atribuir a
   perda ao obstáculo.
6. `BatAlgorithm` escolhe a posição; `BaGaussMarkovMobility` a percorre
   respeitando velocidades horizontal, de subida e de descida.
7. Ao chegar, o drone aguarda a próxima tentativa regular. Somente o ACK de uma
   tentativa **posterior à chegada** conta como recuperação pelo BA.

Não há flooding na camada de aplicação: o transporte multi-hop é do AODV.

Estado do reposicionamento, um por drone:

```text
IDLE → MOVING → AWAITING_VALIDATION → IDLE
```

Outros alertas permanecem pendentes e seguem o retry normal.

## 5. Sensor abstrato de obstáculos

Quando a rede indica degradação, o sensor traça o segmento entre drone e
equipe, procura interseções com os objetos do `PhysicalEnvironment` e verifica
se o primeiro obstáculo está dentro do alcance configurado.

O mesmo objeto geométrico tem dois usos independentes: `DielectricObstacleLoss`
altera fisicamente a potência recebida, e `AbstractObstacleSensor` informa se a
linha de visada cruza o objeto. Um desenho apenas no Qtenv não causaria perda
de rádio — o objeto precisa pertencer ao `PhysicalEnvironment` conectado ao
meio.

A interseção é calculada no sistema local de cada objeto e convertida de volta
ao global, porque cada obstáculo tem posição e orientação próprias.

O sensor devolve `center` (centro geométrico) e `nearestSurfacePoint` (primeira
superfície a partir do drone). O BA recebe `nearestSurfacePoint`, porque é onde
a linha de visada começa a ser bloqueada.

| `reason` | Significado |
|---|---|
| `invalidTargetDirection` | drone e equipe na mesma posição |
| `clearLineOfSight` | nenhum objeto intercepta o segmento |
| `outsideVisualRange` | há interseção, mas fora do alcance configurado |
| `obstacleConfirmed` | há interseção dentro do alcance |

Uma interseção geométrica não equivale a confirmação sensorial: o alcance é
verificado separadamente.

## 6. Bat Algorithm

Cada morcego é uma posição candidata tridimensional, com velocidade,
frequência, amplitude e taxa de pulsos. A melhor posição da população é
`p*`; como a aptidão é um custo, a melhor é a de menor valor.

A população é inicializada dentro de uma esfera centrada na posição atual do
drone, limitada por `maximumRepositionDistance`. A amostragem usa a raiz cúbica
de uma uniforme para não concentrar candidatos no centro. Se nenhum candidato
aleatório for viável, a posição atual pode servir de fallback, desde que também
satisfaça as restrições.

Atualizações, na convenção adotada explicitamente por esta implementação:

```text
f = fmin + (fmax - fmin)·β,        β ~ U(0,1)
v(t) = v(t-1) + (p(t-1) - p*)·f
p(t) = p(t-1) + v(t)
```

Quando uma amostra uniforme excede a taxa de pulsos, o candidato é gerado ao
redor de `p*`, dentro de uma esfera de raio proporcional a
`dmax · batLocalSearchScale · Ā`, onde `Ā` é a amplitude média da população.

Uma posição substitui a do morcego apenas quando sua aptidão não é pior **e**
uma amostra uniforme é menor que a amplitude atual. Ao aceitar, a amplitude
decai por `batAmplitudeDecay` e a taxa de pulsos cresce segundo
`r = r0·(1 - e^(-γ(t+1)))`.

### Função de aptidão

Minimizada, com pesos não negativos que somam 1:

```text
F(p) = wLink·Clink(p) + wObstacle·Cobs(p) + wMove·Cmove(p)

Clink(p) = clamp(‖p - pteam‖ / linkNormalizationDistance, 0, 1)
Cprox(p) = exp(-‖p - pobs‖ / obstacleSigma)
Cobs(p)  = max(Cprox(p), obstruído(p, pteam) ? 1 : 0)
Cmove(p) = clamp(‖p - pdrone‖ / maximumRepositionDistance, 0, 1)
```

Qualquer candidato cuja linha até a equipe permaneça obstruída recebe custo de
obstáculo igual a 1, mesmo estando longe do ponto detectado.

Exemplo numérico — candidato a 200 m da equipe, 20 m do obstáculo, linha
livre, deslocamento de 10 m, com os pesos e normalizações padrão:

```text
Clink = 200/1000 = 0,20
Cobs  = e^(-20/10) ≈ 0,1353
Cmove = 10/25 = 0,40
F = 0,60(0,20) + 0,25(0,1353) + 0,15(0,40) ≈ 0,2138
```

Se a linha continuasse obstruída, `Cobs = 1` e a aptidão subiria para ≈ 0,43.
Como o BA minimiza, o candidato livre é preferido.

O custo de enlace é uma estimativa por distância: não usa RSSI futuro nem
executa antecipadamente uma transmissão no INET. O RSSI real só é medido
**depois** do movimento.

### Restrições de viabilidade

Avaliadas antes da aptidão; candidatos inválidos são descartados sem entrar na
soma ponderada:

- limites X e Y da área e altitude mínima e máxima;
- distância máxima de reposicionamento;
- margem mínima em relação ao obstáculo;
- caminho drone–candidato livre de interseção;
- chegada antes de `flightTimeLimit`, dadas as velocidades configuradas.

### Condições de ativação

O BA só executa quando existe alerta pendente sem `VictimAck`, a janela indica
degradação, a posição da equipe é conhecida, o sensor confirma o obstáculo, o
BA está habilitado dentro do limite de ciclos e o drone não está em outro
reposicionamento. Essa separação evita atribuir automaticamente toda perda de
comunicação a um obstáculo.

## 7. Parâmetros

`simulations/omnetpp.ini` é a **fonte única** dos valores experimentais. Os
defaults NED existem para instanciação fora do INI e não devem ser tratados
como documentação do experimento. Os valores não são repetidos aqui para não
criar uma quarta cópia divergente.

| Grupo | Chaves |
|---|---|
| Área e duração | `sim-time-limit`, `physicalEnvironment.space*`, `repeat` |
| População | `numDrones`, `numTeams`, `numVictims` |
| Rádio | `opMode`, `bitrate`, `carrierFrequency`, `bandwidth`, `receiver.sensitivity`, `transmitter.power`, `backgroundNoise.power` |
| Aplicação | `appPort`, `*PayloadBytes`, `positionUpdateInterval`, `applicationIpTtl` |
| Alertas | `retryInterval`, `ackTimeout`, `maxAttempts`, `alertTtl` |
| Enlace | `linkWindow`, `pdrThreshold`, `rssiThreshold`, `teamSilenceTimeout`, `maintenanceInterval` |
| Sensor | `obstacleSensor.minimumRange`, `obstacleSensor.maximumRange` |
| Movimento | `horizontalSpeed`, `climbSpeed`, `descentSpeed`, `minimumAltitude`, `maximumAltitude`, `flightTimeLimit` |
| Bat Algorithm | `bat*`, `maximumRepositionDistance`, `maxBaCycles` |
| Aptidão | `wLink`, `wObstacle`, `wMove`, `obstacleSigma`, `obstacleSafetyMargin`, `linkNormalizationDistance` |
| Injeção de falha | `ackEnabled`, `ackStartTime` — apenas em cenários de verificação |

## 8. Limitações

- O sensor é uma consulta geométrica abstrata: a geometria real é conhecida
  pelo simulador, não há processamento de imagem, e FOV, iluminação e textura
  são premissas, não modelos.
- Não há ruído na posição estimada do obstáculo.
- A posição da equipe é a última recebida e pode estar desatualizada.
- A aptidão aproxima o enlace por distância e obstrução; não é previsão de
  antena, interferência ou SNIR.
- RSSI de um pacote entregue por múltiplos saltos descreve o último salto.
- Distância adicional é proxy de energia, não descarga eletroquímica.
- O algoritmo otimiza um drone por reposicionamento; não há otimização conjunta
  da formação.
- Os pesos da aptidão são escolhas experimentais, não propriedades físicas nem
  valores aprendidos, e merecem análise de sensibilidade.
- Detectar o obstáculo não prova que ele foi a única causa da perda.
