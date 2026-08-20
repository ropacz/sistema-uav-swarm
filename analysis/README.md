# Ferramentas de análise

- `process_results.py`: processa escalares OMNeT++, aplica os portões de
  integridade do experimento pareado e gera as tabelas e figuras científicas.
- `validate_results.py`: asserções sobre os cenários determinísticos.
- `pcap_batch_to_spreadsheet.py`: consolida capturas em planilha auditável.
- `pcap_core.py`: leitura e correspondência dos PCAPs.
- `tests/`: testes determinísticos dos decodificadores e das métricas de PCAP.

## Portões de integridade

`process_results.py` termina com erro, em vez de emitir uma tabela vazia,
quando o experimento está ausente ou incompleto, quando há seeds duplicadas ou
desemparelhadas, ou quando os dois braços divergem em qualquer parâmetro além
de `baEnabled` — verificado contra os parâmetros gravados nos próprios `.sca`.

Configurações fora do experimento (validação, demonstração, capturas de rede)
são tabuladas à parte e nunca entram nas tabelas científicas.

Nenhum teste de hipótese é aplicado automaticamente: a saída traz o tamanho do
efeito, seu intervalo e as contagens de pares discordantes.

```bash
python3 analysis/process_results.py
python3 analysis/process_results.py --expected-pairs 3   # piloto, marcado no manifesto
python3 -m unittest discover -s analysis/tests -v
```

## Localização dos artefatos

Escalares em `simulations/results/omnetpp/`, capturas em
`simulations/results/pcap/`, eventlogs em `simulations/results/eventlogs/` e
planilhas em `simulations/results/spreadsheets/`. As tabelas e figuras de
`process_results.py` ficam em `analysis/figures/`. Nada disso é versionado.
