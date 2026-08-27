Este documento explica como a simulação registra os dados e como, a partir deles, são calculadas as métricas de **atendimento**, **perda** e **volume de mensagens**.

Os números apresentados foram obtidos dos arquivos da campanha `Scenario1_*`, composta por 480 execuções e armazenada em `simulations/results/omnetpp/`.

---

## 1. Tipos de arquivo gerados

| Arquivo | O que armazena | Quando é gravado | Tamanho típico por execução | Principal uso |
| --- | --- | --- | --- | --- |
| `.sca` | **Escalares**: um valor final para cada métrica | Ao fim da execução, em `finish()` | 400 KB a 1,3 MB | Análise estatística da campanha |
| `.vec` | **Vetores**: séries temporais com um valor por amostra | Durante a execução | 100 KB a 350 KB | Análise da evolução das variáveis ao longo do tempo |
| `.vci` | Índice do arquivo `.vec`, usado para acelerar sua leitura | Junto com o `.vec` | Aproximadamente o tamanho do `.vec` | Uso interno da IDE e do `scavetool` |
| `.elog` | **Eventos e mensagens** registrados individualmente | Durante a execução | Cerca de 132 MB, por estimativa | Depuração e diagramas de sequência |
| `.anf` | Configuração da análise, incluindo filtros e gráficos | Pela IDE | Poucos KB | Reprodução de gráficos |

Em termos simples, o `.sca` funciona como uma **fotografia final** da execução: ele informa, por exemplo, quantos alertas foram confirmados. O `.vec` registra a **evolução de uma variável** ao longo do tempo. Já o `.elog` mantém um histórico detalhado de **todos os eventos**, incluindo as mensagens trocadas.

Exemplo  de gráfcos gerados com .sca e .vec https://doc.omnetpp.org/omnetpp4/tictoc-tutorial/part5.html

---

## 2. `.sca`: o arquivo das métricas finais

O arquivo `.sca` armazena os valores calculados pela implementação. Sua estrutura pode ser dividida em três blocos:

```
version 3
run Scenario1_OneVictim_BaOff-0-20260827-13:46:02-91107

attr configname Scenario1_OneVictim_BaOff      # (1) identificação da execução
attr iterationvars $teams=1
attr repetition 0
attr seedset 0

config sim-time-limit 900s                     # (2) configuração utilizada
config **.drone[*].app[0].baEnabled false

scalar BasicNetwork.experimentMetrics alertsGenerated 24   # (3) métricas
scalar BasicNetwork.experimentMetrics alertsDelivered 18
scalar BasicNetwork.experimentMetrics alertsConfirmed 18
scalar BasicNetwork.experimentMetrics alertsExpired 6
```

Os três blocos são importantes. O primeiro identifica a célula experimental à qual o resultado pertence, incluindo a seed, o número de equipes e o braço do experimento. O segundo registra a configuração utilizada. O terceiro contém os valores das métricas.

Por isso, embora um arquivo `.sca` de 400 KB tenha apenas cerca de 660 escalares, a maior parte de seu conteúdo corresponde à configuração e aos parâmetros da execução. Essas informações permitem rastrear as condições em que cada resultado foi produzido.

!image.png

### Como um escalar é gerado

O processo ocorre em três etapas, todas implementadas em `src/metrics/ExperimentMetrics.cc`.

**1. O módulo se inscreve nos sinais** em `initialize()` (linhas 40–49):

```cpp
generatedSignal = registerSignal("victimAlertGenerated");
deliveredSignal = registerSignal("victimAlertDelivered");
confirmedSignal = registerSignal("victimAlertConfirmed");
expiredSignal   = registerSignal("victimAlertExpired");
```

**2. A aplicação emite um sinal** quando o evento ocorre. O coletor registra o `alertId` em um `std::set`, de modo que um mesmo alerta recebido por mais de uma equipe seja contado apenas uma vez:

```cpp
else if (signalId == deliveredSignal) {
    if (deliveredAlertIds.insert(event->alertId).second) {   // somente se for novo
        record.delivered = true;
    }
}
```

