# Análise

Os arquivos são organizados pelo domínio que atendem:

- `core/`: leitura de `.sca`, estatística e métricas compartilhadas;
- `reports/`: tabelas dos cenários científicos e da sonda de escala;
- `pcap/`: decodificação, planilhas e comparação SCA versus PCAPNG;
- `validation/`: contratos automáticos dos cenários técnicos;
- `plots/`: gráficos específicos derivados dos relatórios;
- `tests/`: testes unitários da análise e da auditoria PCAP;
- `figures/`: saída gerada, ignorada pelo Git.

Use `make professor-scenarios`, `make professor-scaling-test` e
`make professor-pcap`. Três seeds são somente validação preliminar; o protocolo
final exige 30.
