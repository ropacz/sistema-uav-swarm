# Contratos e fórmulas das métricas

## 1. Princípios de medição

As métricas principais são registradas pelo módulo passivo
[`ExperimentMetrics`](../src/metrics/ExperimentMetrics.cc), inscrito em sinais
emitidos no módulo raiz. Isso permite deduplicar eventos de vários UAVs e equipes
sem acoplar as aplicações ao pós-processamento.

Quatro regras orientam a medição:

1. contar entidades pela identidade correta;
2. calcular a métrica por execução antes de agregar seeds;
3. manter contadores brutos junto das razões derivadas;
4. retornar `NaN` quando o denominador não existe, em vez de fabricar zero.

Notação usada neste documento:

- \(G\): conjunto de `alertId` gerados;
- \(D\): conjunto de `alertId` entregues pelo menos uma vez a uma equipe;
- \(C\): conjunto de `alertId` cujo primeiro ACK válido chegou ao UAV;
- \(X\): conjunto de `alertId` expirados;
- \(M_s\): número de tentativas enviadas;
- \(M_d\): conjunto de `messageId` entregues pelo menos uma vez.

## 2. Alerta, tentativa e ACK

Um `alertId` representa um evento único. Cada retry possui outro `messageId`.
Logo, um alerta pode estar em \(G\) uma vez e gerar várias tentativas. Duplicatas
da mesma tentativa não aumentam \(|M_d|\).

O `TeamApp` mantém conjuntos locais, mas somar `uniqueAlertsReceived` de várias
equipes pode contar o mesmo `alertId` mais de uma vez. A cardinalidade global
\(|D|\) vem de `ExperimentMetrics.alertsDelivered`.

Da mesma forma, quantidade de ACKs enviados não é quantidade de alertas
confirmados. ACKs podem ser repetidos; \(|C|\) conta apenas o primeiro ACK válido
aceito pelo UAV.

## 3. Entrega fim a fim

### 3.1 PDR de alertas

\[
PDR_{alert}=\frac{|D|}{|G|}.
\]

- escalar: `pdr`;
- numerador bruto: `alertsDelivered`;
- denominador bruto: `alertsGenerated`;
- unidade: razão no intervalo \([0,1]\);
- sem alertas gerados: `NaN`.

### 3.2 Taxa de perda de alertas

\[
Loss_{alert}=1-PDR_{alert}=\frac{|G|-|D|}{|G|}.
\]

- escalar: `packetLossRate`;
- sem alertas gerados: `NaN`.

O nome histórico contém “packet”, mas o contrato é perda de alertas únicos da
aplicação. Não é PER físico nem descarte MAC.

### 3.3 Taxa de confirmação

\[
ConfirmationRate=\frac{|C|}{|G|}.
\]

- escalar: `confirmationRate`;
- numerador: `alertsConfirmed`;
- sem alertas gerados: `NaN`.

PDR e confirmação não são iguais. Se a equipe recebe o alerta e o ACK se perde,
o alerta pertence a \(D\), mas não a \(C\).

### 3.4 Entrega por tentativa

\[
AttemptDeliveryRate=\frac{|M_d|}{M_s},
\qquad
AttemptLossRate=1-AttemptDeliveryRate.
\]

- escalares: `attemptDeliveryRate`, `attemptLossRate`;
- contadores: `alertAttemptsSent`, `alertAttemptsDelivered`,
  `alertAttemptsLost`;
- sem tentativas: `NaN`.

Essa métrica mede tentativas únicas da aplicação. Ela não mede transmissões e
retransmissões do 802.11.

### 3.5 Retransmissões da aplicação

Para cada alerta \(a\), com \(n_a\) tentativas:

\[
ApplicationRetries=\sum_{a\in G}\max(0,n_a-1).
\]

O primeiro envio não é retry. O valor é registrado em `applicationRetries`.

## 4. Tempos

### 4.1 Atraso fim a fim do alerta

Para cada alerta entregue:

\[
Delay_a=t_{firstDelivery,a}-t_{generation,a}.
\]

\[
\overline{Delay}=
\frac{\texttt{deliveryDelaySum}}{\texttt{deliveryDelayCount}}.
\]

Somente a primeira entrega global do `alertId` participa. Alertas não entregues
não recebem atraso finito e não entram no denominador; por isso PDR deve ser
relatado junto ao atraso, evitando viés de sobrevivência oculto.

### 4.2 Atraso de tentativa

