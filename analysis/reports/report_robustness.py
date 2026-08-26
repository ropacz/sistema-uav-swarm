"""Paired report for the one/two-victim robustness matrix."""

from __future__ import annotations

import argparse
import glob
import re
import sys
from pathlib import Path

import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402
from analysis.core.process_results import ci95, parse_sca  # noqa: E402
from analysis.reports.report_main_experiment import (  # noqa: E402
    parameter_differences,
    recorded_ba_enabled,
)

RESULTS = REPOSITORY_ROOT / "simulations/results/omnetpp"
OUTPUT = REPOSITORY_ROOT / "analysis/tables/robustness"
METRICS = (
    "alert_pdr_pct", "alert_loss_pct", "appack_pct",
    "delivery_delay_mean_s", "confirmation_delay_mean_s",
    "retries_per_alert", "attempt_pdr_pct", "attempt_loss_pct",
    "mean_hop_count", "multi_hop_delivery_rate_pct",
    "never_known_team_selection_events", "expired_known_team_selection_events",
    "known_team_no_ack_timeout_events", "alerts_without_known_team",
    "reposition_triggers", "obstacles_detected", "ba_activations",
    "repositions_started", "repositions_completed",
    "reposition_distance_sum_m", "reposition_distance_mean_m",
    "reposition_duration_mean_s",
)


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
    suffixes = {"_BaOff": False, "_BaOn": True}
    matches = [
        (suffix, enabled)
        for suffix, enabled in suffixes.items()
        if row["config"].endswith(suffix)
    ]
    if len(matches) != 1:
        raise ValueError(f"braço BA não identificável em {row['config']}")
    suffix, enabled = matches[0]
    # O sufixo do nome declara o braço; o .sca diz o que a execução realmente
    # usou. Divergência significa configuração herdada da base errada.
    actual = recorded_ba_enabled(Path(path))
    if actual != enabled:
        raise ValueError(f"{row['config']}: sufixo declara baEnabled={enabled}, "
                         f"mas a execução gravou baEnabled={actual} em {path}")
    row["scenario"] = row["config"][:-len(suffix)]
    row["ba_enabled"] = enabled
    row["result_path"] = path
    if row["alerts_confirmed"] + row["alerts_expired"] > row["alerts_generated"]:
        raise ValueError(f"conservação de alertas violada em {path}")
    return row


def pair_runs(runs: pd.DataFrame, check_parameters: bool = True) -> pd.DataFrame:
    """Pair every robustness cell by scenario, team count and seed."""
    keys = ["scenario", "teams", "seed"]
    control = runs.loc[~runs["ba_enabled"], [*keys, *METRICS, "result_path"]]
    treatment = runs.loc[runs["ba_enabled"], [*keys, *METRICS, "result_path"]]
    paired = control.merge(
        treatment,
        on=keys,
        how="outer",
        suffixes=("_off", "_on"),
        indicator=True,
        validate="one_to_one",
    )
    unpaired = paired.loc[paired["_merge"] != "both", keys]
    if not unpaired.empty:
        raise ValueError(
            "células sem par BA Off/On: "
            + unpaired.head().to_dict(orient="records").__repr__()
        )
    paired = paired.drop(columns="_merge")
    if check_parameters:
        for row in paired.itertuples(index=False):
            differences = parameter_differences(
                Path(row.result_path_off), Path(row.result_path_on)
            )
            if differences:
                preview = ", ".join(
                    f"{key}: {values[0]} -> {values[1]}"
                    for key, values in list(differences.items())[:5]
                )
                raise ValueError(
                    f"{row.scenario}, equipes={row.teams}, seed={row.seed}: "
                    f"parameter drift ({preview})"
                )
    for metric in METRICS:
        paired[f"{metric}_effect"] = (
            paired[f"{metric}_on"] - paired[f"{metric}_off"]
        )
    return paired


def summarize_arms(runs: pd.DataFrame) -> pd.DataFrame:
    """Describe complete executions, never individual packets."""
    return runs.groupby(
        ["scenario", "teams", "ba_enabled"]
    )[list(METRICS)].agg(["count", "mean", "median", "std"])


def summarize_effects(paired: pd.DataFrame) -> pd.DataFrame:
    """Summarize treatment-minus-control effects over paired seeds."""
    rows = []
    for (scenario, teams), group in paired.groupby(["scenario", "teams"]):
        for metric in METRICS:
            effects = group[f"{metric}_effect"].dropna()
            rows.append({
                "scenario": scenario,
                "teams": teams,
                "metric": metric,
                "paired_n": len(effects),
                "effect_mean": effects.mean(),
                "effect_median": effects.median(),
                "effect_std": effects.std(ddof=1),
                "effect_ci95_half_width": ci95(effects),
            })
    return pd.DataFrame(rows)


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
    paired = pair_runs(runs)
    arm_summary = summarize_arms(runs)
    effect_summary = summarize_effects(paired)
    OUTPUT.mkdir(parents=True, exist_ok=True)
    runs.to_csv(OUTPUT / "runs.csv", index=False)
    paired.drop(columns=["result_path_off", "result_path_on"]).to_csv(
        OUTPUT / "paired_effects.csv", index=False
    )
    arm_summary.to_csv(OUTPUT / "arm_summary.csv")
    effect_summary.to_csv(OUTPUT / "summary.csv", index=False)
    print(effect_summary.to_string(index=False))
    print("\nEfeito = BA On - BA Off; unidade experimental = seed pareada.")


if __name__ == "__main__":
    main()
