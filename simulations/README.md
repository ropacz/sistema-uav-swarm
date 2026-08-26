# Simulações

O diretório mantém a rede NED, a configuração global e os dados de ambiente no
mesmo local para que os caminhos de `include` e `xmldoc()` permaneçam simples e
reproduzíveis. São três arquivos de configuração:

- `omnetpp.ini`: ponto único de entrada. `[General]` reúne o que vale para toda
  execução — seeds e fluxos de RNG, gravação de resultados, pilha de rede,
  rádio IEEE 802.11 e os valores independentes de cenário (pesos da aptidão,
  constantes do BA, envelope de voo, alcance do sensor). Qualquer configuração
  pode sobrescrever esses valores;
- `experiment.ini`: o cenário científico — base física e de protocolo, os dois
  cenários da campanha e o contraste pareado principal;
- `validation/smoke-tests.ini`: as validações técnicas, junto dos dados
  determinísticos que só elas usam.

## Experimento principal

`MainExperiment_BaOff` e `MainExperiment_BaOn` formam o contraste pareado mínimo:
mesmo cenário, mesmas seeds, diferindo só por `baEnabled`. `make experiment`
executa somente esses dois braços, que usam comunicação direta.

## Robustez

`Scenario1_OneVictim_*` e `Scenario1_TwoVictims_*` variam `numTeams` em
1, 5, 10 e 15. Esses casos verificam se a conclusão principal resiste à mudança
de carga, mas não ampliam a hipótese confirmatória.

## Ambiente físico

- `professor-scenario-obstacles.xml`: obstáculos estáticos compartilhados pelos
  cenários científico e de robustez.

## Validações técnicas

`validation/` isola tudo o que serve só para validar a implementação:

- `smoke-tests.ini`: validações de BA, ciclo de alertas, conectividade,
  reposicionamento interrompido, ausência de equipe conhecida, entrega
  multissalto, alcance do sensor e comparação do enlace com/sem atenuação
  por obstáculo;
- `ba-smoke-test-obstacle.xml`, `ba-smoke-test-team.xml` e
  `multihop-smoke-test-team.xml`: dados determinísticos desses cenários.

Os caminhos em `xmldoc()` são resolvidos a partir do diretório do `.ini`, por
isso continuam sendo apenas o nome do arquivo.

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
