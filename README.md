# ECHOSAR-Net — reposicionamento de UAV com Bat Algorithm

Simulação de busca e salvamento com OMNeT++ 6.2 e INET 4.5.4. A pergunta central
é se o reposicionamento de um UAV pelo Bat Algorithm melhora a entrega direta de
alertas de vítima. O experimento principal compara somente dois braços pareados:

- `MainExperiment_BaOff`: controle sem reposicionamento;
- `MainExperiment_BaOn`: mesmo cenário e mesmas seeds, alterando apenas
  `baEnabled`.

Os valores experimentais pertencem às configurações em `simulations/`. A
documentação descreve seus significados sem criar uma segunda fonte de valores.

```bash
cp .env.example .env
./run.sh --build -c MainExperiment_BaOn -r 0
make experiment                 # experimento confirmatório BA Off/On
make analysis-tests             # contratos do analisador
make ba-smoke-test              # integração do BA, não experimento científico
make clean-results              # remove resultados e tabelas regeneráveis
```

O escopo principal está em `simulations/experiment.ini`; parâmetros físicos
e de protocolo são herdados das configurações-base. A robustez permanece
separada. A arquitetura, as fórmulas, as hipóteses e as validações
estão organizadas no [índice da documentação técnica](docs/README.md). Os valores
dos parâmetros permanecem exclusivamente nos arquivos em `simulations/`.

## Organização

- `src/app`: protocolo da aplicação e seus estados de enlace/alerta;
- `src/metrics`: coleta global e deduplicada das métricas fim a fim;
- `src/messages`: mensagens OMNeT++ e serialização em rede;
- `src/mobility`: Gauss–Markov 3D comandável pelo reposicionamento;
- `src/optimization`: Bat Algorithm e função de aptidão;
- `src/scenario`, `src/node` e `src/sensing`: cenário, composição e percepção;
- `simulations`: rede, configurações e ambientes físicos;
- `analysis`: relatório principal, robustez e testes;
- `docs`: diretriz normativa, índice técnico e referências externas.

Consulte [simulations/README.md](simulations/README.md) para distinguir cenários
científicos de validações técnicas.

## Limitações do modelo

O modelo utiliza obstáculos estáticos, vítimas identificadas abstratamente,
sensor geométrico simplificado e informações de posição que podem ficar
desatualizadas. A nova posição é uma estimativa geométrica; não há modelo
energético detalhado nem processamento de imagens, e o AODV do INET é utilizado
sem modificações. Os resultados dependem do cenário e da configuração executada.

Distância e duração de movimento representam custo operacional, não consumo de
energia ou carga real de bateria. Uma perda não deve ser atribuída
automaticamente a um obstáculo, e a ausência de ACK não prova que o alerta não
chegou à equipe. O cenário principal direto não constitui evidência de
conectividade multissalto; essa capacidade possui validação técnica separada.
