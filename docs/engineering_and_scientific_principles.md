# Princípios de engenharia do ECHOSAR-Net

Este documento define regras de projeto verificáveis. Ele complementa a
arquitetura experimental e não substitui a descrição metodológica da dissertação.

## Engenharia de software

| Princípio de Hooker | Aplicação no projeto |
|---|---|
| A razão de existir | Toda funcionalidade deve contribuir para medir a entrega confirmada do alerta ou explicar o efeito do reposicionamento. Visão computacional, controle de voo e OcuSync permanecem fora do escopo. |
| KISS | UDP/AODV cuida do transporte multi-hop; não existe flooding paralelo na aplicação. A aptidão usa somente enlace estimado, obstáculo e deslocamento. |
| Manter a visão | `docs/dissertation_architecture.md` é a fonte das decisões científicas. Comunicação, sensoriamento geométrico, BA e mobilidade têm fronteiras explícitas. |
| Outros consumirão | Parâmetros ficam em NED/INI, mensagens em `.msg`, resultados em sinais/escalares e comandos no `README`. `make check` verifica problemas estruturais simples. |
| Aberto ao futuro | Quantidades, mobilidade, limites físicos, pesos e BA são configuráveis. Extensões futuras devem usar esses pontos de variação sem antecipar funcionalidades. |
| Reutilização | Reutilizam-se modelos do INET e componentes do domínio que já possuem mais de um consumidor. Não se criam interfaces genéricas sem um caso real. |
| Pensar | Mudanças em modelo, métrica ou parâmetro exigem hipótese, impacto esperado, teste determinístico e atualização da rastreabilidade. |

## Responsabilidades

- `SarScenarioManager`: eventos abstratos e associação vítima–drone.
- `DroneApp`: protocolo do alerta e coordenação da decisão de reposicionamento.
- `TeamApp`: posição da equipe, deduplicação e `VictimAck`.
- `AbstractObstacleSensor`: confirmação geométrica, sem inferir a causa da perda.
- `BatAlgorithm`: otimização independente da pilha de rede.
- `BaGaussMarkovMobility`: movimento gradual e retomada da mobilidade normal.
- `analysis/`: interpretação estatística; não altera a simulação.

Uma classe somente deve ser dividida quando houver uma segunda razão concreta
para mudança, teste independente ou reutilização. Tamanho isolado não justifica
uma nova abstração.

## Engenharia científica

### Hipótese e falseabilidade

A hipótese é que o reposicionamento orientado por qualidade de rede pode alterar
a probabilidade de confirmação. O código não presume melhora. Sucesso operacional
é exclusivamente o recebimento de `VictimAck` pelo originador.

### Controle experimental

`Experiment_Control_BaOff` e `Experiment_Proposed_BaOn` devem diferir somente
em `baEnabled`. Obstáculos, mobilidade, tráfego, duração e seeds permanecem
iguais. O processamento rejeita conjuntos BA on/off com seeds diferentes ou
com menos de 30 pares.

### Reprodutibilidade e proveniência

- validações determinísticas usam `-r 0`;
- experimentos usam `seed-set=${repetition}` e 30 repetições;
- NED/INI contêm os parâmetros, sem constantes experimentais dispersas;
- `experiment_manifest.json` registra revisão Git, estado do worktree, arquivos
  analisados e SHA-256 do INI e da geometria de obstáculos;
- resultados publicados devem vir de worktree limpo e revisão identificada.

### Validade das medições

- AppACK é a métrica primária;
- métricas secundárias só permanecem quando explicam entrega, atraso,
  tentativas, qualidade do enlace ou custo de deslocamento;
- valores deriváveis devem ser calculados na análise, sem novos contadores no
  simulador; por exemplo, repetições podem ser inferidas de tentativas e
  alertas únicos;
- detalhes internos do algoritmo só viram métricas quando respondem a uma
  hipótese explícita; não se mede apenas porque o valor está disponível;
- aspectos subjetivos ou aproximados são declarados como premissas e
  limitações, sem criar falsa precisão;
- ACK MAC não equivale a `VictimAck`;
- RSSI multi-hop descreve o último salto observado;
- RSSI estimado de candidato não é apresentado como medição futura;
- degradação de rede e confirmação sensorial são eventos separados;
- correlação com obstáculo não elimina distância, interferência, mobilidade
  ou ausência de rota como causas alternativas.

### Fluxo de qualidade

1. Execute `make check` antes de simular.
2. Execute `make validate` apó mudanças de comportamento ou configuração.
3. Execute os dois experimentos com o mesmo conjunto completo de seeds.
4. Processe os resultados com `python3 analysis/process_results.py`.
5. Arquive CSVs, figuras, manifesto, commit e limitações observadas.

Uma mudança científica está concluída apenas quando implementação,
configuração, teste, métrica e documentação continuam coerentes.
