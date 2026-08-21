# Protocolo científico

O único experimento do repositório é o piloto causal descrito em
[`pilot_experiment.md`](pilot_experiment.md). Esta separação evita manter
desenhos experimentais não utilizados ou confundir testes auxiliares com
evidência do artigo.

Configurações executáveis:

- `HypothesisPilot_BaOff`: controle com `baEnabled = false`;
- `HypothesisPilot_BaOn`: tratamento com `baEnabled = true`.

Os braços são pareados pela mesma seed e diferem exclusivamente pela ativação
do reposicionamento. O comando canônico é `make hypothesis-pilot`.

O lote de cinco pares é exploratório: os parâmetros foram ajustados durante o
desenvolvimento e o teste exato bilateral de McNemar observado é `p = 0,0625`.
Qualquer estudo confirmatório deve usar amostra nova, tamanho definido antes da
execução e, para alegar superioridade do BA, um controle ativo com outro método
de reposicionamento.
