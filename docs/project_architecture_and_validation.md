# Arquitetura e validação básica do ECHOSAR-Net

## 1. Finalidade

O ECHOSAR-Net é uma simulação de FANET para avaliar a comunicação entre drones
e equipes móveis de resgate. O experimento verifica se o reposicionamento de um
drone pelo Bat Algorithm (BA) altera a entrega confirmada de alertas quando um
obstáculo degrada um enlace ar-solo.

A conclusão de um alerta ocorre somente quando o drone originador recebe um
`VictimAck` da camada de aplicação. O projeto não implementa visão
computacional, reconhecimento de vítimas, sensores físicos ou controle de voo.

## 2. Tecnologias e modelos

- OMNeT++ 6.2.0;
- INET Framework 4.5.4;
- IEEE 802.11b em modo ad hoc, 2,4 GHz e 1 Mbps;
- UDP na camada de transporte;
- AODV para roteamento com múltiplos saltos;
- `FreeSpacePathLoss` para perda por propagação;
- `DielectricObstacleLoss` para perda adicional por obstáculos;
- Gauss-Markov tridimensional para os drones;
- `MassMobility` como passeio aleatório bidimensional das equipes.

Esses recursos são abstrações da plataforma de pesquisa e não representam
funcionalidades nativas de um DJI Phantom 4 Pro V2.0.

## 3. Organização do projeto

```text
src/
├── app/
│   ├── DroneApp.{h,cc,ned}
│   ├── TeamApp.{h,cc,ned}
│   └── ports.h
├── messages/
│   ├── PositionUpdate.msg
│   ├── VictimAlert.msg
│   ├── VictimAck.msg
│   └── VictimAssignment.msg
├── mobility/
│   └── BaGaussMarkovMobility.{h,cc,ned}
├── optimization/
│   └── BatAlgorithm.{h,cc}
├── scenario/
│   ├── SarScenarioManager.{h,cc,ned}
│   └── StaticVictim.{cc,ned}
├── sensing/
│   └── AbstractObstacleSensor.{h,cc,ned}
└── node/
    └── SarDrone.ned

simulations/
├── BasicNetwork.ned
├── omnetpp.ini
├── dissertation-obstacles.xml
└── run

analysis/
├── process_results.py
└── validate_results.py
```

## 4. Componentes principais

### 4.1 SarScenarioManager

Representa a detecção abstrata das vítimas. No instante configurado:

1. consulta a posição dos drones;
2. calcula a distância até a vítima;
3. seleciona o drone mais próximo;
4. resolve empates pelo menor identificador;
5. envia um `VictimAssignment` diretamente à aplicação selecionada.

Não existe processamento de câmera ou imagem.

### 4.2 DroneApp

É responsável por:

- receber a associação da vítima;
- selecionar uma equipe conhecida;
- enviar `VictimAlert` por UDP;
- repetir o alerta no intervalo configurado;
- receber e validar `VictimAck`;
- manter a janela de RSSI e PDR das equipes;
- indicar degradação sem atribuir automaticamente sua causa;
- solicitar confirmação geométrica do obstáculo;
- executar o BA quando todas as condições forem satisfeitas;
- comandar o deslocamento gradual;
- validar a comunicação na próxima tentativa regular.

O estado global do reposicionamento é intencionalmente simples:

```text
IDLE → MOVING → AWAITING_VALIDATION → IDLE
```

Somente um reposicionamento pode estar ativo por drone. Outros alertas continuam
pendentes e seguem o retry normal.

### 4.3 TeamApp

É responsável por:

- transmitir `PositionUpdate` em broadcast;
- receber e deduplicar `VictimAlert`;
- registrar apenas um atendimento por `alertId`;
- responder novamente a tentativas ou pacotes duplicados;
- enviar `VictimAck` ao endereço IP de origem informado pela pilha INET.

Cada nó utiliza um único socket UDP na porta definida por `SAR_APP_PORT`.

### 4.4 AbstractObstacleSensor

É uma consulta geométrica abstrata. Quando a rede indica degradação, o módulo:

1. traça o segmento entre drone e equipe;
2. procura interseção com objetos do `PhysicalEnvironment`;
3. verifica se o primeiro obstáculo está dentro do alcance configurado;
4. retorna confirmação, identificação e posição aproximada.

O sensor não modela câmera, FOV, textura, iluminação ou classificação visual.

### 4.5 BatAlgorithm

Cada morcego representa uma posição tridimensional candidata. A implementação
usa as atualizações clássicas de frequência, velocidade e posição.

A função de aptidão minimizada é:

```text
F(p) = wLink × CLink + wObstacle × CObstacle + wMove × CMove
```

Onde:

- `CLink`: distância normalizada da posição candidata até a equipe;
- `CObstacle`: proximidade e obstrução geométrica;
- `CMove`: distância de reposicionamento normalizada.

Essa aptidão é deliberadamente simples. O RSSI real é medido somente depois do
movimento; ele não é tratado como informação futura conhecida pelo BA.

### 4.6 BaGaussMarkovMobility

Estende o Gauss-Markov do INET. Durante um reposicionamento:

1. a trajetória normal é temporariamente suspensa;
2. o drone percorre gradualmente a posição candidata;
3. velocidades horizontal, de subida e de descida são respeitadas;
4. ao concluir ou receber ACK, a mobilidade normal é retomada da posição atual.

