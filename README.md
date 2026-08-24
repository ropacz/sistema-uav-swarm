# ECHOSAR-Net — cenários da professora

Simulação de busca e salvamento com OMNeT++ 6.2 e INET 4.5.4. O projeto contém
os dois casos da linha 1 de `docs/references/CenáriosReunião14_8Rodrigo.pdf`:

- cenário 1A: 4 drones e 1 vítima;
- cenário 1B: 4 drones e 2 vítimas, anunciadas por 2 drones diferentes.

Ambos usam 2 obstáculos grandes, 1/5/10/15 equipes, área 1000 × 1000 m,
Random Waypoint terrestre e Gauss–Markov 3D aéreo até 20 m. A sonda atual usa
450 s e 3 seeds; o lote final deve usar 900 s e 30 seeds.

```bash
cp .env.example .env
./run.sh --build -c Scenario1_OneVictim_BaOn -r 0
make professor-scenarios
make professor-pcap
make ba-smoke-test             # integração do BA, não experimento científico
make network-discovery-validation # descoberta local por broadcast
```

Os arquivos experimentais estão separados em `simulations/professor-common.ini`,
`simulations/scenario-1-one-victim.ini` e
`simulations/scenario-1-two-victims.ini`. A especificação, fórmulas, hipóteses,
gatilho do BA e ameaças à validade estão em
[docs/professor_scenarios.md](docs/professor_scenarios.md).

## Organização

- `src/app`: protocolo da aplicação e seus estados de enlace/alerta;
- `src/metrics`: coleta global e deduplicada das métricas fim a fim;
- `src/messages`: mensagens OMNeT++ e serialização em rede;
- `src/mobility`: Gauss–Markov 3D comandável pelo reposicionamento;
- `src/optimization`: Bat Algorithm e função de aptidão;
- `src/scenario`, `src/node` e `src/sensing`: cenário, composição e percepção;
- `simulations`: rede, configurações e ambientes físicos;
- `analysis`: métricas, auditoria PCAPNG, relatórios e testes;
- `docs`: protocolo científico e fontes de parâmetros.

Consulte [simulations/README.md](simulations/README.md) para distinguir cenários
científicos de validações técnicas.