Para cada `messageId` entregue pela primeira vez:

\[
AttemptDelay_m=t_{receive,m}-t_{transmit,m}.
\]

\[
\overline{AttemptDelay}=
\frac{\texttt{attemptDeliveryDelaySum}}
{\texttt{attemptDeliveryDelayCount}}.
\]

### 4.3 RTT da aplicação

Para o `messageId` confirmado:

\[
RTT_a=t_{ACK,a}-t_{send,m(a)}.
\]

`rttSum/rttCount` usa uma amostra por alerta confirmado, correspondente ao
primeiro ACK válido. O tempo inclui o caminho do alerta e do ACK, além de filas,
acesso ao meio e roteamento.

## 5. Expiração e conservação

`alertsExpired` conta `alertId` encerrados por TTL ou limite de tentativas sem
ACK. Como a entrega não encerra o alerta se o ACK se perde, um alerta pode estar
em \(D\cap X\). Um alerta confirmado é removido do estado pendente e não deve
expirar depois.

Ao final, o coletor verifica:

\[
|D|\le|G|,
\qquad |C|\le|D|,
\qquad |C|+|X|\le|G|,
\]

\[
|M_d|\le M_s,
\qquad
N_{recovery}\le N_{repositionStarted}.
\]

Uma violação encerra a simulação com erro. Essas desigualdades detectam
duplicação e ordem impossível de eventos, mas não provam sozinhas que todas as
semânticas estejam corretas.

## 6. PDR das atualizações de posição

`linkWindowPdr` é um indicador local usado pelo gatilho do BA. Ele não é o PDR
fim a fim dos alertas.

A janela contém recepções no intervalo semiaberto:

\[
(t-W,t],
\]

onde \(W=\texttt{linkWindow}\) e o período esperado é
\(\delta=\texttt{expectedPositionUpdateInterval}\).

Se não há amostras, o indicador vale zero. Caso contrário, sejam \(N_{rx}\) as
amostras presentes, \(t_0\) o início da observação local e \(s_{first},s_{last}\)
as sequências nas bordas. O código calcula:

\[
a=\min(W,\max(0,t-t_0)),
\]

\[
N_{capacity}=\max\left(1,\left\lceil\frac{W}{\delta}\right\rceil\right),
\]

\[
N_{time}=\min\left(N_{capacity},
1+\left\lfloor\frac{a}{\delta}\right\rfloor\right),
\]

\[
N_{seq}=s_{last}-s_{first}+1,
\]

\[
PDR_{window}=\operatorname{clip}\left(
\frac{N_{rx}}{\max(N_{time},N_{seq})},0,1\right).
\]

O denominador temporal faz uma equipe recém-descoberta começar em uma observação
sobre uma esperada, sem penalizar beacons anteriores à descoberta. O vão de
sequência retém perdas internas; o tempo detecta silêncio depois da última
recepção.

Esse estimador tem propósito de controle local. Não deve ser agregado como se
cada janela fosse uma réplica experimental independente.

## 7. RSSI e potência

Para uma indicação de potência recebida \(P_W\), a aplicação converte:

\[
P_{mW}=\frac{P_W}{10^{-3}},
\qquad
RSSI_{dBm}=10\log_{10}(P_{mW}).
\]

No gatilho local, `DroneApp` usa a média aritmética dos RSSIs disponíveis na
janela:

\[
\overline{RSSI}_{window}=\frac{1}{N}\sum_j RSSI_j.
\]

No diagnóstico global por execução, o pós-processamento faz a agregação física
correta de potência: soma e conta `positionUpdatePowerMilliwatt` sobre os
módulos, calcula a média linear e só então converte:

\[
RSSI_{run}=10\log_{10}\left(\frac{1}{N}\sum_jP_{mW,j}\right).
\]

Logo, o RSSI exibido como diagnóstico do run não precisa ser numericamente igual
à média usada por um gatilho local específico. `rssiSamplesAvailable` e
`rssiSamplesMissing` tornam explícita a cobertura da indicação de potência.

## 8. Métricas de reposicionamento

Cada movimento iniciado tem um ID de ciclo. As categorias terminais são
mutuamente exclusivas por ciclo:

- `repositionsValidated`: ACK da tentativa enviada depois da chegada e marcada
  para o mesmo ciclo;
- `repositionsRecoveredDuringMovement`: ACK de tentativa anterior chegou durante
  o trajeto;
