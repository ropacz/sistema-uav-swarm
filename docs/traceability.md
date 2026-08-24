# Rastreabilidade e validações

## 1. Objetivo

Este documento liga cada afirmação importante ao código que a implementa, ao
dado que a torna observável e à validação automatizada disponível. A cadeia de
evidência é:

```text
requisito → implementação → sinal/escalar → pós-processamento → validação
```

Nenhuma etapa isolada prova o projeto inteiro. Compilação prova compatibilidade;
smoke test prova integração no cenário forçado; experimento pareado estima
efeito no espaço experimental definido.

## 2. Matriz de rastreabilidade funcional

| Contrato | Implementação principal | Evidência observável | Validação/análise |
| --- | --- | --- | --- |
| IDs de drone/equipe únicos entre nós e IDs de vítima únicos entre vítimas | `SarScenarioManager.cc` | erro de runtime em duplicidade | inicialização de qualquer cenário |
| Uma geração por `alertId` | `DroneApp::handleAssignment` | `victimAlertGenerated`, `alertsGenerated` | invariantes de `ExperimentMetrics` |
| Descoberta apenas por anúncio | `TeamApp::sendPositionUpdate`, `DroneApp::handlePositionUpdate` | `teamEntriesDiscovered/Expired` | `validate_network_discovery.py` |
| Duplicata não renova silêncio | `DroneApp::handlePositionUpdate` | PDR/silêncio do enlace | testes determinísticos de cenário |
| Retry recebe novo `messageId` | `DroneApp::sendAttempt` | `alertAttemptsSent`, `applicationRetries` | coleta central e smoke test |
| Entrega deduplicada por tentativa e alerta | `TeamApp::handleVictimAlert`, `ExperimentMetrics` | `alertsDelivered`, `alertAttemptsDelivered` | invariantes e testes de análise |
| ACK autenticado pelo destino histórico | `DroneApp::handleVictimAck` | `alertsConfirmed`, `rttSum/Count` | smoke test e invariantes |
| PDR de alerta separado de confirmação | `ExperimentMetrics::finish` | `pdr`, `confirmationRate` | `test_network_metrics.py` |
| Gatilho após prazo sem ACK | `DroneApp::performMaintenance` | `degradationIndications` | BA smoke test |
| Sensor distingue motivos | `AbstractObstacleSensor`, `DroneApp::tryReposition` | confirmações/rejeições por categoria | BA smoke test + relatório diagnóstico |
| BA avalia somente candidatos viáveis | `BatAlgorithm`, `RepositionFitness` | ativação, falha, movimento | BA smoke test |
| Movimento sem teletransporte | `BaGaussMarkovMobility::moveTo` | distância comandada e real | BA smoke test |
| Ciclo de movimento tem ID próprio | `PendingVictimAlert`, `RepositionController` | eventos `victimRepositionEvent` | invariantes centrais |
| ACK antigo não vira validação causal | `DroneApp::handleVictimAck` | categorias operacional/validada | `validate_ba_smoke_test.py` |
| Métricas globais não duplicam equipes | `ExperimentMetrics` | conjuntos de `alertId`/`messageId` | testes de análise |
| Média de potência é feita em escala linear | `DroneApp`, `network_metrics.py` | `positionUpdatePowerMilliwatt` | BA smoke test + testes de análise |
| Braços diferem apenas pelo tratamento | `report_main_experiment.py` | parâmetros gravados no `.sca` | falha automática por drift |
| Unidade experimental é a seed | scripts em `analysis/` | uma linha por run/seed | testes de relatório |
| Tratamento precisa ser exercitado | `require_informative_treatment` | quatro métricas de mecanismo | falha automática do relatório |

## 3. Contrato dos sinais centrais

`ExperimentMetrics` se inscreve no módulo raiz e recebe `AlertMetricEvent`.
Eventos sem `alertId` são rejeitados. O significado de cada sinal é:

