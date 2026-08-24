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

from analysis.core.experiment_metrics import central_scalar  # noqa: E402
from analysis.core.process_results import ci95, parse_sca  # noqa: E402

ROOT = str(REPOSITORY_ROOT)
OUTPUT = os.path.join(ROOT, "analysis", "figures", "diagnostics")

# Sufixos de módulo. O MAC publica os mesmos nomes de contador em `.mac` e em
# `.mac.dcf`; somar por nome sem filtrar o módulo contaria cada quadro duas
# vezes.
MAC = r"\.wlan\[\d+\]\.mac$"
IP = r"\.ipv4\.ip$"
UDP = r"\.udp$"


def sum_where(frame: pd.DataFrame, name: str, module_pattern: str | None = None) -> float:
    selected = frame["name"] == name
    if module_pattern:
        selected &= frame["module"].str.contains(module_pattern, regex=True)
    values = frame.loc[selected, "value"]
    return float(values.sum()) if len(values) else 0.0


def ratio(numerator: float, denominator: float, scale: float = 1.0) -> float:
    return scale * numerator / denominator if denominator else math.nan


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

    generated = central_scalar(frame, "alertsGenerated")
    delivered = central_scalar(frame, "alertsDelivered")
    acked = central_scalar(frame, "alertsConfirmed")
    attempts = central_scalar(frame, "alertAttemptsSent")
    retries = central_scalar(frame, "applicationRetries")
    delivery_delay_sum = central_scalar(frame, "deliveryDelaySum")
    delivery_delay_count = central_scalar(frame, "deliveryDelayCount")

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
        # ── Transporte ────────────────────────────────────────────────────
        "udp_packets_sent": udp_sent,
        "udp_packets_received": udp_received,
        "udp_bytes_received": sum_where(frame, "packetReceived:sum(packetBytes)", UDP),
        "udp_drop_wrong_port": sum_where(frame, "droppedPkWrongPort:count", UDP),

        # ── Exposição à política de reposicionamento ──────────────────────
        "reposition_triggers": central_scalar(frame, "repositionTriggers"),
        "obstacles_detected": central_scalar(frame, "obstaclesDetected"),
        "ba_activations": central_scalar(frame, "baActivations"),
        "repositions_started": central_scalar(frame, "repositionsStarted"),
        "repositions_completed": central_scalar(frame, "repositionsCompleted"),
        "reposition_distance_sum_m": central_scalar(frame, "repositionDistanceSum"),

        # ── Aplicação ─────────────────────────────────────────────────────
        "alert_pdr_pct": ratio(delivered, generated, 100),
        "appack_pct": ratio(acked, generated, 100),
        "delivery_delay_mean_s": ratio(delivery_delay_sum, delivery_delay_count),
        "retries_per_alert": ratio(retries, generated),
        "alert_attempts_sent": attempts,
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
    ("delivery_delay_mean_s", "Atraso unidirecional médio (s)", "{:.4f}"),
    ("alert_pdr_pct", "PDR global de alertas (%)", "{:.1f}"),
    ("appack_pct", "AppACK (%)", "{:.1f}"),
    ("retries_per_alert", "Retransmissões por alerta", "{:.2f}"),
    ("reposition_triggers", "Gatilhos de reposicionamento", "{:.1f}"),
    ("ba_activations", "Ativações do BA", "{:.1f}"),
    ("repositions_completed", "Movimentos concluídos", "{:.1f}"),
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
    runs.to_csv(os.path.join(OUTPUT, "runs.csv"), index=False)
    summary.to_csv(os.path.join(OUTPUT, "summary.csv"), index=False)
    report(summary)
    print("\nMétricas diagnósticas descrevem o comportamento da rede; não são "
          "evidência sobre a hipótese. Intervalos são IC95% entre seeds.")


if __name__ == "__main__":
    main()
