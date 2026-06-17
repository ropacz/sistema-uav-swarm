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

- **30 drones** com mobilidade GaussMarkov, altitude constante **100 m**, 8–15 m/s
- **10 embarcações de resgate** com RandomWaypoint, Z = 1,5 m, 1,5–3,0 m/s
- **Área**: 2000 × 2000 m — grau médio k̄ ≈ 2,05 (rede moderadamente esparsa)
- **Tempo**: 300 s, 5 seeds (`repeat = 5`)
- **Roteamento**: AODV multi-hop (`AodvRouter`)
- **Resultado**: PDR ≈ 86,8 % ± 6,9 % (AppACK, 5 seeds)

> Modelo mede **entrega de informação** (alerta chegou à equipe), não sucesso de resgate.

### Fluxos de mensagens

```
equipe  ──[TeamUpdate  bcast 5001]──▶  drone          a cada 5 s (jitter inicial)
drone   ──[DroneStatus uni   5003]──▶  equipe          ACK com posição 3D
drone   ──[VictimAlert uni   5000]──▶  equipe          ao detectar vítima (Exp 40 s)
equipe  ──[VictimAck   uni   5002]──▶  drone origem    confirmação fim a fim (AODV)
drone   ──[VictimAlert bcast 5004]──▶  drones viz.     relay quando sem equipe
```

### Parâmetros de rádio

| Parâmetro | Valor |
|-----------|-------|
| Potência (todos) | 2,9 mW |
| Alcance nominal (FreeSpace) | ≈ 300 m |
| Alcance horizontal drone–equipe (3D) | ≈ 283 m |
| Sensibilidade | −85 dBm |
| Frequência | 2,4 GHz |

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
