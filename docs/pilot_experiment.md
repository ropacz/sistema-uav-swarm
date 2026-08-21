# Experimento piloto do Bat Algorithm

## 1. Problema científico

Investigar se o reposicionamento de um UAV, escolhido pelo Bat Algorithm (BA),
consegue recuperar a entrega de um alerta de vítima quando um obstáculo físico
interrompe o enlace direto entre o UAV e uma equipe móvel de resgate.

Hipótese operacional:

> Sob degradação causada por obstrução confirmada, habilitar o BA aumenta a
> proporção de alertas confirmados por AppACK em relação ao mesmo cenário com o
> BA desligado.

O piloto testa mecanismo e viabilidade. Cinco pares são suficientes para
detectar falhas sistemáticas, mas não substituem um estudo confirmatório com
maior número de réplicas e geometria variada.

## 2. Desenho causal

O experimento possui dois braços:

| Braço | Configuração | `baEnabled` |
|---|---|---:|
| Controle | `HypothesisPilot_BaOff` | false |
| Tratamento | `HypothesisPilot_BaOn` | true |

Cada braço usa cinco seeds. Para cada seed, topologia, trajetória, rádio,
aplicações e obstáculo são idênticos. O BA usa um fluxo aleatório separado
(`rng-0 = 1`), evitando alterar artificialmente o backoff do 802.11 e o AODV.

## 3. Cenário físico e cronologia

- área: 1000 × 1000 × 25 m;
- um drone em `(500, 500, 6)` m;
- uma equipe iniciando em `(520, 500, 0)` m;
- uma vítima em `(500, 500, 0)` m;
- edifício de concreto: 10 × 20 × 10 m, centrado em `(530, 500, 5)` m;
- equipe: 1,5 m/s em direção a `(545, 500, 0)` m;
- detecção da vítima: `t = 13 s`;
- duração: 120 s.

A equipe começa em linha de visada, transmite atualizações a cada 0,1 s,
atravessa o edifício e para atrás dele. Quando o alerta é criado, a comunicação
já está fisicamente obstruída. Não existe RSSI forçado, ACK artificial ou
sensibilidade especial.

Há somente dois nós com rádio: drone e equipe. Logo, não existe retransmissor
possível. `hopCount = 0` confirma entrega direta; AODV descobre a rota, mas não
realiza encaminhamento multihop neste piloto.

## 4. Rádio e propagação

O enlace usa IEEE 802.11b/DSSS, 1 Mbps, 2,4 GHz, potência de 20 mW (13 dBm) e
sensibilidade de −85 dBm. A perda em espaço livre segue:

```text
FSPL(dB) = 20 log10(d) + 20 log10(f) + 20 log10(4π/c)
Pr(dBm)  = Pt(dBm) + Gt(dB) + Gr(dB) − FSPL(dB) − Lobstáculo(dB)
```

`d` é a distância em metros, `f` a frequência em hertz e `c` a velocidade da
luz. O expoente `alpha = 2` representa Friis/espaço livre e não é ajustado pelo
tamanho da área. A atenuação do concreto é calculada separadamente pelo
`DielectricObstacleLoss` do INET.

Como verificação de ordem de grandeza, a distância final drone–equipe antes do
reposicionamento é aproximadamente 45,4 m. Sem obstáculo, o FSPL é cerca de
73,2 dB e a potência ideal recebida é aproximadamente −60,2 dBm, quase 25 dB
acima da sensibilidade de −85 dBm. Assim, distância isolada não explica a perda
do controle; a interseção com o concreto é necessária no modelo configurado.

## 5. Alerta, retry e AppACK

A vítima gera um `VictimAlert`. Cada tentativa recebe um `messageId`; todas as
tentativas do mesmo evento compartilham o `alertId`. A equipe deduplica ambos e
responde com `VictimAck`.

