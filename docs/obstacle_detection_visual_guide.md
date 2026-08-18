# Guia visual: identificação e uso da posição do obstáculo

## Visão geral

O ECHOSAR-Net não implementa uma câmera real. Ele utiliza um sensor abstrato
que consulta a geometria conhecida pelo simulador. A rede detecta apenas sinais
de degradação; quem confirma a existência do obstáculo é o sensor.

```mermaid
flowchart LR
    A[VictimAlert pendente] --> B{VictimAck chegou?}
    B -- Sim --> C[Encerrar alerta]
    B -- Não --> D[Calcular PDR, RSSI e silêncio]
    D --> E{Rede degradada?}
    E -- Não --> F[Aguardar retry normal]
    E -- Sim --> G[Consultar sensor abstrato]
    G --> H{Obstáculo confirmado?}
    H -- Não --> I[Registrar rejeição]
    H -- Sim --> J[Passar ponto da superfície ao BA]
    J --> K[Buscar nova posição]
```

## Três responsabilidades diferentes

```mermaid
flowchart TB
    subgraph Rede[1. Camada de rede]
        R1[PositionUpdate recebido]
        R2[RSSI, PDR e tempo sem atualização]
        R3[Indica degradação]
        R1 --> R2 --> R3
    end

    subgraph Sensor[2. Sensor abstrato]
        S1[Recebe posição do drone e da equipe]
        S2[Consulta PhysicalEnvironment]
        S3[Confirma ou rejeita obstáculo]
        S1 --> S2 --> S3
    end

    subgraph Otimizacao[3. Bat Algorithm]
        O1[Recebe ponto do obstáculo]
        O2[Avalia candidatos]
        O3[Seleciona menor custo viável]
        O1 --> O2 --> O3
    end

    R3 --> S1
    S3 --> O1
```

| Componente | O que sabe | O que não deve concluir sozinho |
|---|---|---|
| Rede | ACK ausente, RSSI, PDR e última recepção | que a perda foi causada por obstáculo |
| Sensor abstrato | interseção geométrica e alcance | que o enlace será recuperado |
| BA | custos estimados de candidatos | que uma aptidão baixa garante AppACK |
| INET | propagação e recepção efetivas | qual decisão operacional deve ser tomada |

## De onde vem o obstáculo?

Os objetos são definidos em `simulations/dissertation-obstacles.xml` e
carregados pelo `PhysicalEnvironment` do INET.

```mermaid
flowchart LR
    XML[dissertation-obstacles.xml] --> PE[PhysicalEnvironment]
    PE --> DOL[DielectricObstacleLoss]
    PE --> AOS[AbstractObstacleSensor]
    DOL --> RADIO[Perda adicional no enlace]
    AOS --> OBS[ObstacleObservation]
```

O mesmo objeto geométrico possui dois usos independentes:

1. `DielectricObstacleLoss` altera fisicamente a potência recebida;
2. `AbstractObstacleSensor` informa se a linha de visada cruza o objeto.

Adicionar apenas um desenho ao Qtenv não causaria perda de rádio. O objeto
precisa pertencer ao `PhysicalEnvironment` conectado ao meio de rádio.

## Como a linha de visada é construída?

O drone utiliza sua posição atual e a última posição da equipe recebida por
`PositionUpdate`.

```text
 z
 ↑
 |                 drone (xd, yd, zd)
 |                       ●
 |                      /|
 |                     / |  linha de visada 3D
 |          obstáculo /  |
 |             ┌─────────┐
 |             │    ×    │ ← primeiro ponto de interseção
 |             │         │
 |             └─────────┘
 |                   /
 |                  ● equipe (xe, ye, ze)
 +------------------------------------------------→ x,y
```

O segmento consultado é:

$$
L(s)=\mathbf{p}_{drone}+s
(\mathbf{p}_{team}-\mathbf{p}_{drone}),
\qquad 0\leq s\leq1
$$

O sensor procura os objetos que interceptam esse segmento.

## Como a interseção é calculada?

```mermaid
flowchart TD
    A[Receber objeto físico] --> B[Converter LOS para coordenadas locais]
    B --> C[Calcular interseção com a forma]
    C --> D{Houve interseção?}
    D -- Não --> E[Ignorar objeto]
    D -- Sim --> F[Converter pontos para coordenadas globais]
    F --> G[Escolher o ponto mais próximo do drone]
    G --> H{Mais próximo que o anterior?}
    H -- Sim --> I[Guardar objeto e ponto]
    H -- Não --> E
```

A conversão para coordenadas locais é necessária porque cada obstáculo pode
ter posição e orientação próprias. Depois do cálculo, o ponto volta ao sistema
global usado pelos drones.

## Centro versus superfície

```text
                    centro geométrico
                           ●
             ┌─────────────┐
 drone ● ───────×──────────│────── ● equipe
             ↑             │
             primeiro ponto   │
             da superfície    │
             └─────────────┘
```

O sensor retorna os dois valores:

- `center`: centro geométrico do objeto;
- `nearestSurfacePoint`: primeira superfície encontrada a partir do drone.

O BA recebe `nearestSurfacePoint`, porque esse ponto representa onde a linha de
visada começa a ser bloqueada.

## Validação do alcance

