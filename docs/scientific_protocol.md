# Protocolo científico do ECHOSAR-Net

Documento normativo do experimento. Enuncia a pergunta, as hipóteses, o
desenho e os critérios de aceitação. Em caso de divergência entre documentos,
prevalecem, nesta ordem:

1. o comportamento executável em `src/` e `simulations/omnetpp.ini`;
2. os portões de integridade em `analysis/`;
3. este protocolo;
4. os demais documentos.

Uma divergência não se resolve mudando apenas o texto: código, configuração,
análise e documentação devem voltar a descrever o mesmo experimento.

## 1. Pergunta

O reposicionamento de um UAV orientado pelo Bat Algorithm altera a entrega
confirmada de alertas quando obstáculos degradam a comunicação ar-solo?

## 2. Hipóteses

**H0** — A diferença pareada média da métrica primária entre `baEnabled=false`
e `baEnabled=true` é nula, sob as condições simuladas.

**H1** — A diferença pareada média da métrica primária é diferente de zero,
sob as condições simuladas.

O teste é bilateral. O projeto **não pressupõe** que o Bat Algorithm melhore a
comunicação: uma diferença negativa é um resultado admissível e deve ser
relatada como tal.

## 3. Unidade experimental

A unidade é **a execução completa da simulação**, identificada por
`seed-set = ${repetition}`. Não é o pacote, nem a tentativa, nem o alerta
individual. Pacotes de uma mesma execução não são réplicas independentes e
nunca devem ser tratados como tal em nenhum teste ou intervalo.

Cada execução do controle é pareada com a execução da proposta que compartilha
a mesma seed.

## 4. Variável independente

Exclusivamente `**.drone[*].app[0].baEnabled`.

| Braço | Configuração | Tratamento |
|---|---|---|
| Controle | `Experiment_Control_BaOff` | `baEnabled = false` |
| Proposta | `Experiment_Proposed_BaOn` | `baEnabled = true` |

Ambas estendem `DissertationBase`, que é `abstract` e não pode ser
executada diretamente. `analysis/process_results.py` compara os parâmetros
gravados nos `.sca` dos dois braços e **falha** se divergirem em qualquer
parâmetro além de `baEnabled`.

## 5. Fatores controlados

Topologia, duração, obstáculos, posições iniciais, evento da vítima, tráfego,
modelos de mobilidade, parâmetros de rádio e conjunto de seeds são idênticos
nos dois braços, por herança de `DissertationBase`.

### Fluxos aleatórios

O Bat Algorithm sorteia do **RNG global 1**, isolado por
`**.drone[*].app[0].rng-0 = 1` com `num-rngs = 2`. Sem esse isolamento, o
otimizador consumiria milhares de valores do RNG 0 — compartilhado com
mobilidade, backoff do 802.11, AODV e jitter das aplicações — apenas no braço
da proposta, dessincronizando os dois braços por um motivo que não é o
tratamento.

A divergência causada **fisicamente** pelo drone se mover é preservada: ela é o
efeito do tratamento e é justamente o que se deseja observar.

## 6. Desenho

- comparação pareada por seed;
- 30 repetições, salvo justificativa estatística explícita e registrada;
- mesma topologia, duração, eventos de vítima, posições iniciais, obstáculos,
  tráfego e mobilidade;
- manifesto de proveniência por lote, com revisão Git, estado do worktree e
  SHA-256 dos insumos.

Execuções determinísticas (`-r 0`) servem para regressão e diagnóstico. **Não
substituem** evidência experimental com múltiplas seeds.

## 7. Separação entre verificação e evidência

| Categoria | Configurações | Papel |
|---|---|---|
| Verificação determinística | `Validation_*` | Confirmam que a implementação satisfaz contratos. Não são evidência científica. |
| Experimento | `Experiment_Control_BaOff`, `Experiment_Proposed_BaOn` | Única fonte de evidência sobre H0/H1. |
| Demonstração | `Visual_Demo` | Inspeção no Qtenv. Altera limiares para tornar o comportamento visível. |
| Diagnóstico de rede | `Network_*` | Auditoria de tráfego em PCAP. Alteram parâmetros de rádio ou injetam falha. |

`Validation_BaOn` usa **injeção controlada de falha** (`ackStartTime`) e
sensibilidade de recepção alterada para exercitar a máquina de estados do
reposicionamento. Isso verifica a implementação e **não constitui evidência
científica** sobre a hipótese.

`analysis/process_results.py` exclui automaticamente toda configuração fora do
experimento das tabelas científicas, tabulando-as separadamente.

## 8. Critérios de aceitação da conclusão