**3. Ao fim da execução, os totais são gravados como escalares** em `finish()`:

```cpp
recordScalar("alertsGenerated", generatedAlertIds.size());
recordScalar("alertsDelivered", deliveredAlertIds.size());
recordScalar("alertsConfirmed", confirmedAlertIds.size());
recordScalar("alertAttemptsSent", sentAttemptIds.size()); // volume de mensagens
recordScalar("applicationRetries", applicationRetries);
```

---

## 3. `.vec` e `.vci`: séries temporais

O arquivo `.vec` armazena amostras coletadas durante a simulação. Cada registro segue o formato `idDoVetor numeroDoEvento tempoSimulado valor`. Antes dos dados, o arquivo contém a declaração do vetor:

```
vector 4 BasicNetwork.drone[0].wlan[0].mac.dcf.channelAccess.pendingQueue queueLength:vector ETV
```

O sufixo `ETV` indica as colunas presentes em cada amostra: **E**vento, **T**empo e **V**alor.

> **Atenção:** neste projeto, os arquivos `.vec` não contêm amostras. As linhas 51–52 de `simulations/omnetpp.ini` definem:
> 
> 
> ```
> **.vector-recording=false
> **.scalar-recording=true
> ```
> 
> Os 480 arquivos `.vec` guardam apenas as 355 declarações de vetores, sem dados associados. No total, são 121 MB de metadados, além de 119 MB dos respectivos arquivos `.vci`. Como todas as métricas utilizadas na dissertação são escalares, esses arquivos podem ser descartados sem perda de dados relevantes para a análise.
> 

A gravação de vetores é útil quando se deseja observar como uma variável evolui durante a execução. É o caso, por exemplo, do tamanho da fila MAC ao longo dos 900 segundos ou da variação do número de saltos. Para obter apenas o total de alertas confirmados, os escalares são suficientes.

---

## 4. `.elog`: o registro detalhado dos eventos

O arquivo `.elog` registra, em texto, cada evento e cada mensagem trocada durante a simulação. Ele também fornece os dados usados pelo **Sequence Chart** da IDE. A estrutura abaixo foi obtida em uma execução deste projeto:

```
SB ov 1538 ev 2 rid Scenario1_OneVictim_BaOff-0-20260827-13:47:32-92463

E # 0 t 0 m 1 ce -1 msg -1        # evento 0, tempo 0, módulo 1
MC id 1 c omnetpp::cModule t echosar.simulations.BasicNetwork n BasicNetwork
MC id 5 c inet::physicallayer::RadioMedium t ...Ieee80211ScalarRadioMedium
E # 2 t 0 m 57 ce 0 msg 182       # evento 2, módulo 57, mensagem 182
E # 3 t 0 m 124 ce 0 msg 183
```

Cada linha iniciada por `E #` representa um evento. As linhas seguintes descrevem os módulos e as mensagens envolvidas. Esse nível de detalhe permite reconstruir quem enviou cada mensagem, quem a recebeu e em que instante isso ocorreu. Em contrapartida, aumenta bastante o tamanho dos arquivos e o tempo de execução da simulação.

!image.png

---

## 5. Comparação entre `.sca` e `.elog`

A comparação abaixo considera a mesma execução (`Scenario1_OneVictim_BaOff`, seed 0), limitada a 60 segundos simulados para viabilizar a gravação do `.elog`.

| Medida | `.sca` | `.vec` | `.elog` |
| --- | --- | --- | --- |
| Tamanho em 60 s, com 12.912 eventos | ~401,6 KB | ~100 KB | **~7,3 MB** |
| Custo por evento | — | — | **567,5 B/evento** |
| Tempo do laço de eventos | 0,45 s | — | **1,97 s (4,3 vezes maior)** |

Ao extrapolar os valores com base no número de eventos medido nos logs da campanha, obtém-se:

