"""Aggregate ECHOSAR-Net OMNeT++ scalars and compare the paired BA experiment.

Usage:
    python3 analysis/process_results.py [results-directory] [--expected-pairs N]

The script separates scientific from diagnostic data. Only the two experiment
configurations feed the paired comparison; every other configuration in the
results directory (deterministic validation, visual demonstration, packet
capture scenarios) is tabulated apart and never mixed into the conclusion.

It fails, rather than emitting an empty table, when the paired experiment is
missing, incomplete, duplicated, or when the two arms differ by anything other
than the treatment parameter.
"""

from __future__ import annotations

import argparse
import glob
import hashlib
import json
import math
import os
import re
import subprocess
import sys
from datetime import datetime, timezone
from statistics import NormalDist

import pandas as pd


ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT = os.path.join(ROOT, "analysis", "figures")

CONTROL_CONFIG = "Experiment_Control_BaOff"
PROPOSED_CONFIG = "Experiment_Proposed_BaOn"
EXPERIMENT_CONFIGS = (CONTROL_CONFIG, PROPOSED_CONFIG)
DEFAULT_EXPECTED_PAIRS = 30

# A única diferença intencional entre os dois braços. Qualquer outro parâmetro
# divergente invalida o pareamento.
TREATMENT_PARAMETER = "baEnabled"

# Métricas que entram na comparação pareada. A primeira é a métrica primária.
PAIRED_METRICS = ["appack_pct", "one_way_delay_s", "attempts_per_alert", "ba_distance_m"]
PRIMARY_METRIC = "appack_pct"


class IntegrityError(RuntimeError):
    """Raised when the results cannot support a scientific conclusion."""


# ── Leitura dos escalares ────────────────────────────────────────────────────

def parse_sca(path: str) -> tuple[dict[str, str], pd.DataFrame, dict[str, str]]:
    """Return run attributes, recorded scalars and recorded module parameters."""
    attrs: dict[str, str] = {}
    params: dict[str, str] = {}
    rows = []
    attr_pattern = re.compile(r'attr (\S+) "?([^"\n]+)"?')
    scalar_pattern = re.compile(r'scalar (\S+) "?([^"\n]+?)"? ([^\s]+)$')
    param_pattern = re.compile(r'par (\S+) (\S+) (.*)$')
    with open(path, encoding="utf-8", errors="replace") as source:
        for line in source:
            if line.startswith("attr "):
                attr = attr_pattern.match(line)
                if attr:
                    attrs[attr.group(1)] = attr.group(2).strip()
                continue
            if line.startswith("par "):
                param = param_pattern.match(line.rstrip("\n"))
                if param:
                    params[f"{param.group(1)} {param.group(2)}"] = param.group(3).strip()
                continue
            scalar = scalar_pattern.match(line)
            if scalar:
                try:
                    value = float(scalar.group(3))
                except ValueError:
                    continue
                rows.append((scalar.group(1), scalar.group(2).strip(), value))
    attrs.setdefault("configname", "unknown")
    attrs.setdefault("seedset", attrs.get("repetition", "0"))
    return attrs, pd.DataFrame(rows, columns=["module", "name", "value"]), params


def scalar_sum(frame: pd.DataFrame, name: str) -> float:
    values = frame.loc[frame["name"] == name, "value"]
    return float(values.sum()) if len(values) else 0.0


def ratio(numerator: float, denominator: float, scale: float = 1.0) -> float:
    """Ratio that is explicitly undefined — not zero — for an empty population."""
    return scale * numerator / denominator if denominator else math.nan


