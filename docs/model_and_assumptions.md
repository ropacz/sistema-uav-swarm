# Modelo e premissas do piloto

## Componentes

| Componente | Papel |
|---|---|
| `SarScenarioManager` | associa a vítima ao único drone |
| `DroneApp` | alerta, retry, gatilho, previsão e coordenação do BA |
| `TeamApp` | posição, deduplicação e `VictimAck` |
| `AbstractObstacleSensor` | interseção geométrica e alcance sensorial |
| `BatAlgorithm` | busca estocástica de menor custo |
| `RepositionFitness` | custo e restrições de viabilidade |
| `BaGaussMarkovMobility` | movimento gradual e interrupção por ACK |

## Modelos do INET

- `Ieee80211ScalarRadioMedium` e `Ieee80211ScalarRadio`;
- IEEE 802.11b ad hoc, UDP/IPv4 e AODV;
- `FreeSpacePathLoss` com `alpha = 2`;
- `DielectricObstacleLoss` sobre `PhysicalEnvironment`;
- `TurtleMobility` para a trajetória determinística da equipe;
- mobilidade do drone parada fora dos comandos do BA.

O cenário tem um drone e uma equipe. AODV é usado, mas não existe nó
intermediário capaz de encaminhar: toda entrega válida deve ter `hopCount = 0`.

## Estados do reposicionamento

```text
IDLE → MOVING → AWAITING_VALIDATION → IDLE
```

Um ACK durante `MOVING` comprova recuperação do enlace, mas não a posição final
planejada. Um ACK de tentativa enviada em `AWAITING_VALIDATION` valida a posição
final. A distância real é medida na posição interpolada em que o ciclo termina;
a distância comandada é registrada separadamente.

## Premissas e limites

- o sensor conhece a geometria do simulador e não modela câmera, FOV, erro de
  detecção, textura ou iluminação;
- não há erro de GPS nem ruído na geometria do obstáculo;
- a equipe é extrapolada por movimento retilíneo uniforme; uma mudança após a
  perda do enlace é desconhecida;
- a aptidão aproxima qualidade de enlace por distância e obstrução, não prevê
  RSSI, SNIR, interferência ou congestionamento futuros;
- pesos da aptidão e raio de 40 m são escolhas do piloto e exigem análise de
  sensibilidade antes de generalização;
- `obstacleSafetyMargin` mede distância ao primeiro ponto de superfície
  detectado, não a menor distância a toda a malha do edifício; as verificações
  de interseção continuam usando o objeto completo;
- distância é proxy cinemático, não consumo eletroquímico;
- vento, bateria, aerodinâmica e controle de voo de baixo nível não são
  modelados;
- uma geometria e cinco seeds validam mecanismo, não superioridade universal.

As fórmulas e valores completos estão em [`pilot_experiment.md`](pilot_experiment.md).
