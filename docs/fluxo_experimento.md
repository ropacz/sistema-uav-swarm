# Funcionamento do Experimento — Esclarecimentos para a Dissertação

---

## 1. Quem envia o quê para quem?

O ponto central a esclarecer: **o ALERTA parte do DRONE, não da equipe.**

A equipe de resgate não sabe onde estão as vítimas. São os drones que sobrevoam
a área e detectam vítimas. Ao detectar, o drone envia o alerta para a equipe.
A equipe, ao receber, confirma com um ACK.

```
DRONE detecta vítima
    │
    │  VictimAlert (256 B)
    ▼
EQUIPE recebe → processa → envia confirmação
    │
    │  VictimAck (64 B)
    ▼
DRONE recebe confirmação → remove alerta da fila de retransmissão
```

---

## 2. Fluxo completo de mensagens

### 2.1 Beacon periódico (equipe → drones)

Antes de qualquer alerta, as equipes transmitem periodicamente sua posição e
disponibilidade em broadcast. Isso permite que os drones saibam onde estão as
equipes e se elas estão livres para atender.

```
EQUIPE  ──[TeamUpdate broadcast, 128 B, a cada 1 s]──▶  todos os DRONES
```

- **Por que:** os drones precisam saber o endereço IP e a posição da equipe
  mais próxima antes de enviar um alerta.
- **ACK:** não há ACK para TeamUpdate — é um beacon broadcast.

### 2.2 Status do drone (drone → equipe)

Os drones também reportam sua posição periodicamente à equipe conhecida mais
próxima.

```
DRONE  ──[DroneStatus unicast, 128 B]──▶  EQUIPE
```

- **ACK:** não há ACK para DroneStatus.

### 2.3 Detecção de vítima e alerta (drone → equipe)  ← **o "alerta"**

Quando um drone detecta uma vítima (evento sintético com intervalo médio de
40 s, distribuição exponencial), ele:

1. Seleciona a equipe **disponível mais próxima** na sua tabela.
2. Envia o `VictimAlert` (256 B com coordenadas da vítima) via unicast AODV.

```
DRONE  ──[VictimAlert unicast, 256 B]──▶  EQUIPE
```

Se nenhuma equipe está na tabela do drone (rede muito esparsa), o alerta é
encaminhado em broadcast para drones vizinhos, que repassam com a mesma lógica
(**relay**).

### 2.4 Confirmação (equipe → drone)  ← **o ACK**

Ao receber o `VictimAlert`, a equipe:

1. Registra a ocorrência e calcula o tempo de deslocamento até a vítima.
2. Marca-se como **ocupada** pelo período de deslocamento + atendimento.
3. Envia o `VictimAck` (64 B) **de volta ao drone de origem** via AODV
   multi-hop (usando o `originIp` do alerta).

```
EQUIPE  ──[VictimAck unicast, 64 B]──▶  DRONE ORIGEM
```

O drone, ao receber o ACK, cancela as retransmissões agendadas para aquele
alerta.

### 2.5 Retransmissão store-forward (drone)

Se o drone não recebe ACK em 10 s, retransmite o alerta para outra equipe
(excluindo as já tentadas). Máximo de 5 tentativas. Após esgotar, o alerta
expira e é contabilizado como perdido.

---

## 3. Resumo do fluxo em tabela

| Passo | Quem envia | Para quem | Mensagem | Tamanho | Porta | ACK? |
|------:|---|---|---|---|---|---|
| 1 | Equipe | Drones (broadcast) | TeamUpdate | 128 B | 5001 | Não |
| 2 | Drone | Equipe (unicast) | DroneStatus | 128 B | 5003 | Não |
| 3 | Drone | Equipe (unicast) | VictimAlert | 256 B | 5000 | Sim |
| 4 | Equipe | Drone origem (unicast) | VictimAck | 64 B | 5002 | — |
| 5 | Drone | Drone vizinho (relay) | VictimAlert relay | 256 B | 5004 | Não |

> **O termo "alerta" na dissertação refere-se sempre ao `VictimAlert` enviado
> pelo drone para a equipe (passo 3), nunca o contrário.**

---

## 4. Número de obstáculos físicos por cenário

| Config | Obstáculos | Modelo de path loss |
|---|---|---|
| `BasicTest` | 0 | FreeSpacePathLoss |
| `BasicTest_Piloto` | 0 | FreeSpacePathLoss |
| `Cenario_SemObstaculos` | 0 | FreeSpacePathLoss |
| `Cenario_ComObstaculos` | **5** | DielectricObstacleLoss + `obstacles.xml` |
| `Cenario_Intermediario` | **5** | DielectricObstacleLoss + `obstacles.xml` |
| `Cenario_Degradado` | **5** | DielectricObstacleLoss + `obstacles.xml` |
| `Cenario_Favoravel` | 0 | FreeSpacePathLoss |

Os 5 obstáculos são blocos dielétricos de 300×300×120 m definidos em
`simulations/obstacles.xml`, representando edificações urbanas.
Cada bloco causa atenuação adicional proporcional à espessura atravessada
(`DielectricObstacleLoss`): uma travessia diagonal de ≈ 424 m gera ≈ 102 dB
de perda adicional, bloqueando praticamente qualquer enlace que o atravesse.

---

## 5. Screenshot do simulador

> **TODO:** rodar o comando abaixo para gerar a tela limpa do Qtenv (sem barras
> de ferramentas), tirar print apenas da área de simulação e inserir aqui.

```bash
./run.sh --gui -c BasicTest_Visual
```

No Qtenv: menu **View → Hide toolbar** e **View → Hide status bar** para
remover os elementos de interface antes do print.

Sugestão de momento para o screenshot: após ≈ 60 s de simulação, quando os
drones estão distribuídos pela área e pelo menos um alerta já foi entregue
(setas visíveis na tela).