| Escopo | Eventos | Tamanho real dos `.sca` | Tamanho estimado dos `.elog` | Razão aproximada |
| --- | --- | --- | --- | --- |
| Uma execução de 900 s, com `teams=1` | 232.440 | 409 KB | Cerca de 132 MB | **322 vezes** |
| Campanha completa, com 480 execuções | 1.192.434.494 | **392 MB** | **Cerca de 677 GB** | **1.700 vezes** |

Na prática, gravar o `.elog` em toda a campanha exigiria aproximadamente **677 GB** e faria o tempo de execução crescer mais de quatro vezes. Já os arquivos `.sca` ocupam 392 MB e acrescentam pouco custo à simulação.

Outro ponto importante é que o `.sca` de 60 segundos, com ~401,6 KB, e o de 900 segundos, com ~409 KB, têm praticamente o mesmo tamanho. Isso ocorre porque o `.sca` registra valores finais e uma cópia da configuração, cujo tamanho não depende diretamente da duração da simulação. O `.elog`, por sua vez, cresce de forma aproximadamente linear com o número de eventos.

!image.png

Gravar `.elog` deixa a simulação **4,3 vezes mais lenta**. Uma campanha de 480 execuções que leva **~3h10min** sem `.elog` passaria para **~13–14 horas** só pelo laço de eventos, sem contar o gargalo de I/O de escrever **677 GB** em disco com vários processos paralelos.

Reconstruir atendimento/perda a partir do `.elog` dá **viés sistemático**: o atendimento fica sempre um pouco **inflado** devido a lógica estar embutida no código não verificando alguns critérios o que gerar um métrica incorreta.

**Conclusão:** só o sinal de aplicação (`.sca`) enxerga o alerta **antes** de ele virar pacote. Por isso, ele é a única fonte confiável para as métricas oficiais.

Assim, a recomendação é:

- manter o `.sca` habilitado em todas as execuções, pois ele contém os dados usados na análise estatística;
- habilitar o `.elog` apenas em execuções curtas e isoladas, quando for necessário investigar por que determinado alerta não foi entregue.

---

## 6. Cálculo de atendimento, perda e volume de mensagens

As métricas de atendimento e perda são calculadas em `analysis/reports/alert_sheet.py` da seguinte forma:

```
Atendimento (%) = alertas com pelo menos um ACK / alertas únicos gerados
Perda (%)       = alertas que não chegaram a nenhuma equipe / alertas únicos gerados
```

Na função `summarize()`, o cálculo é realizado por este trecho:

```python
generated    = len(group)                              # alertas únicos gerados
acknowledged = int(group["acknowledged"].sum())        # alertas confirmados por ACK
undelivered  = int((group["delivered"] == 0).sum())    # alertas que não chegaram

atendimento_pct = 100.0 * acknowledged / generated
perda_pct       = 100.0 * undelivered  / generated
```

!image.png

Regra geral: cada alerta tem um `alertId` único. Ele é guardado num de quatro "baldes" (`std::set<std::string>`, declarados em `ExperimentMetrics.h`). Um sinal diferente joga o `alertId` em cada balde. Arquivo: `src/metrics/ExperimentMetrics.cc`, função `receiveSignal()`.

| Passo | Balde (variável) | Sinal que dispara |
| --- | --- | --- |
| 1. Alerta nasce | `generatedAlertIds` | `victimAlertGenerated` |
| 2. Chega numa equipe | `deliveredAlertIds` | `victimAlertDelivered` |
| 3. ACK volta pro drone | `confirmedAlertIds` | `victimAlertConfirmed` |
| 4. TTL esgota sem ACK | `expiredAlertIds` | `victimAlertExpired` |

Siga um alerta só, do início ao fim:

