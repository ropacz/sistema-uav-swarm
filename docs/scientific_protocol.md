# Protocolo científico

Este documento é normativo para o experimento confirmatório. Detalhes de
implementação pertencem a `model_and_assumptions.md`; contratos de medição, a
`metrics.md`. Os valores experimentais existem somente nos arquivos `.ini`.

## 1. Pergunta e objetivo

Pergunta principal:

> Habilitar a política de reposicionamento por Bat Algorithm melhora o PDR fim
> a fim de alertas de vítima entre UAVs e equipes de resgate?

O estimando é o efeito médio da política completa:

\[
\Delta_Y=E[Y_{On}-Y_{Off}].
\]

O PDR de alertas é o único desfecho primário. Confirmação por ACK, atraso fim a
fim e retransmissões por alerta são desfechos secundários. A unidade analisada é
uma execução completa, nunca um pacote individual.

Hipóteses para o PDR:

\[
H_0:E[PDR_{On}-PDR_{Off}]=0,
\qquad
H_1:E[PDR_{On}-PDR_{Off}]>0.
\]

## 2. Desenho experimental

O contraste confirmatório usa `MainExperiment_BaOff` e
`MainExperiment_BaOn`. Em cada seed, os braços devem diferir exclusivamente no
parâmetro `baEnabled`. Mobilidade, vítimas, equipes, obstáculos, rádio e todos os
demais parâmetros permanecem pareados.

O analisador exige:

- uma correspondência um-para-um entre seeds;
- ausência de seed ausente ou duplicada;
- igualdade dos parâmetros registrados, exceto `baEnabled`;
- presença única de cada escalar central obrigatório.

Multihop, escala, PCAP e diagnósticos por camada são extensões opcionais e não
integram a conclusão confirmatória.

## 3. Métricas

Para uma execução, sejam (G) alertas gerados, (D) entregues a pelo menos uma
equipe, (C) confirmados e (R) retransmissões da aplicação:

\[
PDR=\frac{D}{G},
\qquad
ConfirmationRate=\frac{C}{G},
\qquad
RetriesPerAlert=\frac{R}{G}.
\]

Para (N_D) alertas entregues e atrasos fim a fim (d_i):

\[
MeanDeliveryDelay=\frac{\sum_{i=1}^{N_D}d_i}{N_D}.
\]

O coletor preserva numeradores e denominadores. Razões com denominador zero são
indefinidas, não zero. A deduplicação usa `alertId`, portanto o mesmo alerta
recebido por equipes diferentes conta uma única vez.

## 4. Procedimento

1. Executar build, testes da análise e `BA_SmokeTest`.
2. Realizar um piloto curto para verificar se o cenário pode expor a política.
3. Congelar código e parâmetros antes de observar o contraste final.
4. Executar os dois braços com as mesmas seeds.
5. Guardar `.sca`, configurações e manifesto de proveniência.
6. Validar pareamento, duplicações e deriva de configuração.
7. Calcular efeitos pareados incluindo todas as seeds válidas.

Uma seed sem ativação do BA permanece na análise. A exposição é resultado da
política e não pode ser usada como critério de exclusão pós-tratamento.

## 5. Análise estatística

Para cada seed (i), calcula-se:

\[
d_i=Y_{On,i}-Y_{Off,i}.
\]

O efeito relatado é \(\bar d\), acompanhado por desvio-padrão e intervalo de
confiança de 95% baseado na distribuição t de Student. Cada seed recebe o mesmo
peso. Pacotes e alertas dentro de uma execução não são tratados como réplicas
independentes.

O relatório separa:

- desfecho primário;
- desfechos secundários;
- exposição: gatilhos, obstáculos detectados, ativações, movimentos e distância.

## 6. Interpretação e limitações

Se nenhuma ativação ocorrer, o contraste ainda estima o efeito operacional da
política naquele cenário. A conclusão correta é:

> A política não alterou o desempenho neste cenário porque não foi acionada.

Isso não estima o efeito condicional de um reposicionamento quando acionado.
Também não se deve inferir causalidade de um movimento individual: a evidência
principal é o contraste global BA On menos BA Off.

Limites de validade externa incluem sensor geométrico abstrato, última posição
recebida da equipe, comunicação direta no estudo principal e função de aptidão
baseada em geometria. PCAP e métricas MAC/IP podem explicar casos, mas não
substituem os desfechos fim a fim da aplicação.
