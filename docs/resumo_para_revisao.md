# Resumo do funcionamento e das regras

Este documento reúne anotações diretas para revisar o ECHOSAR-Net. Ele não
substitui o protocolo científico, o modelo ou o contrato das métricas. Em caso
de divergência, devem ser consultados `scientific_protocol.md`,
`model_and_assumptions.md`, `metrics.md` e `traceability.md`.

Os valores numéricos pertencem exclusivamente aos arquivos em `simulations/`.
Aqui são registrados somente significados, relações e regras.

## 1. Objetivo

Avaliar se a habilitação de uma política autônoma de reposicionamento baseada no
algoritmo de morcegos (*Bat Algorithm* — BA) melhora a entrega fim a fim de
alertas de vítimas entre veículos aéreos não tripulados (VANT) e equipes de
resgate.

Pergunta principal:

> Habilitar a política de reposicionamento baseada no BA melhora o PDR fim a fim
> dos alertas de vítimas em comparação com a mesma operação sem
> reposicionamento?

O projeto avalia o **efeito de habilitar a política completa**. Ele não tenta
provar que cada movimento individual causou a recuperação de um alerta.

## 2. Escopo

O sistema modela:

- VANT móveis em ambiente tridimensional;
- equipes móveis em solo;
- vítimas estáticas atribuídas a VANT;
- obstáculos físicos estáticos;
- comunicação sem fio IEEE 802.11;
- descoberta direta das equipes;
- alertas UDP, retransmissões e confirmações;
- detecção geométrica de obstáculo;
- reposicionamento calculado pelo BA.

Não fazem parte do desfecho científico:

- visão computacional para detectar vítimas;
- obstáculos dinâmicos;
- consumo de energia ou bateria como métrica;
- controle de voo de baixo nível;
- decisão humana ou sucesso clínico do resgate;
- comunicação multissalto;
- análise PCAP ou métricas detalhadas por camada.

O AODV permanece inalterado na pilha do INET. O experimento utiliza comunicação
direta e não avalia uma modificação do protocolo de roteamento.

## 3. Componentes

| Componente | Responsabilidade |
| --- | --- |
| `SarScenarioManager` | atribuir uma vítima a um VANT |
| `StaticVictim` | representar vítima, posição e momento de detecção |
| `TeamApp` | divulgar posição, receber alerta e enviar ACK |
| `DroneApp` | criar alerta, retransmitir, validar ACK e decidir reposicionamento |
| `AbstractObstacleSensor` | verificar geometricamente a presença de obstáculo |
| `BatAlgorithm` | procurar uma posição candidata |
| `RepositionFitness` | calcular custo e viabilidade do candidato |
| `BaGaussMarkovMobility` | executar o movimento e retomar a mobilidade normal |
| `ExperimentMetrics` | deduplicar eventos e registrar métricas globais |

## 4. Fluxo principal

```text
equipe divulga posição
    -> VANT guarda a última posição válida
    -> vítima é atribuída ao VANT
    -> alerta lógico é criado
    -> primeira tentativa é enviada
    -> ACK válido chegou?
         sim: confirmar e encerrar
         não: aguardar timeout e retransmitir
    -> limiar de tentativas sem ACK foi atingido?
         não: continuar o ciclo
         sim: consultar o sensor uma única vez
    -> obstáculo foi detectado?
         não: não reposicionar
         sim e BA Off: registrar exposição, mas não mover
         sim e BA On: executar o BA
    -> existe candidato viável e diferente da posição atual?
         não: não mover
         sim: deslocar o VANT
    -> ao chegar, enviar imediatamente uma nova tentativa
    -> ACK válido ou expiração do alerta
```

## 5. Descoberta das equipes

- Cada equipe transmite periodicamente sua identidade, posição e sequência.
- O VANT usa o endereço de origem do pacote, não um endereço declarado dentro da
  mensagem.
- Atualizações duplicadas ou fora de ordem não renovam a entrada.
- Uma entrada sem atualização expira conforme `teamEntryLifetime`.
- O destino de uma tentativa é a equipe conhecida mais próxima do VANT.
- Não há predição de velocidade ou extrapolação da posição da equipe.

