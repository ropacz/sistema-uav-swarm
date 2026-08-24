"""Extract INET network-layer metrics per run and aggregate them across seeds.

Usage:
    python3 analysis/core/network_metrics.py [results-directory] [--configs A B ...]

These metrics are **diagnostic**. They describe how the network behaved —
frames, drops, routing and signal — and are not the primary evidence about the
hypothesis. Their contracts are in docs/metrics.md.

Counters are summed over the nodes of a run, then one row per run is aggregated
across seeds, so every seed carries the same weight. Packets are never treated
as independent replicates.
"""

from __future__ import annotations

import argparse
import glob
import math
import os
import sys
from pathlib import Path

import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.process_results import ci95, parse_sca  # noqa: E402

ROOT = str(REPOSITORY_ROOT)
OUTPUT = os.path.join(ROOT, "analysis", "figures")

# Sufixos de módulo. O MAC publica os mesmos nomes de contador em `.mac` e em
# `.mac.dcf`; somar por nome sem filtrar o módulo contaria cada quadro duas
# vezes.
MAC = r"\.wlan\[\d+\]\.mac$"
IP = r"\.ipv4\.ip$"
UDP = r"\.udp$"
APP = r"\.app\[0\]$"
METRICS = r"\.experimentMetrics$"


def sum_where(frame: pd.DataFrame, name: str, module_pattern: str | None = None) -> float:
    selected = frame["name"] == name
    if module_pattern:
        selected &= frame["module"].str.contains(module_pattern, regex=True)
    values = frame.loc[selected, "value"]
    return float(values.sum()) if len(values) else 0.0


def mean_where(frame: pd.DataFrame, name: str, module_pattern: str | None = None) -> float:
    selected = frame["name"] == name
    if module_pattern:
        selected &= frame["module"].str.contains(module_pattern, regex=True)
    values = frame.loc[selected, "value"].dropna()
    values = values[values.apply(math.isfinite)]
    return float(values.mean()) if len(values) else math.nan


def pooled_statistic_mean(frame: pd.DataFrame, base_name: str,
                          module_pattern: str | None = None) -> float:
    """Pool a signal by sample count, never by averaging module means."""
    total = sum_where(frame, f"{base_name}:sum", module_pattern)
    count = sum_where(frame, f"{base_name}:count", module_pattern)
    return ratio(total, count) if count else math.nan


def received_power_mean_dbm(frame: pd.DataFrame) -> float:
    """Average received power in mW, then convert that physical mean to dBm."""
    mean_milliwatt = pooled_statistic_mean(
        frame, "positionUpdatePowerMilliwatt")
    if math.isfinite(mean_milliwatt) and mean_milliwatt > 0:
        return 10 * math.log10(mean_milliwatt)
    # Compatibility for result files generated before linear power was recorded.
    return mean_where(frame, "positionUpdateRssi:mean")


def extreme_where(frame: pd.DataFrame, name: str, lowest: bool) -> float:
    values = frame.loc[frame["name"] == name, "value"].dropna()
    values = values[values.apply(math.isfinite)]
    if not len(values):
        return math.nan
    return float(values.min() if lowest else values.max())


def ratio(numerator: float, denominator: float, scale: float = 1.0) -> float:
    return scale * numerator / denominator if denominator else math.nan


def global_or_legacy(frame: pd.DataFrame, global_name: str,
                     legacy_name: str, legacy_pattern: str = APP) -> float:
    """Read a global ExperimentMetrics scalar, falling back for old result files."""
    global_rows = (frame["name"] == global_name) & frame["module"].str.contains(
        METRICS, regex=True)
    if global_rows.any():
        return float(frame.loc[global_rows, "value"].sum())
    return sum_where(frame, legacy_name, legacy_pattern)


def global_scalar(frame: pd.DataFrame, name: str) -> float:
    selected = (frame["name"] == name) & frame["module"].str.contains(
        METRICS, regex=True)
    values = frame.loc[selected, "value"]
    return float(values.sum()) if len(values) else math.nan