| Sinal | Emissor | Identidade necessária | Efeito no coletor |
| --- | --- | --- | --- |
| `victimAlertGenerated` | `DroneApp` | `alertId` | inclui em \(G\) e guarda geração |
| `victimAlertAttemptSent` | `DroneApp` | `alertId`, `messageId` | incrementa tentativa e guarda envio |
| `victimAlertDelivered` | `TeamApp` | `alertId`, `messageId` | inclui em \(D\)/\(M_d\) e soma atrasos |
| `victimAlertConfirmed` | `DroneApp` | `alertId`, `messageId` | inclui em \(C\) e soma RTT |
| `victimAlertExpired` | `DroneApp` | `alertId` | inclui em \(X\) |
| `victimDegradationIndicated` | `DroneApp` | `alertId` | conta indicação |
| `victimSensorEvaluated` | `DroneApp` | `alertId`, categoria | separa confirmação e motivos |
| `victimBaActivated` | `DroneApp` | `alertId` | conta ativação |
| `victimRepositionEvent` | `DroneApp` | `alertId`; ID de ciclo quando iniciado | fecha estados, tempos e distâncias |

As fórmulas derivadas desses eventos estão em [`metrics.md`](metrics.md).

## 4. Configuração e proveniência

[`simulations/omnetpp.ini`](../simulations/omnetpp.ini) é o ponto de entrada e
inclui arquivos por domínio:

- [`professor-common.ini`](../simulations/professor-common.ini): modelo físico,
  protocolo e parâmetros compartilhados;
- [`main-experiment.ini`](../simulations/main-experiment.ini): contraste principal;
- `scenario-1-*.ini`: robustez e referências de cenário;
- [`ba-smoke-test.ini`](../simulations/ba-smoke-test.ini): integração forçada;
- [`network-discovery-validation.ini`](../simulations/network-discovery-validation.ini):
  semântica de descoberta;
- `professor-scaling-test.ini`: sonda exploratória.

O `.sca` registra atributos do run e parâmetros efetivos. A análise lê esses
valores para verificar pareamento e configuração. A documentação não replica os
valores, evitando divergência silenciosa.

## 5. Níveis de validação

### 5.1 Validação estática

`make check` verifica sintaxe do launcher, compilação dos módulos Python,
imports e ausência de nomes antigos de experimento na documentação ativa.

Essa etapa encontra erros de sintaxe e organização; não executa o modelo.

### 5.2 Testes unitários da análise

`make analysis-tests` exercita parser `.sca`, agregações, pareamento, detecção de
drift, gates do tratamento e auditoria PCAP. Esses testes validam o cálculo em
Python com dados controlados; não validam o comportamento do INET.

### 5.3 Compilação C++/NED

`make makefiles` regenera o makefile do OMNeT++ quando necessário e `make`
compila o modelo. A compilação detecta interfaces incompatíveis e erros de tipo,
mas não garante correção temporal do protocolo.

### 5.4 BA smoke test

`make ba-smoke-test` executa um cenário determinístico com obstrução idealizada e
verifica a cadeia:

```text
degradação → sensor → BA → movimento → ACK
```

O validador exige geração, confirmação, ausência de expiração, indicação de
degradação, confirmação sensorial, ativação, deslocamento real, RSSI disponível
e consistência entre métricas locais e centrais.

Há uma verificação causal deliberada: no traço esperado, o ACK pode pertencer a
uma tentativa anterior e chegar após o movimento. Nesse caso a recuperação deve
ser classificada como operacional, `repositionsValidated` deve permanecer zero
e as somas pré/pós causais devem permanecer vazias. Assim, o teste comprova que
o software não promove um ACK tardio a evidência do candidato BA.

Esse cenário usa bloqueio ideal proposital e não é evidência científica.

### 5.5 Validação de descoberta

`make network-discovery-validation` executa dois casos:

- entrega direta, na qual o ACK chega sem intermediário;
- equipe remota separada por relay, na qual o relay ouve o broadcast, mas o UAV
  de origem não descobre a equipe sem encaminhamento de descoberta.

Isso confirma a fronteira atual do protocolo: AODV pode encaminhar unicasts, mas
os broadcasts de descoberta não constituem um serviço de descoberta multi-hop.

### 5.6 Validação do experimento principal

`make experiment` executa ambos os braços e o relatório principal. O relatório
falha quando:

- um braço não tem resultados;
- uma seed está ausente ou duplicada;
- os conjuntos de seeds diferem;
- qualquer parâmetro além de `baEnabled` difere no par;
- o tratamento não apresenta degradação, sensor confirmado, ativação e movimento.

Passar por esses gates torna a execução analisável. Não garante, por si só,
efeito estatístico nem validade externa.

## 6. Procedimento reproduzível

Preparação:

```bash
cp .env.example .env
make makefiles
make
make analysis-tests
```

Validação de integração:

```bash
make ba-smoke-test
make network-discovery-validation
```

Experimento principal e análise:

```bash
make experiment
```

Robustez e extensões, sempre relatadas separadamente:

