# Documentação técnica e científica

[`Diretrizes_para_implementao_do_projeto_no_OMNeT_e_INET.pdf`](Diretrizes_para_implementao_do_projeto_no_OMNeT_e_INET.pdf)
é a especificação normativa da implementação, das métricas, do desenho
experimental e dos testes obrigatórios. A rastreabilidade executável entre os
testes da seção 28, configurações e validadores está em
[`simulations/README.md`](../simulations/README.md#rastreabilidade-dos-testes-obrigatórios).

[`desvios_e_extensoes.md`](desvios_e_extensoes.md) registra apenas o que difere
da diretriz — desvios, extensões e lacunas, cada um com a seção do PDF, a
justificativa e o arquivo correspondente. O que não aparece lá segue a diretriz.

## Fonte única dos parâmetros

Os valores usados pela simulação pertencem exclusivamente aos arquivos em
[`simulations/`](../simulations/). Esta documentação cita nomes de parâmetros e
arquivos de configuração, mas deliberadamente não repete seus valores. Se houver
divergência, prevalecem, nesta ordem:

1. código que define a semântica do modelo;
2. configuração `.ini` usada e registrada no arquivo `.sca`;
3. a diretriz normativa;
4. material histórico ou externo.

## Material complementar

- [`references/cenarios_solicitados.pdf`](references/cenarios_solicitados.pdf)
  contém os cenários indicados para a campanha experimental;
- [`references/arguicao_banca_qualificacao.pdf`](references/arguicao_banca_qualificacao.pdf)
  preserva as recomendações da banca de qualificação;
- [`references/especificacoes_dji_phantom_4_pro_v2.docx`](references/especificacoes_dji_phantom_4_pro_v2.docx)
  reúne a referência técnica do VANT.

Esses arquivos fundamentam decisões do estudo, mas não alteram diretamente o
comportamento da simulação.

Resultados de execução são gerados em `simulations/results/`; análises derivadas
em `analysis/figures/`. Ambos são artefatos reproduzíveis, não documentação
versionada.