def load_runs(results: str) -> tuple[pd.DataFrame, dict[tuple[str, int], dict[str, str]]]:
    records = []
    parameters: dict[tuple[str, int], dict[str, str]] = {}
    for path in sorted(glob.glob(os.path.join(results, "*.sca"))):
        attrs, frame, params = parse_sca(path)
        generated = scalar_sum(frame, "uniqueAlertsGenerated")
        acked = scalar_sum(frame, "uniqueAlertsAcked")
        attempts = scalar_sum(frame, "alertAttemptsSent")
        received_attempts = scalar_sum(frame, "attemptsReceived")
        recovery_samples = scalar_sum(frame, "recoverySamples")
        validation_samples = scalar_sum(frame, "repositionValidationSamples")
        pre_rssi_samples = scalar_sum(frame, "preRepositionRssiSamples")
        post_rssi_samples = scalar_sum(frame, "postRepositionRssiSamples")
        config = attrs["configname"]
        seed = int(float(attrs["seedset"]))
        if config in EXPERIMENT_CONFIGS:
            parameters[(config, seed)] = params
        records.append({
            "config": config,
            "seed": seed,
            "run_file": os.path.basename(path),
            # ── Métrica primária ──────────────────────────────────────────
            "appack_pct": ratio(acked, generated, 100),
            # ── Secundárias ───────────────────────────────────────────────
            # Razão entre tentativas da aplicação, não entre quadros MAC.
            "alert_attempt_delivery_pct": ratio(received_attempts, attempts, 100),
            # Condicionadas às tentativas efetivamente recebidas.
            "one_way_delay_s": ratio(scalar_sum(frame, "totalDeliveryDelay"), received_attempts),
            "alert_age_at_reception_s": ratio(
                scalar_sum(frame, "totalAlertAgeAtReception"), received_attempts),
            "rtt_s": ratio(scalar_sum(frame, "totalRTT"), acked),
            "attempts_per_alert": ratio(attempts, generated),
            "ba_distance_m": ratio(scalar_sum(frame, "baDistance"), generated),
            "recovery_time_s": ratio(scalar_sum(frame, "totalRecoveryTime"), recovery_samples),
            "alerts_generated": generated,
            "alerts_acked": acked,
            "alerts_expired": scalar_sum(frame, "alertsExpired"),
            # ── Diagnósticas ──────────────────────────────────────────────
            "duplicate_packets": scalar_sum(frame, "duplicatePackets"),
            "degradation_indications": scalar_sum(frame, "degradationIndications"),
            "sensor_confirmations": scalar_sum(frame, "sensorConfirmations"),
            "sensor_rejections": scalar_sum(frame, "sensorRejections"),
            "team_unknown_for_reposition": scalar_sum(frame, "teamUnknownForReposition"),
            "ba_activations": scalar_sum(frame, "baActivations"),
            "successful_repositions": scalar_sum(frame, "successfulRepositions"),
            "failed_repositions": scalar_sum(frame, "failedRepositions"),
            "ba_no_feasible_solution": scalar_sum(frame, "baNoFeasibleSolution"),
            "ba_redundant_candidate": scalar_sum(frame, "baRedundantCandidate"),
            "reposition_expired_before_ack": scalar_sum(frame, "repositionExpiredBeforeAck"),
            "reposition_acked_before_validation": scalar_sum(
                frame, "repositionAckedBeforeValidation"),
            "pre_reposition_pdr": ratio(scalar_sum(frame, "preRepositionPdrSum"), validation_samples),
            "post_reposition_pdr": ratio(scalar_sum(frame, "postRepositionPdrSum"), validation_samples),
            "pre_reposition_rssi_dbm": ratio(
                scalar_sum(frame, "preRepositionRssiSum"), pre_rssi_samples),
            "post_reposition_rssi_dbm": ratio(
                scalar_sum(frame, "postRepositionRssiSum"), post_rssi_samples),
        })
    if not records:
        raise IntegrityError(f"No .sca files in {results}")
    return pd.DataFrame(records), parameters


# ── Portões de integridade ───────────────────────────────────────────────────

