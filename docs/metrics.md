# Contrato das métricas do piloto

## Métrica primária

```text
AppACK = uniqueAlertsAcked / uniqueAlertsGenerated
```

É sucesso fim a fim da aplicação, não ACK do MAC. Cada execução gera um alerta,
logo o desfecho por seed é binário. A comparação relevante é pareada entre a
mesma seed com BA desligado e ligado.

## Métricas secundárias

| Métrica | Definição | Limite |
|---|---|---|
| Entrega por tentativa | `attemptsReceived / alertAttemptsSent` | não equivale a alertas únicos |
| Tentativas por alerta | `alertAttemptsSent / uniqueAlertsGenerated` | limitada por `maxAttempts` |
| RTT | `totalRTT / uniqueAlertsAcked` | existe apenas quando há ACK |
| Atraso | `totalDeliveryDelay / attemptsReceived` | condicionado à recepção |
| Expiração | `alertsExpired` | TTL ou tentativas esgotadas |
| Distância real do BA | `baDistance` | percurso até ACK, chegada ou expiração |
| Distância comandada | `commandedBaDistance` | destino planejado; pode superar o percurso real |
| Hop count | TTL inicial menos TTL recebido | zero é enlace direto |

## Métricas do mecanismo

| Escalar | Significado |
|---|---|
| `degradationIndications` | PDR, RSSI ou silêncio indicou degradação |
| `sensorConfirmations` | obstáculo confirmado no alcance do sensor |
| `sensorClearLineOfSight` | degradação sem obstáculo na visada |
| `sensorOutsideRange` | obstáculo presente, mas fora do alcance sensorial |
| `baActivations` | execuções do otimizador |
| `baNoFeasibleSolution` | nenhum candidato utilizável |
| `baRedundantCandidate` | candidato repetido ou deslocamento nulo |
| `repositionExpiredBeforeAck` | alerta expirou durante o ciclo |
| `successfulRepositions` | ACK de tentativa enviada após a chegada |
| `repositionAckedBeforeValidation` | ACK durante o movimento; enlace recuperado, posição final não validada |
| `predictedTeamPositions` | consultas que extrapolaram a equipe |
| `teamPredictionAgeSum/Max` | idade acumulada e máxima das previsões |

`failedRepositions` soma apenas solução inviável, candidato redundante e
expiração. Recuperação durante o movimento não é contabilizada como falha.

## Evidência e agregação

- `.sca`: contadores, parâmetros e decisões; fonte principal;
- `.vec`: séries temporais habilitadas;
- `.pcapng`: auditoria opcional dos pacotes;
- uma seed é uma réplica e todas recebem o mesmo peso;
- razão com denominador zero é indefinida, não zero.

`analysis/report_hypothesis_pilot.py` exige cinco resultados por braço,
compara ativação e entrega e rejeita uma alegação de recuperação direta se o
hop count recebido for diferente de zero. O analisador também rejeita escalares
ausentes, seeds não pareadas e qualquer diferença entre braços além de
`baEnabled`; ao final grava `simulations/results/pilot_manifest.json` com
revisão Git e SHA-256 dos insumos e resultados.
O arquivo `simulations/results/pilot_runs.csv` preserva uma linha por seed para
que o agregado possa ser reconstruído.
