# Protocolo científico

## 1. Pergunta e estimando

Pergunta principal:

> No cenário direto definido para o experimento principal, habilitar a política
> de reposicionamento do UAV pelo Bat Algorithm altera o desempenho fim a fim da
> entrega de alertas de vítima, mantendo todo o restante da configuração e a
> realização aleatória pareados?

O estimando é o efeito médio pareado de habilitar o BA. Para uma métrica \(Y\) e
uma seed \(i\):

\[
d_i=Y_{BA\ On,i}-Y_{BA\ Off,i},
\qquad
\Delta_Y=E[d_i].
\]

O contraste não estima o efeito isolado da meta-heurística abstrata. Estima o
efeito da cadeia implementada: gatilho de rede + confirmação sensorial + função
de aptidão + movimento + retry/ACK.

## 2. Hipóteses

O desfecho central é o PDR de alertas únicos. A hipótese nula principal é:

\[
H_0:\Delta_{PDR}=0.
\]

A hipótese direcional de engenharia é:

\[
H_1:\Delta_{PDR}>0.
\]

A taxa de confirmação e o atraso de entrega são desfechos de suporte. Para
confirmação espera-se efeito positivo; para atraso, efeito negativo é favorável.
O relatório atual exporta os três no conjunto `PRIMARY_METRICS`. Caso se queira
uma decisão confirmatória única com teste de significância, o desfecho primário,
a regra de multiplicidade e o limiar de decisão devem ser definidos antes de
inspecionar os resultados. Sem essa pré-especificação, devem ser apresentados
efeitos e intervalos de confiança, sem seleção oportunista da métrica favorável.

## 3. Unidade experimental e pareamento

A unidade experimental é uma execução completa identificada pela seed, não um
pacote, tentativa, vítima ou amostra temporal. Pacotes dentro da mesma execução
compartilham mobilidade, topologia e canal e, portanto, não são réplicas
independentes.

Cada seed aparece uma vez no controle e uma vez no tratamento. O arquivo
[`main-experiment.ini`](../simulations/main-experiment.ini) define:

- `MainExperiment_BaOff`: controle;
- `MainExperiment_BaOn`: tratamento.

Ambos herdam a mesma base. A única diferença autorizada é `baEnabled`. A
configuração global associa RNGs por componente e usa `seed-set` a partir da
repetição, preservando a realização aleatória pareada. Quantidade de repetições,
duração e demais valores permanecem como fonte única nos arquivos `.ini`.

O script [`report_main_experiment.py`](../analysis/reports/report_main_experiment.py)
verifica a correspondência um-para-um das seeds e compara os parâmetros
registrados em cada par de `.sca`. Qualquer deriva além de `baEnabled` causa
falha, em vez de ser atribuída silenciosamente ao tratamento.

## 4. Escopos experimentais

| Escopo | Finalidade | Pode sustentar a conclusão principal? |
| --- | --- | --- |
| Experimento principal | Contraste pareado mínimo BA Off/On em comunicação direta | Sim, se os gates forem satisfeitos |
| Robustez | Variar carga de vítimas e quantidade de equipes, preservando o contraste | Sustenta generalização interna, não substitui o principal |
| Multihop opcional | Investigar efeito de roteamento/descoberta | Não; responde outra pergunta |
| Escalabilidade opcional | Explorar dimensões do cenário e custo | Não sem protocolo inferencial próprio |
| BA smoke test | Forçar a cadeia de integração de modo determinístico | Não; valida software |
| Validação de descoberta | Verificar o alcance semântico dos anúncios | Não; valida software/modelo |
| PCAP | Auditar pacotes e caminhos selecionados | Não; é evidência diagnóstica |

Essa hierarquia impede que um cenário construído para forçar um evento seja
misturado com evidência científica sobre sua frequência ou efeito.

## 5. Critérios de informatividade do tratamento

Uma comparação BA Off/On só informa sobre reposicionamento quando o mecanismo é
exercitado. Antes de produzir o relatório, o braço BA On deve conter ao menos:

1. indicação de degradação;
2. confirmação do sensor;
3. ativação do BA;
4. movimento de reposicionamento iniciado.

Esses gates estão implementados em `require_informative_treatment()`. Se algum
for zero, a análise termina com erro. PDR semelhante entre braços sem exposição
ao mecanismo não é evidência de ausência de efeito; é um experimento não
informativo.

A calibração do cenário deve ser prospectiva: verificar, antes da análise final,
se os gates podem ocorrer e congelar os parâmetros. Alterar limiares ou ambiente
depois de observar qual braço teve resultado melhor introduz viés de pesquisador.

## 6. Procedimento experimental

