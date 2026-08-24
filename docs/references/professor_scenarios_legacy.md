# Cenários científicos da professora — arquivo histórico

> Documento anterior à simplificação do protocolo. Não é normativo e contém
> gatilhos, métricas e valores removidos. Consulte os quatro documentos em
> `docs/` e as configurações `.ini` para o estado atual.

## Pergunta e hipóteses

A pergunta operacional é: **quando o enlace direto entre o drone que detecta
uma vítima e as equipes degrada, o reposicionamento escolhido pelo Bat
Algorithm aumenta o atendimento sem depender de encaminhamento multihop?**

- H0: ligar o BA não altera atendimento, perdas ou atraso frente ao controle.
- H1: o BA aumenta atendimento e/ou reduz perdas e atraso.
- Hipótese auxiliar: o multihop também recupera alertas, mas por mecanismo
  diferente e identificável pelo número de saltos.

O experimento confirmatório possui apenas dois braços: `MainExperiment_BaOff`
usa enlace direto sem reposicionamento e `MainExperiment_BaOn` muda somente
`baEnabled`. Assim, a diferença pareada `BaOn−BaOff` estima o efeito do mecanismo
de movimento. O cenário `Multihop` é uma referência opcional para outra pergunta
— o efeito do roteamento — e não integra o teste principal.

## Hierarquia do escopo

1. **Confirmatório:** uma vítima, carga fixa e BA Off/On pareados.
2. **Robustez:** variação de equipes e vítimas, preservando BA Off/On.
3. **Complementar:** multihop, escala, PCAP e diagnósticos MAC/IP.

As conclusões sobre a hipótese vêm do primeiro nível. Os demais explicam limites
e mecanismos, sem aumentar retrospectivamente a pergunta principal.

## Desenho solicitado

| Item | Cenário 1A | Cenário 1B |
|---|---:|---:|
| Área | 1000 × 1000 × 20 m | igual |
| Drones | 4 | 4 |
| Vítimas fixas | 1 | 2 |
| Drones solicitantes | 1 (drone 2) | 2 (drones 2 e 1) |
| Equipes | 1, 5, 10, 15 | igual |
| Obstáculos | 2 edifícios ocos, 120 × 120 × 18 m | igual |
| Velocidade dos drones | 13 m/s | igual |
| Mobilidade das equipes | `RandomWaypointMobility`, 1–2 m/s | igual |
| Mobilidade dos drones | Gauss–Markov 3D | igual |
| Rádio | 802.11b DSSS, 1 Mbit/s, 2,4 GHz | igual |

A configuração preliminar executa 3 repetições de 450 s por combinação. Isso
serve para encontrar erros e observar variabilidade; não sustenta conclusão
inferencial. Para o artigo, alterar em `professor-common.ini` para 30 repetições
e 900 s, como determina o PDF.

## Mobilidade e referência física

O `RandomWaypointMobility` sorteia destino, velocidade terrestre e pausa; Z é
fixo em zero. O módulo nativo `GaussMarkovMobility` do INET 4.5.4 é bidimensional.
Por isso `BaGaussMarkovMobility` implementa também elevação correlacionada:

\[
v_t=\alpha v_{t-1}+(1-\alpha)\bar v+\sqrt{1-\alpha^2}\,\epsilon_v
\]

e a mesma recorrência é aplicada ao azimute \(\theta\) e elevação \(\phi\).
O deslocamento tridimensional no intervalo \(\Delta t\) é

\[
\Delta\mathbf p=v_t\Delta t
(\cos\phi\cos\theta,\cos\phi\sin\theta,\sin\phi).
\]

Os limites refletem a trajetória em X, Y e Z; a altitude permanece entre 6 e
20 m. O DJI Phantom 4 Pro V2.0 é a referência física: 13 m/s está abaixo de sua
velocidade máxima, e o reposicionamento usa 5 m/s de subida e 3 m/s de descida
do modo P. A faixa abstrata do sensor frontal é 0,7–30 m. Peso, bateria e câmera
não entram no balanço de energia: essa ausência é uma limitação explícita.

## Rádio, obstáculo e FSPL

Em espaço livre, o INET calcula

\[
FSPL(dB)=20\log_{10}(d)+20\log_{10}(f)+20\log_{10}(4\pi/c).
\]

