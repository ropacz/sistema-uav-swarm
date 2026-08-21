# Ferramentas de análise

- `report_hypothesis_pilot.py`: relatório e portões de integridade do piloto;
- `process_results.py`: entrada compatível e funções compartilhadas para SCA;
- `network_metrics.py`: diagnóstico MAC, IP, rádio e transporte;
- `pcap_batch_to_spreadsheet.py` e `pcap_core.py`: auditoria PCAPNG;
- `tests/`: testes dos decodificadores e da agregação PCAP.

```bash
python3 analysis/report_hypothesis_pilot.py
python3 analysis/network_metrics.py simulations/results/omnetpp \
  --configs HypothesisPilot_BaOff HypothesisPilot_BaOn
python3 -m unittest discover -s analysis/tests -v
```

O relatório exige cinco seeds em cada braço, um alerta por execução, escalares
obrigatórios, pareamento de seed, igualdade de configuração exceto
`baEnabled` e salto direto nas entregas. Ele grava o manifesto de proveniência
em `simulations/results/pilot_manifest.json`.
