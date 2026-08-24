"""Analyze the minimal paired BA Off/On confirmatory experiment.

The seed is the experimental unit. Before calculating effects, this report
requires a one-to-one seed pairing and verifies that recorded module parameters
differ only in ``baEnabled``. This turns accidental configuration drift into a
hard failure instead of silently attributing it to the Bat Algorithm.
"""

from __future__ import annotations

import math
from pathlib import Path
import sys

import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.network_metrics import collect  # noqa: E402
from analysis.core.process_results import ci95, parse_sca  # noqa: E402

RESULTS = REPOSITORY_ROOT / "simulations/results/omnetpp"
OUTPUT = REPOSITORY_ROOT / "analysis/figures"
CONTROL = "MainExperiment_BaOff"
TREATMENT = "MainExperiment_BaOn"
PRIMARY_METRICS = (
    "alert_pdr_pct",
    "appack_pct",
    "delivery_delay_mean_s",
)

MECHANISM_METRICS = (
    "repositions_started",
    "repositions_validated",
    "reposition_validation_pct",
    "validated_recovery_time_mean_s",
    "reposition_distance_sum_m",
)


def result_files(config: str) -> list[Path]:
    return sorted(RESULTS.glob(f"{config}-*.sca"))


def parameter_differences(control_path: Path, treatment_path: Path) -> dict:
    """Return recorded parameter differences other than the treatment flag."""
    _, _, control = parse_sca(str(control_path))
    _, _, treatment = parse_sca(str(treatment_path))
    differences = {}
    for key in sorted(set(control) | set(treatment)):
        before = control.get(key)
        after = treatment.get(key)
        if before != after and not key.endswith(" baEnabled"):
            differences[key] = (before, after)
    return differences


def load_arm(config: str, enabled: bool) -> pd.DataFrame:
    paths = result_files(config)
    if not paths:
        raise ValueError(f"{config}: no result files found")
    rows = []
    for path in paths:
        row = collect(str(path))
        row["ba_enabled"] = enabled
        row["result_path"] = str(path)
        rows.append(row)
    frame = pd.DataFrame(rows)
    if frame["seed"].nunique() != len(paths):
        raise ValueError(f"{config}: seeds are missing or duplicated")
    return frame


def pair_runs(control: pd.DataFrame, treatment: pd.DataFrame) -> pd.DataFrame:
    """Pair arms by seed and calculate neutral treatment-minus-control effects."""
    control_seeds = set(control["seed"])
    treatment_seeds = set(treatment["seed"])
    if control_seeds != treatment_seeds:
        raise ValueError(
            f"unpaired seeds: control-only={sorted(control_seeds - treatment_seeds)}, "
            f"treatment-only={sorted(treatment_seeds - control_seeds)}")

    paired = control[["seed", *PRIMARY_METRICS, "result_path"]].merge(
        treatment[["seed", *PRIMARY_METRICS, "result_path"]],
        on="seed", suffixes=("_off", "_on"), validate="one_to_one")
    for row in paired.itertuples(index=False):
        differences = parameter_differences(
            Path(row.result_path_off), Path(row.result_path_on))
        if differences:
            preview = ", ".join(
                f"{key}: {values[0]} -> {values[1]}"
                for key, values in list(differences.items())[:5])
            raise ValueError(f"seed {row.seed}: parameter drift ({preview})")

    for metric in PRIMARY_METRICS:
        paired[f"{metric}_effect"] = (
            paired[f"{metric}_on"] - paired[f"{metric}_off"])
    return paired


def require_informative_treatment(treatment: pd.DataFrame) -> None:
    """Reject a study in which the treatment was never actually exercised."""
    checks = {
        "network degradation indication": treatment["degradation_indications"].sum(),
        "sensor-confirmed obstacle": treatment["sensor_confirmations"].sum(),
        "BA activation": treatment["ba_activations"].sum(),
        "reposition movement": treatment["repositions_started"].sum(),
    }
    missing = [name for name, total in checks.items() if total <= 0]
    if missing:
        raise ValueError(
            "uninformative treatment: no " + ", ".join(missing) +
            ". Calibrate the scenario before drawing conclusions; do not change "
            "thresholds after inspecting treatment outcomes.")


def summarize(control: pd.DataFrame, treatment: pd.DataFrame,
              paired: pd.DataFrame) -> pd.DataFrame:
    rows = []
    for metric in PRIMARY_METRICS:
        effects = paired[f"{metric}_effect"].dropna()
        rows.append({
            "metric": metric,
            "paired_n": len(effects),
            "control_mean": control[metric].mean(),
            "treatment_mean": treatment[metric].mean(),
            "effect_on_minus_off": effects.mean() if len(effects) else math.nan,
            "effect_std": effects.std(ddof=1) if len(effects) > 1 else math.nan,
            "effect_ci95_half_width": ci95(effects),
        })
    return pd.DataFrame(rows)


def main() -> None:
    control = load_arm(CONTROL, False)
    treatment = load_arm(TREATMENT, True)
    paired = pair_runs(control, treatment)
    require_informative_treatment(treatment)
    summary = summarize(control, treatment, paired)

    OUTPUT.mkdir(exist_ok=True)
    pd.concat([control, treatment], ignore_index=True).to_csv(
        OUTPUT / "main_experiment_runs.csv", index=False)
    paired.drop(columns=["result_path_off", "result_path_on"]).to_csv(
        OUTPUT / "main_experiment_paired_effects.csv", index=False)
    summary.to_csv(OUTPUT / "main_experiment_summary.csv", index=False)
    treatment[["seed", *MECHANISM_METRICS]].to_csv(
        OUTPUT / "main_experiment_ba_mechanism.csv", index=False)

    print(summary.to_string(index=False, float_format=lambda value: f"{value:.4f}"))
    print("\nEfeito = BA On - BA Off; unidade experimental = seed pareada.")
    print("Métricas do mecanismo BA foram exportadas separadamente e não são "
          "comparadas ao controle quando o denominador não existe.")


if __name__ == "__main__":
    main()
