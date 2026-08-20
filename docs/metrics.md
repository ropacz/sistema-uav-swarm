# Contrato das métricas

Cada métrica declara definição, unidade, população, numerador, denominador,
ponto de coleta, agregação e limite de interpretação. Contadores de camadas
diferentes não são combinados sem regra explícita.

Toda razão com população vazia é registrada como **indefinida**, nunca como
zero, e aparece em `analysis/figures/data_quality.csv`.

## 1. Métrica primária

### AppACK

| Campo | Valor |
|---|---|
| Definição | Fração de alertas únicos que receberam `VictimAck` no drone originador |
| Unidade | por cento |
| População | alertas únicos gerados na execução |
| Numerador | `uniqueAlertsAcked`, somado sobre os drones |
| Denominador | `uniqueAlertsGenerated`, somado sobre os drones |
| Coleta | `DroneApp::finish()` |
| Agregação | uma linha por execução; depois, diferença pareada por seed |
| Interpretação | sucesso fim a fim da aplicação |
| Limite | ACK de MAC **não** equivale a `VictimAck`. Com uma vítima por execução, é uma variável de Bernoulli — ver D2 no protocolo |
| Hipótese | é a métrica de decisão sobre H0 |

## 2. Métricas secundárias

Descrevem o mecanismo. **Não substituem** a conclusão baseada na métrica
primária.

| Métrica | Numerador ÷ denominador | Unidade | Limite de interpretação |
|---|---|---|---|
| `alert_attempt_delivery_pct` | `attemptsReceived` ÷ `alertAttemptsSent` | % | Mede eficiência das retransmissões, não sucesso de alertas únicos |
| `one_way_delay_s` | `totalDeliveryDelay` ÷ `attemptsReceived` | s | Condicionada às tentativas recebidas: não inclui as perdidas |
| `alert_age_at_reception_s` | `totalAlertAgeAtReception` ÷ `attemptsReceived` | s | Idade desde a geração, não desde a última tentativa |
| `rtt_s` | `totalRTT` ÷ `uniqueAlertsAcked` | s | Só existe para alertas confirmados |
| `attempts_per_alert` | `alertAttemptsSent` ÷ `uniqueAlertsGenerated` | — | Limitado superiormente por `maxAttempts` |
| `alerts_expired` | contagem | — | TTL esgotado ou tentativas exauridas |
| `recovery_time_s` | `totalRecoveryTime` ÷ `recoverySamples` | s | Só definida quando houve recuperação validada |
| `ba_distance_m` | `baDistance` ÷ `uniqueAlertsGenerated` | m | Proxy de energia adicional, não descarga de bateria |
| `pre/post_reposition_pdr` | somas ÷ `repositionValidationSamples` | — | Janela deslizante de `PositionUpdate` |
| `pre/post_reposition_rssi_dbm` | somas ÷ amostras válidas | dBm | Em multi-hop descreve o **último salto**, não o caminho |

## 3. Métricas diagnósticas

Explicam por que o mecanismo agiu ou deixou de agir. Nunca sustentam a
conclusão.

| Contador | Significado |
|---|---|
| `degradationIndications` | Janela indicou degradação com alerta pendente |
| `sensorConfirmations` | Sensor confirmou obstáculo dentro do alcance |
| `sensorRejections` | Sensor consultado e **não** confirmou obstáculo no alcance |
| `teamUnknownForReposition` | Degradação sem posição conhecida da equipe: o sensor não pôde ser consultado |
| `baActivations` | Execuções do Bat Algorithm |
| `successfulRepositions` | ACK de uma tentativa **posterior** à chegada |
| `failedRepositions` | Total; igual à soma exata das quatro causas abaixo |
| `baNoFeasibleSolution` | BA não devolveu posição utilizável, ou mobilidade não comandável |
| `baRedundantCandidate` | Candidato já testado neste alerta, ou deslocamento nulo |
| `repositionExpiredBeforeAck` | TTL ou tentativas esgotaram durante o reposicionamento |
| `repositionAckedBeforeValidation` | Alerta entregue por tentativa anterior à chegada: sucesso de entrega, sem validar a posição |
| `duplicatePackets` | Mesmo `messageId` recebido novamente |
| `hopCount` | Saltos IP inferidos pelo TTL |

