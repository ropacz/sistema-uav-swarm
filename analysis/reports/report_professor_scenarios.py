"""Resumo descritivo dos cenários da professora, com uma linha por execução."""

from __future__ import annotations

import glob
import os
import re
import sys
from pathlib import Path

import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.network_metrics import (  # noqa: E402
    APP, collect, global_or_legacy, sum_where,
)
from analysis.core.process_results import parse_sca  # noqa: E402

ROOT = str(REPOSITORY_ROOT)
RESULTS = os.path.join(ROOT, "simulations", "results", "omnetpp")
OUTPUT = os.path.join(ROOT, "analysis", "figures")
PREFIX = "Scenario1_"


def configured_teams(path: str) -> int:
    with open(path, encoding="utf-8") as stream:
        for line in stream:
            match = re.match(r"config \*\.numTeams\s+(\d+)", line)
            if match:
                return int(match.group(1))
    raise ValueError(f"numTeams ausente em {path}")


def run_record(path: str) -> dict:
    attrs, frame, _ = parse_sca(path)
    row = collect(path)
    generated = global_or_legacy(frame, "alertsGenerated", "uniqueAlertsGenerated")
    delivered = global_or_legacy(frame, "alertsDelivered", "uniqueAlertsReceived")
    acked = global_or_legacy(frame, "alertsConfirmed", "uniqueAlertsAcked")
    expired = global_or_legacy(frame, "alertsExpired", "alertsExpired")
    attempts = global_or_legacy(frame, "alertAttemptsSent", "alertAttemptsSent")
    received = global_or_legacy(frame, "alertAttemptsDelivered", "attemptsReceived")
    row.update({
        "repetition": int(float(attrs.get("repetition", row["seed"]))),
        "teams": configured_teams(path),
        "alerts_generated": generated,
        "alerts_delivered": delivered,
        "alerts_acked": acked,
        "alerts_expired": expired,
        "pdr_pct": 100 * delivered / generated if generated else float("nan"),
        "confirmation_pct": 100 * acked / generated if generated else float("nan"),
        # Alias histórico: os gráficos existentes chamam confirmação de atendimento.
        "attendance_pct": 100 * acked / generated if generated else float("nan"),
        "attempt_loss_pct": 100 * (1 - received / attempts) if attempts else float("nan"),
        "degradation_indications": global_or_legacy(
            frame, "degradationIndications", "degradationIndications"),
        "sensor_obstacle_confirmed": global_or_legacy(
            frame, "sensorConfirmations", "sensorConfirmations"),
        "ba_activations": global_or_legacy(frame, "baActivations", "baActivations"),
        "successful_repositions": global_or_legacy(
            frame, "successfulRepositions", "successfulRepositions"),
    })
    if generated != acked + expired:
        raise ValueError(f"Conservação violada em {path}: {generated} != {acked}+{expired}")
    return row


def main() -> None:
    paths = sorted(glob.glob(os.path.join(RESULTS, f"{PREFIX}*.sca")))
    if not paths:
        raise SystemExit(f"Nenhum resultado {PREFIX} em {RESULTS}")
    runs = pd.DataFrame(run_record(path) for path in paths)
    metrics = ["pdr_pct", "confirmation_pct", "attendance_pct",
               "attempt_loss_pct", "hop_count_mean",
               "delivery_delay_mean_s", "degradation_indications",
               "sensor_obstacle_confirmed", "ba_activations",
               "successful_repositions"]
    summary = runs.groupby(["config", "teams"])[metrics].agg(["count", "mean", "std"])
    os.makedirs(OUTPUT, exist_ok=True)
    runs.to_csv(os.path.join(OUTPUT, "professor_runs.csv"), index=False)
    summary.to_csv(os.path.join(OUTPUT, "professor_summary.csv"))
    print(summary.to_string())
    print("\nA unidade experimental é a seed; confira a contagem antes de inferência.")


if __name__ == "__main__":
    main()
