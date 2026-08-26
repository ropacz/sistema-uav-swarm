#!/usr/bin/env bash
# Gera .elog + .sca + CSV de referência para a auditoria de atendimento/perda.
# Roda 5 seeds de cada braço do experimento principal — as mesmas seeds já
# usadas na campanha oficial, só que com o event log ligado. Não altera RNG
# nem resultado, só acrescenta o log (~130 MB por execução, ~10 min no total).
#
# Uso: bash analysis/audit/run_audit.sh
# Precisa rodar dentro do shell do opp_env (ver README.md deste diretório).
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$REPO/out/clang-debug/src/sistema_dbg"
NED="$REPO/simulations:$REPO/src:${WORKSPACE:-$(cd "$REPO/.." && pwd)}/${INET_VERSION:-inet-4.5.4}/src"
OUT="$REPO/analysis/audit/raw"

if [[ ! -x "$BIN" ]]; then
    echo "Binário ausente: $BIN" >&2
    echo "Compile primeiro: opp_env run \${INET_VERSION:-inet-4.5.4} -w \${WORKSPACE} --no-isolated -c 'make MODE=debug'" >&2
    exit 1
fi

mkdir -p "$OUT"
cd "$REPO/simulations"

for config in MainExperiment_BaOff MainExperiment_BaOn; do
    for seed in 0 1 2 3 4; do
        echo ">>> $config seed=$seed"
        "$BIN" -n "$NED" -u Cmdenv -c "$config" -r "$seed" \
            --cmdenv-express-mode=true "--**.cmdenv-log-level=OFF" \
            --record-eventlog=true --eventlog-file="$OUT/$config-$seed.elog" \
            --output-scalar-file="$OUT/$config-$seed.sca" \
            --**.experimentMetrics.alertRecordDirectory="\"$OUT\"" \
            2>&1 | tail -3
    done
done
echo "Auditoria pronta em $OUT — rode: python3 analysis/audit/compare.py"