1. Registrar a versão do código, da configuração, do OMNeT++ e do INET.
2. Executar os testes de análise e as validações técnicas.
3. Executar um piloto apenas para verificar duração, armazenamento, estabilidade
   e exposição ao mecanismo; não incorporar suas escolhas orientadas por
   resultado à análise confirmatória.
4. Congelar os arquivos de configuração.
5. Executar os braços completos BA Off e BA On.
6. Confirmar igualdade dos conjuntos de seeds e ausência de deriva de parâmetros.
7. Confirmar os quatro gates de informatividade no tratamento.
8. Calcular cada métrica primeiro por execução.
9. Calcular os efeitos pareados e seus intervalos de confiança.
10. Relatar também falhas, `NaN`, exclusões e métricas de mecanismo, sem substituir
    o desfecho primário por um diagnóstico de camada inferior.

Os comandos e artefatos concretos estão em
[`traceability.md`](traceability.md#procedimento-reproduzível).

## 7. Estimação e intervalo de confiança

Para \(n\) pares válidos:

\[
\bar d=\frac{1}{n}\sum_{i=1}^{n}d_i,
\qquad
s_d=\sqrt{\frac{\sum_i(d_i-\bar d)^2}{n-1}},
\]

\[
IC_{95\%}=\bar d\pm t_{0.975,n-1}\frac{s_d}{\sqrt n}.
\]

`analysis/core/process_results.py` aproxima o quantil t de Student por expansão
de Cornish–Fisher. Com menos de dois pares, a largura do intervalo é `NaN`.

O relatório usa a convenção neutra `BA On − BA Off`. Assim:

- efeito positivo favorece BA para PDR e confirmação;
- efeito negativo favorece BA para atraso;
- o sinal deve sempre ser interpretado junto à unidade e à direção desejável.

Média, desvio-padrão e IC são calculados sobre os resultados por seed. Não se
deve agrupar todos os pacotes de todas as seeds antes da inferência.

## 8. Métricas do mecanismo

Ativações, movimentos, validações, distância e tempo de recuperação explicam
como o tratamento operou. Elas são exportadas separadamente porque não possuem
um denominador equivalente no controle quando nenhum reposicionamento existe.

Especialmente:

- recuperação operacional inclui ACK válido associado ao alerta enquanto o
  ciclo está ativo, mesmo que venha de tentativa anterior;
- validação causal exige ACK da tentativa enviada após a chegada, para o mesmo
  ciclo;
- somente a segunda sustenta comparação pré/pós da posição escolhida pelo BA.

Usar recuperação operacional como prova causal superestima o mecanismo. A
definição formal está em [Métricas de reposicionamento](metrics.md#8-métricas-de-reposicionamento).

## 9. Validade interna

Controles implementados:

- pareamento por seed;
- diferença de configuração restrita a `baEnabled`;
- RNGs separados por componente para reduzir perturbações entre cenários;
- desempates determinísticos por identidade;
- IDs distintos para alerta, tentativa e ciclo de reposicionamento;
- coleta global deduplicada;
- invariantes de conservação ao final da execução;
- distinção entre ACK causal e ACK tardio;
- falha automática quando o tratamento não foi exercitado.

Riscos remanescentes:

- o tratamento muda a trajetória e, a partir daí, os braços não podem manter o
  mesmo histórico físico, ainda que compartilhem seed;
- eventos raros podem produzir poucos ciclos validados e alta incerteza;
- várias vítimas no mesmo run compartilham condições e não são replicações;
- três desfechos analisados sem regra de multiplicidade não autorizam escolher
  retrospectivamente o mais significativo.

## 10. Validade de construto e externa

PDR de alerta mede entrega à aplicação; confirmação mede retorno do ACK. Nenhuma
das duas isoladamente representa “resgate bem-sucedido”. O sensor abstrato mede
interseção geométrica idealizada, não desempenho de uma câmera real. A aptidão
usa distância como proxy de enlace, não previsão perfeita de rádio.

Resultados valem para a pilha, mobilidade, obstáculos, parâmetros e região do
espaço experimental. Generalizações para outras aeronaves, ambientes, espectro,
densidades ou sensores exigem nova análise de sensibilidade ou validação
empírica.

## 11. Regra de conclusão

Uma conclusão defensável deve apresentar:

- efeito pareado e IC do PDR;
- efeitos de suporte de confirmação e atraso;
- número de pares válidos;
- exposição do tratamento e número de ciclos causalmente validados;
- resultados de robustez claramente separados;
- limitações e qualquer falha de validação.

Se os gates não forem satisfeitos, se houver deriva de parâmetros ou se os
pares estiverem incompletos, o resultado correto é “execução inválida ou não
informativa”, não “BA sem efeito”.