Uma interseção geométrica não significa automaticamente que o sensor confirmou
o obstáculo.

```mermaid
flowchart LR
    A[Interseção encontrada] --> B[Calcular distância drone–superfície]
    B --> C{0,7 m ≤ distância ≤ 30 m?}
    C -- Sim --> D[obstacleConfirmed]
    C -- Não --> E[outsideVisualRange]
```

Os resultados possíveis são:

| `reason` | Significado |
|---|---|
| `invalidTargetDirection` | drone e equipe ocupam a mesma posição |
| `clearLineOfSight` | nenhum objeto intercepta o segmento |
| `outsideVisualRange` | existe interseção, mas fora do alcance configurado |
| `obstacleConfirmed` | existe interseção dentro do alcance |

## Dados entregues ao DroneApp

```text
ObstacleObservation
├── confirmed             confirmação final do sensor
├── obstacleId            identificador numérico do objeto
├── obstacleName          nome definido no XML
├── center                centro geométrico
├── nearestSurfacePoint   primeiro ponto da superfície
├── distance              distância drone–superfície
├── timestamp             instante da consulta
└── reason                motivo da confirmação ou rejeição
```

O objeto não envia uma mensagem pela rede. A estrutura é retornada por uma
chamada local e síncrona:

```cpp
auto observation = sensor->inspect(dronePosition, teamPosition);
```

## Como a posição chega ao BA?

```mermaid
sequenceDiagram
    participant Team as Equipe
    participant Drone as DroneApp
    participant Sensor as AbstractObstacleSensor
    participant Env as PhysicalEnvironment
    participant BA as BatAlgorithm
    participant Mob as Mobilidade

    Team->>Drone: PositionUpdate(posição da equipe)
    Note over Drone: Alerta pendente e rede degradada
    Drone->>Sensor: inspect(posição drone, posição equipe)
    Sensor->>Env: visitar objetos na LOS
    Env-->>Sensor: objetos interceptados
    Sensor-->>Drone: ObstacleObservation
    alt obstáculo confirmado
        Drone->>BA: optimize(..., nearestSurfacePoint)
        BA-->>Drone: melhor posição viável
        Drone->>Mob: moveTo(posição)
    else rejeitado
        Drone->>Drone: registrar sensorRejections
    end
```

O ponto é capturado pela função de aptidão:

```cpp
computeFitness(candidate, current, team, observation.nearestSurfacePoint)
```

## Uso na função de aptidão

O primeiro uso penaliza candidatos próximos da superfície detectada:

$$
C_{prox}(\mathbf{p})=
\exp\left(
-\frac{\lVert\mathbf{p}-\mathbf{p}_{obs}\rVert}{\sigma_{obs}}
\right)
$$

O segundo uso verifica se a nova linha candidato–equipe continua obstruída:

$$
C_{obs}(\mathbf{p})=
\max\left(C_{prox}(\mathbf{p}),I_{LOS}(\mathbf{p})\right)
$$

```mermaid
flowchart LR
    C[Candidato do BA] --> P[Distância ao ponto do obstáculo]
    C --> L[Interseção candidato–equipe]
    P --> CP[Custo de proximidade]
    L --> IL[0 livre ou 1 obstruído]
    CP --> MAX[máximo]
    IL --> MAX
    MAX --> CO[Custo do obstáculo]
```

Se a linha continuar obstruída, o custo de obstáculo será 1, mesmo que o
candidato esteja distante do primeiro ponto detectado.

## O que confirma o sucesso?

```mermaid
flowchart LR
    A[Obstáculo confirmado] --> B[BA escolhe candidato]
    B --> C[Drone se move gradualmente]
    C --> D[Nova tentativa VictimAlert]
    D --> E{VictimAck recebido?}
    E -- Sim --> F[Reposicionamento validado]
    E -- Não --> G[Continuar alerta ou novo ciclo permitido]
```

Nem a confirmação sensorial nem uma aptidão baixa representam sucesso. O
reposicionamento somente é considerado bem-sucedido quando uma equipe envia um
`VictimAck` e ele chega ao drone originador.

## Arquivos relacionados

| Arquivo | Responsabilidade |
|---|---|
| `simulations/dissertation-obstacles.xml` | geometria e material dos obstáculos |
| `simulations/omnetpp.ini` | ativa ambiente, perda e alcance sensorial |
| `src/sensing/AbstractObstacleSensor.*` | interseção, alcance e observação |
| `src/app/DroneApp.*` | decide quando consultar e acionar o BA |
| `src/optimization/BatAlgorithm.*` | procura a posição candidata |
| `src/mobility/BaGaussMarkovMobility.*` | executa o deslocamento gradual |
| `docs/bat_algorithm_and_fitness.md` | equações completas da otimização |

## Limitações do modelo

- a geometria real é conhecida pelo simulador;
- não existe processamento de imagem;
- o sensor é orientado abstratamente para a equipe;
- FOV, iluminação e textura são premissas, não modelos implementados;
- não há ruído na posição estimada do obstáculo;
- a equipe pode ter se movido desde seu último `PositionUpdate`;
- detectar o obstáculo não prova que ele foi a única causa da perda.

Essas limitações devem acompanhar a interpretação dos resultados da
dissertação.