## 6. Identidades

- `victimId`: identifica a vítima.
- `alertId`: identifica o alerta lógico e deduplica resultados fim a fim.
- `messageId`: identifica uma tentativa específica do alerta.

Uma retransmissão cria outro `messageId`, mas preserva o mesmo `alertId`.
Portanto, várias tentativas ou recepções não podem aumentar artificialmente o
número de alertas entregues.

## 7. Degradação e obstáculo

No modelo, a suspeita operacional de degradação é:

\[
N_{tentativas\ sem\ ACK}\ge
\texttt{repositionAfterUnackedAttempts}.
\]

Regras:

- o gatilho usa ausência de ACK válido;
- RSSI, PDR temporal e silêncio não participam do gatilho;
- deve valer `repositionAfterUnackedAttempts < maxAttempts`;
- essa desigualdade reserva uma tentativa posterior ao movimento;
- a decisão de verificar reposicionamento ocorre no máximo uma vez por alerta.

O timeout não prova que um obstáculo causou a falha. Depois do gatilho, o sensor
verifica separadamente se existe uma superfície física observável na direção da
última posição conhecida da equipe. O resultado científico é binário:
`detected` ou `notDetected`.

Interpretação correta:

> Após tentativas sem confirmação, o VANT verifica se há um obstáculo geométrico
> observável e, quando a política está habilitada, pode reposicionar-se.

## 8. Regras do BA e do movimento

O BA é executado somente quando:

```text
gatilho atingido
AND equipe ainda conhecida
AND controlador de movimento livre
AND obstáculo detectado
AND baEnabled = true
```

A função de aptidão combina:

\[
J(x)=w_{link}C_{link}(x)+w_{obstacle}C_{obstacle}(x)
     +w_{move}C_{move}(x).
\]

Um candidato só é aceito se respeitar área, altitude, distância máxima,
segurança em relação ao obstáculo, linha de visada e tempo de voo restante.

Durante o movimento:

- o alerta proprietário não envia tentativas periódicas;
- o deslocamento é gradual, não um teletransporte;
- ao chegar, o VANT registra conclusão e distância;
- a nova tentativa é enviada antes de retomar Gauss--Markov;
- cada alerta pode comandar no máximo um reposicionamento;
- um VANT executa no máximo um reposicionamento simultâneo.

## 9. Entrega e confirmação

Entrega e confirmação são eventos diferentes:

```text
alerta chegou à equipe + ACK foi perdido
    -> alerta entregue
    -> alerta não confirmado
```

Um ACK somente é aceito quando são coerentes:

- `alertId`;
- `victimId`;
- VANT de origem;
- `messageId` recebido;
- equipe associada à tentativa;
- endereço de origem associado à tentativa.

O histórico por tentativa permanece válido mesmo que a equipe expire da tabela
de descoberta.

## 10. Métricas

Para uma execução, sejam `G` alertas gerados, `D` entregues, `C` confirmados e
`R` retransmissões da aplicação.

Desfecho primário:

\[
PDR=\frac{D}{G}.
\]

Taxa de perda derivada:

\[
LossRate=1-PDR.
\]

Desfechos secundários:

\[
ConfirmationRate=\frac{C}{G},
\qquad
RetriesPerAlert=\frac{R}{G}.
\]

\[
MeanDeliveryDelay=
\frac{\sum d_i}{N_D}.
\]

Regras de medição:

- a unidade de contagem fim a fim é `alertId`;
- a unidade experimental é uma execução completa;
- pacotes ou alertas dentro do mesmo run não são réplicas independentes;
- razões com denominador zero são indefinidas (`NaN`), não zero;
- atraso médio é condicionado à entrega e deve ser interpretado junto ao PDR;
- contadores brutos são preservados para auditoria.

Diagnósticos de exposição:

```text
repositionTriggers
obstaclesDetected
baActivations
repositionsStarted
repositionsCompleted
repositionDistanceSum
```

Invariantes principais:

\[
D\le G,
\qquad C\le D,
\qquad C+X\le G,
\]

