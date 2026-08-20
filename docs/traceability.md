# Rastreabilidade

Ligação entre requisito, implementação e verificação. Os cenários
`Validation_*` verificam contratos da implementação; a evidência sobre a
hipótese vem apenas do experimento pareado.

| Requisito | Implementação | Verificação |
|---|---|---|
| Associação vítima–drone | `SarScenarioManager` | menor distância, desempate por identificador |
| Mensagens estruturadas | `src/messages/*.msg` | build pelo `opp_msgtool` e entrega direta |
| Retry, ACK e deduplicação | `DroneApp`, `TeamApp`; `alertId` evita novo atendimento, `messageId` identifica duplicatas | `Validation_Direct`, `Validation_Obstacle_BaOff`, `Validation_BaOn` |
| AODV multi-hop | `BasicNetwork` sobre `AodvRouter` | `hopCount >= 1` em `Validation_Multihop` |
| Perda por obstáculo | XML de geometria com `DielectricObstacleLoss` | contador de interseções e comparação de RSSI |
| Indicação de degradação | janela por equipe no `DroneApp` | sinais RSSI/PDR e contadores de decisão |
| Sensor abstrato | `AbstractObstacleSensor` | confirmação geométrica e rejeição por alcance |
| BA e aptidão | `BatAlgorithm` e `DroneApp` | seed fixa, restrições e métricas de resultado |
| Movimento gradual | `BaGaussMarkovMobility` | deslocamento gradual, interrupção por ACK, tentativa pós-chegada |
| Isolamento do fluxo aleatório | `num-rngs` e `rng-0` do `DroneApp` | equivalência entre braços com BA desligado |
| Experimento pareado | `Experiment_Control_BaOff`, `Experiment_Proposed_BaOn` | 30 repetições com `seed-set=${repetition}` |
| Tratamento isolado | `abstract` na base; comparação de parâmetros gravados | `require_treatment_isolation` falha se algo além de `baEnabled` divergir |
| Integridade do lote | `analysis/process_results.py` | falha com seeds ausentes, duplicadas ou desemparelhadas |
| Estatística | `analysis/process_results.py` | diferenças pareadas, IC95%, pares discordantes |
| Denominadores vazios | `analysis/process_results.py` | `data_quality.csv` |
| Proveniência | `experiment_manifest.json` | commit, estado do worktree, SHA-256 dos insumos |
| Decomposição de falhas | contadores do `DroneApp` | soma das causas igual ao total, em todo cenário |
| Aceitação determinística | `analysis/validate_results.py` | ACK, retry, RSSI, obstáculo, alcance, BA, multi-hop, duas vítimas |
| Qualidade do repositório | `make check`, `make validate` | sintaxe, referências obsoletas, oito cenários |
| Reprodução ponta a ponta | `make reproduce` | build, testes, validação, experimento e análise |

## Cenários de verificação

| Cenário | Contrato verificado |
|---|---|
| `Validation_Direct` | entrega direta com AppACK, sem BA |
| `Validation_Multihop` | alerta confirmado com ao menos um encaminhador IP |
| `Validation_Clear_Rssi` | RSSI de referência sem obstáculo |
| `Validation_Obstacle_Rssi` | RSSI ao menos 10 dB inferior e interseção registrada |
| `Validation_Obstacle_BaOff` | múltiplas tentativas e expiração sem ACK |
| `Validation_BaOn` | ativação, movimento e recuperação — com injeção controlada de falha |
| `Validation_Sensor_RejectRange` | obstáculo fora do alcance não aciona o BA |
| `Validation_TwoVictims` | dois alertas únicos gerados e confirmados |

Os oito cobrem contratos distintos, sem sobreposição. `Validation_BaOn` usa
injeção de falha e sensibilidade alterada: verifica a máquina de estados e
**não** é evidência científica.