Uma conclusão sobre H0 só pode ser publicada quando:

1. os dois braços possuem o mesmo conjunto de seeds, sem duplicatas;
2. nenhum parâmetro além de `baEnabled` difere entre eles;
3. o número de pares atinge o previsto;
4. denominadores vazios estão registrados em `data_quality.csv`;
5. a conclusão se apoia na métrica primária, não em métricas secundárias;
6. o efeito é relatado com sua incerteza, não apenas como decisão binária;
7. ausência de diferença significativa **não** é relatada como equivalência.

## 9. Decisões de desenho

### D1 e D2 — cenário e alertas por execução — decididas em 20/08/2026

**Problema medido.** O cenário original gerava um único alerta em `t = 2 s`,
confirmado na primeira tentativa. Como a avaliação de degradação só ocorre para
alertas pendentes, o caminho degradação → sensor → reposicionamento nunca era
exercitado. As 30 repetições pareadas registraram `baActivations = 0`,
`degradationIndications = 0` e AppACK de 100 % nos dois braços, com diferença
pareada exatamente nula. A hipótese não era falseável.

**Decisão.** O experimento passa a usar a configuração `UrbanMission`, com três
alterações, todas justificadas fisicamente e aplicadas **igualmente aos dois
braços**:

| Alteração | De | Para | Justificativa |
|---|---|---|---|
| Setor de busca | 1 km × 1 km | 200 m × 200 m | Uma sortie cobre um setor delimitado pelo comando do incidente, não a área inteira |
| Obstáculos | 2 blocos de 3×2×5 m | 4 edificações de 30×30×10 m | A proposta trata de degradação ar-solo em terreno construído; um bloco de 3×2×5 m é um anteparo pontual |
| Vítimas | 1 em `t = 2 s` | 10 ao longo de 660 s | Uma sortie acumula detecções; AppACK passa a ser proporção com variância interna (resolve D2) |

Consequências deliberadas: o piso de voo sobe para 12 m, acima do topo das
edificações, para que um drone nunca ocupe o volume de um prédio — inclusive os
candidatos propostos pelo BA. A geometria antiga permanece em
`dissertation-obstacles.xml`, usada pelos cenários determinísticos, cujos testes
de RSSI e de alcance do sensor dependem daquelas dimensões exatas.

**Nenhum parâmetro do Bat Algorithm, da função de aptidão ou do sensor foi
alterado.** A variável independente continua sendo apenas `baEnabled`.

### Critério de aceitação do cenário

Fixado **antes** da execução e deliberadamente silencioso quanto ao resultado.
Um piloto de 5 seeds por braço aceita o cenário quando:

1. `baActivations > 0` no braço da proposta, em pelo menos 4 das 5 seeds;
2. `degradationIndications > 0` nos **dois** braços — o caminho de degradação é
   exercitado independentemente do tratamento;
3. AppACK fica estritamente entre 0 % e 100 % em pelo menos um braço, isto é,
   fora do teto e do piso, de modo que a métrica tenha variância.

O critério **não menciona o sinal nem a magnitude da diferença pareada**. Ajustar
o cenário até que a diferença fique favorável à proposta seria desenho enviesado;
por isso a condição de parada é a ativação do mecanismo e a variância da métrica,
não o seu resultado.

Se o piloto reprovar, o cenário pode ser ajustado novamente, e cada ajuste é
registrado aqui. O cenário é congelado antes do lote definitivo de 30 pares.

### D3 — Instrumento estatístico — em aberto

Com AppACK agora sobre 10 alertas por execução, a métrica primária é uma
proporção, e a diferença pareada por seed com IC95% é o instrumento natural.
Resta decidir se um teste de hipótese é reportado e, em caso afirmativo, com
qual justificativa e verificação de premissas. `paired_comparison.csv` publica o
efeito, o intervalo e as contagens de pares discordantes; nenhum teste é
aplicado automaticamente.

### D4 — Instrumentação de PCAP — em aberto

Definir se permanece como diagnóstico rotulado, é consolidada, ou é promovida a
verificação formal com contrato de métrica próprio. A auditoria de nível de fio
que ela fornece é hoje a única evidência independente de que os pacotes
observados correspondem ao que o modelo afirma enviar.

## 10. Limites de validade

Correlação com obstáculo não elimina distância, interferência, mobilidade ou
ausência de rota como causas alternativas. Confirmação sensorial e degradação
de rede são eventos diferentes e permanecem separados na análise.

As demais limitações do modelo estão em
[`model_and_assumptions.md`](model_and_assumptions.md); os limites de cada
medida, em [`metrics.md`](metrics.md).
