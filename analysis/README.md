# Análise

- `report_professor_scenarios.py`: casos de uma e duas vítimas por equipes/seed;
- `report_professor_scaling_test.py`: sonda 1/40 vítimas × 1/20 obstáculos;
- `validate_ba_smoke_test.py`: contrato automático do gatilho e recuperação;
- `network_metrics.py`: diagnóstico MAC/IP/UDP;
- `pcap_batch_to_spreadsheet.py`: consolidação dos PCAPNG.

Use `make professor-scenarios`, `make professor-scaling-test` e
`make professor-pcap`. Três seeds são somente validação preliminar; o protocolo
final exige 30.