O FSPL não é calibrado para “caber” na área. Distância, frequência, potência,
sensibilidade, ruído e perda dielétrica determinam o enlace. Com 20 mW e
−85 dBm, enlaces livres podem cobrir grande parte de 1 km²; logo, o fenômeno do
Cada edifício tem quatro paredes e cobertura de concreto com 0,30 m de
espessura. O envelope continua grande, mas um enlace atravessa paredes reais,
não um bloco de até 120 m de concreto maciço. No material padrão do INET, uma
parede de 0,30 m produz aproximadamente 4,6 dB de absorção dielétrica em
2,4 GHz, além da reflexão calculada pelo modelo. O BA só aparece quando há
degradação **e** obstáculo confirmado no alcance do sensor; a geometria não
garante ativação em toda seed porque os nós se movem.

## Gatilho e Bat Algorithm

Atualizações da equipe alimentam uma janela de enlace e sua posição futura é
estimada pelas últimas posições, velocidades e direções. O PDR desses beacons
combina o tempo durante o qual a equipe foi efetivamente observável com a
amplitude das sequências recebidas. O denominador cresce até completar a janela:
uma equipe recém-descoberta não é penalizada por beacons anteriores à descoberta,
enquanto lacunas internas de sequência e silêncio no fim da janela continuam
contando como perdas. A janela temporal é semiaberta, evitando contar as duas
extremidades. Há indicação de degradação quando esse PDR ou o RSSI cruza os
limiares configurados. O BA só inicia se:

Os drones começam sem diretório de equipes. Cada entrada temporária é criada
somente quando um `PositionUpdate` broadcast é recebido: o identificador e a
posição vêm do payload, e o IP vem do endereço de origem indicado pelo INET.
Sem novos broadcasts, a entrada expira após `teamEntryLifetime` (30 s no
cenário principal). Como esses broadcasts têm TTL 1, apenas equipes diretamente
ouvidas ficam disponíveis; AODV não descobre sozinho o IP de uma equipe remota.

1. existe alerta ainda pendente;
2. existe posição atual ou prevista da equipe;
3. a degradação está indicada;
4. o sensor confirma um obstáculo na linha de visada e entre 0,7 e 30 m;
5. ainda existem ciclos e tempo de voo disponíveis.

O item 4 é obrigatório nos cenários científicos (`requireObstacleConfirmation
= true`). O parâmetro pode ser desligado somente em uma ablação: nesse modo o
gatilho usa silêncio/PDR/RSSI e a aptidão ignora completamente a geometria e a
penalidade de obstáculos.

A aptidão minimizada é

\[
J(x)=0{,}60J_{link}(x)+0{,}25J_{obstáculo}(x)+0{,}15J_{movimento}(x).
\]

`J_link` é uma aproximação geométrica baseada na distância prevista até a equipe;
o otimizador não conhece RSSI, SNIR, PDR ou interferência futuros. Portanto, o BA
escolhe uma posição geometricamente promissora. Qualquer melhoria de rede é um
resultado experimental posterior ao movimento, não uma propriedade assumida da
função de aptidão.

O BA gera candidatos dentro de 120 m e entre 6–20 m, atualiza frequência,
velocidade, posição, amplitude e pulso por 20 indivíduos e 50 iterações. O
drone voa gradualmente ao melhor candidato, com eixos simultâneos; não há
teletransporte. Ele mantém hover apenas na posição candidata enquanto aguarda
validação/ACK e então retoma Gauss–Markov. Esse hover favorece estabilidade de
câmera e enlace, mas seu impacto visual não é modelado.

## Métricas e validade

Unidade experimental é a seed, nunca cada pacote. O `ExperimentMetrics`
deduplica os eventos globais por `alertId` e registra separadamente alertas
gerados, entregues a alguma equipe, confirmados por ACK e expirados. Assim,
`pdr` mede `alertsDelivered/alertsGenerated`, enquanto `confirmationRate` mede
`alertsConfirmed/alertsGenerated`. O PDR de PositionUpdate usado no gatilho e a
entrega por tentativa são métricas distintas. A soma de `uniqueAlertsReceived`
das equipes representa recepções únicas **locais** e pode repetir um `alertId`;
esses contadores locais permanecem apenas para auditoria.
As tentativas são deduplicadas globalmente por `messageId`; portanto,
`alertAttemptsDelivered/alertAttemptsSent` não soma a mesma tentativa recebida
por equipes diferentes. O ciclo de reposicionamento distingue ativação do BA,
movimento iniciado, chegada à posição, validação final, recuperação durante o
trajeto, recuperação após a chegada por uma tentativa anterior e expiração.
Cada movimento iniciado possui um identificador de ciclo. Uma tentativa enviada
na posição final só pode validar o ciclo e a posição em que foi transmitida; ACK
tardio de ciclo anterior não valida o ciclo atual. `repositionValidationRate`
mede exclusivamente `repositionsValidated/repositionsStarted` e é a medida
conservadora da posição escolhida pelo BA. `operationalRepositionRecoveryRate`
também inclui recuperação durante o movimento ou por pacote anterior e responde
a uma pergunta operacional, não causal. Os nomes históricos `successfulRepositions`
e `repositionSuccessRate` permanecem apenas como aliases dessa segunda definição.
Pelo mesmo motivo, `validatedRecoveryTime*` mede somente ciclos validados, enquanto
`operationalRecoveryTime*` inclui todas as recuperações posteriores ao início do
movimento. Comparações pré/pós de PDR e RSSI são registradas apenas na validação
causal; pacotes antigos não entram nessas somas.

