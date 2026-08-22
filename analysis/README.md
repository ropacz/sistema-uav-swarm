# Análise

- `report_professor_scenarios.py`: casos de uma e duas vítimas por equipes/seed;
- `report_professor_scaling_test.py`: sonda 1/40 vítimas × 1/20 obstáculos;
- `validate_ba_smoke_test.py`: contrato automático do gatilho e recuperação;
- `validate_network_discovery.py`: descoberta direta e limite do relay TTL 1;
- `network_metrics.py`: diagnóstico MAC/IP/UDP;
- `pcap_batch_to_spreadsheet.py`: consolidação dos PCAPNG.
- `compare_sca_pcap_scenario1.py`: auditoria SCA versus PCAP por seed;
- `plot_scenario1_line1.py`: gráfico da execução específica de 900 s/10 seeds.

Use `make professor-scenarios`, `make professor-scaling-test` e
`make professor-pcap`. Três seeds são somente validação preliminar; o protocolo
final exige 30.
