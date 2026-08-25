# Análise

Os arquivos Python são organizados pelo domínio que atendem:

- `core/`: leitura de `.sca`, métricas centrais, estatística e manifesto;
- `reports/`: experimento confirmatório e robustez opcional;
- `validation/`: contratos automáticos dos cenários técnicos;
- `tests/`: testes unitários da análise;
- `figures/`: artefatos derivados e ignorados pelo Git.

As saídas também são separadas por domínio:

```text
figures/
  main_experiment/  # runs, efeitos pareados, resumo e exposição
  robustness/       # extensão não confirmatória
```

`reports/report_main_experiment.py` é a entrada científica principal. Ele exige
seeds pareadas, igualdade de parâmetros exceto `baEnabled` e todos os escalares
centrais. `reports/report_robustness.py` reutiliza esse contrato para as
variações de vítimas e equipes, sem misturá-las à inferência confirmatória.

Atualmente não há gerador de PNG: o plot histórico foi removido porque dependia
de métricas extintas. Um novo gráfico só deve ser implementado quando representar
um resultado científico definido, sem converter ausência de exposição ao BA em
uma visualização enganosa.
