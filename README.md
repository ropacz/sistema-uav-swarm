# ECHOSAR-Net

Simulação de uma FANET de busca e resgate em OMNeT++ 6.2 e INET 4.5.4.

**Pergunta:** o reposicionamento de um UAV orientado pelo Bat Algorithm altera
a entrega confirmada de alertas quando obstáculos degradam a comunicação
ar-solo?

**Desenho:** comparação pareada por seed entre um controle com o algoritmo
desligado e uma proposta com ele ligado, variando **apenas** `baEnabled`.

**Métrica primária:** `AppACK = alertas únicos confirmados ÷ alertas únicos
gerados`, uma medida por execução.

A hipótese é bilateral e falseável. O projeto não pressupõe que o Bat Algorithm
melhore a comunicação; uma diferença negativa é resultado admissível.

Hipóteses formais, unidade experimental e critérios de aceitação estão em
[`docs/scientific_protocol.md`](docs/scientific_protocol.md).

## O que é modelado

IEEE 802.11b ad hoc a 2,4 GHz, UDP e AODV multi-hop, mobilidade dos drones e
das equipes, perda adicional por obstáculos via `DielectricObstacleLoss`,
detecção geométrica abstrata do obstáculo, Bat Algorithm, reposicionamento
gradual e confirmação do alerta por `VictimAck`.

Fora do escopo: visão computacional, câmera, controle de voo, vento, modelo de
bateria e fidelidade a uma plataforma comercial. Detalhes e limites em
[`docs/model_and_assumptions.md`](docs/model_and_assumptions.md).

```text
SarScenarioManager ── associa vítima ao UAV mais próximo
          │
          ▼
      DroneApp ── VictimAlert / UDP / AODV ──▶ TeamApp
          ▲                                       │
          └──────────────  VictimAck  ────────────┘
          │
          ├── janela de RSSI e PDR
          ├── AbstractObstacleSensor
          ├── BatAlgorithm
          └── BaGaussMarkovMobility
```

## Estrutura

```text
src/app/           aplicações UDP dos drones e das equipes
src/messages/      esquemas das mensagens
src/mobility/      mobilidade Gauss-Markov comandável pelo BA
src/optimization/  Bat Algorithm
src/scenario/      vítimas e associação vítima–drone
src/sensing/       sensor geométrico abstrato
src/node/          nó UAV composto
simulations/       rede, obstáculos e configurações
analysis/          processamento científico, validação e testes
docs/              protocolo, modelo, métricas e rastreabilidade
```

Fontes `*_m.{cc,h}`, binários, resultados e figuras são artefatos gerados e não
pertencem ao controle de versão.

## Compilação

```bash
cp .env.example .env      # ajuste WORKSPACE e INET_VERSION
./run.sh --build -c Validation_Direct -r 0
```

Ou diretamente no ambiente do INET:

```bash
opp_env run inet-4.5.4 -w /caminho/do/workspace --no-isolated \
  -c 'make makefiles && make'
```

## Reprodução completa

```bash
make reproduce
```

Compila, roda os testes dos analisadores, executa os oito cenários
determinísticos, executa os dois braços pareados e processa os resultados. A
análise **falha** se um braço estiver incompleto, se houver seeds duplicadas ou
desemparelhadas, ou se os dois braços divergirem em qualquer parâmetro além de
`baEnabled`.

### Etapas isoladas

```bash
make validate          # oito cenários determinísticos + asserções
make analysis-tests    # testes dos analisadores de PCAP
make experiment        # os dois braços pareados + análise
make analyze           # apenas a análise
```

Verificação e evidência são coisas diferentes: os cenários `Validation_*`
confirmam que a implementação satisfaz seus contratos e **não** sustentam
conclusões sobre a hipótese. `Validation_BaOn` usa injeção controlada de falha
para exercitar a máquina de estados.

## Saídas da análise

`analysis/figures/` recebe `runs.csv` (uma linha por execução do experimento),
`summary.csv`, `paired_comparison.csv` (diferenças pareadas, IC95% e contagens
de pares discordantes), `data_quality.csv` (denominadores vazios),
`diagnostic_runs.csv` para as configurações fora do experimento, e
`experiment_manifest.json` com revisão Git e SHA-256 dos insumos.

Nenhum teste de hipótese é aplicado automaticamente — ver decisão D3 no
protocolo. O contrato de cada métrica está em
[`docs/metrics.md`](docs/metrics.md).

## Diagnóstico de rede

As configurações `Network_*` gravam PCAPNG por nó para auditoria de tráfego.
Elas alteram parâmetros de rádio ou injetam falha e por isso **não** são
evidência científica; a análise as separa automaticamente.

```bash
./run.sh -c Network_Realistic_Evaluation
python3 analysis/pcap_batch_to_spreadsheet.py simulations/results/pcap \
  --configuration Network_Realistic_Evaluation \
  --output simulations/results/spreadsheets/Network_Realistic_Evaluation.xlsx
```

## Estado atual

O cenário científico ainda não aciona o caminho degradação → sensor → BA, de
modo que os dois braços produziriam resultados idênticos por construção. As
decisões de desenho pendentes (D1–D4) estão registradas em
[`docs/scientific_protocol.md`](docs/scientific_protocol.md#9-decisões-de-desenho-ainda-em-aberto)
e precisam ser resolvidas antes do lote definitivo.
