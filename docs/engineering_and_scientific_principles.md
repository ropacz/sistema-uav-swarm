# Princípios de engenharia do ECHOSAR-Net

## 1. Finalidade e autoridade

Este documento define as regras de engenharia usadas para implementar, medir e
validar o ECHOSAR-Net. As palavras **DEVE**, **NÃO DEVE** e **RECOMENDA-SE**
indicam, respectivamente, requisito, proibição e boa prática.

Em caso de divergência, prevalecem, nesta ordem:

1. o comportamento executável em `src/` e `simulations/omnetpp.ini`;
2. o contrato de medição e os testes em `analysis/`;
3. a rastreabilidade em `docs/requirements_traceability.md`;
4. a descrição arquitetural e metodológica.

Uma divergência não deve ser resolvida apenas mudando o texto: código,
configuração, testes e documentação devem voltar a descrever o mesmo experimento.
Este documento não repete algoritmos ou parâmetros; esses detalhes estão nos
documentos relacionados da seção 9.

## 2. Regras de projeto de software

| Princípio | Regra verificável no ECHOSAR-Net |
|---|---|
| Razão de existir | Toda funcionalidade DEVE medir a entrega confirmada do alerta ou explicar o efeito do reposicionamento. Visão computacional, controle de voo e OcuSync estão fora do escopo. |
| KISS | UDP/AODV realiza o transporte multi-hop. A aplicação NÃO DEVE implementar um segundo mecanismo de flooding. |
| Manter a visão | Comunicação, detecção geométrica, otimização e mobilidade DEVEM manter fronteiras explícitas. |
| Código para outros | Parâmetros experimentais ficam em NED/INI, mensagens em `.msg`, resultados em sinais/escalares e comandos reproduzíveis na documentação. |
| Aberto ao futuro | Quantidades, mobilidade, limites físicos, pesos e BA DEVEM ser configuráveis, sem criar abstrações para extensões ainda inexistentes. |
| Reutilização responsável | RECOMENDA-SE reutilizar componentes do INET e extrair código do domínio somente quando houver mais de um consumidor ou um teste independente. |
| Pensar antes de alterar | Mudanças em modelo, métrica ou parâmetro DEVEM declarar hipótese, efeito esperado, validação e impacto na rastreabilidade. |

Uma classe somente deve ser dividida quando existir uma segunda razão concreta
para mudança, reutilização ou teste independente. Tamanho isolado não justifica
uma nova abstração.

### 2.1 Responsabilidades e direção das dependências

| Componente | Responsabilidade |
|---|---|
| `SarScenarioManager` | Criar eventos abstratos e associar vítima e drone. |
| `DroneApp` | Executar o protocolo de alerta e coordenar a decisão de reposicionamento. |
| `TeamApp` | Publicar a posição da equipe, deduplicar alertas e emitir `VictimAck`. |
| `AbstractObstacleSensor` | Confirmar a geometria do obstáculo, sem atribuir sozinho a causa de uma perda. |
| `BatAlgorithm` | Otimizar uma posição candidata sem depender da pilha de rede. |
| `BaGaussMarkovMobility` | Executar movimento gradual e retomar a mobilidade normal. |
| `analysis/` | Interpretar resultados sem alterar o estado da simulação. |

O fluxo esperado é cenário → aplicação → mensagens/INET → sinais e arquivos de
resultado → análise. Código de análise NÃO DEVE ser usado pelo simulador para
decidir seu comportamento.

## 3. Hipótese e controle experimental

A hipótese falseável é: **o reposicionamento orientado por qualidade estimada da
rede pode alterar a probabilidade de confirmação do alerta**. O código não
presume melhora. O sucesso operacional ocorre somente quando o originador recebe
o `VictimAck` correspondente.

Comparações BA desligado/ligado DEVEM:

- alterar somente `baEnabled` entre os dois grupos;
- usar os mesmos obstáculos, mobilidade, tráfego, duração e conjunto de seeds;
- usar `seed-set=${repetition}` e, no experimento principal, ao menos 30 pares;
- rejeitar grupos com seeds ausentes ou não pareadas;
- identificar claramente qualquer injeção de estresse, como atraso artificial
  de ACK, para que ela não seja confundida com comportamento natural da rede.

Execuções determinísticas, normalmente com `-r 0`, servem para regressão e
diagnóstico. Elas NÃO substituem evidência experimental com múltiplas seeds.

## 4. Contrato das métricas

Cada métrica DEVE declarar unidade, população, numerador, denominador e fonte.
Contadores de camadas diferentes não devem ser combinados sem regra explícita.

