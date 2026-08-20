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

- IEEE 802.11b ad hoc, 2,4 GHz, canal de 22 MHz e 1 Mbps;
- roteamento multi-hop AODV;
- `PositionUpdate` periódico das equipes;
- retransmissão de aplicação a cada 30 s até `VictimAck` ou expiração;
- obstáculos conectados ao `DielectricObstacleLoss` do INET;
- comparação pareada BA ligado/desligado com as mesmas seeds.

Detalhes: [princípios de engenharia](docs/engineering_and_scientific_principles.md),
[Bat Algorithm e função de aptidão](docs/bat_algorithm_and_fitness.md),
[guia visual de obstáculos](docs/obstacle_detection_visual_guide.md),
[guia de implementação e validação](docs/project_architecture_and_validation.md),
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
./run.sh --gui                    # demonstração móvel no Qtenv
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

O mesmo conjunto pode ser executado sequencialmente com `make validate`.

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
- manifesto JSON com revisão Git e hashes dos insumos experimentais.

### Perdas a partir das capturas de rede

A configuração `Network_Log_Demo` grava um PCAPNG por nó. Para consolidar
automaticamente todas as capturas existentes em `simulations/results/`:

```bash
python3 analysis/pcap_batch_to_spreadsheet.py \
  simulations/results \
  --output simulations/results/metricas-rede.xlsx
```

Para uma análise reprodutível de uma execução específica, filtre explicitamente
a configuração e o número da execução. Os demais arquivos continuam listados
na aba `Inventário`, mas não entram nos denominadores:

```bash
python3 analysis/pcap_batch_to_spreadsheet.py simulations/results \
  --configuration Network_Log_Demo --run 0 \
  --output simulations/results/Network_Log_Demo-0-metricas.xlsx
```

A planilha contém resumo por configuração, métricas por execução e enlace, comparação
pacote a pacote, eventos decodificados e inventário dos arquivos processados.
O script identifica automaticamente os nós e IPs registrados pelo INET. Os
payloads atuais usam um cabeçalho binário versionado `ECHO`; assim, tipo,
`messageId`, `alertId`, sequência e tentativa são lidos diretamente do PCAP.

Para comparar apenas dois arquivos, use `analysis/pcap_to_spreadsheet.py`.
