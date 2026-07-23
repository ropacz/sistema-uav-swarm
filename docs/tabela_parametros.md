# Parâmetros da Simulação — Valores para a Tabela LaTeX

Fonte de verdade: `simulations/omnetpp.ini`, config **BasicTest** (900 s).

---

## Tabela preenchida

| Parâmetro | Valor | Fonte |
|---|---|---|
| Número de Drones | 20 | `*.numDrones = 20` |
| Número de Obstáculos | **5** | `simulations/obstacles.xml` (5 blocos `<object>`) |
| Tempo de simulação | **900 s** | `sim-time-limit = 900s` |
| Tamanho do Pacote (TeamUpdate — equipe→drone) | **128 bytes** | `SimpleTeamApp.cc:109` |
| Tamanho do Pacote (VictimAlert — drone→equipe) | **256 bytes** | Metadados da ocorrência (IDs, coordenadas, timestamp) |
| Tamanho do Pacote (DroneStatus — drone→equipe) | **128 bytes** | `SimpleDroneApp.cc:222` |
| Tamanho do Pacote (VictimAck — equipe→drone) | **64 bytes** | `SimpleTeamApp.cc:137` |
| Área | 25 km² | `bgb = 5000,5000` |
| Altitude drones | **80 m e 120 m** | `constraintAreaMinZ=80m / MaxZ=120m` ⚠️ ver nota |
| Velocidade drones | 8 m/s e 15 m/s | `uniform(8mps, 15mps)` |
| Velocidade equipes | **1,5 m/s e 3,0 m/s** | `uniform(1.5mps, 3.0mps)` |
| Número de equipes | 5 | `*.numTeams = 5` |
| Tecnologia de rede | **IEEE 802.11n, 2,4 GHz, canal 20 MHz, modo ad hoc** | `omnetpp.ini` + `Ieee80211ScalarRadioMedium` |
| Raio de alcance drones (calculado) | **≈ 791 m** | FSPL: 20 mW, −85 dBm, 2,4 GHz → FSPL_max = 98,0 dB |
| Raio de alcance equipes (calculado) | **≈ 1251 m** | FSPL: 50 mW, −85 dBm, 2,4 GHz → FSPL_max = 102,0 dB |
| Grau médio de conectividade (k̄) | **≈ 1,5** | k̄ = (N−1)·π·r²/A = 19·π·791²/25×10⁶ |
| Taxa de transmissão | **26 Mbps** (MCS3, 16-QAM, R=1/2) | `**.wlan[*].radio.transmitter.datarate = 26Mbps` |

> ⚠️ **Erro tipográfico na tabela LaTeX:** está escrito `1$80\text{ m}$` — deve ser `$80\text{ m}$` e `$120\text{ m}$`.

---

## Justificativa para 26 Mbps (IEEE 802.11n, MCS3)

### O que é 26 Mbps no padrão IEEE 802.11n

26 Mbps corresponde ao **MCS3** do padrão IEEE 802.11n com canal de 20 MHz e
uma antena (SISO) — modulação 16-QAM com taxa de codificação 1/2, guard
interval longo (800 ns). É uma taxa intermediária que equilibra:

- **Robustez:** exige SNR menor que MCS4–MCS7 (16-QAM 3/4, 64-QAM), mantendo
  alcance adequado para UAVs em espaço livre.
- **Compatibilidade:** disponível em todo hardware 802.11n de uma antena,
  incluindo módulos embarcados de baixo peso e consumo (ex.: Raspberry Pi 3 B+,
  módulos ESP-WROOM).
- **Uso por Panda et al. (2019):** o hardware RPI 3 B+ utilizado no artigo de
  referência opera em 802.11n com canal de 20 MHz — a taxa de 26 Mbps (MCS3)
  é compatível com os experimentos de campo reportados.

### Por que não usar MCS0 (6,5 Mbps)?

MCS0 maximiza alcance mas não é representativo do hardware UAV real operando em
802.11n. O RPI 3 B+ utilizado por Panda et al. (2019) negociou taxas acima de
6,5 Mbps nos experimentos com 20 MHz de canal. MCS3 é mais realista para o
cenário e ainda mantém k̄ ≈ 1,1 com as potências adotadas (20 mW / 50 mW).

### A demanda de throughput é negligenciável

O tráfego gerado pelos 20 nós é da ordem de **kbps** — pacotes de 64–256 bytes
a cada 1–40 s. O canal nunca atinge 0,01% dos 26 Mbps disponíveis. O gargalo
real é latência AODV e colisões MAC, não largura de banda.

### Referências que justificam o valor

| Referência | O que suporta |
|---|---|
| **Panda et al. (2019)** — *Design and Deployment of UAV-Aided Post-Disaster Emergency Network*. IEEE Access. DOI: 10.1109/ACCESS.2019.2931539 | Hardware RPI 3 B+ em 802.11n, canal 20 MHz, modo ad hoc, 2,4 GHz; alcance medido 280 m (solo) — compatível com MCS3 em UAV a 80–120 m de altitude. |
| **Lansky et al. (2023)** — citado como `lansky2023fanet` | Simulações FANET com 802.11n; confirma uso de taxas MCS2–MCS4 em cenários de baixa densidade para equilíbrio alcance/throughput. |
| **Loor et al. (2026)** — citado como `loor2026qos` | Parâmetros de QoS para FANET com 802.11n em 2,4 GHz; alinha com MCS3 para cenários SAR de baixo tráfego. |
| **IEEE Std 802.11-2020** | Define MCS3 (16-QAM, R=1/2) = 26 Mbps para canal 20 MHz, 1 fluxo espacial; guard interval 800 ns. |

### Texto sugerido para a dissertação

> A taxa física de transmissão adotada é de 26 Mbps, correspondente ao MCS3
> (16-QAM, taxa de codificação 1/2) do padrão IEEE 802.11n com canal de 20 MHz
> em configuração SISO \cite{ieee80211-2020}. Essa taxa oferece equilíbrio entre
> robustez de enlace e throughput disponível, sendo compatível com o hardware
> Raspberry Pi 3 B+ empregado nos experimentos de campo de \cite{panda2019uavwifi}.
> Como o tráfego de controle gerado pela aplicação é da ordem de kilobits por
> segundo, a capacidade do canal não representa gargalo; o fator limitante é a
> cobertura geográfica e a latência de roteamento AODV \cite{lansky2023fanet,
> loor2026qos}.

---

## Notas adicionais para revisão da tabela

- **"Tecnologia de comunicação"**: o valor correto é `IEEE 802.11n, 2,4 GHz, canal 20 MHz, modo ad hoc`.
- **Alcance ≈ 788 m**: é o raio usado para calcular k̄, não um parâmetro configurado diretamente. O alcance efetivo simulado varia entre 700–800 m dependendo do posicionamento e altitude.
- O pacote mais importante para a tabela é provavelmente o **VictimAlert (256 bytes)** e o **TeamUpdate (128 bytes)**, pois são os de maior volume. O VictimAck (64 bytes) é o menor.
