# Análise

Os arquivos Python são organizados pelo domínio que atendem:

- `core/`: leitura de `.sca`, métricas centrais, estatística e manifesto;
- `reports/`: experimento confirmatório, robustez, figuras e planilha;
- `validation/`: contratos automáticos dos cenários técnicos;
- `tests/`: testes unitários da análise;
- `tables/` e `figures/`: artefatos derivados, ignorados pelo Git.

As saídas são separadas por natureza, e não só por domínio:

```text
tables/
  main_experiment/   # runs, efeitos pareados, resumo e exposição
  robustness/        # extensão não confirmatória
  verificacao.xlsx   # planilha de verificação da configuração executada
figures/
  efeito_pareado.pdf     # resultado confirmatório, com IC de 95%
  funil_exposicao.pdf    # alcance do mecanismo por etapa
  efeito_por_equipes.pdf # robustez à quantidade de equipes
```

`reports/report_main_experiment.py` é a entrada científica principal. Ele exige
seeds pareadas, igualdade de parâmetros exceto `baEnabled` e todos os escalares
centrais. `reports/report_robustness.py` reutiliza esse contrato para as
variações de vítimas e equipes, sem misturá-las à inferência confirmatória.

## Figuras e planilha

`make deliverables` gera as figuras e a planilha a partir das tabelas já
existentes, sem reexecutar simulação.

`reports/figures.py` não recalcula nada: todo número exibido vem de um CSV
produzido pelos relatórios. As figuras saem em PDF vetorial e **sem título
embutido**, porque na ABNT o título é legenda acima da figura e a fonte vem
abaixo, ambos escritos no LaTeX. Métricas em unidades diferentes não dividem o
mesmo eixo — atrasos, retransmissões e saltos ficam nas tabelas, não no gráfico
de efeito pareado.

`reports/verification_sheet.py` lê os parâmetros que o OMNeT++ gravou num `.sca`
real, não o que a documentação afirma; é isso que a torna uma verificação. Cada
linha é classificada como conforme, desvio justificado ou não especificada pela
diretriz, com a seção correspondente. Os desvios conhecidos são marcados
explicitamente: a comparação textual classificaria `RandomWaypointMobility` como
"Random Walk" conforme, escondendo justamente o que a planilha existe para
revelar.
