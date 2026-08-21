"""Resumo descritivo dos cenários da professora, com uma linha por execução.

Três seeds são apenas uma sonda de funcionamento. Troque repeat para 30 antes
de usar inferência estatística no artigo.
"""

from __future__ import annotations

import glob
import os
import re

import pandas as pd

from network_metrics import APP, collect, sum_where
from process_results import parse_sca

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
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
    generated = sum_where(frame, "uniqueAlertsGenerated", APP)
    acked = sum_where(frame, "uniqueAlertsAcked", APP)
    expired = sum_where(frame, "alertsExpired", APP)
    attempts = sum_where(frame, "alertAttemptsSent", APP)
    received = sum_where(frame, "attemptsReceived", APP)
    row.update({
        "repetition": int(float(attrs.get("repetition", row["seed"]))),
        "teams": configured_teams(path),
        "alerts_generated": generated,
        "alerts_acked": acked,
        "alerts_expired": expired,
        "attendance_pct": 100 * acked / generated if generated else float("nan"),
        "attempt_loss_pct": 100 * (1 - received / attempts) if attempts else float("nan"),
        "degradation_indications": sum_where(frame, "degradationIndications", APP),
        "sensor_obstacle_confirmed": sum_where(frame, "sensorObstacleConfirmed", APP),
        "ba_activations": sum_where(frame, "baActivations", APP),
        "successful_repositions": sum_where(frame, "successfulRepositions", APP),
    })
    if generated != acked + expired:
        raise ValueError(f"Conservação violada em {path}: {generated} != {acked}+{expired}")
    return row


def main() -> None:
    paths = sorted(glob.glob(os.path.join(RESULTS, f"{PREFIX}*.sca")))
    if not paths:
        raise SystemExit(f"Nenhum resultado {PREFIX} em {RESULTS}")
    runs = pd.DataFrame(run_record(path) for path in paths)
    metrics = ["attendance_pct", "attempt_loss_pct", "hop_count_mean",
               "delivery_delay_mean_s", "degradation_indications",
               "sensor_obstacle_confirmed", "ba_activations",
               "successful_repositions"]
    summary = runs.groupby(["config", "teams"])[metrics].agg(["count", "mean", "std"])
    os.makedirs(OUTPUT, exist_ok=True)
    runs.to_csv(os.path.join(OUTPUT, "professor_runs.csv"), index=False)
    summary.to_csv(os.path.join(OUTPUT, "professor_summary.csv"))
    print(summary.to_string())
    print("\nAtenção: n=3 é validação preliminar; o protocolo final exige n=30.")


if __name__ == "__main__":
    main()
