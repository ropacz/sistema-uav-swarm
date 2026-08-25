# Modelo, funcionamento e premissas

## 1. Fronteira do modelo

O ECHOSAR-Net simula UAVs que recebem detecções de vítimas, enviam alertas UDP a
equipes móveis e, quando tentativas consecutivas ficam sem ACK, podem executar
um reposicionamento calculado pelo Bat Algorithm (BA).

O modelo responde ao efeito de habilitar essa política sobre entrega fim a fim.
Ele não modela visão computacional, controle de voo de baixo nível, consumo de
bateria, decisão humana ou sucesso clínico do resgate.

## 2. Componentes

| Componente | Responsabilidade |
| --- | --- |
| `SarScenarioManager` / `StaticVictim` | gerar uma atribuição de vítima |
| `DroneApp` | manter alerta, tentativas, ACK e decisão de reposicionamento |
| `TeamApp` | anunciar posição, deduplicar tentativa e responder com ACK |
| `AbstractObstacleSensor` | produzir observação binária e ponto de superfície |
| `BatAlgorithm` | buscar um candidato viável dentro da região permitida |
| `RepositionFitness` | avaliar enlace geométrico, obstáculo e movimento |
| `BaGaussMarkovMobility` | executar o deslocamento e retomar a mobilidade normal |
| `ExperimentMetrics` | deduplicar eventos e registrar resultados do run |

## 3. Fluxo do alerta

```text
alerta gerado
    -> tentativa UDP
    -> timeout sem ACK
    -> após repositionAfterUnackedAttempts
    -> sensor binário
    -> BA (somente se baEnabled e obstáculo detectado)
    -> movimento físico
    -> tentativa imediata na chegada
    -> ACK ou expiração
```

O braço BA Off observa o mesmo gatilho e o mesmo sensor. A única intervenção
desabilitada é otimizar e mover o UAV.

Durante o movimento, o alerta proprietário não envia tentativas periódicas. Na
chegada, `DroneApp` registra conclusão e distância, envia uma tentativa antes de
retomar Gauss–Markov e libera o controlador. Um UAV executa no máximo um
movimento simultâneo e cada alerta toma no máximo uma decisão de reposicionamento.

## 4. Identidades, ACK e estado mínimo

- `alertId` identifica o evento lógico e deduplica entrega e confirmação;
- `messageId` identifica uma tentativa e vincula o ACK ao destino histórico;
- `victimId` identifica a vítima.

Para cada tentativa, o UAV preserva equipe e endereço IP. Um ACK só é aceito se
`alertId`, `victimId`, drone de origem, `messageId`, equipe e IP de origem forem
coerentes. Esse histórico permanece válido mesmo que a entrada descoberta da
equipe expire.

`TeamLinkState` contém apenas endereço, última posição recebida, instante e
sequência. Não há estimação de velocidade nem extrapolação da equipe. Entradas
sem atualização são removidas segundo `teamEntryLifetime`.

`PendingVictimAlert` contém IDs, posição da vítima, tempos de vida/envio/ACK,
número de tentativas, destino, histórico por tentativa e o estado mínimo do
único movimento possível.

## 5. Gatilho e sensor

O gatilho ocorre uma única vez quando:

\[
N_{tentativas\ sem\ ACK}\ge
\texttt{repositionAfterUnackedAttempts}.
\]

Deve valer:

\[
\texttt{repositionAfterUnackedAttempts}<\texttt{maxAttempts},
\]

reservando ao menos uma tentativa posterior ao movimento.

O sensor retorna apenas `detected` ou `notDetected` para o coletor. Razões como
visada livre ou interseção fora do alcance aparecem somente em log de depuração.
Ele usa o rumo da última posição recebida e varre esse rumo até seu alcance; isso
evita que o intervalo entre broadcasts esconda uma parede imediatamente após a
última posição, sem prever velocidade ou posição futura da equipe.

## 6. Bat Algorithm e aptidão

Cada morcego mantém posição (x_i), velocidade (v_i), frequência (f_i),
amplitude (A_i) e taxa de pulso (r_i). Em cada iteração:

\[
f_i=f_{min}+(f_{max}-f_{min})U(0,1),
\]

\[
v_i\leftarrow v_i+(x_i-x_*)f_i,
\qquad
x_i\leftarrow x_i+v_i.
\]

A busca local perturba a melhor solução e soluções melhores podem ser aceitas
segundo amplitude e pulso. Os candidatos iniciais são obtidos por rejeição em
uma bola unitária e escalados uma única vez pelo raio, produzindo amostragem
volumétrica uniforme.

A aptidão minimizada é:

\[
J(x)=w_{link}C_{link}(x)+w_{obstacle}C_{obstacle}(x)
     +w_{move}C_{move}(x),
\]

com pesos somando um. O custo de enlace é um proxy de distância à última posição
da equipe; obstáculo penaliza proximidade e bloqueio geométrico; movimento
penaliza distância ao ponto atual. Os limites espaciais, velocidades, margens e
parâmetros do BA são lidos exclusivamente das configurações `.ini`.

Um candidato só é viável se respeitar área, altitude, distância máxima, margem
do obstáculo, linha de visada e tempo de voo restante.

## 7. Premissas e limitações

- o sensor é geométrico e idealizado, não um detector embarcado calibrado;
- a última posição recebida pode estar desatualizada;
- a função de aptidão não prevê RSSI, interferência ou canal futuro;
- o experimento principal usa descoberta direta por broadcast;
- o BA pode ser ativado e não iniciar movimento se não houver candidato viável;
- o efeito científico é atribuído à política BA On/Off, não a uma classificação
  causal de cada ACK individual;
- o smoke test valida o mecanismo, mas não constitui evidência confirmatória.
