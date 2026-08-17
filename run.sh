#!/usr/bin/env bash
# Executa a simulação ECHOSAR dentro do ambiente opp_env.
#
# Uso:
#   ./run.sh                       → validação direta determinística
#   ./run.sh -r 2                  → seed específico
#   ./run.sh -c Cenario_ComObstaculos → config diferente
#   ./run.sh --gui                 → Qtenv (janela gráfica)
#   ./run.sh --info                → log INFO global (MUITO verboso: PHY/MAC/AODV inclusos)
#   ./run.sh --build               → compila antes de rodar
#
# Todos os seeds rodam em UMA invocação do OMNeT++ (sem loop externo).
# Status impresso a cada 30 s para confirmar que não travou.

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$PROJECT_DIR/.env" ]]; then
    set -a
    source "$PROJECT_DIR/.env"
    set +a
fi
: "${WORKSPACE:=$(cd "$PROJECT_DIR/.." && pwd)}"
: "${INET_VERSION:=inet-4.5.4}"
CONFIG=Validation_Direct
RUN=""        # vazio = todos os seeds definidos por repeat= no ini
UI=Cmdenv
LOG_LEVEL=WARN
EXPRESS=true
BUILD=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --gui)    UI=Qtenv ;;
        --info)   LOG_LEVEL=INFO; EXPRESS=false ;;
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
        -c "cd '$PROJECT_DIR' && make makefiles && make -C '$PROJECT_DIR/src' MODE=debug"
fi

RUN_ARG=${RUN:+-r "$RUN"}
echo ">>> Config=$CONFIG  seeds=${RUN:-todos}  UI=$UI  log=$LOG_LEVEL"

if [[ "$UI" == "Qtenv" ]]; then
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c "cd '$PROJECT_DIR/simulations' && ./run -u Qtenv -c $CONFIG $RUN_ARG"
else
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c "cd '$PROJECT_DIR/simulations' && ./run -u Cmdenv -c $CONFIG $RUN_ARG \
            --cmdenv-express-mode=$EXPRESS \
            --**.cmdenv-log-level=$LOG_LEVEL \
            --**.app[0].cmdenv-log-level=INFO \
            --cmdenv-status-frequency=30s"
fi