`sensorRejections` e `teamUnknownForReposition` são causas distintas e foram
separadas deliberadamente: a segunda não é uma rejeição sensorial, pois a
consulta sequer ocorre. `analysis/validate_results.py` verifica em todo cenário
determinístico que a decomposição de `failedRepositions` soma exatamente o
total.

## 4. Métricas derivadas de capturas de rede

Diagnósticas e auditáveis, produzidas por `analysis/pcap_batch_to_spreadsheet.py`.

| Medida | Regra | Limite |
|---|---|---|
| PDR de broadcast | Recepções observadas ÷ oportunidades observáveis para os pares capturados | Uma oportunidade não prova que todos os nós estavam no alcance |
| Tráfego unicast | Contado no IP originador; encaminhamentos tratados por salto | Evita contar um datagrama multi-hop como várias mensagens |
| Estatística multiseed | Cada execução tem o mesmo peso | PDR médio por seed e PDR agregado respondem a perguntas diferentes e devem ter nomes distintos |

Capturas em nós diferentes registram o mesmo pacote. A análise deduplica ou
classifica cada registro como transmissão, recepção ou encaminhamento antes de
totalizar.

### Identidade do pacote

A correlação usa, nesta ordem: o identificador ECHO no payload; os campos da
aplicação; e a tupla de rede com janela temporal, apenas como fallback
documentado. O tamanho do pacote **não** é identidade principal.

Formato do cabeçalho, em ordem de rede (big endian):

| Campo | Tamanho | Valor |
|---|---:|---|
| magic | 4 B | ASCII `ECHO` |
| version | 1 B | `1` |
| message type | 1 B | `1` PositionUpdate, `2` VictimAlert, `3` VictimAck |
| encoded body length | 2 B | tamanho dos campos significativos |

Strings são `uint16` de tamanho seguido de UTF-8; inteiros e doubles são big
endian; timestamps são doubles em segundos. Após os campos, o payload recebe
zeros até o tamanho experimental configurado. A identificação por tamanho
existe apenas para ler capturas anteriores à versão 1 do formato.

## 5. Fontes de evidência

| Arquivo | Papel |
|---|---|
| `.sca` | Estado interno, decisões do BA e escalares. Fonte da métrica primária |
| `.vec` | Séries temporais dos sinais registrados |
| `.pcapng` | Tráfego efetivamente observado nas interfaces |
| `.elog` | Diagnóstico. **Não** é resultado científico isolado |

Valores deriváveis são calculados em `analysis/`, evitando contadores
redundantes no simulador. Detalhes internos do algoritmo só viram métrica
quando respondem a uma pergunta explícita.

## 6. Saídas da análise

| Arquivo | Conteúdo |
|---|---|
| `runs.csv` | Uma linha por execução **do experimento** |
| `summary.csv` | Descritivas por braço, com n efetivo por métrica |
| `paired_comparison.csv` | Diferenças pareadas, IC95% e contagens de pares discordantes |
| `data_quality.csv` | Métricas indefinidas e denominadores vazios |
| `diagnostic_runs.csv`, `diagnostic_summary.csv` | Configurações fora do experimento |
| `experiment_manifest.json` | Proveniência do lote |

## 7. Proveniência

Todo resultado publicável registra configuração, repetição, seed, duração,
revisão Git, estado do worktree, SHA-256 dos insumos e limitações da execução.
`experiment_manifest.json` é o registro do lote.

Resultados são artefatos gerados, ficam fora do Git e são organizados em
`simulations/results/`: `omnetpp/` para `.sca`, `.vec` e `.vci`; `pcap/` para
capturas; `eventlogs/` para `.elog`; `spreadsheets/` para CSV e XLSX.

Excluir resultados locais não substitui o arquivamento externo do conjunto que
sustenta uma tabela, figura ou conclusão publicada.
