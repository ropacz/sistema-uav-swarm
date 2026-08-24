# Rastreabilidade e validações

## 1. Matriz funcional

| Requisito | Implementação | Evidência |
| --- | --- | --- |
| Gerar identidade lógica única | `DroneApp::handleAssignment` | `alertsGenerated` |
| Retransmitir até ACK/TTL | `sendAttempt`, `performMaintenance` | tentativas e retries |
| Gatilho por tentativas sem ACK | `performMaintenance` | `repositionTriggers` |
| Sensor binário | `tryReposition`, `AbstractObstacleSensor` | `obstaclesDetected` |
| Uma decisão BA por alerta | `repositionDecisionMade` | ativações por alertId |
| Um movimento por UAV | `RepositionController` | started/completed |
| Tentar imediatamente na chegada | timer `movementComplete` | smoke: segunda tentativa |
| Validar origem do ACK | mapas por `messageId` | confirmação central |
| Deduplicar entrega global | conjuntos de `ExperimentMetrics` | (D\le G) |
| Manter todas as seeds | relatório pareado | tabela de efeitos |
| Detectar drift entre braços | `parameter_differences` | falha do relatório |

## 2. Contrato dos sinais

Todos carregam `AlertMetricEvent` com `alertId`. `category` e `value` só são
usados quando indicados.

| Sinal | Emissor | Uso central |
| --- | --- | --- |
| `victimAlertGenerated` | `DroneApp` | inserir em gerados e guardar tempo |
| `victimAlertAttemptSent` | `DroneApp` | contar tentativas por alerta |
| `victimAlertDelivered` | `TeamApp` | primeira entrega e atraso fim a fim |
| `victimAlertConfirmed` | `DroneApp` | inserir em confirmados |
| `victimAlertExpired` | `DroneApp` | inserir em expirados |
| `victimRepositionTriggered` | `DroneApp` | contar exposição ao gatilho |
| `victimSensorEvaluated` | `DroneApp` | contar somente `detected` |
| `victimBaActivated` | `DroneApp` | contar execução do BA |
| `victimRepositionEvent` | `DroneApp` | `started`, `completed`, `distance` |

O coletor assina os sinais no módulo raiz e é a única origem normativa dos
escalares da aplicação.

## 3. Níveis de validação

### Estática e unitária

```bash
make analysis-tests
```

Verifica sintaxe, imports, leitura estrita dos escalares, razões com denominador
zero, exposição sem exclusão, estatística pareada e ferramentas PCAP opcionais.

### Build C++/NED

```bash
make
```

Deve ser executado no ambiente OMNeT++/INET configurado. Valida mensagens,
sinais, parâmetros NED e ligações C++.

### Smoke do mecanismo

```bash
make ba-smoke-test
```

O cenário é técnico, não científico. Ele injeta uma janela sem ACK e exige a
sequência:

```text
timeout -> sensor detecta -> BA ativa -> movimento conclui
        -> tentativa imediata -> ACK
```

O validador lê somente o contrato central e exige distância positiva.

### Descoberta direta

```bash
make network-discovery-validation
```

Confirma entrega direta e que uma equipe conhecida apenas por outro nó não leva
o UAV de origem a enviar uma tentativa com broadcasts limitados.

### Experimento confirmatório

```bash
make experiment
```

O relatório falha com seeds ausentes/duplicadas, drift além de `baEnabled` ou
escalares centrais ausentes. Ausência de ativação produz aviso e diagnóstico de
exposição, nunca exclusão.

## 4. Artefatos e proveniência

| Caminho | Papel |
| --- | --- |
| `simulations/*.ini` | valores e configurações executáveis |
| `simulations/results/omnetpp/*.sca` | fonte bruta normativa |
| `analysis/figures/main_experiment/runs.csv` | uma linha por run |
| `analysis/figures/main_experiment/paired_effects.csv` | efeitos por seed |
| `analysis/figures/main_experiment/summary.csv` | estimativas e IC95% |
| `analysis/figures/main_experiment/ba_exposure.csv` | exposição por seed |
| `analysis/figures/main_experiment/exposure_summary.csv` | exposição agregada |
| `analysis/figures/diagnostics/` | diagnóstico opcional MAC/IP/UDP |
| `analysis/figures/robustness/` | resultados da extensão de robustez |
| `simulations/results/pcap/` | auditoria opcional |

Antes da campanha final, devem ser preservados commit, estado da árvore, hashes
dos `.ini`, versões de OMNeT++/INET, configurações e seeds. Resultados derivados
podem ser regenerados e não substituem os `.sca`.

## 5. Checklist de publicação

- [ ] build e  testes passaram no commit congelado;
- [ ] smoke confirmou movimento e tentativa imediata;
- [ ] braços possuem exatamente as mesmas seeds;
- [ ] somente `baEnabled` difere entre braços;
- [ ] todas as seeds válidas foram incluídas, ativadas ou não;
- [ ] PDR foi tratado como desfecho primário;
- [ ] confirmação, atraso e retries foram tratados como secundários;
- [ ] exposição foi relatada sem interpretação causal individual;
- [ ] valores vieram dos `.ini`, sem cópia divergente na documentação;
- [ ] MAC/IP/PCAP foram usados apenas como diagnóstico.

## 6. Limitações conhecidas

- descoberta de equipes é direta; multihop é uma extensão separada;
- sensor e obstáculos são abstrações geométricas;
- o BA usa a última posição recebida, sem previsão de velocidade;
- o custo de enlace é geométrico e não prevê o canal;
- múltiplos alertas concorrentes compartilham um controlador de movimento; se o
  UAV estiver ocupado, a decisão do novo alerta é registrada, mas não enfileirada.