- `repositionsRecoveredAfterArrival`: ACK de tentativa anterior chegou depois da
  chegada, antes de validar a nova posição;
- `repositionsExpired`: alerta terminou sem recuperação.

Falhas `noFeasibleSolution` e `redundantCandidate` ocorrem antes de existir
movimento e são contadas em `repositionsFailedBeforeMovement`, não em
`repositionsStarted`.

Defina:

\[
N_{operational}=N_{validated}+N_{during}+N_{after}.
\]

Então:

\[
OperationalRecoveryRate=\frac{N_{operational}}{N_{started}},
\]

\[
RepositionValidationRate=\frac{N_{validated}}{N_{started}}.
\]

Sem movimentos iniciados, ambas são `NaN`.

`successfulRepositions` e `repositionSuccessRate` são aliases históricos de
semântica operacional. Para texto novo, use
`operationallySuccessfulRepositions` e `operationalRepositionRecoveryRate`.

### 8.1 Tempo de recuperação

Para todo ciclo operacionalmente recuperado:

\[
T_{operational}=t_{ACK}-t_{repositionStart}.
\]

Os escalares normativos são `operationalRecoveryTimeSum/Count`.
`recoveryTimeSum/Count` são aliases históricos dessa mesma semântica.

Para validação causal:

\[
T_{validated}=t_{ACK(validationMessageId)}-t_{repositionStart}.
\]

Os escalares são `validatedRecoveryTimeSum/Count`. ACKs antigos nunca entram
nesse denominador nem nas comparações locais pré/pós.

### 8.2 Distância

- `commandedRepositionDistanceSum`: soma da distância até o candidato quando o
  movimento começa;
- `repositionDistanceSum`: soma da distância realmente percorrida até ACK,
  chegada ou expiração;
- `repositionDistanceCount`: número de ciclos cuja distância real foi fechada.

Um ACK durante o trajeto pode fazer a distância real ser menor que a comandada.

## 9. Métricas diagnósticas por camada

[`network_metrics.py`](../analysis/core/network_metrics.py) extrai métricas que
explicam mecanismos, mas não substituem os desfechos da aplicação.

### MAC

\[
MACRetryLimit\%=100\frac{N_{drop,retryLimit}}{N_{framesSent}},
\]

\[
MACCorrupt\%=100\frac{N_{corrupt}}
{N_{received}+N_{corrupt}}.
\]

Também são reportados overflow de fila, duplicatas e quebra de enlace. O parser
filtra o módulo MAC exato para não somar o mesmo contador publicado também pelo
DCF.

### IP, UDP e saltos

São contados descartes por ausência de rota, hop limit e resolução de endereço,
além de pacotes/bytes UDP. Para um alerta entregue:

\[
HopCount=TTL_{initial}-TTL_{received}.
\]

No contrato do projeto, zero significa entrega direta, um significa um nó
intermediário. A média usa soma global dividida pela contagem global, não média
simples das médias de módulos.

Descartes de MAC/IP podem coexistir com entrega final após retry ou nova rota.
Por isso `MAC drops / alertas gerados` não é PDR fim a fim.

## 10. Agregação entre execuções

Para uma métrica \(Y_i\) calculada em cada seed:

\[
\bar Y=\frac{1}{n}\sum_iY_i,
\qquad
s_Y=\sqrt{\frac{\sum_i(Y_i-\bar Y)^2}{n-1}}.
\]

No contraste principal, a inferência usa \(d_i=Y_{on,i}-Y_{off,i}\), conforme
[`scientific_protocol.md`](scientific_protocol.md#7-estimação-e-intervalo-de-confiança).

`NaN` significa “métrica indefinida para esse run”, normalmente por denominador
zero. A análise remove `NaN` apenas daquela métrica e informa `paired_n`; não deve
converter indefinição em desempenho zero.

## 11. Fonte adequada para cada resultado

| Fonte | Uso |
| --- | --- |
| `.sca` / `ExperimentMetrics` | contadores e resultados por execução; fonte principal |
| `.vec` | evolução temporal selecionada; fonte complementar |
| Python | composição, auditoria de parâmetros, efeitos e IC |
| PCAPNG | inspeção de pacotes, rotas e casos específicos |
| visualizadores | inspeção visual da dinâmica |
| event log | depuração detalhada de eventos raros |

Contar linhas de PCAPNG não produz PDR de alertas: o mesmo conteúdo pode aparecer
em interfaces, saltos e retransmissões diferentes.