def collect(path: str) -> dict:
    attrs, frame, _ = parse_sca(path)

    mac_sent = sum_where(frame, "packetSentToLower:count", MAC)
    mac_received = sum_where(frame, "packetReceivedFromLower:count", MAC)
    mac_retry_limit = sum_where(frame, "packetDropRetryLimitReached:count", MAC)
    mac_overflow = sum_where(frame, "packetDropQueueOverflow:count", MAC)
    mac_corrupt = sum_where(frame, "packetDropIncorrectlyReceived:count", MAC)
    mac_duplicate = sum_where(frame, "packetDropDuplicateDetected:count", MAC)

    udp_sent = sum_where(frame, "packetSent:count", UDP)
    udp_received = sum_where(frame, "packetReceived:count", UDP)

    generated = global_or_legacy(frame, "alertsGenerated", "uniqueAlertsGenerated")
    delivered = global_or_legacy(frame, "alertsDelivered", "uniqueAlertsReceived")
    acked = global_or_legacy(frame, "alertsConfirmed", "uniqueAlertsAcked")
    attempts = global_or_legacy(frame, "alertAttemptsSent", "alertAttemptsSent")
    attempts_received = global_or_legacy(
        frame, "alertAttemptsDelivered", "attemptsReceived")
    delivery_delay_sum = global_scalar(frame, "deliveryDelaySum")
    delivery_delay_count = global_scalar(frame, "deliveryDelayCount")
    attempt_delay_sum = global_scalar(frame, "attemptDeliveryDelaySum")
    attempt_delay_count = global_scalar(frame, "attemptDeliveryDelayCount")
    recovery_sum = global_scalar(frame, "recoveryTimeSum")
    recovery_count = global_scalar(frame, "recoveryTimeCount")
    validated_recovery_sum = global_scalar(frame, "validatedRecoveryTimeSum")
    validated_recovery_count = global_scalar(frame, "validatedRecoveryTimeCount")

    return {
        "config": attrs["configname"],
        "seed": int(float(attrs["seedset"])),
        "run_file": os.path.basename(path),

        # ── Camada de enlace ──────────────────────────────────────────────
        "mac_frames_sent": mac_sent,
        "mac_frames_received": mac_received,
        "mac_drop_retry_limit": mac_retry_limit,
        "mac_drop_queue_overflow": mac_overflow,
        "mac_drop_corrupt": mac_corrupt,
        "mac_drop_duplicate": mac_duplicate,
        "mac_link_broken": sum_where(frame, "linkBroken:count", MAC),
        # Quadros descartados por esgotar retransmissões, sobre os enviados.
        "mac_retry_limit_pct": ratio(mac_retry_limit, mac_sent, 100),
        # Quadros recebidos com erro, sobre o total que chegou ao receptor.
        "mac_corrupt_pct": ratio(mac_corrupt, mac_received + mac_corrupt, 100),

        # ── Camada de rede ────────────────────────────────────────────────
        "ip_drop_no_route": sum_where(frame, "packetDropNoRouteFound:count", IP),
        "ip_drop_hop_limit": sum_where(frame, "packetDropHopLimitReached:count", IP),
        "ip_drop_address_resolution": sum_where(
            frame, "packetDropAddressResolutionFailed:count", IP),
        "hop_count_mean": (
            pooled_statistic_mean(frame, "hopCount")
            if sum_where(frame, "hopCount:count")
            else mean_where(frame, "hopCount:mean")
        ),

        # ── Transporte ────────────────────────────────────────────────────
        "udp_packets_sent": udp_sent,
        "udp_packets_received": udp_received,
        "udp_bytes_received": sum_where(frame, "packetReceived:sum(packetBytes)", UDP),
        "udp_drop_wrong_port": sum_where(frame, "droppedPkWrongPort:count", UDP),

        # ── Decisão do reposicionamento ───────────────────────────────────
        # Separam causas opostas de rejeição: não havia obstáculo na visada,
        # ou havia e estava fora do alcance do sensor.
        "degradation_indications": global_or_legacy(
            frame, "degradationIndications", "degradationIndications"),
        "sensor_confirmations": global_or_legacy(
            frame, "sensorConfirmations", "sensorConfirmations"),
        "sensor_clear_line_of_sight": global_or_legacy(
            frame, "sensorClearLineOfSight", "sensorClearLineOfSight"),
        "sensor_outside_range": global_or_legacy(
            frame, "sensorOutsideRange", "sensorOutsideRange"),
        "ba_activations": global_or_legacy(
            frame, "baActivations", "baActivations"),
        "repositions_started": global_scalar(frame, "repositionsStarted"),
        "repositions_validated": global_scalar(frame, "repositionsValidated"),
        "operationally_successful_repositions": global_or_legacy(
            frame, "operationallySuccessfulRepositions", "successfulRepositions"),
        "reposition_validation_pct": 100 * global_scalar(
            frame, "repositionValidationRate"),
        "operational_reposition_recovery_pct": 100 * global_scalar(
            frame, "operationalRepositionRecoveryRate"),
        "validated_recovery_time_mean_s": ratio(
            validated_recovery_sum, validated_recovery_count),
        "operational_recovery_time_mean_s": ratio(recovery_sum, recovery_count),
        "reposition_distance_sum_m": global_scalar(frame, "repositionDistanceSum"),
        "commanded_reposition_distance_sum_m": global_scalar(
            frame, "commandedRepositionDistanceSum"),
        # Soma de conjuntos locais: pode contar o mesmo alertId em equipes
        # distintas e, portanto, não é uma cardinalidade global.
        "team_local_unique_alert_receptions": sum_where(
            frame, "uniqueAlertsReceived", APP),
        # Cardinalidade global operacional: o drone aceita apenas o primeiro
        # ACK válido e encerra o alerta pendente.
        "globally_confirmed_unique_alerts": acked,
        "application_acks_sent": sum_where(frame, "applicationAcksSent", APP),

        # ── Rádio ─────────────────────────────────────────────────────────
        "rssi_mean_dbm": received_power_mean_dbm(frame),
        "rssi_min_dbm": extreme_where(frame, "positionUpdateRssi:min", lowest=True),
        "rssi_max_dbm": extreme_where(frame, "positionUpdateRssi:max", lowest=False),
        "rssi_samples_available": sum_where(frame, "rssiSamplesAvailable", APP),
        "rssi_samples_missing": sum_where(frame, "rssiSamplesMissing", APP),
        "team_entries_discovered": sum_where(frame, "teamEntriesDiscovered", APP),
        "team_entries_expired": sum_where(frame, "teamEntriesExpired", APP),

        # ── Aplicação ─────────────────────────────────────────────────────
        "alert_pdr_pct": ratio(delivered, generated, 100),
        "alert_loss_pct": ratio(generated - delivered, generated, 100),
        "appack_pct": ratio(acked, generated, 100),
        "attempt_delivery_pct": ratio(attempts_received, attempts, 100),
        "delivery_delay_mean_s": (
            ratio(delivery_delay_sum, delivery_delay_count)
            if math.isfinite(delivery_delay_sum)
            else mean_where(frame, "deliveryDelay:mean")
        ),
        "attempt_delivery_delay_mean_s": (
            ratio(attempt_delay_sum, attempt_delay_count)
            if math.isfinite(attempt_delay_sum) else math.nan
        ),
        "position_updates_sent": sum_where(frame, "positionUpdatesSent", APP),
        "duplicate_packets": sum_where(frame, "duplicatePackets", APP),
    }


