# ECHOSAR-Net — piloto da hipótese

Simulação OMNeT++ 6.2/INET 4.5.4 para verificar se o reposicionamento de um
UAV pelo Bat Algorithm recupera a comunicação com uma equipe móvel após uma
obstrução física.

O projeto possui somente um experimento pareado:

| Configuração | Tratamento |
|---|---|
| `HypothesisPilot_BaOff` | controle, BA desligado |
| `HypothesisPilot_BaOn` | proposta, BA ligado |

Os braços possuem a mesma topologia, trajetória, rádio, obstáculo e seeds;
somente `baEnabled` muda. Cada braço executa cinco repetições.

## Executar

```bash
cp .env.example .env
./run.sh --build -c HypothesisPilot_BaOn -r 0
make hypothesis-pilot
make network-metrics
```

O último comando executa os dez runs pareados e imprime o resumo. Para auditar
pacotes de uma seed:

```bash
./run.sh -c HypothesisPilot_BaOff -r 0 --pcap
./run.sh -c HypothesisPilot_BaOn -r 0 --pcap
```

Os escalares ficam em `simulations/results/omnetpp/` e os PCAPNG opcionais em
`simulations/results/pcap/`.

## Resultado atual do piloto

Em cinco seeds, o controle obteve 0/5 AppACK e o BA ligado obteve 5/5 AppACK.
Todas as entregas tiveram `hopCount = 0`: o movimento recuperou um enlace
direto; não houve encaminhamento multihop. Os ACKs chegaram durante o
deslocamento, após 31,51 m reais em média, antes da validação da posição final.
O teste exato bilateral de McNemar é `p = 0,0625`; o resultado é exploratório,
não uma rejeição confirmatória de H0 a 5%.

A descrição científica completa, incluindo fórmulas, gatilho, Bat Algorithm,
métricas e limites de validade está em
[docs/pilot_experiment.md](docs/pilot_experiment.md).

## Estrutura

```text
src/          aplicações, sensor, mobilidade e otimização
simulations/  cenário, trajetória, obstáculo e resultados
analysis/     relatório do piloto e auditoria PCAP
docs/         protocolo, métricas e premissas
```
