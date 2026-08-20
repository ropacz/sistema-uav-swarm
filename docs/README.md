# Documentação do ECHOSAR-Net

Quatro documentos autorais, com papéis distintos e sem sobreposição:

- [`scientific_protocol.md`](scientific_protocol.md) — pergunta, H0 e H1,
  unidade experimental, desenho pareado, separação entre verificação e
  evidência, e as decisões de desenho ainda em aberto. **Documento normativo.**
- [`model_and_assumptions.md`](model_and_assumptions.md) — o que é modelado,
  como funcionam sensor, Bat Algorithm e mobilidade, quais modelos do INET são
  usados, e os limites do modelo.
- [`metrics.md`](metrics.md) — contrato de cada métrica: definição, unidade,
  população, numerador, denominador, coleta, agregação e limite.
- [`traceability.md`](traceability.md) — requisito → implementação →
  verificação, e o que cada cenário determinístico cobre.

Valores de parâmetros vivem em `simulations/omnetpp.ini`, fonte única. A
documentação cita as chaves, não repete os valores.

## Referências externas

Arquivos recebidos, especificações de fabricante e material de reunião ficam em
`references/`. São insumos e não representam necessariamente o comportamento
implementado; a rastreabilidade aponta para a documentação autoral acima.