def require_complete_experiment(runs: pd.DataFrame, expected_pairs: int) -> pd.Index:
    """Validate the paired design and return the common seed index."""
    present = [config for config in EXPERIMENT_CONFIGS if (runs.config == config).any()]
    if not present:
        raise IntegrityError(
            "No experiment runs found. Expected both "
            f"{CONTROL_CONFIG} and {PROPOSED_CONFIG} in the results directory. "
            "Deterministic validation and demonstration scenarios are not evidence "
            "and cannot substitute for the paired experiment."
        )
    missing = [config for config in EXPERIMENT_CONFIGS if config not in present]
    if missing:
        raise IntegrityError(f"Paired experiment is missing entirely: {', '.join(missing)}")

    control = runs[runs.config == CONTROL_CONFIG].set_index("seed")
    proposed = runs[runs.config == PROPOSED_CONFIG].set_index("seed")
    for name, frame in ((CONTROL_CONFIG, control), (PROPOSED_CONFIG, proposed)):
        if frame.index.has_duplicates:
            repeated = sorted(frame.index[frame.index.duplicated()].unique())
            raise IntegrityError(f"{name} contains duplicate seeds: {repeated}")
    if set(control.index) != set(proposed.index):
        only_control = sorted(set(control.index) - set(proposed.index))
        only_proposed = sorted(set(proposed.index) - set(control.index))
        raise IntegrityError(
            "Paired experiment is incomplete: seed sets differ. "
            f"Only in control: {only_control}. Only in proposed: {only_proposed}."
        )
    common = control.index.intersection(proposed.index)
    if len(common) != expected_pairs:
        raise IntegrityError(
            f"Paired experiment requires {expected_pairs} seeds; found {len(common)}. "
            "Run both arms to completion, or pass --expected-pairs to mark the "
            "output explicitly as a pilot."
        )
    return common


def require_treatment_isolation(
    parameters: dict[tuple[str, int], dict[str, str]], seeds: pd.Index
) -> int:
    """Assert the two arms differ only by the treatment parameter.

    Both arms record every module parameter in their .sca file. Comparing those
    dictionaries seed by seed is a direct check that the experiment changed the
    treatment and nothing else, instead of trusting the INI to be correct.
    """
    offending: dict[str, tuple[str, str]] = {}
    treatment_changes = 0
    for seed in seeds:
        control = parameters.get((CONTROL_CONFIG, seed))
        proposed = parameters.get((PROPOSED_CONFIG, seed))
        if control is None or proposed is None:
            raise IntegrityError(f"Missing recorded parameters for seed {seed}")
        for key in set(control) | set(proposed):
            control_value = control.get(key, "<absent>")
            proposed_value = proposed.get(key, "<absent>")
            if control_value == proposed_value:
                continue
            if key.split(" ")[-1] == TREATMENT_PARAMETER:
                treatment_changes += 1
                continue
            offending.setdefault(key, (control_value, proposed_value))
    if offending:
        detail = "; ".join(
            f"{key}: control={values[0]!r} proposed={values[1]!r}"
            for key, values in sorted(offending.items())[:10]
        )
        raise IntegrityError(
            f"Control and proposed differ in {len(offending)} parameter(s) beyond "
            f"'{TREATMENT_PARAMETER}'. The comparison is confounded. {detail}"
        )
    if not treatment_changes:
        raise IntegrityError(
            f"Control and proposed recorded an identical '{TREATMENT_PARAMETER}'. "
            "The treatment was never applied."
        )
    return treatment_changes


def data_quality(runs: pd.DataFrame) -> pd.DataFrame:
    """Report undefined metrics and empty denominators, per configuration."""
    rows = []
    metrics = [column for column in runs.columns if column not in {"config", "seed", "run_file"}]
    for config, frame in runs.groupby("config"):
        for metric in metrics:
            undefined = int(frame[metric].isna().sum())
            if undefined:
                rows.append({
                    "config": config,
                    "metric": metric,
                    "runs": len(frame),
                    "undefined_runs": undefined,
                    "reason": "empty denominator (population of size zero)",
                })
    return pd.DataFrame(rows, columns=["config", "metric", "runs", "undefined_runs", "reason"])


# ── Estatística ──────────────────────────────────────────────────────────────

