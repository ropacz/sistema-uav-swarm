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

## Ambiente físico

- `professor-scenario-obstacles.xml`: obstáculos estáticos compartilhados pelos
  cenários científico e de robustez.

O arquivo `omnetpp.ini` continua sendo o ponto único de entrada e inclui os
arquivos de configuração por domínio.

## Validações técnicas

- `ba-smoke-test.ini`: reúne validações de BA, ciclo de alertas,
  conectividade, reposicionamento interrompido, ausência de equipe conhecida e
  entrega multissalto, alcance do sensor e comparação do enlace com/sem
  atenuação por obstáculo;
- `ba-smoke-test-obstacle.xml`, `ba-smoke-test-team.xml` e
  `multihop-smoke-test-team.xml`: dados determinísticos usados somente pelos
  smoke tests.

Essas configurações validam implementação. Seus resultados não constituem
evidência dos experimentos da professora.

### Rastreabilidade dos testes obrigatórios

| Diretriz | Configuração ou contrato | Validador |
| --- | --- | --- |
| 28.1 Comunicação direta | `AlertLifecycle_SmokeTest` | `validate_alert_lifecycle_smoke_test.py` |
| 28.2 Comunicação multissalto | `Multihop_SmokeTest` | `validate_multihop_smoke_test.py` |
| 28.3 Obstáculo | `ObstacleClear_SmokeTest` / `ObstacleBlocked_SmokeTest` | `validate_obstacle_smoke_test.py` |
| 28.4 Sensor | `BA_SmokeTest` / `SensorOutOfRange_SmokeTest` | `validate_ba_smoke_test.py` / `validate_sensor_range_smoke_test.py` |
| 28.5 Reposicionamento | `BA_SmokeTest` / `RepositionInterrupted_SmokeTest` | validadores homônimos |
| 28.6 Preservação da conectividade | `Connectivity_SmokeTest` | `validate_connectivity_smoke_test.py` |
| 28.7 Deduplicação | `BA_SmokeTest` | `validate_ba_smoke_test.py` |
| 28.8 Métricas | `ExperimentMetrics` e contratos de análise | `make analysis-tests` e smoke tests |
| 28.9 Comparação experimental | braços `*_BaOff` / `*_BaOn` | relatórios principal e de robustez |

## Resultados

`results/` é gerado durante as execuções e ignorado pelo Git. Os escalares ficam
em `results/omnetpp/`; vetores permanecem desabilitados.
