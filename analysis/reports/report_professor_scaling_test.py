"""Resume a sonda 2x2 de vítimas e obstáculos; n=3 é apenas diagnóstico."""

from pathlib import Path
import re
import sys

import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.network_metrics import (  # noqa: E402
    APP, collect, global_or_legacy, sum_where,
)
from analysis.core.process_results import parse_sca  # noqa: E402

ROOT = REPOSITORY_ROOT
RESULTS = ROOT / "simulations/results/omnetpp"
OUTPUT = ROOT / "analysis/figures"


def configured_victims(path):
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        match = re.match(r"config \*\.numVictims\s+(\d+)", line)
        if match:
            return int(match.group(1))
    raise ValueError(f"numVictims ausente: {path}")


def record(path):
    base = collect(str(path))
    _, frame, _ = parse_sca(str(path))
    attempts = global_or_legacy(frame, "alertAttemptsSent", "alertAttemptsSent")
    received = global_or_legacy(frame, "alertAttemptsDelivered", "attemptsReceived")
    base.update({
        "victims": configured_victims(path),
        "obstacles": 1 if "Obs01" in base["config"] else 20,
        "ba_enabled": "BaOn" in base["config"],
        "alerts_generated": global_or_legacy(
            frame, "alertsGenerated", "uniqueAlertsGenerated"),
        "alerts_delivered": global_or_legacy(
            frame, "alertsDelivered", "uniqueAlertsReceived"),
        "alerts_acked": global_or_legacy(frame, "alertsConfirmed", "uniqueAlertsAcked"),
        "alerts_expired": global_or_legacy(frame, "alertsExpired", "alertsExpired"),
        "attempts": attempts,
        "attempt_loss_pct": 100 * (1 - received / attempts) if attempts else float("nan"),
        "degradation_indications": global_or_legacy(
            frame, "degradationIndications", "degradationIndications"),
        "sensor_confirmations": global_or_legacy(
            frame, "sensorConfirmations", "sensorConfirmations"),
        "ba_activations": global_or_legacy(frame, "baActivations", "baActivations"),
        "successful_repositions": global_or_legacy(
            frame, "successfulRepositions", "successfulRepositions"),
        "recovered_without_validation": global_or_legacy(
            frame, "repositionsRecoveredWithoutValidation",
            "repositionAckedBeforeValidation"),
        "obstacle_intersections": sum_where(frame, "Obstacle loss intersection count"),
    })
    return base


def main():
    paths = sorted(RESULTS.glob("ProfessorScaling_Obs*.sca"))
    if not paths:
        raise SystemExit("resultados ProfessorScaling ausentes")
    runs = pd.DataFrame(record(path) for path in paths)
    metrics = ["appack_pct", "attempt_loss_pct", "delivery_delay_mean_s",
               "mac_frames_received", "mac_retry_limit_pct", "rssi_mean_dbm",
               "degradation_indications", "sensor_confirmations",
               "ba_activations", "successful_repositions", "recovered_without_validation",
               "obstacle_intersections"]
    summary = runs.groupby(["victims", "obstacles", "ba_enabled"])[metrics].agg(["count", "mean", "std"])
    OUTPUT.mkdir(exist_ok=True)
    runs.to_csv(OUTPUT / "professor_scaling_runs.csv", index=False)
    summary.to_csv(OUTPUT / "professor_scaling_summary.csv")
    print(summary.to_string())
    print("\nSonda exploratória n=3; não usar como teste confirmatório.")


if __name__ == "__main__":
    main()