```text
AppACK = alertas únicos confirmados / alertas únicos gerados
perda  = 1 − tentativas recebidas / tentativas enviadas
RTT    = instante do ACK − instante de envio da tentativa confirmada
```

O alerta usa `ackTimeout = 2 s`, `retryInterval = 10 s`, no máximo oito
tentativas e TTL de 100 s. A ausência do AppACK dispara uma avaliação; sozinha,
ela não prova que existe obstáculo.

## 6. Gatilho de degradação

Para a janela de atualizações de posição da equipe:

```text
esperados = seq_última − seq_primeira + 1
PDR       = recebidos / esperados
RSSI_médio = Σ RSSI_i / n
silêncio  = agora − última_recepção ≥ 3 s
```

A degradação é indicada quando pelo menos uma condição é verdadeira:

```text
degradado = silêncio OR PDR < 0,8 OR RSSI_médio < −80 dBm
```

A cadeia de ativação é:

```text
AppACK ausente após 2 s
        ↓
degradação de enlace indicada
        ↓
sensor confirma obstáculo na linha drone–equipe estimada
        ↓
baEnabled = true, ciclos disponíveis e nenhum movimento concorrente
        ↓
Bat Algorithm executa
```

O sensor geométrico aceita obstáculos entre 0,7 e 30 m. Assim, perdas por
distância ou interferência não são automaticamente atribuídas a obstrução.

## 7. Estimativa de posição da equipe

Quando duas atualizações válidas são `(p₁,t₁)` e `(p₂,t₂)`, a velocidade e a
direção observadas são:

```text
v = (p₂ − p₁) / (t₂ − t₁)
```

Se `|v| > 5 m/s`, o vetor é normalizado para 5 m/s. Durante o silêncio:

```text
Δt = clamp(agora − t₂, 0, 15 s)
p_estimado = p₂ + v · Δt
```

As coordenadas X e Y são limitadas à área. A previsão é movimento retilíneo
uniforme: ela não conhece mudanças de direção ocorridas depois da perda do
enlace. `predictedTeamPositions`, `teamPredictionAgeSum` e
`teamPredictionAgeMax` tornam a extrapolação auditável.

## 8. Função de aptidão

Para uma posição candidata `p`, posição atual do drone `p_d`, posição estimada
da equipe `p_e` e ponto do obstáculo `p_o`:

```text
C_link(p) = clamp(||p − p_e|| / 1000, 0, 1)
C_prox(p) = exp(−||p − p_o|| / 10)
C_obs(p)  = max(C_prox(p), obstruído(p,p_e) ? 1 : 0)
C_move(p) = clamp(||p − p_d|| / 40, 0, 1)

F(p) = 0,60 C_link + 0,25 C_obs + 0,15 C_move
```

O BA minimiza `F(p)`. O enlace recebe maior peso, seguido por obstáculo e custo
de movimento. Antes de avaliar a aptidão, o candidato deve satisfazer:

- distância ao drone ≤ 40 m;
- limites da área e altitude entre 6 e 20 m;
- margem mínima de 1 m da superfície detectada;
- trajeto drone–candidato sem interseção;
- linha candidato–equipe estimada sem interseção;
- chegada antes do limite de voo.

Com raio de 25 m a geometria deixava uma região viável estreita demais. O raio
de 40 m permite contornar lateralmente o edifício sem alterar o rádio.

## 9. Bat Algorithm

São usados 20 morcegos virtuais e 50 iterações. Cada morcego tem posição `x`,
velocidade `v`, frequência `f`, amplitude `A` e taxa de pulsos `r`.

Inicialização: posições são amostradas uniformemente no volume da esfera de
40 m, usando `raio · U^(1/3)`, e candidatos inviáveis são descartados.

Atualização principal:

```text
f_i = f_min + (f_max − f_min) β,                 β ~ U(0,1)
v_i(t) = v_i(t−1) + (x_i(t−1) − x*) f_i
x_i(t) = x_i(t−1) + v_i(t)
```

