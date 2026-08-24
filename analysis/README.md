# Análise

Os arquivos são organizados pelo domínio que atendem:

- `core/`: leitura de `.sca`, estatística e métricas compartilhadas;
- `reports/`: tabelas dos cenários científicos e da sonda de escala;
- `pcap/`: decodificação, planilhas e comparação SCA versus PCAPNG;
- `validation/`: contratos automáticos dos cenários técnicos;
- `plots/`: gráficos específicos derivados dos relatórios;
- `tests/`: testes unitários da análise e da auditoria PCAP;
- `figures/`: saída gerada, ignorada pelo Git.

Use `make experiment` para o relatório confirmatório pareado. Use
`make robustness-experiment`, `make optional-scaling` e `make optional-pcap`
somente para extensões. Os relatórios são gravados em `analysis/figures/`.

`reports/report_main_experiment.py` é a entrada principal. Ele falha se as seeds
não estiverem pareadas ou se os braços diferirem por parâmetros além de
`baEnabled`. Os relatórios antigos e os diagnósticos por camada permanecem como
análises complementares.