\[
repositionsCompleted\le repositionsStarted\le baActivations,
\]

\[
obstaclesDetected\le repositionTriggers.
\]

## 11. Experimento científico

O contraste principal compara:

- `MainExperiment_BaOff`: controle sem reposicionamento;
- `MainExperiment_BaOn`: tratamento com reposicionamento habilitado.

Regras:

- os braços usam as mesmas sementes pseudoaleatórias;
- somente `baEnabled` pode diferir entre os braços;
- cada seed recebe o mesmo peso;
- nenhuma seed válida é removida por não ativar o BA;
- ausência de ativação é diagnóstico de exposição, não erro experimental;
- o efeito é calculado como BA On menos BA Off para cada seed;
- a média das diferenças é acompanhada por IC de 95% com distribuição
  *t* de Student;
- variações de vítimas e equipes pertencem à robustez, não à hipótese principal.

Se o BA não for acionado, a conclusão permitida é:

> A política não alterou o desempenho nesse cenário porque não foi acionada.

Isso não permite estimar o efeito condicional de um reposicionamento efetivo.

## 12. Validações mínimas

```bash
make analysis-tests
```

Valida leitura dos escalares, denominadores, exposição, pareamento e manifesto.

```bash
./run.sh --build -c MainExperiment_BaOff -r 0
```

Valida build, mensagens, NED e uma execução do braço de controle.

```bash
make ba-smoke-test
```

Deve confirmar:

```text
timeout -> sensor -> BA -> movimento -> tentativa imediata -> ACK
```

```bash
make experiment
```

Executa os braços confirmatórios, valida pareamento e gera as tabelas finais.

## 13. Afirmações que devem ser evitadas

Evitar afirmar que:

- o obstáculo foi comprovadamente a causa da perda;
- confirmação por ACK é igual ao PDR;
- cada pacote constitui uma repetição independente;
- uma seed sem ativação deve ser excluída;
- o trabalho mede resiliência sem antes definir esse conceito;
- o projeto avalia energia, obstáculos dinâmicos ou conectividade de enxame;
- o BA sempre encontra uma posição viável;
- uma ativação do BA implica necessariamente movimento concluído.

## 14. Limitações que devem ser declaradas

- O sensor é geométrico e idealizado.
- A posição da equipe pode estar desatualizada.
- O custo de enlace é geométrico e não prevê o canal futuro.
- Uma falha de ACK pode ocorrer no alerta ou no caminho de retorno.
- Alertas concorrentes compartilham um controlador de movimento; decisões não
  são enfileiradas quando o VANT já está reposicionando-se.
- Os obstáculos do experimento são estáticos.
- A comunicação científica avaliada é direta.

## 15. Lista de verificação rápida

- [ ] O objetivo está formulado como efeito de BA On versus BA Off.
- [ ] PDR é o único desfecho primário.
- [ ] Perda foi apresentada como `1 - PDR`.
- [ ] Confirmação, atraso e retransmissões são secundários.
- [ ] `alertId` foi usado para deduplicação.
- [ ] Somente `baEnabled` difere entre os braços.
- [ ] Todas as seeds válidas foram incluídas.
- [ ] A exposição ao BA foi relatada separadamente.
- [ ] O smoke test confirmou movimento e tentativa imediata.
- [ ] Build e testes passaram no commit utilizado.
- [ ] Código, arquivos `.ini` e resultados pertencem à mesma versão.
- [ ] As limitações foram declaradas sem alegar causalidade individual.

## 16. Resumo em um parágrafo

Uma equipe divulga sua posição e o VANT envia um alerta de vítima diretamente
por UDP. Se tentativas consecutivas não recebem ACK, o VANT consulta um sensor
geométrico. Quando há obstáculo e o BA está habilitado, o algoritmo procura uma
posição viável, o VANT desloca-se e transmite novamente assim que chega. O efeito
da política é avaliado comparando execuções pareadas BA On e BA Off por meio do
PDR de alertas; confirmação, atraso e retransmissões são resultados secundários,
e ativações e movimentos descrevem a exposição ao mecanismo.
