# Simulações

O diretório mantém a rede NED, a configuração global e os dados de ambiente no
mesmo local para que os caminhos de `include` e `xmldoc()` permaneçam simples e
reproduzíveis.

## Experimento principal

- `main-experiment.ini`: contraste pareado mínimo entre BA desligado e ligado;
- `professor-common.ini`: parâmetros físicos e de protocolo compartilhados;
- `scenario-1-one-victim.ini`: cenário-base herdado pelo experimento principal.

`make experiment` executa somente os dois braços principais. Eles usam comunicação
direta e devem diferir exclusivamente por `baEnabled`.

## Robustez

- `scenario-1-two-victims.ini`: cenário com duas vítimas;
- variações de `numTeams` nos cenários `Scenario1_*`.

Esses casos verificam se a conclusão principal resiste à mudança de carga, mas
não ampliam a hipótese confirmatória.

## Extensões opcionais

- configurações `Multihop`: referência de roteamento AODV, não terceiro braço da
  pergunta principal;
- `professor-scaling-test.ini`: sonda exploratória de vítimas e obstáculos;
- PCAPNG: auditoria de pacotes habilitada sob demanda;
- `professor-scenario-obstacles.xml` e `professor-scaling-obstacles-*.xml`:
  ambientes físicos usados nos cenários.

O arquivo `omnetpp.ini` continua sendo o ponto único de entrada e inclui os
arquivos de configuração por domínio.

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
