# Simulações

O diretório mantém a rede NED, a configuração global e os dados de ambiente no
mesmo local para que os caminhos de `include` e `xmldoc()` permaneçam simples e
reproduzíveis.

## Cenários científicos

- `professor-common.ini`: parâmetros comuns do protocolo experimental;
- `scenario-1-one-victim.ini`: cenário com uma vítima;
- `scenario-1-two-victims.ini`: cenário com duas vítimas;
- `professor-scaling-test.ini`: sonda preliminar de vítimas e obstáculos;
- `professor-scenario-obstacles.xml` e `professor-scaling-obstacles-*.xml`:
  ambientes físicos usados nesses cenários.

As configurações científicas comparam os braços `BaOff`, `BaOn` e `Multihop`.
O arquivo `omnetpp.ini` é o ponto único de entrada e inclui todos os arquivos de
configuração.

## Validações técnicas

- `ba-smoke-test.ini`: verifica a cadeia degradação, sensor, BA, movimento e ACK;
- `network-discovery-validation.ini`: verifica descoberta direta e o limite do
  broadcast através de relay;
- `ba-smoke-test-obstacle.xml` e `ba-smoke-test-team.xml`: dados determinísticos
  usados somente pelo smoke test.

Essas configurações validam implementação. Seus resultados não constituem
evidência dos experimentos da professora.

## Resultados

`results/` é gerado durante as execuções e ignorado pelo Git. Escalares e vetores
ficam em `results/omnetpp/`; PCAPNG em `results/pcap/`; planilhas derivadas em
`results/spreadsheets/`.
