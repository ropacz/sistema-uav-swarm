# Cenários científicos da professora

## Pergunta e hipóteses

A pergunta operacional é: **quando o enlace direto entre o drone que detecta
uma vítima e as equipes degrada, o reposicionamento escolhido pelo Bat
Algorithm aumenta o atendimento sem depender de encaminhamento multihop?**

- H0: ligar o BA não altera atendimento, perdas ou atraso frente ao controle.
- H1: o BA aumenta atendimento e/ou reduz perdas e atraso.
- Hipótese auxiliar: o multihop também recupera alertas, mas por mecanismo
  diferente e identificável pelo número de saltos.

Os três braços isolam as causas: `BaOff` usa enlace direto (TTL 1), `BaOn` muda
somente `baEnabled`, e `Multihop` mantém BA desligado com TTL 32. Assim,
`BaOn−BaOff` estima o efeito do movimento e `Multihop−BaOff` o do roteamento.

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
estimada pelas últimas posições, velocidades e direções. Há indicação de
degradação quando PDR ou RSSI cruza os limiares configurados. O BA só inicia se:

1. existe alerta ainda pendente;
2. existe posição atual ou prevista da equipe;
3. a degradação está indicada;
4. o sensor confirma um obstáculo na linha de visada e entre 0,7 e 30 m;
5. ainda existem ciclos e tempo de voo disponíveis.

A aptidão minimizada é

\[
J(x)=0{,}60J_{link}(x)+0{,}25J_{obstáculo}(x)+0{,}15J_{movimento}(x).
\]

O BA gera candidatos dentro de 120 m e entre 6–20 m, atualiza frequência,
velocidade, posição, amplitude e pulso por 20 indivíduos e 50 iterações. O
drone voa gradualmente ao melhor candidato, com eixos simultâneos; não há
teletransporte. Ele mantém hover apenas na posição candidata enquanto aguarda
validação/ACK e então retoma Gauss–Markov. Esse hover favorece estabilidade de
câmera e enlace, mas seu impacto visual não é modelado.

## Métricas e validade

Unidade experimental é a seed, nunca cada pacote. As métricas primárias são
atendimento (`ACKs únicos/alertas únicos`), perda por tentativa, atraso e
expiração. Diagnósticos incluem RSSI, PDR, saltos, ativações do BA, distância de
reposicionamento, confirmações do sensor e descartes MAC/IP. Deve valer a
conservação `gerados = atendidos + expirados`.

PCAPNG é auditoria de rede, não substitui os escalares de aplicação: habilite
com `--pcap`; `make professor-pcap` captura todos os nós de todas as seeds.
No lote final, arquive INI, commit, seeds, `.sca`, PCAPNG e hashes. Com n=30,
compare braços pareados por seed e reporte efeito, intervalo de confiança e
distribuição; não selecione seeds após observar resultados.

## Execução

```bash
make ba-smoke-test             # teste determinístico da integração, não evidência
make professor-scenarios        # 72 runs preliminares e resumo CSV
make professor-pcap             # repete os 72 runs com PCAPNG
./run.sh -c Scenario1_TwoVictims_BaOn -r 0 --pcap
python3 analysis/report_professor_scenarios.py
```

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

Resultado exploratório de 21 de agosto de 2026:

| Vítimas | Obstáculos | BA | Atendimento | Perda/tentativa | Degradações | Confirmações |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 1 | não | 100,00% | 0,00% | 0,00 | 0,00 |
| 1 | 20 | não | 100,00% | 0,00% | 0,00 | 0,00 |
| 40 | 1 | não | 100,00% | 6,88% | 0,67 | 0,00 |
| 40 | 20 | não | 94,17% | 53,60% | 46,67 | 4,33 |
| 40 | 20 | sim | 93,33% | 52,46% | 43,33 | 2,33 |

No braço BA ligado ocorreram em média 2 ativações e 0,33 reposicionamento
bem-sucedido por execução. O resultado não demonstra benefício médio do BA e
deve permanecer identificado como diagnóstico, não conclusão do artigo.

Arquivos gerados: `analysis/figures/professor_{runs,summary}.csv`, escalares em
`simulations/results/omnetpp/` e capturas em `simulations/results/pcap/`.
