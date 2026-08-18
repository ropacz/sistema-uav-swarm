# Bat Algorithm e função de aptidão

> Formatação compatível com Markdown+Math (`mdmath`) usando o delimitador
> padrão `dollars`: `$...$` para expressões inline e `$$...$$` para equações
> em bloco. No VS Code, abra **Markdown: Open Preview** para renderizar o KaTeX.

## 1. Finalidade no ECHOSAR-Net

O Bat Algorithm (BA) procura uma nova posição tridimensional para o drone
originador de um `VictimAlert`. Seu objetivo não é controlar toda a missão nem
garantir que a comunicação melhore. Ele apenas seleciona uma posição candidata
que será testada pela simulação.

O BA somente é executado quando todas as condições abaixo são satisfeitas:

1. existe um alerta pendente sem `VictimAck`;
2. a janela da rede indica degradação;
3. a posição da equipe foi recebida por `PositionUpdate`;
4. o sensor abstrato confirma um obstáculo na linha de visada;
5. o BA está habilitado e ainda não atingiu o limite de ciclos;
6. o drone não está executando outro reposicionamento.

Essa separação evita atribuir automaticamente toda perda de comunicação a
um obstáculo.

## 2. Representação de um morcego

Cada morcego virtual representa uma posição candidata:

$$
\mathbf{p}_j=(x_j,y_j,z_j)
$$

O estado do morcego contém:

- posição $\mathbf{p}_j$;
- velocidade $\mathbf{v}_j$;
- frequência $f_j$;
- amplitude $A_j$;
- taxa de emissão de pulsos $r_j$;
- aptidão da posição atual.

A melhor posição conhecida pela população é representada por
$\mathbf{p}^{*}$. Como a aptidão é um custo, a melhor posição é aquela com
menor valor.

## 3. Inicialização da população

Os morcegos são distribuídos dentro de uma esfera centrada na posição atual
do drone $\mathbf{p}_d$, limitada por `maximumRepositionDistance`:

$$
\lVert\mathbf{p}_j-\mathbf{p}_d\rVert \leq d_{max}
$$

A amostragem utiliza a raiz cúbica de uma variável uniforme para evitar
concentrar candidatos no centro da esfera. Cada morcego possui um número
limitado de tentativas para encontrar uma posição viável. Se nenhum candidato
for válido, a posição atual do drone pode ser usada como solução de fallback,
desde que também satisfaça as restrições.

## 4. Atualizações do BA

### 4.1 Frequência

Em cada iteração, a frequência é sorteada por:

$$
f_j=f_{min}+(f_{max}-f_{min})\beta,
\qquad \beta\sim U(0,1)
$$

### 4.2 Velocidade

A implementação adota explicitamente a convenção:

$$
\mathbf{v}_j^{t}=\mathbf{v}_j^{t-1}+
(\mathbf{p}_j^{t-1}-\mathbf{p}^{*})f_j
$$

O sinal da diferença deve permanecer consistente com essa convenção no
código e na dissertação.

### 4.3 Posição

$$
\mathbf{p}_j^{t}=\mathbf{p}_j^{t-1}+\mathbf{v}_j^{t}
$$

### 4.4 Busca local

Quando uma amostra uniforme é maior que a taxa de pulsos do morcego, o
candidato é gerado ao redor da melhor posição:

$$
\mathbf{p}_{local}=\mathbf{p}^{*}+\epsilon,
$$

em que $\epsilon$ é um deslocamento aleatório dentro de uma esfera cujo raio
é proporcional a:

$$
d_{max}\,s_{local}\,\overline{A}
$$

`batLocalSearchScale` corresponde a $s_{local}$, e $\overline{A}$ é a
amplitude média da população.

### 4.5 Aceitação, amplitude e pulsos

Uma posição candidata somente substitui a posição do morcego quando:

1. sua aptidão não é pior;
2. uma amostra uniforme é menor que a amplitude atual.

Quando aceita:

$$
A_j^{t+1}=\alpha A_j^t
$$

$$
r_j^{t+1}=r_0\left(1-e^{-\gamma(t+1)}\right)
$$

Na configuração, `batAmplitudeDecay` representa $\alpha$,
`batPulseGrowth` representa $\gamma$ e `batInitialPulseRate` representa
$r_0$.

## 5. Função de aptidão

A função é normalizada e minimizada:

$$
F(\mathbf{p})=
w_{link}C_{link}(\mathbf{p})+
w_{obs}C_{obs}(\mathbf{p})+
w_{move}C_{move}(\mathbf{p})
$$

Os pesos devem ser não negativos e satisfazer:

$$
w_{link}+w_{obs}+w_{move}=1
$$

Os valores iniciais são:

| Componente | Parâmetro | Peso |
|---|---|---:|
| Comunicação | `wLink` | 0,60 |
| Obstáculo | `wObstacle` | 0,25 |
| Movimento | `wMove` | 0,15 |

Esses pesos são escolhas experimentais, não características físicas do
drone e nem valores aprendidos automaticamente.

### 5.1 Custo estimado de comunicação

$$
C_{link}(\mathbf{p})=
\operatorname{clamp}\left(
\frac{\lVert\mathbf{p}-\mathbf{p}_{team}\rVert}{d_{norm}},0,1
\right)
$$

