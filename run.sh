#!/usr/bin/env bash
# Executa a simulação ECHOSAR dentro do ambiente opp_env.
#
# Uso:
#   ./run.sh              → Cmdenv, log INFO, BasicTest run 0
#   ./run.sh --gui        → Qtenv (janela gráfica)
#   ./run.sh -c OutraConf → Config diferente
#   ./run.sh -r 2         → Run/seed 2
#   ./run.sh --warn       → reduz log para WARN (mais rápido)
#   ./run.sh --build      → compila antes de rodar

set -euo pipefail

WORKSPACE=/Users/rodrigo/omnetpp-workspace
INET_VERSION=inet-4.5.4
CONFIG=BasicTest
RUN=0
UI=Cmdenv
LOG_LEVEL=INFO
BUILD=false

# ── Parse argumentos ────────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
    case "$1" in
        --gui)     UI=Qtenv ;;
        --warn)    LOG_LEVEL=WARN ;;
        --build)   BUILD=true ;;
        -c)        CONFIG="$2"; shift ;;
        -r)        RUN="$2"; shift ;;
        -h|--help)
            echo "Uso: $0 [--gui] [--warn] [--build] [-c Config] [-r run]"
            exit 0 ;;
        *)
            echo "Opção desconhecida: $1 (use --help)" >&2
            exit 1 ;;
    esac
    shift
done

# ── Build opcional ───────────────────────────────────────────────────────────
if $BUILD; then
    echo ">>> Compilando..."
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c 'make makefiles && make'
fi

# ── Executar ─────────────────────────────────────────────────────────────────
echo ">>> Config=$CONFIG  run=$RUN  UI=$UI  log=$LOG_LEVEL"

if [[ "$UI" == "Qtenv" ]]; then
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c "cd simulations && ./run -u Qtenv -c $CONFIG"
else
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c "cd simulations && ./run -u Cmdenv -c $CONFIG -r $RUN \
            --cmdenv-express-mode=false \
            --cmdenv-log-level=$LOG_LEVEL"
fi
