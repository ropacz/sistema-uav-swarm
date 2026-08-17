# ECHOSAR-Net

Simulação de uma FANET para busca e resgate, desenvolvida com OMNeT++ 6.2.0 e
INET 4.5.4. O experimento avalia se o reposicionamento de um UAV pelo Bat
Algorithm altera a entrega confirmada de alertas quando obstáculos degradam a
comunicação ar-solo.

O projeto modela comunicação, mobilidade e reposicionamento. Detecção visual,
processamento de imagens, controle de voo e operação de resgate estão fora do
escopo.

## Arquitetura

```text
SarScenarioManager ── atribui vítima ao UAV mais próximo
          │
          ▼
DroneApp ── VictimAlert/UDP/AODV ──▶ TeamApp
       ▲                                      │
       └──────────── VictimAck ───────────────┘
          │
          ├── janela RSSI/PDR
          ├── AbstractObstacleSensor
          ├── BatAlgorithm
          └── BaGaussMarkovMobility
```

- IEEE 802.11b ad hoc, 2,4 GHz e 1 Mbps;
- roteamento multi-hop AODV;
- `PositionUpdate` periódico das equipes;
- retransmissão de aplicação a cada 30 s até `VictimAck` ou expiração;
- obstáculos conectados ao `DielectricObstacleLoss` do INET;
- comparação pareada BA ligado/desligado com as mesmas seeds.

Detalhes: [arquitetura e validação básica](docs/project_architecture_and_validation.md),
[arquitetura experimental](docs/dissertation_architecture.md) e
[rastreabilidade](docs/requirements_traceability.md).

## Estrutura

```text
src/app/           aplicações UDP dos drones e equipes
src/messages/      esquemas das mensagens da aplicação
src/mobility/      mobilidade Gauss-Markov comandável pelo BA
src/optimization/  implementação do Bat Algorithm
src/scenario/      vítimas e associação vítima–drone
src/sensing/       sensor geométrico abstrato
src/node/          nó UAV composto
simulations/       rede, obstáculos e configurações INI
analysis/          agregação estatística e exportação de resultados
docs/              arquitetura, parâmetros e rastreabilidade
```

Fontes `*_m.cc` e `*_m.h`, binários, resultados e figuras são
artefatos gerados e não pertencem ao controle de versão.

## Compilação

Configure o workspace local:

```bash
cp .env.example .env
```

Compile com o ambiente do INET:

```bash
opp_env run inet-4.5.4 -w /caminho/do/workspace --no-isolated \
  -c 'make makefiles && make'
```

Também é possível usar:

```bash
./run.sh --build -c Validation_Direct -r 0
```

## Validações determinísticas

```bash
./run.sh -c Validation_Direct -r 0
./run.sh -c Validation_Multihop -r 0
./run.sh -c Validation_Clear_Rssi -r 0
./run.sh -c Validation_Obstacle_Rssi -r 0
./run.sh -c Validation_Obstacle_BaOff -r 0
./run.sh -c Validation_BaOn -r 0
./run.sh -c Validation_Sensor_RejectRange -r 0
./run.sh -c Validation_TwoVictims -r 0
python3 analysis/validate_results.py
```

## Experimentos

Cada configuração possui 30 repetições com `seed-set=${repetition}`:

```bash
./run.sh -c Experiment_Control_BaOff
./run.sh -c Experiment_Proposed_BaOn
```

O controle mantém obstáculos e sensoriamento ativos, alterando apenas
`baEnabled`. A implementação não pressupõe que o BA produza melhora.

## Análise

```bash
python3 analysis/process_results.py
```

O script lê `simulations/results/*.sca` e gera em `analysis/figures/`:

- métricas por execução;
- média, mediana, desvio-padrão e IC95%;
- comparação pareada por seed;
- gráficos de AppACK, atraso, tentativas e deslocamento.
