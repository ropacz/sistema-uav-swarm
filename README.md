# sistema-uav-swarm — ECHOSAR-Net

Simulação OMNeT++/INET de enxame de drones para busca e resgate (SAR).  
Dissertação de mestrado — PPGCAP/UDESC.

## Stack

| Componente | Versão |
|------------|--------|
| OMNeT++ | 6.2.0 |
| INET Framework | 4.5.4 |
| Gerenciador de ambiente | `opp_env` |

## Cenário BasicTest

- **20 drones** com mobilidade GaussMarkov, altitude **80–120 m**, 8–15 m/s
- **5 embarcações de resgate** com RandomWaypoint, Z = 1,5 m, 1,5–3,0 m/s
- **Área**: 5000 × 5000 m — grau médio k̄ ≈ 1,5 (rede esparsa)
- **Tempo**: 900 s, 10 seeds (`repeat = 10`) — `BasicTest_Piloto` roda o mesmo cenário em 300 s
  para validação rápida
- **Roteamento**: AODV multi-hop (`AodvRouter`)
- Não há vítima física nem sensor de detecção: os drones geram **alertas sintéticos**
  (intervalo exponencial, parâmetro `alertInterval`) para avaliar a comunicação FANET.

> Modelo mede **entrega de informação** (alerta chegou à equipe), não sucesso de resgate.

### Fluxos de mensagens

```
equipe  ──[TeamUpdate  bcast 5001]──▶  drone          a cada sendInterval (jitter inicial)
drone   ──[DroneStatus uni   5003]──▶  equipe          ACK com posição 3D
drone   ──[VictimAlert uni   5000]──▶  equipe          alerta sintético (Exp alertInterval)
equipe  ──[VictimAck   uni   5002]──▶  drone origem    confirmação fim a fim (AODV multi-hop)
drone   ──[VictimAlert bcast 5004]──▶  drones viz.     relay quando sem equipe conhecida
```

### Parâmetros de rádio

| Parâmetro | Valor |
|-----------|-------|
| Potência drone | 20 mW |
| Potência equipe | 50 mW |
| Alcance efetivo (FreeSpace) drone–drone | ≈ 700–800 m |
| Alcance efetivo (FreeSpace) equipe | ≈ 1200 m |
| Sensibilidade | −85 dBm |
| Frequência | 2,4 GHz |

> Detalhes e justificativa dos parâmetros em [`docs/scenario_reference.md`](docs/scenario_reference.md)
> e [`docs/tabela_parametros.md`](docs/tabela_parametros.md).

## Build & Run

```bash
# Copiar ambiente
cp .env.example .env   # ajuste WORKSPACE se necessário

# Build
opp_env run inet-4.5.4 -w /caminho/workspace --no-isolated \
  -c 'make makefiles && make'

# Rodar (terminal)
./run.sh

# Rodar com GUI
./run.sh --gui

# Opções do run.sh
./run.sh --build      # compila antes de rodar
./run.sh --info       # log INFO (desliga express-mode; padrão é WARN)
./run.sh -c Config    # outra configuração
./run.sh -r 2         # seed diferente
```

## Estrutura

```
src/
  app/
    SimpleDroneApp.{h,cc,ned}   # lógica do drone
    SimpleTeamApp.{h,cc,ned}    # lógica da equipe
    ports.h                     # constantes de porta UDP
  messages/
    TeamUpdate.msg              # beacon da equipe
    DroneStatus.msg             # ACK de posição do drone
    VictimAlert.msg             # alerta sintético do drone
    VictimAck.msg               # confirmação da equipe
simulations/
  BasicNetwork.ned              # topologia
  omnetpp.ini                   # configurações
analysis/
  process_results.py            # pós-processamento: PDR, atraso, retries, overhead, AppACK
docs/
  scenario_reference.md         # parâmetros ↔ literatura, hipótese de pesquisa
  tabela_parametros.md          # valores prontos para a tabela LaTeX da dissertação
  fluxo_experimento.md          # esclarecimentos sobre o fluxo drone→equipe→ACK
```

## Busca no UserGuide

```bash
python3 search_guide.py "communication range visualization"
python3 search_guide.py -n 5 "ini file parameters"
python3 search_guide.py --list-sections
```

> O `UserGuide.txt` não é versionado (`.gitignore`).  
> Baixe em: https://omnetpp.org/documentation/
