#!/usr/bin/env bash
# Executa a simulação ECHOSAR dentro do ambiente opp_env.
#
# Uso:
#   ./run.sh                       → piloto com BA habilitado
#   ./run.sh -r 2                  → seed específico
#   ./run.sh -c HypothesisPilot_BaOff → braço de controle
#   ./run.sh -c HypothesisPilot_BaOn -r 0 --pcap → seed com PCAPNG
#   ./run.sh --gui                 → piloto BA ligado no Qtenv
#   ./run.sh --info                → log INFO global (MUITO verboso: PHY/MAC/AODV inclusos)
#   ./run.sh --build               → compila antes de rodar
#
# Todos os seeds rodam em UMA invocação do OMNeT++ (sem loop externo).
# Status impresso a cada 30 s para confirmar que não travou.

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
mkdir -p \
    "$PROJECT_DIR/simulations/results/omnetpp" \
    "$PROJECT_DIR/simulations/results/pcap" \
    "$PROJECT_DIR/simulations/results/eventlogs" \
    "$PROJECT_DIR/simulations/results/spreadsheets"
if [[ -f "$PROJECT_DIR/.env" ]]; then
    set -a
    source "$PROJECT_DIR/.env"
    set +a
fi
: "${WORKSPACE:=$(cd "$PROJECT_DIR/.." && pwd)}"
: "${INET_VERSION:=inet-4.5.4}"
CONFIG=HypothesisPilot_BaOn
CONFIG_EXPLICIT=false
RUN=""        # vazio = todos os seeds definidos por repeat= no ini
UI=Cmdenv
LOG_LEVEL=WARN
EXPRESS=true
BUILD=false
PCAP=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --gui)    UI=Qtenv ;;
        --info)   LOG_LEVEL=INFO; EXPRESS=false ;;
        --build)  BUILD=true ;;
        --pcap)   PCAP=true ;;
        -c)       CONFIG="$2"; CONFIG_EXPLICIT=true; shift ;;
        -r)       RUN="$2";   shift ;;
        -h|--help)
            echo "Uso: $0 [-c Config] [-r run] [--gui] [--info] [--build] [--pcap]"
            exit 0 ;;
        *)
            echo "Opção desconhecida: $1 (use --help)" >&2
            exit 1 ;;
    esac
    shift
done

if [[ "$UI" == "Qtenv" && "$CONFIG_EXPLICIT" == false ]]; then
    CONFIG=HypothesisPilot_BaOn
fi

if $BUILD; then
    echo ">>> Compilando (MODE=debug)..."
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c "cd '$PROJECT_DIR' && make makefiles && make -C '$PROJECT_DIR/src' MODE=debug"
fi

RUN_ARG=${RUN:+-r "$RUN"}
PCAP_ARGS=""
if $PCAP; then
    PCAP_ARGS="--*.drone[*].numPcapRecorders=1 --*.team[*].numPcapRecorders=1"
fi
SIMULATION_BINARY="$PROJECT_DIR/out/clang-debug/src/sistema_dbg"
NED_PATH="$PROJECT_DIR/simulations:$PROJECT_DIR/src:$WORKSPACE/$INET_VERSION/src"
if [[ ! -x "$SIMULATION_BINARY" ]]; then
    echo "Binário ausente: $SIMULATION_BINARY" >&2
    echo "Execute ./run.sh --build ou make antes de iniciar a simulação." >&2
    exit 1
fi
echo ">>> Config=$CONFIG  run=${RUN:-todos}  UI=$UI  log=$LOG_LEVEL  pcap=$PCAP"

if [[ "$UI" == "Qtenv" ]]; then
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c "cd '$PROJECT_DIR/simulations' && '$SIMULATION_BINARY' -n '$NED_PATH' -u Qtenv -c $CONFIG $RUN_ARG $PCAP_ARGS"
else
    opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
        -c "cd '$PROJECT_DIR/simulations' && '$SIMULATION_BINARY' -n '$NED_PATH' -u Cmdenv -c $CONFIG $RUN_ARG $PCAP_ARGS \
            --cmdenv-express-mode=$EXPRESS \
            --**.cmdenv-log-level=$LOG_LEVEL \
            --**.app[0].cmdenv-log-level=INFO \
            --cmdenv-status-frequency=30s"
fi