1. `DroneApp` cria o alerta → emite `victimAlertGenerated` → cai no balde `generatedAlertIds`. **Este balde é o "alertas únicos gerados".**
2. Se uma equipe recebe → emite `victimAlertDelivered` → cai e `deliveredAlertIds`. Só a **primeira** equipe conta (se duas equipes recebem o mesmo alerta, só a primeira gera esse evento).
3. Se o drone recebe o ACK de volta → emite `victimAlertConfirmed` → cai em `confirmedAlertIds`. **Este balde é o "atendimento".**
4. Se o alerta expira (TTL vencido, sem ACK) → emite `victimAlertExpired` → cai em `expiredAlertIds`. **Este balde é a "perda".**

Não existe um balde "perda" com outro nome disfarçado nem uma conta `gerado - entregue`: perda **é** `expiredAlertIds`.

**Como o código se auto-confere:** todo alerta tem que acabar em (3) ou em (4), nunca nos dois nem em nenhum. O código calcula isso em `ExperimentMetrics.cc:`:

```cpp
bool incompleteAlerts = confirmedAlertIds.size() + expiredAlertIds.size()
                         != generatedAlertIds.size();
```

**Do balde até a planilha:** os quatro baldes viram dado de duas formas, na mesma função `finish()`:

- **Escalar agregado no `.sca`** — `recordScalar("alertsConfirmed", confirmedAlertIds.size())` etc. (visto na seção 2).
- **Uma linha por alerta no CSV** — função `writeAlertRecords()` : para cada `alertId`, grava `1`/`0` nas colunas `delivered`/`acknowledged`. É esse CSV que `alert_sheet.py` lê e soma seção anterior).

Os dois vêm dos mesmos quatro baldes só que resumidos de jeitos diferentes.

### Por que atendimento e perda não somam necessariamente 100%

As duas taxas não são complementares porque existe uma terceira situação possível: o alerta chega a uma equipe, mas o ACK enviado como resposta se perde. O próprio coletor verifica, em `ExperimentMetrics.cc`, que o conjunto de alertas confirmados está contido no conjunto de alertas entregues:

```cpp
!isSubset(confirmedAlertIds, deliveredAlertIds)
```

Um exemplo real, obtido no cenário com uma vítima, cinco equipes e o BA desligado, apresenta 720 alertas:

| Categoria | Alertas | Percentual |
| --- | --- | --- |
| Confirmados (**atendimento**) | 703 | 97,6% |
| Entregues sem confirmação | 1 | 0,1% |
| Não entregues (**perda**) | 16 | 2,2% |
| **Total** | **720** | **100%** |

Por esse motivo, a planilha inclui a coluna `entregue_sem_confirmacao_pct`. Sem essa informação, a soma de 97,6% de atendimento com 2,2% de perda resulta em 99,8%, o que poderia ser interpretado incorretamente como uma inconsistência.

### Volume de mensagens

O volume de mensagens é representado por três escalares, cada um associado a uma medida diferente:

| Escalar | O que representa |
| --- | --- |
| `alertsGenerated` | Número de alertas **distintos** criados pela aplicação |
| `alertAttemptsSent` | Número de **tentativas de transmissão**, incluindo as retentativas |
| `applicationRetries` | Número de transmissões que correspondem a **retentativas** |

Os valores da campanha para o cenário com uma vítima, considerando 2.880 alertas gerados em cada braço, são:

| Métrica | BA desligado | BA ligado |
| --- | --- | --- |
| Alertas gerados | 2.880 | 2.880 |
| Tentativas enviadas | 3.167 | 3.176 |
| Retentativas | 348 | 344 |
| Entregues | 2.717 | 2.736 |
| Confirmados | 2.716 | 2.734 |

Essa tabela pode ser gerada com o comando:

```bash
python3 analysis/reports/mechanism_summary.py
```

O resultado é salvo em `analysis/tables/mecanismo_resumo.csv`.

### Intervalo de confiança do efeito pareado: bootstrap, não Student-t

**Correção importante:** existe uma função `ci95()` em
`analysis/core/process_results.py` cujo docstring diz "Student-t approximate
95% confidence-interval". Ela **não é usada em lugar nenhum do projeto** —
nenhum script importa `ci95`, desde o commit que a criou. É código morto.

