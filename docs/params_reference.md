# Referência de Parâmetros de Simulação FANET — OMNeT++/INET

Fontes:
- **[FEA-2024]** — "Trusted Fuzzy Routing Scheme in Flying Ad-hoc Network", journal-fea.com, 2024 (600 drones, ACO/BWOA/fuzzy)
- **[SCI-2019]** — Tropea et al., "A Simulator for Drones and FANET Management", Scitepress 2019 (9 drones LAP, cobertura local)
- **[OPP-MAN]** — "Steps to implement Flying Vehicle Communication in OMNeT++", omnet-manual.com

---

## Área e Topologia

| Parâmetro | Valor | Fonte |
|-----------|-------|-------|
| Área mínima para swarms densos | 5000 × 5000 × 500 m | [FEA-2024] |
| Área para 9 drones LAP | ~1700 × 1300 m | [FEA-2024] |
| Grid de drones LAP | 3 × 3 a 120 m de altitude | [SCI-2019] |
| Espaçamento entre drones no grid | 252 m (ortogonal) | [SCI-2019] |
| `bgb` NED (canvas visual) | igual ao constraint de X,Y | convenção INET |

## Rádio e Alcance

| Parâmetro | Valor | Fonte |
|-----------|-------|-------|
| Alcance de comunicação drone–drone | 300 m (prático) | [SCI-2019] |
| Raio de cobertura de solo (footprint) | 175 m por drone a 120 m | [SCI-2019] |
| Potência do transmissor (drone) | 20 mW | [OPP-MAN] |
| Datarate | 6 Mbps | [OPP-MAN] |
| Banda IEEE 802.11 | 10 Mbps (LAP) | [SCI-2019] |
| Protocolo MAC/PHY | IEEE 802.11 ad-hoc | [OPP-MAN] |

## Mobilidade

| Parâmetro | Valor | Fonte |
|-----------|-------|-------|
| Velocidade do drone | 11–31 m/s (25–70 mph) | [FEA-2024] |
| Velocidade recomendada (omnet-manual) | uniform(10, 20) mps | [OPP-MAN] |
| Altitude ótima LAP | 120 m | [SCI-2019] |
| `constraintAreaMinZ` swarm denso | 200 m | [FEA-2024] |
| Modelo de mobilidade recomendado | MassMobility (inércia 3D) | [FEA-2024] |
| Velocidade humana no solo | 3 km/h | [SCI-2019] |

## Tráfego e Aplicação

| Parâmetro | Valor | Fonte |
|-----------|-------|-------|
| Payload de telemetria | 512–1024 bytes | [FEA-2024] |
| Tamanho de pacote alternativo | 4700 bits (~588 B) | [FEA-2024] |
| `sendInterval` (beacon/heartbeat) | 1 s | [OPP-MAN] |
| Buffer MAC máximo | 50 pacotes | [SCI-2019] |
| Timer de atualização Link State | 30 s | [SCI-2019] |
| Tempo de simulação | 300–350 s | [FEA-2024] / [OPP-MAN] |

## Trust Management (FUBA / Fuzzy)

| Variável de Entrada | Métrica | Função |
|---------------------|---------|--------|
| Battery (Energy Level) | Energia residual (%) | Exclui nós com fadiga energética |
| RSSI | Intensidade do sinal (dBm) | Viabilidade do enlace físico |
| PDR | Pacotes entregues / recebidos | Detecção de gray hole attacks |
| Transmission Delay | Atraso ponta-a-ponta (ms) | Penaliza nós congestionados |
| Umidade do ar | Fator de atenuação (%) | Evita falsos positivos por clima |
| **Trust Score (output)** | 0,0 – 1,0 normalizado | Seleção de rota / Cluster Head |

Fonte de toda a seção: [FEA-2024] — motor de inferência Mamdani com histerese anti-flapping.

## Clustering e Roteamento

| Mecanismo | Função | Fonte |
|-----------|--------|-------|
| BWOA (Binary Whale Optimization) | Define número ideal de clusters | [FEA-2024] |
| ACO (Ant Colony Optimization) | Seleção de rota por feromônio + trust | [FEA-2024] |
| WAA-CR | Eleição de Cluster Head multivariável | [FEA-2024] |
| LFEAR | Filtragem geométrica de RREQ (elipse) | [FEA-2024] |
| Link State + LU timer 30 s | Roteamento LAP 9 drones | [SCI-2019] |

---

## Mapeamento nos parâmetros atuais do projeto (`omnetpp.ini`)

| Parâmetro no `.ini` | Valor adotado | Justificativa / Fonte |
|---------------------|---------------|----------------------|
| `sim-time-limit` | 300 s | [FEA-2024] / [OPP-MAN] |
| `constraintAreaMax{X,Y}` | 5000 m | [FEA-2024] — swarm de baixa densidade |
| `constraintAreaMin/MaxZ` | 80–150 m | [SCI-2019] ótimo 120 m; [FEA-2024] min 200 m para alta densidade |
| `mobility.speed` | 15 mps ± 3 | mediana 11–31 m/s [FEA-2024] |
| `transmitter.power` (drone) | 20 mW | [OPP-MAN] |
| `transmitter.power` (equipe) | 50 mW | proporcional: 2,5× drone → raio ~1,6× |
| `transmitter.datarate` | 6 Mbps | [OPP-MAN] |
| `receiver.sensitivity` | -85 dBm | padrão INET / IEEE 802.11 |
| Portas UDP (5000/5001/5003) | aplicação customizada | projeto ECHOSAR |