def aggregate(runs: pd.DataFrame) -> pd.DataFrame:
    metrics = [column for column in runs.columns
               if column not in {"config", "seed", "run_file"}]
    rows = []
    for config, frame in runs.groupby("config"):
        row = {"config": config, "seeds": frame["seed"].nunique()}
        for metric in metrics:
            clean = frame[metric].dropna()
            row[f"{metric}_mean"] = clean.mean() if len(clean) else math.nan
            row[f"{metric}_median"] = clean.median() if len(clean) else math.nan
            row[f"{metric}_std"] = clean.std(ddof=1) if len(clean) > 1 else math.nan
            row[f"{metric}_ci95"] = ci95(clean)
        rows.append(row)
    return pd.DataFrame(rows)


HIGHLIGHT = [
    ("mac_frames_sent", "Quadros MAC enviados", "{:.0f}"),
    ("mac_retry_limit_pct", "Perda MAC por limite de retransmissão (%)", "{:.2f}"),
    ("mac_corrupt_pct", "Quadros recebidos com erro (%)", "{:.2f}"),
    ("ip_drop_no_route", "Descartes IP sem rota", "{:.1f}"),
    ("hop_count_mean", "Saltos por alerta entregue", "{:.2f}"),
    ("rssi_mean_dbm", "RSSI médio (dBm)", "{:.1f}"),
    ("rssi_min_dbm", "RSSI mínimo (dBm)", "{:.1f}"),
    ("rssi_samples_available", "Amostras RSSI disponíveis", "{:.0f}"),
    ("rssi_samples_missing", "Amostras RSSI ausentes", "{:.0f}"),
    ("team_entries_discovered", "Entradas de equipe descobertas", "{:.0f}"),
    ("team_entries_expired", "Entradas de equipe expiradas", "{:.0f}"),
    ("delivery_delay_mean_s", "Atraso unidirecional médio (s)", "{:.4f}"),
    ("attempt_delivery_delay_mean_s", "Atraso médio por tentativa (s)", "{:.4f}"),
    ("sensor_outside_range", "Rejeições por obstáculo fora de alcance", "{:.1f}"),
    ("sensor_clear_line_of_sight", "Rejeições por visada livre", "{:.1f}"),
    ("reposition_validation_pct", "Posições do BA validadas por tentativa (%)", "{:.1f}"),
    ("operational_reposition_recovery_pct", "Recuperação operacional após movimento (%)", "{:.1f}"),
    ("validated_recovery_time_mean_s", "Tempo até validação causal (s)", "{:.4f}"),
    ("operational_recovery_time_mean_s", "Tempo até recuperação operacional (s)", "{:.4f}"),
    ("attempt_delivery_pct", "Entrega por tentativa (%)", "{:.1f}"),
    ("alert_pdr_pct", "PDR global de alertas (%)", "{:.1f}"),
    ("alert_loss_pct", "Perda global de alertas (%)", "{:.1f}"),
    ("appack_pct", "AppACK (%)", "{:.1f}"),
]