```bash
make robustness-experiment
make optional-multihop
make optional-scaling
make optional-pcap
```

Quando as ferramentas OMNeT++ não estiverem no `PATH`, os mesmos comandos devem
ser executados no ambiente `opp_env` configurado pelo projeto. `run.sh` é a
entrada padrão para uma configuração/seed específica.

## 7. Artefatos e responsabilidade de cada arquivo

| Local | Conteúdo | Versionar? |
| --- | --- | --- |
| `simulations/results/omnetpp/*.sca` | escalares, atributos e parâmetros do run | Não; reproduzível |
| `simulations/results/omnetpp/*.vec` | amostras temporais habilitadas | Não; reproduzível |
| `simulations/results/omnetpp/*.vci` | índice do `.vec` | Não; regenerável |
| `simulations/results/pcap/*.pcapng` | captura opcional de pacotes | Não; diagnóstico |
| `simulations/results/spreadsheets/` | exportações da auditoria PCAP | Não; derivado |
| `analysis/figures/main_experiment_runs.csv` | uma linha de métricas por run | Não; derivado |
| `analysis/figures/main_experiment_paired_effects.csv` | efeitos BA On − BA Off por seed | Não; derivado |
| `analysis/figures/main_experiment_summary.csv` | média e IC dos efeitos | Não; derivado |
| `analysis/figures/main_experiment_ba_mechanism.csv` | exposição e resposta do BA | Não; derivado |

`.sca` é a fonte quantitativa principal. `.vec` deve ser habilitado apenas para
sinais temporais necessários. `.vci` não contém nova evidência; ele acelera
acesso ao vetor. PCAPNG e visualização são ferramentas de auditoria.

## 8. Estado e limitações conhecidas

- O smoke test valida a cadeia técnica, mas sua recuperação esperada pode ser
  operacional e não causal. Ele não substitui runs com tentativas de validação
  pós-chegada.
- O experimento principal possui gate explícito para evitar concluir a partir de
  um tratamento nunca exercitado. Se um piloto não alcançar esse gate, o cenário
  precisa de calibração prospectiva antes da campanha final.
- A descoberta multi-hop não está implementada na aplicação; o caso de relay
  existe justamente para impedir interpretação contrária.
- Escalares locais em `DroneApp` e `TeamApp` permanecem úteis para depuração e
  compatibilidade. Resultados globais devem preferir `ExperimentMetrics`.
- Aliases históricos de sucesso/tempo de reposicionamento têm semântica
  operacional; análises novas devem usar os nomes explícitos documentados.
- A função `ci95` usa aproximação do quantil t. Para amostras muito pequenas ou
  decisões regulatórias, valide numericamente com biblioteca estatística
  independente e reporte o método.
- `randomInSphere()` aplica uma escala radial adicional a um ponto que já foi
  amostrado na bola, concentrando candidatos perto do centro. A heurística roda,
  mas sua distribuição não é uniforme em volume e precisa ser tratada como
  desvio conhecido do BA pretendido.

## 9. Checklist antes de publicar resultados

- [ ] Código e configurações estão identificados por commit.
- [ ] Arquivos `.ini` foram congelados antes da análise final.
- [ ] `make analysis-tests` passou.
- [ ] Validações de BA e descoberta passaram.
- [ ] Seeds BA Off/On estão completas e pareadas.
- [ ] Não há drift de parâmetros além de `baEnabled`.
- [ ] O tratamento atingiu todos os gates de informatividade.
- [ ] PDR, confirmação e atraso usam contadores globais deduplicados.
- [ ] `NaN` e denominadores efetivos foram relatados.
- [ ] Recuperação operacional não foi chamada de validação causal.
- [ ] Métricas de MAC/IP/PCAP foram tratadas como diagnóstico.
- [ ] Robustez, multihop, escala e smoke test estão separados do resultado principal.
- [ ] Limitações do sensor, previsão, fitness e generalização foram declaradas.

## 10. Manutenção da rastreabilidade

Ao alterar comportamento ou métrica:

1. atualizar código e configuração no domínio correto;
2. atualizar o contrato em `model_and_assumptions.md` ou `metrics.md`;
3. atualizar a linha correspondente desta matriz;
4. adicionar ou adaptar a validação proporcional ao risco;
5. executar os testes relevantes;
6. executar `graphify update .` para atualizar o grafo do projeto.

Esse procedimento mantém documentação, observabilidade e evidência alinhadas.
