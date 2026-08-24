# Contratos e fórmulas das métricas

## 1. Unidade de contagem

Um `alertId` representa um alerta lógico. Um alerta pode originar vários
`messageId`, um por tentativa. O coletor central deduplica entrega e confirmação
por `alertId`; portanto retransmissões e recepções por equipes diferentes não
inflam o PDR.

Os arquivos `.sca` são a fonte normativa. O PCAP e os escalares do INET por
camada servem apenas a diagnóstico.

## 2. Contadores centrais

`ExperimentMetrics` registra exatamente:

| Escalar | Contrato |
| --- | --- |
| `alertsGenerated` | alertas lógicos criados |
| `alertsDelivered` | alertas recebidos por ao menos uma equipe |
| `alertsConfirmed` | alertas com ACK válido aceito pelo UAV |
| `alertsExpired` | alertas encerrados sem ACK |
| `alertAttemptsSent` | transmissões da aplicação |
| `applicationRetries` | tentativas além da primeira, somadas por alerta |
| `deliveryDelaySum` | soma dos atrasos até a primeira entrega |
| `deliveryDelayCount` | alertas entregues incluídos na soma |
| `pdr` | razão entregue/gerado, também recalculada na análise |
| `confirmationRate` | razão confirmado/gerado, também recalculada |
| `repositionTriggers` | alertas que atingiram o limiar sem ACK |
| `obstaclesDetected` | avaliações binárias positivas do sensor |
| `baActivations` | execuções iniciadas do otimizador |
| `repositionsStarted` | movimentos físicos iniciados |
| `repositionsCompleted` | movimentos que chegaram ao candidato |
| `repositionDistanceSum` | distância física total efetivamente percorrida |
| `repositionDistanceCount` | movimentos com distância registrada |

Não há aliases históricos no contrato novo. Um arquivo antigo que não possua um
escalar obrigatório não pode alimentar o relatório confirmatório.

## 3. Desfecho primário

Se (G) é o conjunto de `alertId` gerados e (D\subseteq G) o conjunto
entregue:

\[
PDR=\frac{|D|}{|G|}.
\]

Esse PDR responde se o alerta chegou a alguma equipe. Ele não é calculado a
partir de descartes MAC, linhas de PCAP ou ACKs.

## 4. Desfechos secundários

Para confirmados (C), retransmissões (R), alertas entregues (N_D) e atraso
fim a fim (d_i=t_{primeira\ entrega,i}-t_{geração,i}):

\[
ConfirmationRate=\frac{|C|}{|G|},
\]

\[
RetriesPerAlert=\frac{R}{|G|},
\]

\[
MeanDeliveryDelay=\frac{\sum_{i=1}^{N_D}d_i}{N_D}.
\]

PDR e confirmação não são equivalentes: um alerta pode ser entregue e seu ACK
ser perdido. O atraso é condicionado à entrega e deve sempre ser apresentado
junto ao PDR.

Razões cujo denominador é zero são `NaN`. Não se atribui zero a uma quantidade
que não foi observável.

## 5. Diagnóstico de exposição

A cadeia esperada obedece:

\[
repositionsCompleted\le repositionsStarted\le baActivations,
\]

\[
obstaclesDetected\le repositionTriggers.
\]

Esses contadores descrevem quanto o braço BA On foi efetivamente exposto à
intervenção. Eles não são critérios para excluir seeds e não substituem o PDR.
Uma ativação sem movimento indica ausência de candidato viável ou candidato
redundante, observável em log de depuração.

## 6. Invariantes fim a fim

Para (G,D,C,X) correspondendo a gerados, entregues, confirmados e expirados:

\[
D\le G,\qquad C\le D,\qquad C+X\le G.
\]

`ExperimentMetrics::finish()` encerra a execução com erro se essas relações ou
as relações de exposição forem violadas. Contadores brutos são preservados para
auditar qualquer razão calculada em Python.

## 7. Agregação entre execuções

Cada seed é uma observação. Para o contraste pareado:

\[
d_i=Y_{On,i}-Y_{Off,i},
\qquad
\bar d=\frac{1}{n}\sum_i d_i.
\]

O relatório fornece média, desvio-padrão e IC95% de (d_i) com t de Student.
Não se agrupam todos os alertas de todas as seeds em uma única proporção, pois
isso daria pesos diferentes às execuções.

## 8. Diagnósticos opcionais

`analysis/core/network_metrics.py` pode resumir quadros, descartes MAC/IP e UDP
para investigar mecanismos. `analysis/pcap/` audita pacotes específicos. Esses
artefatos respondem “por que ocorreu?”; os escalares centrais respondem “a
política melhorou a entrega?”.

Vetores ficam desabilitados por padrão. Devem ser habilitados somente para uma
pergunta temporal explícita, evitando arquivos grandes sem uso analítico.