def ci95(values: pd.Series) -> float:
    """Half-width of the 95% confidence interval for the mean."""
    clean = values.dropna()
    if len(clean) < 2:
        return math.nan
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
            row[f"{metric}_n"] = len(clean)
            row[f"{metric}_mean"] = clean.mean() if len(clean) else math.nan
            row[f"{metric}_median"] = clean.median() if len(clean) else math.nan
            row[f"{metric}_std"] = clean.std(ddof=1) if len(clean) > 1 else math.nan
            row[f"{metric}_ci95"] = ci95(clean)
        rows.append(row)
    return pd.DataFrame(rows)


def paired(runs: pd.DataFrame, seeds: pd.Index) -> pd.DataFrame:
    """Per-seed differences, effect size and uncertainty.

    Reports the estimate and its interval, plus the discordant-pair counts that
    a test for paired binary data would consume. It deliberately runs no
    hypothesis test: the primary metric's distribution depends on how many
    alerts each run generates, and choosing the test is a documented decision.
    """
    control = runs[runs.config == CONTROL_CONFIG].set_index("seed")
    proposed = runs[runs.config == PROPOSED_CONFIG].set_index("seed")
    rows = []
    for metric in PAIRED_METRICS:
        difference = (proposed.loc[seeds, metric] - control.loc[seeds, metric]).dropna()
        half_width = ci95(difference)
        mean = difference.mean() if len(difference) else math.nan
        rows.append({
            "metric": metric,
            "primary": metric == PRIMARY_METRIC,
            "n_pairs": len(seeds),
            "n_comparable_pairs": len(difference),
            "mean_difference_proposed_minus_control": mean,
            "median_difference": difference.median() if len(difference) else math.nan,
            "std_difference": difference.std(ddof=1) if len(difference) > 1 else math.nan,
            "ci95_half_width": half_width,
            "ci95_low": mean - half_width if len(difference) > 1 else math.nan,
            "ci95_high": mean + half_width if len(difference) > 1 else math.nan,
            "pairs_proposed_higher": int((difference > 0).sum()),
            "pairs_control_higher": int((difference < 0).sum()),
            "pairs_tied": int((difference == 0).sum()),
        })
    return pd.DataFrame(rows)


# ── Proveniência ─────────────────────────────────────────────────────────────

