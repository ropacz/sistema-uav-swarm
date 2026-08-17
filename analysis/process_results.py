"""Aggregate ECHOSAR-Net OMNeT++ scalars and compare paired BA runs.

Usage: python3 analysis/process_results.py [results-directory]
Outputs CSV tables and four PNG/PDF figures under analysis/figures.
"""

from __future__ import annotations

import glob
import math
import os
import re
import sys
from statistics import NormalDist

import pandas as pd


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RESULTS = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "simulations", "results")
OUTPUT = os.path.join(ROOT, "analysis", "figures")
os.makedirs(OUTPUT, exist_ok=True)


def parse_sca(path: str) -> tuple[dict[str, str], pd.DataFrame]:
    attrs: dict[str, str] = {}
    rows = []
    with open(path, encoding="utf-8", errors="replace") as source:
        for line in source:
            attr = re.match(r'attr (\S+) "?([^"\n]+)"?', line)
            if attr:
                attrs[attr.group(1)] = attr.group(2).strip()
                continue
            scalar = re.match(r'scalar (\S+) "?([^"\n]+?)"? ([^\s]+)$', line)
            if scalar:
                try:
                    value = float(scalar.group(3))
                except ValueError:
                    continue
                rows.append((scalar.group(1), scalar.group(2).strip(), value))
    attrs.setdefault("configname", "unknown")
    attrs.setdefault("seedset", attrs.get("repetition", "0"))
    return attrs, pd.DataFrame(rows, columns=["module", "name", "value"])


def scalar_sum(frame: pd.DataFrame, name: str) -> float:
    values = frame.loc[frame["name"] == name, "value"]
    return float(values.sum()) if len(values) else 0.0


def load_runs() -> pd.DataFrame:
    records = []
    for path in glob.glob(os.path.join(RESULTS, "*.sca")):
        attrs, frame = parse_sca(path)
        generated = scalar_sum(frame, "uniqueAlertsGenerated")
        acked = scalar_sum(frame, "uniqueAlertsAcked")
        attempts = scalar_sum(frame, "alertAttemptsSent")
        received_attempts = scalar_sum(frame, "attemptsReceived")
        rtt_total = scalar_sum(frame, "totalRTT")
        delay_total = scalar_sum(frame, "totalDeliveryDelay")
        alert_age_total = scalar_sum(frame, "totalAlertAgeAtReception")
        distance = scalar_sum(frame, "baDistance")
        recovery_samples = scalar_sum(frame, "recoverySamples")
        validation_samples = scalar_sum(frame, "repositionValidationSamples")
        pre_rssi_samples = scalar_sum(frame, "preRepositionRssiSamples")
        post_rssi_samples = scalar_sum(frame, "postRepositionRssiSamples")
        records.append({
            "config": attrs["configname"],
            "seed": int(float(attrs["seedset"])),
            "run_file": os.path.basename(path),
            "appack_pct": 100 * acked / generated if generated else math.nan,
            "pdr_pct": 100 * received_attempts / attempts if attempts else math.nan,
            "one_way_delay_s": delay_total / received_attempts if received_attempts else math.nan,
            "alert_age_at_reception_s": alert_age_total / received_attempts if received_attempts else math.nan,
            "rtt_s": rtt_total / acked if acked else math.nan,
            "attempts_per_alert": attempts / generated if generated else math.nan,
            "ba_distance_m": distance / generated if generated else 0.0,
            "alerts_generated": generated,
            "alerts_acked": acked,
            "alerts_expired": scalar_sum(frame, "alertsExpired"),
            "duplicates": scalar_sum(frame, "duplicateAlerts"),
            "ba_activations": scalar_sum(frame, "baActivations"),
            "successful_repositions": scalar_sum(frame, "successfulRepositions"),
            "failed_repositions": scalar_sum(frame, "failedRepositions"),
            "degradation_indications": scalar_sum(frame, "degradationIndications"),
            "sensor_confirmations": scalar_sum(frame, "sensorConfirmations"),
            "sensor_rejections": scalar_sum(frame, "sensorRejections"),
            "recovery_time_s": scalar_sum(frame, "totalRecoveryTime") / recovery_samples
                if recovery_samples else math.nan,
            "pre_reposition_pdr": scalar_sum(frame, "preRepositionPdrSum") / validation_samples
                if validation_samples else math.nan,
            "post_reposition_pdr": scalar_sum(frame, "postRepositionPdrSum") / validation_samples
                if validation_samples else math.nan,
            "pre_reposition_rssi_dbm": scalar_sum(frame, "preRepositionRssiSum") / pre_rssi_samples
                if pre_rssi_samples else math.nan,
            "post_reposition_rssi_dbm": scalar_sum(frame, "postRepositionRssiSum") / post_rssi_samples
                if post_rssi_samples else math.nan,
        })
    if not records:
        raise FileNotFoundError(f"No .sca files in {RESULTS}")
    return pd.DataFrame(records)