## 5. Fluxo das mensagens

```text
Equipe ── PositionUpdate broadcast ──▶ drones

SarScenarioManager
        │ VictimAssignment
        ▼
   Drone originador
        │ VictimAlert / UDP / AODV
        ▼
     Equipe de resgate
        │ VictimAck / UDP / AODV
        ▼
   Drone originador
```

O `alertId` permanece constante para o mesmo evento. Cada tentativa recebe um
novo `messageId` e um novo número de sequência.

## 6. Ciclo básico de um alerta

```text
vítima detectada
      ↓
seleção do drone mais próximo
      ↓
envio de VictimAlert
      ↓
ACK recebido? ── sim ──▶ alerta confirmado
      │
      não
      ↓
avaliar RSSI, PDR e silêncio
      ↓
degradação indicada?
      │ não
      └──────────────▶ aguardar retry de 30 s
      │ sim
      ↓
sensor confirma obstáculo?
      │ não
      └──────────────▶ registrar rejeição e aguardar retry
      │ sim
      ↓
executar BA e mover gradualmente
      ↓
aguardar próxima tentativa regular
      ↓
ACK dessa tentativa?
      │ sim: reposicionamento validado
      │ não: novo ciclo, respeitando os limites configurados
```

## 7. Métricas centrais

- alertas únicos gerados e confirmados;
- tentativas enviadas;
- alertas expirados;
- PDR fim a fim;
- atraso unidirecional por tentativa;
- idade do alerta na recepção;
- RTT entre `VictimAlert` e `VictimAck`;
- indicações de degradação;
- confirmações e rejeições do sensor;
- ativações do BA;
- reposicionamentos validados ou malsucedidos;
- distância adicional;
- tempo de recuperação;
- RSSI e PDR antes e depois do reposicionamento;
- número de saltos.

A métrica primária é:

```text
AppACK = alertas únicos confirmados / alertas únicos gerados
```

## 8. Compilação

Crie o arquivo local de ambiente:

```bash
cp .env.example .env
```

Ajuste `WORKSPACE` e `INET_VERSION` e execute:

```bash
./run.sh --build -c Validation_Direct -r 0
```

Também é possível compilar manualmente dentro do `opp_env`:

```bash
opp_env run inet-4.5.4 -w /caminho/do/workspace --no-isolated \
  -c 'cd /caminho/do/projeto && make makefiles && make'
```

## 9. Validação determinística

Para observar drones e equipe em movimento, com trajetórias e enlaces no
Qtenv, use o cenário não determinístico de demonstração:

```bash
./run.sh --gui
```

Os cenários `Validation_Direct`, `Validation_Obstacle_BaOff` e
`Validation_Multihop` mantêm os nós parados intencionalmente para isolar os
efeitos de comunicação e garantir reprodutibilidade.

Execute os cenários abaixo com a mesma seed:

```bash
./run.sh -c Validation_Direct -r 0
./run.sh -c Validation_Multihop -r 0
./run.sh -c Validation_Clear_Rssi -r 0
./run.sh -c Validation_Obstacle_Rssi -r 0
./run.sh -c Validation_Obstacle_BaOff -r 0
./run.sh -c Validation_BaOn -r 0
./run.sh -c Validation_Sensor_RejectRange -r 0
./run.sh -c Validation_TwoVictims -r 0
```

Depois, execute as verificações automáticas:

```bash
python3 analysis/validate_results.py
```

O resultado esperado é:

```text
All deterministic ECHOSAR-Net validation checks passed.
```

### Critérios verificados automaticamente

| Cenário | Resultado esperado |
|---|---|
| `Validation_Direct` | um alerta recebe AppACK sem BA |
| `Validation_Multihop` | alerta confirmado com pelo menos um encaminhador IP |
| `Validation_Clear_Rssi` | RSSI de referência sem obstáculo |
| `Validation_Obstacle_Rssi` | RSSI pelo menos 10 dB inferior e interseção registrada |
| `Validation_Obstacle_BaOff` | múltiplas tentativas e expiração sem ACK |
| `Validation_BaOn` | BA ativado e recuperação confirmada após reposicionamento |
| `Validation_Sensor_RejectRange` | obstáculo fora do alcance não aciona o BA |
| `Validation_TwoVictims` | dois alertas únicos são gerados e confirmados |

## 10. Experimentos principais

O cenário de controle e o cenário proposto utilizam os mesmos parâmetros e
seeds. A única diferença intencional é `baEnabled`:

```bash
./run.sh -c Experiment_Control_BaOff
./run.sh -c Experiment_Proposed_BaOn
```

Cada configuração possui 30 repetições. Depois das execuções:

```bash
python3 analysis/process_results.py
```

O script gera tabelas CSV, estatísticas descritivas, IC95%, diferenças pareadas
e gráficos. Os resultados devem ser interpretados como avaliação experimental;
o projeto não pressupõe que o BA melhore a comunicação.

## 11. Limitações

- o sensor é geométrico e abstrato;
- a aptidão usa distância e obstrução, não uma previsão completa do receptor;
- RSSI multi-hop representa o último salto de rádio;
- distância adicional é apenas um proxy de energia;
- não há modelo de bateria, vento, câmera ou controle de baixo nível;
- IEEE 802.11 ad hoc, AODV e BA são abstrações de pesquisa, não recursos
  comerciais nativos do Phantom 4 Pro V2.0.
