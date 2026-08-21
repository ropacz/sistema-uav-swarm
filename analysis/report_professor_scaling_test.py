"""Resume a sonda 2x2 de vítimas e obstáculos; n=3 é apenas diagnóstico."""

from pathlib import Path
import re

import pandas as pd

from network_metrics import APP, collect, sum_where
from process_results import parse_sca

ROOT = Path(__file__).resolve().parents[1]
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
    attempts = sum_where(frame, "alertAttemptsSent", APP)
    received = sum_where(frame, "attemptsReceived", APP)
    base.update({
        "victims": configured_victims(path),
        "obstacles": 1 if "Obs01" in base["config"] else 20,
        "ba_enabled": "BaOn" in base["config"],
        "alerts_generated": sum_where(frame, "uniqueAlertsGenerated", APP),
        "alerts_acked": sum_where(frame, "uniqueAlertsAcked", APP),
        "alerts_expired": sum_where(frame, "alertsExpired", APP),
        "attempts": attempts,
        "attempt_loss_pct": 100 * (1 - received / attempts) if attempts else float("nan"),
        "degradation_indications": sum_where(frame, "degradationIndications", APP),
        "sensor_confirmations": sum_where(frame, "sensorConfirmations", APP),
        "ba_activations": sum_where(frame, "baActivations", APP),
        "successful_repositions": sum_where(frame, "successfulRepositions", APP),
        "recovered_during_movement": sum_where(frame, "repositionAckedBeforeValidation", APP),
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
               "ba_activations", "successful_repositions", "recovered_during_movement",
               "obstacle_intersections"]
    summary = runs.groupby(["victims", "obstacles", "ba_enabled"])[metrics].agg(["count", "mean", "std"])
    OUTPUT.mkdir(exist_ok=True)
    runs.to_csv(OUTPUT / "professor_scaling_runs.csv", index=False)
    summary.to_csv(OUTPUT / "professor_scaling_summary.csv")
    print(summary.to_string())
    print("\nSonda exploratória n=3; não usar como teste confirmatório.")


if __name__ == "__main__":
    main()
