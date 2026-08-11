#!/usr/bin/env bash
# Roda a simulação com log de mensagens (--msglog), parseia e converte pra parquet.
#
# Uso:
#   analysis/export_messages_parquet.sh BasicTest_Piloto 0
#
# Saída: analysis/parquet/<config>-<seed>_messages.parquet (1 linha por evento
# de troca de mensagem: alerta gerado, enviado, relay, dedup, ack, timeout...)
#
# Requer: duckdb (CLI), python3.

set -euo pipefail

CONFIG="${1:?uso: $0 <ConfigName> <seed>}"
SEED="${2:?uso: $0 <ConfigName> <seed>}"
OUT_DIR=analysis/parquet
TMP_LOG=$(mktemp)
TMP_CSV=$(mktemp)
trap 'rm -f "$TMP_LOG" "$TMP_CSV"' EXIT

mkdir -p "$OUT_DIR"

echo ">>> Rodando $CONFIG seed=$SEED com --msglog..."
./run.sh --msglog -c "$CONFIG" -r "$SEED" > "$TMP_LOG" 2>&1

echo ">>> Parseando log em eventos..."
python3 analysis/parse_message_log.py "$TMP_LOG" > "$TMP_CSV"

echo ">>> Convertendo para Parquet (duckdb)..."
duckdb -c "
COPY (SELECT * FROM read_csv_auto('$TMP_CSV'))
  TO '$OUT_DIR/${CONFIG}-${SEED}_messages.parquet' (FORMAT PARQUET);
"

echo ">>> Pronto:"
ls -la "$OUT_DIR/${CONFIG}-${SEED}_messages.parquet"
echo
echo "Exemplos de consulta:"
echo "  -- linha do tempo de um alerta específico:"
echo "  duckdb -c \"SELECT * FROM read_parquet('$OUT_DIR/${CONFIG}-${SEED}_messages.parquet') WHERE msg_id='drone[0]_1' ORDER BY time\""
echo "  -- contagem de eventos por tipo:"
echo "  duckdb -c \"SELECT event, count(*) FROM read_parquet('$OUT_DIR/${CONFIG}-${SEED}_messages.parquet') GROUP BY event ORDER BY 2 DESC\""