Razões sem denominador — por exemplo, execução sem alerta, tentativa ou movimento
— são registradas como indefinidas (`NaN`), nunca como sucesso ou perda de 0%/100%.
RSSI médio global é calculado agregando potência em escala linear por amostra e
só depois convertendo para dBm. Saltos usam soma/contagem global por execução, e
não média não ponderada das médias de cada módulo.
Diagnósticos incluem RSSI, quantidade de tags RSSI presentes/ausentes, PDR,
roteadores intermediários, ativações do BA, distância de reposicionamento,
confirmações do sensor e descartes MAC/IP. `hopCount=0` significa entrega
direta. Valores positivos só existem depois que o originador conhece o IP e o
AODV constrói uma rota; o broadcast local, sozinho, não fornece descoberta
multihop. Deve valer a conservação `gerados = atendidos + expirados`.

O sensor de obstáculos é abstrato: consulta a geometria exata na linha de visada
orientada para a equipe. Campo de visão, atitude da aeronave, iluminação, falsos
positivos e falsos negativos não são modelados e limitam a validade externa.

PCAPNG é auditoria de rede, não substitui os escalares de aplicação: habilite
com `--pcap`; `make professor-pcap` captura todos os nós de todas as seeds.
No lote final, arquive INI, commit, seeds, `.sca`, PCAPNG e hashes. Com n=30,
compare braços pareados por seed e reporte efeito, intervalo de confiança e
distribuição; não selecione seeds após observar resultados.

## Execução

```bash
make ba-smoke-test             # teste determinístico da integração, não evidência
make network-discovery-validation # descoberta direta e limite do relay
make experiment                 # contraste confirmatório BA Off/On
make robustness-experiment      # variações de equipes e vítimas
make optional-multihop          # referência de roteamento
make optional-pcap              # auditoria de pacotes
make optional-scaling           # sonda exploratória
```

O relatório confirmatório gera uma linha por seed, calcula `BaOn−BaOff` e recusa
execuções não pareadas ou com deriva de parâmetros além de `baEnabled`. Ele também
interrompe a análise se o tratamento não contiver degradação, confirmação do
sensor, ativação do BA e movimento. Ausência de exposição ao mecanismo exige
revisão prospectiva do cenário, não interpretação como “BA sem efeito”.

`BA_SmokeTest` usa uma parede com `IdealObstacleLoss` somente para verificar a
cadeia degradação → sensor → BA → voo → ACK. A equipe começa visível e cruza a
parede antes da detecção. O teste falha automaticamente se não houver ativação,
deslocamento e recuperação direta. Ele não deve ser agregado aos cenários da
professora nem citado como estimativa de desempenho.

### Sonda de escala 2 × 2

`make professor-scaling-test` compara 1/40 vítimas e 1/20 barreiras com cinco
equipes, 450 s e três seeds. O controle mantém BA desligado; no ponto extremo
há também um braço pareado com BA ligado. As detecções são distribuídas entre
30–300 s. Esta sonda verifica mecanismo e variabilidade, mas `n=3` não autoriza
inferência confirmatória. Os resultados ficam em
`analysis/figures/professor_scaling_{runs,summary}.csv`.

Resultados exploratórios anteriores à descoberta dinâmica e ao novo PDR não
são comparáveis com a implementação atual e foram removidos deste documento.
Qualquer tabela científica deve ser regenerada a partir do commit final.

Arquivos gerados: `analysis/figures/professor_{runs,summary}.csv`, escalares em
`simulations/results/omnetpp/` e capturas em `simulations/results/pcap/`.