`linkNormalizationDistance` corresponde a $d_{norm}$. Quanto maior a
distância entre o candidato e a última posição conhecida da equipe, maior o
custo.

Este termo é uma estimativa por distância. Ele não usa RSSI futuro nem executa
antecipadamente uma transmissão no INET.

### 5.2 Custo do obstáculo

Primeiro é calculada a penalidade de proximidade:

$$
C_{prox}(\mathbf{p})=
\exp\left(-\frac{\lVert\mathbf{p}-\mathbf{p}_{obs}\rVert}{\sigma_{obs}}\right)
$$

Depois, verifica-se se a linha entre o candidato e a equipe cruza algum
obstáculo:

$$
I_{LOS}(\mathbf{p})=
\begin{cases}
1, & \text{se a linha está obstruída}\\
0, & \text{caso contrário}
\end{cases}
$$

O custo final é:

$$
C_{obs}(\mathbf{p})=\max(C_{prox}(\mathbf{p}),I_{LOS}(\mathbf{p}))
$$

Assim, qualquer candidato ainda obstruído recebe custo de obstáculo igual a
1. `obstacleSigma` corresponde a $\sigma_{obs}$.

### 5.3 Custo de movimento

$$
C_{move}(\mathbf{p})=
\operatorname{clamp}\left(
\frac{\lVert\mathbf{p}-\mathbf{p}_d\rVert}{d_{max}},0,1
\right)
$$

Esse termo evita escolher deslocamentos longos quando dois candidatos possuem
qualidade de comunicação semelhante. Ele também funciona como proxy simples de
energia adicional, mas não representa um modelo eletroquímico da bateria.

## 6. Exemplo numérico

Considere um candidato com:

- distância até a equipe: 200 m;
- `linkNormalizationDistance`: 1000 m;
- distância até o obstáculo: 20 m;
- `obstacleSigma`: 10 m;
- linha de visada livre;
- deslocamento: 10 m;
- `maximumRepositionDistance`: 25 m.

Os custos são:

$$
C_{link}=200/1000=0{,}20
$$

$$
C_{obs}=e^{-20/10}\approx0{,}1353
$$

$$
C_{move}=10/25=0{,}40
$$

Logo:

$$
F=0{,}60(0{,}20)+0{,}25(0{,}1353)+0{,}15(0{,}40)
\approx0{,}2138
$$

Se a linha de visada ainda estivesse obstruída, $C_{obs}=1$, e a aptidão
subiria para aproximadamente $0{,}43$. Como o BA minimiza a função, o
candidato livre seria preferido.

## 7. Restrições de viabilidade

Uma posição é avaliada pela aptidão somente quando respeita:

- limites X e Y da área;
- altitude mínima e máxima;
- distância máxima de reposicionamento;
- margem mínima em relação ao obstáculo;
- caminho drone–candidato sem interseção com obstáculos;
- velocidades horizontal, de subida e de descida;
- chegada antes de `flightTimeLimit`.

As restrições não fazem parte da soma ponderada. Candidatos inválidos são
descartados antes do cálculo da aptidão.

## 8. Execução e validação da comunicação

Depois de selecionar $\mathbf{p}^{*}$:

1. a posição é registrada como testada;
2. a distância e o tempo de viagem são calculados;
3. `BaGaussMarkovMobility` move o drone gradualmente;
4. o drone permanece na posição enquanto aguarda validação;
5. a próxima tentativa regular do `VictimAlert` é enviada;
6. RSSI e PDR posteriores são medidos;
7. somente um `VictimAck` confirma a recuperação.

Portanto, uma aptidão baixa não é registrada automaticamente como sucesso.
O sucesso depende do comportamento efetivamente observado depois do movimento.

## 9. Parâmetros configuráveis

| Parâmetro | Finalidade |
|---|---|
| `batPopulation` | quantidade de morcegos virtuais |
| `batIterations` | iterações por execução |
| `batInitializationAttempts` | tentativas de inicialização viável |
| `batFrequencyMin`, `batFrequencyMax` | intervalo de frequência |
| `batInitialAmplitude` | amplitude inicial |
| `batInitialPulseRate` | taxa inicial de pulsos |
| `batAmplitudeDecay` | fator $\alpha$ |
| `batPulseGrowth` | fator $\gamma$ |
| `batLocalSearchScale` | escala da busca local |
| `maximumRepositionDistance` | raio $d_{max}$ |
| `obstacleSigma` | escala $\sigma_{obs}$ |
| `wLink`, `wObstacle`, `wMove` | pesos da aptidão |

## 10. Limitações e análise recomendada

- O custo de enlace usa distância normalizada e obstrução, não o modelo
  completo de propagação do INET.
- A posição da equipe é a última recebida e pode estar desatualizada.
- O custo de movimento é apenas um proxy de energia.
- O algoritmo otimiza um drone por reposicionamento; não existe otimização
  conjunta de toda a formação do enxame.
- Os pesos devem passar por análise de sensibilidade. Uma comparação inicial
  pode avaliar combinações como 0,50/0,30/0,20, 0,60/0,25/0,15 e
  0,70/0,20/0,10.

Os resultados devem ser interpretados pela comparação pareada entre BA
habilitado e desabilitado. A implementação não pressupõe melhoria da comunicação.