O intervalo de confiança que de fato aparece na planilha (colunas
`efeito_atendimento_ic95_inf_pp`/`_sup_pp`, `efeito_perda_ic95_inf_pp`/
`_sup_pp`) vem de outra função: `paired_effects()`, em
`analysis/reports/alert_sheet.py`, e o método é **bootstrap percentil**, não
Student-t.

**Como o bootstrap funciona, passo a passo:**

Para cada célula (cenário × numTeams), há até 30 valores — um efeito
(BA-On − BA-Off) por seed pareada. O bootstrap:

1. Reamostra esses valores **com reposição**, formando um novo conjunto do
   mesmo tamanho (alguns valores originais podem repetir, outros podem
   ficar de fora);
2. Calcula a média dessa reamostra;
3. Repete os passos 1–2 **10.000 vezes** (`BOOTSTRAP_RESAMPLES`,
   `alert_sheet.py`), gerando 10.000 médias diferentes;
4. O intervalo de confiança de 95% é o percentil 2,5% e o percentil 97,5%
   dessas 10.000 médias.

```python
means = rng.choice(data, size=(BOOTSTRAP_RESAMPLES, len(data)), replace=True).mean(axis=1)
return tuple(np.quantile(means, [0.025, 0.975]))
```

Nenhuma fórmula de desvio-padrão, nenhum `z` ou `t`: o intervalo vem
diretamente da variação observada nas próprias reamostras. O RNG usa seed
fixa (`20260826`), então o resultado é reprodutível.

**Exemplo, com as mesmas 5 seeds hipotéticas de antes** (efeito em pontos
percentuais): `2, -1, 3, 0, 4`, média 1,6. Rodando o mesmo código do
projeto (10.000 reamostras, seed `20260826`):

```
IC bootstrap 95%: [0,00 ; 3,20]
```

**Exemplo real do projeto**, cenário `Scenario1_OneVictim`, `numTeams=1`,
30 pares reais: efeito de atendimento = 1,39 pp, IC bootstrap 95% =
`[-2,08 ; 5,56]` — cruza zero, então não há evidência de efeito nessa
célula (é o mesmo padrão documentado no restante da campanha).

!image.png
*(sugestão: histograma das 10.000 médias reamostradas do exemplo com 5
seeds, com duas linhas verticais marcando os percentis 2,5% e 97,5% — mostra
visualmente de onde vem o intervalo, sem fórmula nenhuma)*

**Por que bootstrap, e não uma fórmula fechada (Student-t ou normal):**

- Não exige assumir que o efeito segue distribuição normal — atendimento e
  perda são percentuais, com limites naturais (0% e 100%) e podem ser
  assimétricos, especialmente perto desses limites (ex.: `numTeams=1`, onde
  a perda é ~18-19%, longe de 0, mas `numTeams=15` tem perda ~0,1-0,4%,
  bem perto do limite inferior).
- Não precisa estimar um "desvio-padrão populacional": usa diretamente a
  variação observada nas reamostras dos próprios dados.
- Funciona igual bem com `n` pequeno ou grande, sem precisar trocar de
  fórmula ou de tabela.

**Limitação a que prestar atenção:** com `n` muito pequeno, o bootstrap só
consegue reamostrar dentro dos valores já observados — no exemplo de 5
seeds, existem apenas `5⁵=3.125` reamostras possíveis. Isso faz o intervalo
bootstrap (`[0,00 ; 3,20]`) sair **mais estreito** que o de Student-t
(`[-0,97 ; 4,17]`, calculado abaixo) para essa mesma amostra pequena — o
bootstrap não "inventa" incerteza além do que os 5 pontos observados
mostram. Com `n=30` (o caso real da campanha), essa limitação é bem menor,
porque `30³⁰` reamostras possíveis já cobre o espaço de variação de forma
muito mais densa.

### O que `ci95()` faria, se fosse chamada (Student-t)

Ainda que não seja usada, `ci95()` existe no código e resolve um problema
didaticamente relacionado — vale entender, porque explica **por que** o
bootstrap é uma alternativa válida ao invés de simplesmente "consertar" essa
função e usá-la.

