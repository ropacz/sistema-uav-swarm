# Análise

Os arquivos Python são organizados pelo domínio que atendem:

- `core/`: leitura de `.sca` e métricas centrais, usadas pelos smoke tests
  obrigatórios (§28) e pelo manifesto de proveniência;
- `reports/`: geração da planilha e das figuras de atendimento e perda;
- `validation/`: contratos automáticos dos cenários técnicos;
- `tests/`: testes unitários da análise;
- `tables/` e `figures/`: artefatos derivados, ignorados pelo Git.

> Por hora, a análise entrega apenas atendimento e perda. Estatística pareada
> detalhada (efeito por métrica, intervalo de confiança, exposição do
> mecanismo, verificação de configuração) foi removida temporariamente — ver
> histórico do Git para recuperar.

## Planilha de atendimento

A simulação grava um CSV por execução com **uma linha por `alertId`**
(`ExperimentMetrics.cc`, ao lado do `.sca`). `reports/alert_sheet.py` junta
esses arquivos com o contexto lido do `.sca` (seed, quantidade de equipes,
política ligada ou desligada) e calcula:

    Atendimento(%) = alertId com ao menos um ACK / alertId únicos gerados
    Perda(%)       = alertId sem entrega a nenhuma equipe / alertId únicos gerados

A deduplicação acontece dentro da simulação: um mesmo `alertId` recebido por
várias equipes ou retransmitido várias vezes já chega como uma única linha. Por
isso `retryCount` conta retransmissões, e não alertas.

Execuções de smoke test (`*_SmokeTest`) são excluídas: alguns desses testes
ligam a política em só um drone por desenho do próprio teste, e misturar isso
à campanha contaminaria as taxas.

```text
tables/atendimento.xlsx    # abas "Alertas" (uma linha por alerta) e "Resumo"
figures/atendimento.pdf    # taxa de atendimento por braço
figures/perda.pdf          # taxa de perda por braço
```

As duas figuras saem no mesmo comando que a planilha. O eixo horizontal é a
quantidade de equipes, de modo que a mesma figura serve a uma única célula (o
experimento principal, hoje) ou a uma matriz maior (a robustez, quando rodar),
sem trocar de formato. Cada braço tem cor e hachura próprias, legíveis em
impressão em tons de cinza, e a legenda fica acima da área do gráfico.

Legenda ABNT sugerida, para colar no LaTeX — as figuras não têm título
embutido, porque na ABNT o título é legenda acima e a fonte vem abaixo:

```latex
\begin{figure}[htb]
  \centering
  \caption{Taxa de atendimento de alertas por braço experimental}
  \includegraphics[width=\textwidth]{figuras/atendimento.pdf}
  \legend{Fonte: elaborado pelo autor (2026).}
\end{figure}
```

## Segunda via: opp_scavetool

`make scavetool-check` exporta os `.sca` da campanha oficial com a ferramenta
padrão do OMNeT++ (`opp_scavetool`, formato longo — uma linha por
escalar/parâmetro/atributo) e recalcula atendimento/perda a partir *desse*
CSV, sem passar pelo `parse_sca()` que `alert_sheet.py` usa. Serve como
confirmação independente, com uma ferramenta de terceiros em vez de código
deste projeto: verificado que os dois caminhos dão exatamente 80,7 %/84,9 %,
sem arredondamento — diferença zero.

```text
simulations/results/campanha_scavetool.csv   # exportado pelo opp_scavetool (gitignored)
analysis/tables/atendimento_via_scavetool.csv
analysis/figures/atendimento_via_scavetool.pdf
analysis/figures/perda_via_scavetool.pdf
```

## Como gerar

```bash
make experiment          # roda os dois braços e chama alert-sheet
make alert-sheet          # só a planilha e as figuras, a partir de resultados já gravados
make analysis-tests       # valida fórmulas e figuras com dados sintéticos, ~1 s
```

`tests/test_deliverables.py` roda antes de qualquer campanha: confere que
atendimento/perda seguem a definição em pontos (caso sintético com resultado
conhecido), que as figuras saem em PDF com desenho de verdade e escalam de uma
célula até uma matriz, e que smoke tests nunca entram na planilha.

## Auditoria independente (`audit/`)

`analysis/audit/` reconstrói atendimento e perda a partir do `.elog` (event
log do kernel do OMNeT++) e compara com o `.sca` — uma segunda fonte,
totalmente independente do C++ que grava a planilha oficial. Ver
[`audit/README.md`](audit/README.md) para a metodologia, quais logs foram
usados e por quê, e o resultado da comparação nas 10 execuções auditadas.
