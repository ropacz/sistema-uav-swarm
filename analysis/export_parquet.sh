#!/usr/bin/env bash
# Exporta escalares (.sca) e vetores (.vec) de uma config para Parquet.
#
# Uso:
#   analysis/export_parquet.sh BasicTest_Piloto
#   analysis/export_parquet.sh SmokeTest_Beacons
#
# Saída: analysis/parquet/<config>_scalars.parquet
#        analysis/parquet/<config>_vectors.parquet   (formato longo: 1 linha por amostra)
#
# Requer: opp_env (via workspace), duckdb (https://duckdb.org, CLI standalone).
#
# Escalares (.sca) sempre têm dado (métricas finais por seed). Vetores (.vec)
# só têm dado se a config habilitar vector-recording para aquele sinal — hoje
# só **.app[0].deliveryDelay.vector-recording=true no omnetpp.ini.

set -euo pipefail

CONFIG="${1:?uso: $0 <ConfigName>}"
WORKSPACE=/Users/rodrigo/omnetpp-workspace
INET_VERSION=inet-4.5.4
RESULTS_DIR=simulations/results
OUT_DIR=analysis/parquet
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

mkdir -p "$OUT_DIR"

echo ">>> Exportando .sca de $CONFIG para CSV..."
opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
  -c "opp_scavetool export -T s -F CSV-R -o $TMP_DIR/scalars.csv $RESULTS_DIR/${CONFIG}-*.sca"

echo ">>> Exportando .vec de $CONFIG para CSV..."
opp_env run "$INET_VERSION" -w "$WORKSPACE" --no-isolated \
  -c "opp_scavetool export -T v -F CSV-R -o $TMP_DIR/vectors.csv '$RESULTS_DIR/${CONFIG}-#*.vec'"

echo ">>> Convertendo para Parquet (duckdb)..."
duckdb -c "
COPY (SELECT * FROM read_csv_auto('$TMP_DIR/scalars.csv'))
  TO '$OUT_DIR/${CONFIG}_scalars.parquet' (FORMAT PARQUET);

COPY (
  SELECT run, module, name, CAST(t AS DOUBLE) AS time, CAST(v AS DOUBLE) AS value
  FROM (
    SELECT run, module, name,
           unnest(string_split(vectime, ' ')) AS t,
           unnest(string_split(vecvalue, ' ')) AS v
    FROM read_csv_auto('$TMP_DIR/vectors.csv')
    WHERE type='vector' AND vectime IS NOT NULL AND vectime != ''
  )
) TO '$OUT_DIR/${CONFIG}_vectors.parquet' (FORMAT PARQUET);
"

echo ">>> Pronto:"
ls -la "$OUT_DIR/${CONFIG}_scalars.parquet" "$OUT_DIR/${CONFIG}_vectors.parquet"
echo
echo "Exemplo de consulta:"
echo "  duckdb -c \"SELECT * FROM read_parquet('$OUT_DIR/${CONFIG}_scalars.parquet') LIMIT 10\""
