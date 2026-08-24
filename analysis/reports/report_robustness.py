"""Descriptive report for the single optional robustness experiment."""

from __future__ import annotations

import argparse
import glob
import os
import re
import sys
from pathlib import Path

import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402
from analysis.core.process_results import parse_sca  # noqa: E402

RESULTS = REPOSITORY_ROOT / "simulations/results/omnetpp"
OUTPUT = REPOSITORY_ROOT / "analysis/figures"


def configured_teams(path: str) -> int:
    with open(path, encoding="utf-8") as stream:
        for line in stream:
            match = re.match(r"config \*\.numTeams\s+(\d+)", line)
            if match:
                return int(match.group(1))
    raise ValueError(f"numTeams ausente em {path}")


def run_record(path: str) -> dict:
    attrs, _, _ = parse_sca(path)
    row = collect(path)
    row["repetition"] = int(float(attrs.get("repetition", row["seed"])))
    row["teams"] = configured_teams(path)
    if row["alerts_confirmed"] + row["alerts_expired"] != row["alerts_generated"]:
        raise ValueError(f"conservação terminal violada em {path}")
    return row


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--configs", nargs="+", required=True)
    arguments = parser.parse_args()
    paths = sorted(
        path
        for config in arguments.configs
        for path in glob.glob(str(RESULTS / f"{config}-*.sca"))
    )
    if not paths:
        raise SystemExit(f"nenhum resultado solicitado em {RESULTS}")

    runs = pd.DataFrame(run_record(path) for path in paths)
    metrics = [
        "alert_pdr_pct", "appack_pct", "delivery_delay_mean_s",
        "retries_per_alert", "reposition_triggers", "obstacles_detected",
        "ba_activations", "repositions_started", "repositions_completed",
        "reposition_distance_sum_m",
    ]
    summary = runs.groupby(["config", "teams"])[metrics].agg(
        ["count", "mean", "std"]
    )
    OUTPUT.mkdir(exist_ok=True)
    runs.to_csv(OUTPUT / "robustness_runs.csv", index=False)
    summary.to_csv(OUTPUT / "robustness_summary.csv")
    print(summary.to_string())
    print("\nRelatório descritivo opcional; a unidade experimental é a seed.")


if __name__ == "__main__":
    main()
