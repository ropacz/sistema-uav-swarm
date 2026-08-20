# Ferramentas de análise

- `process_results.py`: processa escalares OMNeT++ e gera estatísticas e figuras.
- `validate_results.py`: valida invariantes dos resultados experimentais.
- `pcap_batch_to_spreadsheet.py`: consolida nós, execuções e estatísticas multiseed.
- `pcap_core.py`: módulo interno de leitura e correspondência dos PCAPs.
- `tests/`: testes determinísticos dos decodificadores e das métricas.

Escalares de simulação ficam em `simulations/results/omnetpp/`, capturas em
`simulations/results/pcap/`, eventlogs em `simulations/results/eventlogs/` e
planilhas em `simulations/results/spreadsheets/`. Gráficos e tabelas
intermediárias gerados por `process_results.py` ficam em `analysis/figures/` e
não são versionados.

Para executar os testes:

```bash
python3 -m unittest discover -s analysis/tests -v
```