def report(summary: pd.DataFrame) -> None:
    for _, row in summary.iterrows():
        print(f"\n── {row['config']}  ({int(row['seeds'])} seeds) "
              f"{'─' * max(0, 46 - len(str(row['config'])))}")
        for metric, label, fmt in HIGHLIGHT:
            mean = row.get(f"{metric}_mean", math.nan)
            half = row.get(f"{metric}_ci95", math.nan)
            if isinstance(mean, float) and math.isnan(mean):
                print(f"  {label:<44} indefinido")
                continue
            value = fmt.format(mean)
            if isinstance(half, float) and math.isfinite(half) and half > 0:
                print(f"  {label:<44} {value} ± {fmt.format(half)}")
            else:
                print(f"  {label:<44} {value}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("results", nargs="?",
                        default=os.path.join(ROOT, "simulations", "results", "omnetpp"))
    parser.add_argument("--configs", nargs="*", default=None,
                        help="restrict the report to these configuration names")
    arguments = parser.parse_args()
    os.makedirs(OUTPUT, exist_ok=True)

    records = [collect(path)
               for path in sorted(glob.glob(os.path.join(arguments.results, "*.sca")))]
    if not records:
        print(f"No .sca files in {arguments.results}", file=sys.stderr)
        sys.exit(1)
    runs = pd.DataFrame(records)
    if arguments.configs:
        runs = runs[runs.config.isin(arguments.configs)]
        if runs.empty:
            print(f"No runs for {arguments.configs}", file=sys.stderr)
            sys.exit(1)

    summary = aggregate(runs)
    runs.to_csv(os.path.join(OUTPUT, "network_metrics_runs.csv"), index=False)
    summary.to_csv(os.path.join(OUTPUT, "network_metrics_summary.csv"), index=False)
    report(summary)
    print("\nMétricas diagnósticas — descrevem o comportamento da rede, não são "
          "\nevidência sobre a hipótese. Intervalos são IC95% entre seeds.")


if __name__ == "__main__":
    main()
