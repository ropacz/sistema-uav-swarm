# Auditoria: atendimento e perda via `.elog`, comparado ao `.sca`

Pergunta que este diretório responde: **os números da planilha oficial
(`analysis/tables/atendimento.xlsx`, derivada dos sinais da aplicação) batem
com uma reconstrução feita a partir do log bruto do kernel de simulação, sem
usar em nada o C++ que calcula a métrica?**

## Quais logs foram usados, e por quê

| Log | Usado? | Motivo |
| --- | --- | --- |
| **Event log (`.elog`)** | **Sim** | É o único log do OMNeT++ que registra, por padrão, a criação/envio/entrega/descarte de **cada mensagem**, no nível do kernel — sem precisar rodar a simulação de novo com instrumentação especial. `record-eventlog = true` liga. |
| PCAP (`PcapRecorder`) | Não | Grava quadro na interface (camada 2/3/4). Não sabe o que é `VictimAlert`, nem enxerga a lógica de aceite/rejeição da aplicação (TTL, equipe alvo, endereço da tentativa) — mesma limitação do `.elog` bruto, só que com um passo a mais de dissecação binária. Ver discussão na conversa. |
| Texto do Cmdenv (`EV_INFO`) | Não | Não estruturado, pensado para leitura humana durante depuração, não para parsing em lote. |
| `eventlog-message-detail-pattern` | Testado e descartado | Permitiria extrair campos do payload (`alertId` etc.) direto do `.elog`, mas **trava a simulação** nesta versão do INET/OMNeT++ ao tentar introspectar objetos internos da camada de rádio (`SequenceChunkDescriptor`, `WirelessSignal`) — reproduzido com `*Chunk` e também com o padrão restrito a `VictimAlertChunk`/`VictimAckChunk`. Erro: `check_and_cast(): Cannot cast nullptr to type 'const inet::physicallayer::IRadio *'`. |

## Como o `alertId` chegou no `.elog` sem tocar no protocolo

O `.elog` registra o **nome** de cada mensagem (`cMessage::getName()`), mas não
os campos internos do chunk. Os pacotes `VictimAlert`/`VictimAck` agora nascem
com o nome `"VictimAlert:<alertId>"` / `"VictimAck:<alertId>"`
(`DroneApp::sendAttempt`, `TeamApp::handleVictimAlert`) — o despacho por
`socketDataArrived`, que antes comparava a string inteira, passou a comparar só
o prefixo (`hasTypePrefix`, presente nos dois `.cc`).

Isso é metadado do simulador: **não faz parte do wire format serializado**
(`SarMessageSerializers.cc`, `WIRE_VERSION`, ficaram intocados) e não muda
nenhuma decisão de protocolo — só dá identidade visível ao log. Verificado:
os 8 smoke tests obrigatórios (§28) passam sem alteração depois da mudança.

## Metodologia da reconstrução (`eventlog_metrics.py`)

O sinal usado é a linha `DM` (*dispose message* — quando um módulo processa e
descarta o pacote):

- **gerado**: primeira linha `CM` (criação) com nome `VictimAlert:<id>` —
  uma por tentativa real de envio (`sendAttempt` só cria o pacote se a equipe
  estiver conhecida naquele instante);
- **entregue**: primeira `DM` de `VictimAlert:<id>` cujo módulo consumidor é
  uma instância de `echosar::TeamApp`;
- **confirmado**: primeira `DM` de `VictimAck:<id>` cujo módulo consumidor é
  uma instância de `echosar::DroneApp`.

Isso é **exatamente o limite do método**, declarado de propósito: `TeamApp` e
`DroneApp` chamam `delete packet;` tanto quando aceitam quanto quando
**rejeitam** um pacote (TTL vencido, equipe errada, ACK que não bate com a
tentativa). O log de kernel não distingue as duas coisas — só o sinal da
aplicação (usado no `.sca`) sabe.

## Execuções auditadas

5 seeds de cada braço do experimento principal (`MainExperiment_BaOff`/`_BaOn`,
seeds 0–4) — as **mesmas** execuções que já fazem parte da campanha oficial,
rodadas de novo só com `--record-eventlog=true` ligado (não muda RNG nem
resultado, só acrescenta o log). Cada execução gera ~130 MB de `.elog`
(regenerável, por isso `analysis/audit/raw/` está no `.gitignore` — rode
`run_audit.sh` de novo para reproduzir).

## Resultado

```
config                seed  sca_ger  elog_ger  sca_atend%  elog_atend%  só_no_sca  diferentes
MainExperiment_BaOff  0     25       24        76.0        79.17        1          0
MainExperiment_BaOff  1     25       23        92.0        100.0        2          0
MainExperiment_BaOff  2     25       25        76.0        76.0         0          0
MainExperiment_BaOff  3     25       25        96.0        96.0         0          0
MainExperiment_BaOff  4     25       23        64.0        69.57        2          0
MainExperiment_BaOn   0     25       24        76.0        79.17        1          0
MainExperiment_BaOn   1     25       23        92.0        100.0        2          0
MainExperiment_BaOn   2     25       25        76.0        76.0         0          0
MainExperiment_BaOn   3     25       25        96.0        96.0         0          0
MainExperiment_BaOn   4     25       23        72.0        78.26        2          0

agregado: 250 alertas (.sca) vs 240 alertas (.elog)
          10 só no .sca | 0 divergências de delivered/acked entre os 240 em comum
```

Tabela completa por execução: [`comparison/comparativo.csv`](comparison/comparativo.csv).

### Leitura

**Zero divergência** em `delivered`/`acknowledged` nos 240 alertas presentes
nas duas fontes — toda vez que um pacote existiu na rede, as duas contagens
concordam sobre o desfecho. Isso valida, por um caminho totalmente
independente do C++ que grava o `.sca`, que os sinais da aplicação refletem
corretamente o que aconteceu na rede.

A única diferença — 10 alertas que aparecem no `.sca` e **não existem em
lugar nenhum** do `.elog` — não é erro de nenhuma das duas contagens. É um
caso legítimo: o `startAlertCycle` gera o alerta (emite
`alertGeneratedSignal`), mas **nenhuma tentativa chega a virar pacote** porque
`selectTargetTeam()` não encontra equipe válida em nenhuma oportunidade de
envio antes do fim da simulação — confirmado pelo escalar
`expiredKnownTeamSelectionEvents` (18 no seed 0 do braço ligado, coerente com
o corredor de equipe intermitente perto do fim das execuções). Nesse caso, o
`alertId` **nunca existiu como pacote** — nenhum log de rede, `.elog` ou PCAP,
poderia vê-lo, porque literalmente nada foi transmitido.

Consequência prática: `elog_atend%` fica sistematicamente **um pouco acima**
de `sca_atend%`, porque o denominador do `.elog` (alertas que viraram pacote)
é menor que o denominador correto (alertas que a aplicação gerou). É viés
mensurável, na direção esperada, e serve como demonstração concreta de por
que a métrica oficial usa o sinal da aplicação — que sabe a diferença entre
"a vítima foi atribuída" e "um pacote saiu para a rede" — e não um log que só
enxerga o segundo.

## Como reproduzir

```bash
# 1. Recompilar (necessário só a primeira vez, para os nomes de pacote novos)
opp_env run inet-4.5.4 -w <workspace> --no-isolated -c "make MODE=debug"

# 2. Gerar os .elog + CSVs de referência (~10 execuções, ~10 min)
bash analysis/audit/run_audit.sh

# 3. Comparar
python3 analysis/audit/compare.py
```