A fórmula clássica de intervalo de confiança é

```
IC = média ± crítico × (desvio-padrão / √n)
```

O valor `crítico` só pode ser o da distribuição normal (`z = 1,96` para 95%)
quando o desvio-padrão usado é o **verdadeiro** da população. Na prática, esse
valor nunca é conhecido: ele é estimado a partir da própria amostra (`s`), e
essa estimativa carrega erro — maior quanto menor for `n`. A distribuição t
existe para incorporar esse erro extra: usa um valor crítico maior que `z`,
que diminui conforme `n` cresce, até convergir para o próprio `z` quando
`n → ∞`.

A estatística t e seus graus de liberdade (ν):

```
t = (x̄ − μ) / (s/√n)          ν = n − 1
```

Com as mesmas 5 seeds hipotéticas (média 1,6; desvio-padrão amostral
s=2,074, dividido por `n−1=4` — daí vem ν=4; erro-padrão = s/√5 = 0,927):

| Método | Crítico | Intervalo | Conclusão |
| --- | --- | --- | --- |
| Normal (`z`, incorreto para `n` pequeno) | 1,960 | [-0,22 ; 3,42] | quase não cruza zero |
| t, ν=4 | 2,776 | [-0,97 ; 4,17] | cruza zero claramente |
| Bootstrap (o que o projeto realmente usa) | — | [0,00 ; 3,20] | cruza zero, no limite |

Os três métodos concordam na conclusão qualitativa desse exemplo (efeito não
é claramente diferente de zero), mas discordam na largura exata — cada um
lida com a incerteza de `n=5` de um jeito diferente.

!image.png
*(sugestão: sobreposição das curvas de densidade da distribuição t para
ν=1, ν=4 e ν=29 contra a densidade normal padrão — mostra as caudas mais
pesadas da t encolhendo até quase coincidir com a normal em ν=29)*

**Por que t, e não outra distribuição fechada (se fosse essa a escolha):**

- **Normal (z)** — só é exata se o desvio-padrão populacional é conhecido, ou
  `n` é grande o suficiente para `s ≈ σ`.
- **Qui-quadrado** — modela variância, não média.
- **F** — compara duas variâncias (ex.: ANOVA); não é o caso de uma média
  pareada.
- **t** — é a distribuição exata da razão "média amostral padronizada pelo
  próprio desvio estimado da amostra".

Com `n=30` (as seeds de cada célula da campanha), ν=29 e o crítico t seria
2,045 — só ~4% maior que `z=1,960`. Com `n=5`, a diferença já é 41%.

`ci95()` não usa tabela t — usa uma aproximação (expansão de Cornish-Fisher)
que calcula o crítico t a partir do `z` normal, corrigido pelos graus de
liberdade:

```python
z = NormalDist().inv_cdf(0.975)
critical = z + (z**3 + z) / (4 * degrees) + \
    (5 * z**5 + 16 * z**3 + 3 * z) / (96 * degrees**2)
```

Conferido numericamente: para ν=29, a fórmula dá 2,0451 contra o valor exato
de tabela 2,0452 — diferença desprezível, caso algum dia essa função venha a
ser usada.

---

## 7. `scavetool`: consulta e exportação pela linha de comando

O `opp_scavetool` permite consultar arquivos `.sca` e `.vec` ou exportar seus dados para CSV. Ele utiliza o mesmo mecanismo empregado internamente pela IDE e, por isso, também pode ser usado como uma verificação independente dos scripts Python. Se ambos produzem os mesmos valores, há uma evidência adicional de que o processamento não introduziu erros. No projeto, essa verificação é executada pelo alvo `make scavetool-check`.

### 7.1 Consultar o conteúdo de um arquivo

```bash
opp_scavetool query results/omnetpp/Scenario1_OneVictim_BaOff-0.sca
```

Saída:

```
runs: 1   scalars: 662  parameters: 2654  vectors: 0  statistics: 0  histograms: 60
```

