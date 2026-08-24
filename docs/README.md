# Documentação técnica e científica

Esta pasta documenta o modelo implementado, o protocolo experimental, os
contratos das métricas e as evidências de validação do ECHOSAR-Net. Os quatro
documentos abaixo são normativos e devem permanecer coerentes com o código e
com a análise.

1. [`model_and_assumptions.md`](model_and_assumptions.md) — arquitetura,
   protocolo, sensor, Bat Algorithm, mobilidade e limites do modelo.
2. [`scientific_protocol.md`](scientific_protocol.md) — pergunta científica,
   hipóteses, desenho pareado, escopos e critérios para aceitar uma execução.
3. [`metrics.md`](metrics.md) — definições operacionais, fórmulas, unidades,
   denominadores e interpretação de cada família de métricas.
4. [`traceability.md`](traceability.md) — relação entre requisitos, código,
   sinais, escalares, scripts e testes de validação.

Leitura recomendada: modelo → protocolo científico → métricas →
rastreabilidade. Para executar o projeto diretamente, a seção
[Procedimento reproduzível](traceability.md#procedimento-reproduzível) reúne os
comandos e os artefatos esperados.

## Fonte única dos parâmetros

Os valores usados pela simulação pertencem exclusivamente aos arquivos em
[`simulations/`](../simulations/). Esta documentação cita nomes de parâmetros e
arquivos de configuração, mas deliberadamente não repete seus valores. Se houver
divergência, prevalecem, nesta ordem:

1. código que define a semântica do modelo;
2. configuração `.ini` usada e registrada no arquivo `.sca`;
3. estes documentos;
4. material histórico ou externo.

## Material complementar

- [`professor_scenarios.md`](professor_scenarios.md) preserva a descrição ampla
  dos cenários solicitados e deve ser lido como material complementar; os
  contratos normativos estão nos quatro documentos acima.
- [`references/`](references/) contém especificações e material externo. Esses
  arquivos não alteram o comportamento da simulação.

Resultados de execução são gerados em `simulations/results/`; análises derivadas
em `analysis/figures/`. Ambos são artefatos reproduzíveis, não documentação
versionada.