`x*` é a melhor solução conhecida. Na busca local:

```text
x_novo = x* + esfera(40 · escala_local · amplitude_média)
```

Uma solução viável é aceita quando não piora a aptidão do morcego e uma
amostra uniforme é menor que sua amplitude. Após aceitação:

```text
A_i(t+1) = 0,9 A_i(t)
r_i(t)   = r₀ [1 − exp(−0,9(t+1))]
```

A melhor posição é percorrida gradualmente a 13 m/s horizontalmente, 5 m/s em
subida e 3 m/s em descida. Não há teletransporte.

## 10. Resultado observado

| Métrica, cinco seeds | BA desligado | BA ligado |
|---|---:|---:|
| AppACK | 0/5 | 5/5 |
| Expirações | 5 | 0 |
| Tentativas | 40 | 5 |
| Ativações do BA | 0 | 5 |
| Distância real total | 0 m | 157,569 m |
| Distância comandada total | 0 m | 174,838 m |
| `hopCount:mean` nas entregas | sem entrega | 0 |

A distância real média foi 31,51 m; a distância comandada média foi 34,97 m.
O controle permaneceu obstruído; no tratamento, o movimento restabeleceu um
enlace direto. O AODV entregou o pacote que estava enfileirado assim que a
conectividade retornou. A diferença entre as distâncias existe porque o ACK
interrompe o movimento antes da chegada ao candidato.

Os ACKs chegaram durante o movimento e antes de uma tentativa posterior à
chegada. Portanto, o resultado sustenta **recuperação do enlace causada pelo
movimento do BA**, mas não valida especificamente a posição final calculada.
Essa distinção aparece em `repositionAckedBeforeValidation` e
`successfulRepositions`.

Uma auditoria PCAPNG da seed 0 confirmou, no braço ligado, 1/1 `VictimAlert`,
1/1 `VictimAck` e `hopCount = 0`. As atualizações de posição observadas foram
45/1200 no controle e 1072/1200 com BA. A captura é diagnóstico de uma seed; a
conclusão primária continua baseada nos cinco pares de escalares.

Os cinco pares são discordantes na direção favorável ao BA (5 melhoras, zero
regressões). O teste exato bilateral de McNemar resulta em `p = 0,0625`.
Portanto, o efeito observado é compatível com a hipótese e valida o mecanismo,
mas este piloto **não rejeita H0 ao nível de 5% em teste bilateral**. Um teste
unilateral não deve ser escolhido depois de observar os dados.

## 11. Variáveis e validade

- variável independente: `baEnabled`;
- variável primária: AppACK;
- secundárias: perdas, tentativas, expiração, atraso e distância;
- diagnósticas: degradação, sensor, ativações, previsões e hop count.

A inferência vale para esta geometria, trajetória e configuração de rádio. O
piloto não demonstra superioridade geral do BA, não compara outros algoritmos,
não modela erro de GPS, vento, bateria ou controle de voo e não mede energia
real. Um estudo confirmatório deve variar obstáculos e trajetórias, aumentar o
número de seeds e preservar o controle pareado.

Além disso, o raio de 40 m foi escolhido durante o desenvolvimento deste mesmo
piloto. Esses cinco pares são exploratórios e não devem ser reutilizados como
amostra confirmatória. Demonstrar vantagem específica do BA também exige um
controle ativo com outro método de reposicionamento; o braço atual compara BA
com ausência de movimento.

## 12. Reprodução

```bash
make hypothesis-pilot
python3 analysis/report_hypothesis_pilot.py
```

PCAPNG é opcional para auditoria de quadros e rotas. Os escalares OMNeT++ são a
fonte das métricas agregadas. O relatório também gera
`simulations/results/pilot_manifest.json`, com revisão Git e hashes SHA-256 dos
insumos e dos dez arquivos `.sca`. `simulations/results/pilot_runs.csv` contém
uma linha auditável por configuração e seed.