def file_sha256(relative_path: str) -> str:
    digest = hashlib.sha256()
    with open(os.path.join(ROOT, relative_path), "rb") as source:
        for chunk in iter(lambda: source.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def write_manifest(runs: pd.DataFrame, results: str, expected_pairs: int,
                   treatment_changes: int) -> None:
    """Record the provenance needed to reproduce the processed dataset."""
    try:
        revision = subprocess.check_output(
            ["git", "rev-parse", "HEAD"], cwd=ROOT, text=True
        ).strip()
        dirty = bool(subprocess.check_output(
            ["git", "status", "--porcelain"], cwd=ROOT, text=True
        ).strip())
    except (OSError, subprocess.CalledProcessError):
        revision, dirty = "unavailable", None
    inputs = [
        "simulations/omnetpp.ini",
        "simulations/dissertation-obstacles.xml",
        "analysis/process_results.py",
    ]
    experiment = runs[runs.config.isin(EXPERIMENT_CONFIGS)]
    manifest = {
        "generatedAtUtc": datetime.now(timezone.utc).isoformat(),
        "gitRevision": revision,
        "workingTreeDirty": dirty,
        "resultDirectory": os.path.abspath(results),
        "controlConfig": CONTROL_CONFIG,
        "proposedConfig": PROPOSED_CONFIG,
        "treatmentParameter": TREATMENT_PARAMETER,
        "treatmentParameterChanges": treatment_changes,
        "expectedPairedRuns": expected_pairs,
        "isPilot": expected_pairs != DEFAULT_EXPECTED_PAIRS,
        "experimentRunFiles": sorted(experiment["run_file"].tolist()),
        "diagnosticConfigs": sorted(
            runs.loc[~runs.config.isin(EXPERIMENT_CONFIGS), "config"].unique().tolist()),
        "inputSha256": {path: file_sha256(path) for path in inputs},
    }
    with open(os.path.join(OUTPUT, "experiment_manifest.json"), "w", encoding="utf-8") as target:
        json.dump(manifest, target, indent=2, ensure_ascii=False)
        target.write("\n")


def plot(runs: pd.DataFrame) -> None:
    selected = runs[runs.config.isin(EXPERIMENT_CONFIGS)]
    if selected.empty:
        return
    os.environ.setdefault("MPLCONFIGDIR", "/tmp/echosar-matplotlib")
    os.environ.setdefault("XDG_CACHE_HOME", "/tmp/echosar-cache")
    import matplotlib.pyplot as plt
    labels = {
        "appack_pct": "AppACK (%)",
        "one_way_delay_s": "One-way delay (s)",
        "attempts_per_alert": "Attempts per alert",
        "ba_distance_m": "BA displacement per alert (m)",
    }
    for metric in PAIRED_METRICS:
        summary = selected.groupby("config")[metric].agg(["mean", "std"])
        ax = summary["mean"].plot.bar(yerr=summary["std"].fillna(0), capsize=5,
                                      color=["#777777", "#2878b5"])
        ax.set_ylabel(labels[metric])
        ax.set_xlabel("")
        ax.tick_params(axis="x", rotation=12)
        ax.spines[["top", "right"]].set_visible(False)
        plt.tight_layout()
        slug = metric.replace("_pct", "")
        plt.savefig(os.path.join(OUTPUT, f"metric_{slug}.png"), dpi=160)
        plt.savefig(os.path.join(OUTPUT, f"metric_{slug}.pdf"))
        plt.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("results", nargs="?",
                        default=os.path.join(ROOT, "simulations", "results", "omnetpp"),
                        help="directory holding the .sca files")
    parser.add_argument("--expected-pairs", type=int, default=DEFAULT_EXPECTED_PAIRS,
                        help="number of paired seeds required; any value other than "
                             f"{DEFAULT_EXPECTED_PAIRS} marks the output as a pilot")
    arguments = parser.parse_args()
    os.makedirs(OUTPUT, exist_ok=True)

    runs, parameters = load_runs(arguments.results)
    experiment = runs[runs.config.isin(EXPERIMENT_CONFIGS)].copy()
    diagnostic = runs[~runs.config.isin(EXPERIMENT_CONFIGS)].copy()

    seeds = require_complete_experiment(runs, arguments.expected_pairs)
    treatment_changes = require_treatment_isolation(parameters, seeds)

    summary = aggregate(experiment)
    pairs = paired(experiment, seeds)
    quality = data_quality(runs)

    experiment.to_csv(os.path.join(OUTPUT, "runs.csv"), index=False)
    summary.to_csv(os.path.join(OUTPUT, "summary.csv"), index=False)
    pairs.to_csv(os.path.join(OUTPUT, "paired_comparison.csv"), index=False)
    quality.to_csv(os.path.join(OUTPUT, "data_quality.csv"), index=False)
    if not diagnostic.empty:
        diagnostic.to_csv(os.path.join(OUTPUT, "diagnostic_runs.csv"), index=False)
        aggregate(diagnostic).to_csv(os.path.join(OUTPUT, "diagnostic_summary.csv"), index=False)
    write_manifest(runs, arguments.results, arguments.expected_pairs, treatment_changes)
    plot(experiment)

    print(summary.to_string(index=False))
    print(f"\nPaired comparison over {len(seeds)} seeds "
          f"({PROPOSED_CONFIG} minus {CONTROL_CONFIG}):")
    print(pairs.to_string(index=False))
    if not quality.empty:
        print("\nUndefined metrics (empty denominators):")
        print(quality.to_string(index=False))
    if not diagnostic.empty:
        print(f"\nExcluded from the comparison as diagnostic: "
              f"{', '.join(sorted(diagnostic.config.unique()))}")
    print("\nNo hypothesis test is applied here. Report the effect estimate and its "
          "interval; absence of a significant difference is not evidence of equivalence.")


if __name__ == "__main__":
    try:
        main()
    except IntegrityError as error:
        print(f"Experiment integrity check failed: {error}", file=sys.stderr)
        sys.exit(1)