| Medida | Fonte e cálculo | Interpretação e limite |
|---|---|---|
| AppACK/PDR primário | Alertas únicos confirmados ÷ alertas únicos gerados, correlacionados pela identidade da mensagem | Mede sucesso fim a fim da aplicação. ACK de MAC NÃO equivale a `VictimAck`. |
| Entrega por tentativa | Tentativas recebidas ÷ tentativas enviadas | Mede eficiência das retransmissões, não sucesso de alertas únicos. |
| Atraso fim a fim | Instante do `VictimAck` menos instante de criação do alerta correspondente | Deve usar relógio e unidade consistentes e excluir pares sem correspondência. |
| PDR de broadcast no PCAP | Recepções observadas ÷ oportunidades de recepção observáveis para os pares capturados | Uma oportunidade não prova que todos os nós estavam no alcance de rádio. |
| Tráfego unicast no PCAP | Pacote de aplicação contado no IP originador; encaminhamentos são tratados por salto | Evita contar o mesmo datagrama multi-hop como várias mensagens da aplicação. |
| RSSI observado | Valor do salto registrado pelo receptor | Em multi-hop descreve o último salto, não todo o caminho. |
| RSSI de candidato | Estimativa usada pela função de aptidão | É previsão do modelo, não medição futura. |

Valores deriváveis DEVEM ser calculados em `analysis/`, evitando contadores
redundantes no simulador. Detalhes internos do algoritmo só devem virar métricas
quando respondem a uma hipótese explícita.

## 5. Identificação e auditoria do tráfego

O tamanho do pacote NÃO DEVE ser a identidade principal de uma mensagem. A
correlação deve usar, nesta ordem:

1. identificador ECHO estável no payload, quando disponível;
2. campos da aplicação, como tipo, origem e número de sequência;
3. tupla de rede e janela temporal apenas como fallback documentado.

O PCAPNG é a evidência primária para o tráfego efetivamente observado nas
interfaces. Os arquivos `.sca` e `.vec` são a fonte para estado interno,
decisões do BA e estatísticas emitidas pelo simulador. O `.elog` é auxiliar para
diagnóstico e NÃO DEVE ser usado isoladamente como resultado científico.

Capturas em nós diferentes podem registrar o mesmo pacote. A análise DEVE
deduplicar ou classificar esses registros como transmissão, recepção ou
encaminhamento antes de calcular totais.

## 6. Múltiplas seeds e inferência estatística

Cada seed é uma unidade experimental e DEVE ter o mesmo peso na comparação
entre configurações. A planilha consolidada deve preservar a linha por seed e
apresentar, no mínimo:

- média e mediana;
- desvio-padrão amostral;
- mínimo e máximo;
- intervalo de confiança de 95% baseado em t de Student;
- número de seeds válidas e pares efetivamente comparados.

O PDR médio por seed e o PDR agregado de todos os pacotes respondem a perguntas
diferentes. Se ambos forem publicados, DEVEM possuir nomes distintos e seus
denominadores devem ser informados.

## 7. Reprodutibilidade e proveniência

Todo resultado publicável DEVE registrar:

- configuração, número da repetição, seed efetiva e duração;
- revisão Git e indicação de worktree limpo ou sujo;
- SHA-256 do INI, geometria de obstáculos e arquivos analisados;
- versões de OMNeT++, INET e scripts de análise;
- limitações ou falhas ocorridas durante a execução.

`experiment_manifest.json` é o registro de proveniência do lote. Resultados são
artefatos gerados, ficam fora do Git e do Graphify e devem ser organizados em:

- `simulations/results/omnetpp/` para `.sca`, `.vec`, `.vci` e `.elog`;
- `simulations/results/pcap/` para `.pcap` e `.pcapng`;
- `simulations/results/eventlogs/` para exportações textuais de eventos;
- `simulations/results/spreadsheets/` para CSV/XLSX e resumos auditáveis.

Excluir resultados locais não substitui o arquivamento externo do conjunto que
sustenta uma tabela, figura ou conclusão publicada.

## 8. Portões de qualidade e definição de pronto

Antes de aceitar uma mudança:

1. execute `make check` para verificações estruturais;
2. execute `make validate` após mudanças de comportamento ou configuração;
3. execute `python3 -m unittest discover -s analysis/tests` após mudanças no
   processamento de PCAP;
4. rode uma seed determinística para inspecionar eventos e regressões;
5. rode os pares completos de seeds quando a mudança afetar uma conclusão;
6. processe escalares com `python3 analysis/process_results.py`;
7. processe capturas com `python3 analysis/pcap_batch_to_spreadsheet.py`;
8. confira manifesto, planilha, logs de erro e denominadores das métricas.

Uma mudança científica está pronta somente quando implementação, configuração,
teste, métrica, proveniência e documentação permanecem coerentes.

## 9. Limites de validade e documentos relacionados

Correlação com obstáculo não elimina distância, interferência, mobilidade ou
ausência de rota como causas alternativas. Confirmação sensorial e degradação de
rede são eventos diferentes e devem permanecer separados na análise.

Consulte:

- `docs/project_architecture_and_validation.md`: arquitetura executável e fluxo
  de validação;
- `docs/dissertation_architecture.md`: decisões metodológicas da dissertação;
- `docs/requirements_traceability.md`: origem e estado dos requisitos;
- `docs/bat_algorithm_and_fitness.md`: algoritmo BA e função de aptidão;
- `docs/obstacle_detection_visual_guide.md`: semântica geométrica dos obstáculos;
- `docs/echosar_wire_format.md`: identidade ECHO e formato auditável no PCAP;
- `analysis/README.md`: uso das ferramentas de análise e estrutura das saídas.
