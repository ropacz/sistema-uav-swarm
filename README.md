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

- **10 drones** com mobilidade GaussMarkov 3D (100–150 m altitude, 11–31 m/s)
- **3 equipes terrestres** estacionárias em triângulo na área de busca
- **Área**: 5000 × 5000 m
- **Tempo**: 300 s

### Fluxos de mensagens

```
equipe  ──[TeamUpdate  bcast 5001]──▶  drone(×10)   a cada 5s
drone   ──[DroneStatus uni   5003]──▶  equipe        ACK com posição 3D
drone   ──[VictimAlert uni   5000]──▶  equipe        ao detectar vítima
```

### Parâmetros de rádio

| Nó | Potência | Alcance (FreeSpace) |
|----|----------|---------------------|
| Drone | 20 mW | ~800 m |
| Equipe | 50 mW | ~1260 m |

> Referências: [FEA-2024] journal-fea.com · [SCI-2019] Tropea et al. (Scitepress) · [OPP-MAN] omnet-manual.com  
> Detalhes em [`docs/params_reference.md`](docs/params_reference.md)

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
./run.sh --warn       # log WARN (mais rápido)
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
    DroneStatus.msg             # ACK do drone
    VictimAlert.msg             # alerta de vítima
simulations/
  BasicNetwork.ned              # topologia
  omnetpp.ini                   # configurações
analysis/
  process_results.py            # pós-processamento: PDR, latência, energia
docs/
  params_reference.md           # rastreabilidade de parâmetros ↔ literatura
```

## Busca no UserGuide

```bash
python3 search_guide.py "communication range visualization"
python3 search_guide.py -n 5 "ini file parameters"
python3 search_guide.py --list-sections
```

> O `UserGuide.txt` não é versionado (`.gitignore`).  
> Baixe em: https://omnetpp.org/documentation/
