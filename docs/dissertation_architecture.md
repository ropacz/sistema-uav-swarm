# ECHOSAR-Net — arquitetura experimental

## Fluxo

1. `SarScenarioManager` agenda a detecção abstrata de cada vítima e associa o
   drone ativo mais próximo; empates usam o menor identificador lexicográfico.
2. Equipes difundem `PositionUpdate` a cada segundo. O drone mede RSSI pelo
   `SignalPowerInd`, PDR por lacunas de sequência e tempo desde a última amostra.
3. O originador envia uma tentativa `VictimAlert` por UDP/AODV. `alertId`
   identifica a vítima; `messageId` identifica a tentativa.
4. A equipe deduplica atendimento por `alertId`, mas responde a toda tentativa
   com `VictimAck`. Somente esse ACK encerra o alerta no originador.
5. Sem ACK e com degradação, o sensor geométrico consulta a LOS. Confirmação no
   intervalo 0,7–30 m autoriza o BA; rejeição é registrada sem atribuir a perda
   a obstáculo.
6. `BatAlgorithm` usa distância normalizada, obstrução geométrica e custo de
   movimento. `BaGaussMarkovMobility` percorre a posição escolhida respeitando
   velocidades horizontal, de subida e de descida.
7. Ao chegar, o drone aguarda a próxima tentativa regular do alerta. Somente o
   ACK de uma tentativa posterior à chegada conta como recuperação pelo BA. Ao
   confirmar ou esgotar o alerta, o Gauss-Markov prossegue da posição corrente.

Não há visão computacional, controle de voo, flooding de aplicação ou modelo
do OcuSync. IEEE 802.11 ad hoc, AODV e BA são abstrações de pesquisa.

## Convenções experimentais

- Controle pareado: `Experiment_Control_BaOff`.
- Proposto: `Experiment_Proposed_BaOn`.
- Ambos mantêm obstáculos, sensoriamento, seeds, tráfego e mobilidade; somente
  `baEnabled` varia.
- O ponto do obstáculo fornecido ao BA é a primeira interseção da superfície;
  candidatos respeitam também uma margem de segurança configurável.
- O sensor abstrato confirma somente interseção geométrica e alcance. Não há
  modelagem de câmera, orientação, iluminação, textura ou processamento visual.
- INET 4.5.4 não contém `RandomWalkMobility`; `MassMobility` é a alternativa
  nativa documentada para o passeio aleatório bidimensional das equipes.
- A convenção da velocidade do BA é `v(t)=v(t-1)+(p(t-1)-p*)f`, seguida de
  `p(t)=p(t-1)+v(t)`.

## Parâmetros iniciais

| Grupo | Valores |
|---|---|
| Área e duração | 1000×1000 m; 900 s |
| População | 4 drones, 1 equipe, 1 vítima, 2 obstáculos |
| Rádio | 2,4 GHz; 802.11b/DSSS; 1 Mbps; UDP; AODV |
| Alertas | retry 30 s; ACK timeout 5 s; 5 tentativas; TTL 180 s |
| Enlace | janela 10 s; PDR 0,8; RSSI −80 dBm; silêncio 3 s |
| Sensor | consulta geométrica abstrata; alcance 0,7–30 m |
| Movimento | 13 m/s; subida 5 m/s; descida 3 m/s; altitude 6–20 m |
| BA | 20 morcegos; 50 iterações; demais constantes conforme a implementação |
| Aptidão | link 0,60; obstáculo 0,25; movimento 0,15 |

## Limitações

RSSI de um pacote entregue por múltiplos saltos descreve o último salto de
rádio. A aptidão candidata é uma aproximação por distância e obstrução, não uma
previsão de antena, interferência ou SNIR. O proxy de energia é
distância adicional, não descarga eletroquímica da bateria. O sensor é uma
consulta geométrica abstrata, e não reconhecimento visual.
Resultados do BA devem ser inferidos da comparação; a implementação não
pressupõe melhora.