O campo `vectors: 0` confirma que `vector-recording` estava desabilitado, conforme explicado na Seção 3.

### 7.2 Filtrar as métricas de interesse

A opção `-l` lista os valores encontrados:

```bash
opp_scavetool query -l --filter 'name =~ alerts*' \
  results/omnetpp/Scenario1_OneVictim_BaOff-0.sca
```

Saída:

```
scalar  BasicNetwork.experimentMetrics  alertsGenerated         24
scalar  BasicNetwork.experimentMetrics  alertsDelivered         18
scalar  BasicNetwork.experimentMetrics  alertsConfirmed         18
scalar  BasicNetwork.experimentMetrics  alertsExpired           6
scalar  BasicNetwork.experimentMetrics  alertsWithoutKnownTeam  3
```

O filtro `name =~ alerts*` é um *glob* que seleciona os escalares cujo nome começa com `alerts`. Também é possível filtrar pelos campos `module`, `run`, `configname` e `attr:*`.

### 7.3 Conferir todos os arquivos de um braço

O mesmo filtro pode ser aplicado aos 120 arquivos de um braço experimental:

```bash
opp_scavetool query --filter 'name =~ alerts*' \
  'results/omnetpp/Scenario1_OneVictim_BaOff-*.sca'
```

Saída:

```
runs: 120   scalars: 600  parameters: 0  vectors: 0  statistics: 0  histograms: 0
```

O total de 600 corresponde a cinco escalares em cada uma das 120 execuções. Essa consulta também permite verificar se a campanha está completa antes da análise: se o resultado indicasse `runs: 119`, por exemplo, uma execução estaria ausente.

### 7.4 Exportar os dados para CSV

```bash
opp_scavetool export -F CSV-R -o metricas.csv \
  -f 'name =~ alerts* or name =~ pdr' \
  'results/omnetpp/Scenario1_OneVictim_BaOff-*.sca'
```

Saída:

```
Exported 7 scalars
```

!image.png

---

## 8. `.anf`: análise pela IDE

O arquivo `.anf` não armazena os resultados da simulação. Ele contém apenas a configuração da análise, como os arquivos de entrada, os filtros aplicados e os gráficos gerados. Os dados permanecem nos arquivos `.sca`. Por ser pequeno, o `.anf` pode ser versionado no Git, permitindo que outra pessoa reproduza a mesma análise depois de executar a campanha.

Na IDE, o fluxo consiste em acessar *File → New → Analysis File*, adicionar `results/omnetpp/*.sca` como entrada, aplicar os filtros desejados e escolher o tipo de gráfico.

!image.png

!image.png

Desde a versão 6 do OMNeT++, cada gráfico do `.anf` corresponde a um script Python baseado em Matplotlib. Esse script pode ser editado e executado fora da IDE por meio do módulo `omnetpp.scave.analysis`: `load_anf_file()` carrega o arquivo, enquanto `Analysis.export_image()` e `Analysis.export_data()` exportam imagens e dados sem abrir a interface gráfica.

Este projeto não utiliza arquivos `.anf` na geração das figuras finais. Os gráficos são produzidos diretamente por `analysis/reports/alert_sheet.py`, com saída em PDF vetorial. Dessa forma, a geração das figuras e o cálculo estatístico permanecem no mesmo script versionado. O `.anf` continua sendo útil na etapa exploratória, quando se deseja observar os dados de diferentes formas antes de escolher o que será apresentado no texto.

---

## 9. Resumo das recomendações

| Objetivo | Recurso indicado |
| --- | --- |
| Obter os valores para a análise estatística da campanha | `.sca`, já habilitado |
| Acompanhar a evolução de uma variável no tempo | `.vec`, com `vector-recording` habilitado |
| Investigar por que um alerta específico falhou | `.elog`, em uma execução curta e isolada |
| Explorar os dados visualmente | `.anf`, pela IDE |
| Conferir os scripts de análise | `opp_scavetool` |

---