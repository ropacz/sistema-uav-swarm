# Rastreabilidade resumida

| Requisito | Implementação | Validação |
|---|---|---|
| Associação vítima–drone | `SarScenarioManager` | menor distância e desempate por identificador |
| Mensagens estruturadas | `src/messages/*.msg` | build pelo `opp_msgtool` e entrega direta |
| Retry/ACK/dedup | aplicações drone/equipe | `Validation_Direct`, `Validation_Obstacle_BaOff` |
| AODV multi-hop | `BasicNetwork`/`AodvRouter` | sensibilidade calibrada e `hopCount >= 1` em `Validation_Multihop` |
| Perda por obstáculo | XML + `DielectricObstacleLoss` | contador de interseções e comparação de RSSI |
| Degradação | janela por equipe no drone | sinais RSSI/PDR e contadores de decisão |
| Sensor abstrato | `AbstractObstacleSensor` | confirmação geométrica, rejeição por alcance e limite de 30 m |
| BA e aptidão | `BatAlgorithm`/drone app | seed fixa, limites e métricas de execução |
| Movimento gradual | `BaGaussMarkovMobility` | deslocamento gradual, interrupção por ACK e tentativa pós-chegada |
| Experimento pareado | configs BA off/on | 30 repetições com `seed-set=${repetition}` |
| Estatística | `analysis/process_results.py` | CSV, IC95%, diferenças pareadas e gráficos |
| Aceitação determinística | `analysis/validate_results.py` | assertions sobre ACK, retry, RSSI, obstáculo, alcance do sensor, BA, multi-hop e duas vítimas |
