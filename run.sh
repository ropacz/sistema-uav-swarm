#!/usr/bin/env bash
# Executa a simulação ECHOSAR dentro do ambiente opp_env.
#
# Uso:
#   ./run.sh                       → todos os seeds da config (repeat=N no ini)
#   ./run.sh -r 2                  → seed específico
#   ./run.sh -c Cenario_ComObstaculos → config diferente
#   ./run.sh --gui                 → Qtenv (janela gráfica)
#   ./run.sh --info                → log INFO (verboso; padrão é WARN)
#   ./run.sh --build               → compila antes de rodar
#
# Todos os seeds rodam em UMA invocação do OMNeT++ (sem loop externo).
# Status impresso a cada 30 s para confirmar que não travou.

set -euo pipefail

WORKSPACE=/Users/rodrigo/omnetpp-workspace
INET_VERSION=inet-4.5.4
CONFIG=BasicTest_Piloto
RUN=""        # vazio = todos os seeds definidos por repeat= no ini
UI=Cmdenv
LOG_LEVEL=WARN
BUILD=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --gui)    UI=Qtenv ;;
        --info)   LOG_LEVEL=INFO ;;
        --build)  BUILD=true ;;
        -c)       CONFIG="$2"; shift ;;
        -r)       RUN="$2";   shift ;;
        -h|--help)
            echo "Uso: $0 [-c Config] [-r seed] [--gui] [--info] [--build]"
            exit 0 ;;
        *)
            echo "Opção desconhecida: $1 (use --help)" >&2
            exit 1 ;;
    esac
    shift
done

if $BUILD; then
    echo ">>> Compilando (MODE=debug)..."
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c 'make makefiles && make -C /Users/rodrigo/omnetpp-workspace/sistema/src MODE=debug'
fi

RUN_ARG=${RUN:+-r "$RUN"}
echo ">>> Config=$CONFIG  seeds=${RUN:-todos}  UI=$UI  log=$LOG_LEVEL"

if [[ "$UI" == "Qtenv" ]]; then
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c "cd simulations && ./run -u Qtenv -c $CONFIG $RUN_ARG"
else
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c "cd simulations && ./run -u Cmdenv -c $CONFIG $RUN_ARG \
            --cmdenv-express-mode=true \
            --cmdenv-log-level=$LOG_LEVEL \
            --cmdenv-status-frequency=30s"
fi