def ci95(values: pd.Series) -> float:
    clean = values.dropna()
    if len(clean) < 2:
        return 0.0
    degrees = len(clean) - 1
    z = NormalDist().inv_cdf(0.975)
    # Cornish-Fisher expansion for the two-sided Student-t critical value.
    critical = z + (z**3 + z) / (4 * degrees) + \
        (5 * z**5 + 16 * z**3 + 3 * z) / (96 * degrees**2)
    return critical * clean.std(ddof=1) / math.sqrt(len(clean))


def aggregate(runs: pd.DataFrame) -> pd.DataFrame:
    metrics = [column for column in runs.columns if column not in {"config", "seed", "run_file"}]
    rows = []
    for config, frame in runs.groupby("config"):
        row = {"config": config, "n": frame["seed"].nunique()}
        for metric in metrics:
            clean = frame[metric].dropna()
            row[f"{metric}_mean"] = clean.mean() if len(clean) else math.nan
            row[f"{metric}_median"] = clean.median() if len(clean) else math.nan
            row[f"{metric}_std"] = clean.std(ddof=1) if len(clean) > 1 else math.nan
            row[f"{metric}_ci95"] = ci95(clean)
        rows.append(row)
    return pd.DataFrame(rows)


def paired(runs: pd.DataFrame) -> pd.DataFrame:
    control = "Experiment_Control_BaOff"
    proposed = "Experiment_Proposed_BaOn"
    metrics = ["appack_pct", "one_way_delay_s", "attempts_per_alert", "ba_distance_m"]
    left = runs[runs.config == control].set_index("seed")
    right = runs[runs.config == proposed].set_index("seed")
    if left.index.has_duplicates or right.index.has_duplicates:
        raise ValueError("Experiment configurations contain duplicate seeds")
    if (not left.empty or not right.empty) and set(left.index) != set(right.index):
        raise ValueError("Paired experiment is incomplete: BA on/off seed sets differ")
    common = left.index.intersection(right.index)
    rows = []
    for metric in metrics:
        difference = right.loc[common, metric] - left.loc[common, metric]
        t_statistic = difference.mean() / (difference.std(ddof=1) / math.sqrt(len(difference))) \
            if len(difference) > 1 and difference.std(ddof=1) > 0 else math.nan
        rows.append({"metric": metric, "n_pairs": len(common),
                     "mean_difference_proposed_minus_control": difference.mean(),
                     "median_difference": difference.median(),
                     "std_difference": difference.std(ddof=1), "ci95": ci95(difference),
                     "paired_t_statistic": t_statistic})
    return pd.DataFrame(rows)


def plot(runs: pd.DataFrame) -> None:
    selected = runs[runs.config.isin(["Experiment_Control_BaOff", "Experiment_Proposed_BaOn"])]
    if selected.empty:
        return
    os.environ.setdefault("MPLCONFIGDIR", "/tmp/echosar-matplotlib")
    os.environ.setdefault("XDG_CACHE_HOME", "/tmp/echosar-cache")
    import matplotlib.pyplot as plt
    for metric, label in [("appack_pct", "AppACK (%)"),
                          ("one_way_delay_s", "One-way delay (s)"),
                          ("attempts_per_alert", "Attempts per alert"),
                          ("ba_distance_m", "BA displacement per alert (m)")]:
        summary = selected.groupby("config")[metric].agg(["mean", "std"])
        ax = summary["mean"].plot.bar(yerr=summary["std"].fillna(0), capsize=5,
                                      color=["#777777", "#2878b5"])
        ax.set_ylabel(label)
        ax.set_xlabel("")
        ax.tick_params(axis="x", rotation=12)
        ax.spines[["top", "right"]].set_visible(False)
        plt.tight_layout()
        slug = metric.replace("_pct", "")
        plt.savefig(os.path.join(OUTPUT, f"metric_{slug}.png"), dpi=160)
        plt.savefig(os.path.join(OUTPUT, f"metric_{slug}.pdf"))
        plt.close()


def main() -> None:
    runs = load_runs()
    summary = aggregate(runs)
    pairs = paired(runs)
    runs.to_csv(os.path.join(OUTPUT, "runs.csv"), index=False)
    summary.to_csv(os.path.join(OUTPUT, "summary.csv"), index=False)
    pairs.to_csv(os.path.join(OUTPUT, "paired_comparison.csv"), index=False)
    plot(runs)
    print(summary.to_string(index=False))
    print("\nPaired BA comparison:\n", pairs.to_string(index=False))


if __name__ == "__main__":
    main()
