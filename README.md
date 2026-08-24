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
make robustness-experiment      # equipes e vítimas adicionais
make optional-multihop          # pergunta complementar de roteamento
make optional-pcap              # auditoria, não fonte primária
make optional-scaling           # sonda exploratória
make ba-smoke-test             # integração do BA, não experimento científico
make network-discovery-validation # descoberta local por broadcast
```

O escopo principal está em `simulations/main-experiment.ini`; parâmetros físicos
e de protocolo são herdados das configurações-base. Robustez e diagnósticos
permanecem separados. A arquitetura, as fórmulas, as hipóteses e as validações
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
- `analysis`: relatório principal, robustez, auditoria opcional e testes;
- `docs`: protocolo científico e fontes de parâmetros.

Consulte [simulations/README.md](simulations/README.md) para distinguir cenários
científicos de validações técnicas.
