#!/usr/bin/env python3
"""Report the natural obstacle pilot without equating BA activation to recovery."""

from pathlib import Path
from datetime import datetime, timezone
import csv
import hashlib
import json
import math
import re
import subprocess

RESULTS = Path("simulations/results/omnetpp")
CONFIGS = ("HypothesisPilot_BaOff", "HypothesisPilot_BaOn")
METRICS = (
    "uniqueAlertsGenerated", "uniqueAlertsAcked", "alertAttemptsSent",
    "alertsExpired", "degradationIndications", "sensorConfirmations",
    "baActivations", "successfulRepositions", "failedRepositions", "baDistance",
    "commandedBaDistance",
    "repositionAckedBeforeValidation", "predictedTeamPositions",
    "teamPredictionAgeSum", "teamPredictionAgeMax",
    "baNoFeasibleSolution", "baRedundantCandidate", "repositionExpiredBeforeAck",
    "attemptsReceived", "uniqueAlertsReceived", "applicationAcksSent",
)


def parse_scalars(path: Path) -> dict[str, float]:
    """Sum scalar values by name in one OMNeT++ result file."""
    values: dict[str, float] = {}
    pattern = re.compile(r'scalar \S+ "?([^"\n]+?)"? ([^\s]+)$')
    with path.open(encoding="utf-8", errors="replace") as source:
        for line in source:
            match = pattern.match(line)
            if not match:
                continue
            try:
                value = float(match.group(2))
            except ValueError:
                continue
            name = match.group(1).strip()
            values[name] = values.get(name, 0.0) + value
    return values


def parse_run_contract(path: Path) -> tuple[dict[str, str], tuple[str, ...], str]:
    """Return pairing attributes and configuration lines relevant to causality."""
    attrs = {}
    comparable = []
    ba_enabled = ""
    ignored_prefixes = (
        "config extends ", "config description ",
        "config **.drone[0].app[0].baEnabled ",
        "config output-scalar-file ", "config output-vector-file ",
        "config seed-set ",
    )
    with path.open(encoding="utf-8", errors="replace") as source:
        for line in source:
            line = line.rstrip("\n")
            if line.startswith("attr "):
                _, key, value = line.split(" ", 2)
                attrs[key] = value.strip('"')
            elif line.startswith("config **.drone[0].app[0].baEnabled "):
                ba_enabled = line.rsplit(" ", 1)[1]
            elif line.startswith("config ") and not line.startswith(ignored_prefixes):
                comparable.append(line)
    return attrs, tuple(comparable), ba_enabled


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    digest.update(path.read_bytes())
    return digest.hexdigest()


def main() -> None:
    rows = []
    contracts = {}
    run_records = []
    for config in CONFIGS:
        paths = sorted(RESULTS.glob(f"{config}-*.sca"))
        if len(paths) != 5:
            raise SystemExit(f"{config}: expected 5 results, found {len(paths)}")
        totals = {metric: 0.0 for metric in METRICS}
        hop_values = []
        for path in paths:
            scalars = parse_scalars(path)
            missing = sorted(set(METRICS) - set(scalars))
            if missing:
                raise SystemExit(f"{path.name}: missing required scalars: {', '.join(missing)}")
            if scalars["uniqueAlertsGenerated"] != 1:
                raise SystemExit(f"{path.name}: expected exactly one generated alert")
            if scalars["uniqueAlertsAcked"] + scalars["alertsExpired"] != 1:
                raise SystemExit(f"{path.name}: alert is neither uniquely acked nor expired")
            failure_causes = (
                scalars["baNoFeasibleSolution"] + scalars["baRedundantCandidate"] +
                scalars["repositionExpiredBeforeAck"]
            )
            if scalars["failedRepositions"] != failure_causes:
                raise SystemExit(f"{path.name}: failed-reposition decomposition is inconsistent")
            if scalars["attemptsReceived"] > scalars["alertAttemptsSent"]:
                raise SystemExit(f"{path.name}: received more attempts than were sent")
            if scalars["uniqueAlertsAcked"] > scalars["applicationAcksSent"]:
                raise SystemExit(f"{path.name}: drone ACK count exceeds team ACK transmissions")
            attrs, comparable, ba_enabled = parse_run_contract(path)
            repetition = int(attrs.get("repetition", "-1"))
            if repetition in contracts.get(config, {}):
                raise SystemExit(f"{config}: duplicate repetition {repetition}")
            contracts.setdefault(config, {})[repetition] = (attrs, comparable, ba_enabled)
            hop = scalars.get("hopCount:mean", math.nan)
            run_records.append({
                "config": config,
                "repetition": repetition,
                "seedset": attrs.get("seedset", ""),
                "generated": scalars["uniqueAlertsGenerated"],
                "acked": scalars["uniqueAlertsAcked"],
                "attempts_sent": scalars["alertAttemptsSent"],
                "attempts_received": scalars["attemptsReceived"],
                "expired": scalars["alertsExpired"],
                "degradations": scalars["degradationIndications"],
                "sensor_confirmations": scalars["sensorConfirmations"],
                "ba_activations": scalars["baActivations"],
                "recovered_during_movement": scalars["repositionAckedBeforeValidation"],
                "actual_distance_m": scalars["baDistance"],
                "commanded_distance_m": scalars["commandedBaDistance"],
                "hop_count_mean": "" if not math.isfinite(hop) else hop,
            })
            hop = scalars.get("hopCount:mean", math.nan)
            if math.isfinite(hop):
                hop_values.append(hop)
            for metric in METRICS:
                value = scalars.get(metric, 0.0)
                if metric == "teamPredictionAgeMax":
                    totals[metric] = max(totals[metric], value)
                else:
                    totals[metric] += value
        rows.append((config, totals, hop_values))

    off_contracts = contracts[CONFIGS[0]]
    on_contracts = contracts[CONFIGS[1]]
    if set(off_contracts) != set(on_contracts) or set(off_contracts) != set(range(5)):
        raise SystemExit("arms do not contain the same repetitions 0..4")
    reference_off = off_contracts[0][1]
    reference_on = on_contracts[0][1]
    for repetition in range(5):
        off_attrs, off_config, off_ba = off_contracts[repetition]
        on_attrs, on_config, on_ba = on_contracts[repetition]
        if off_attrs.get("seedset") != on_attrs.get("seedset"):
            raise SystemExit(f"repetition {repetition}: seedsets are not paired")
        if off_config != on_config:
            raise SystemExit(f"repetition {repetition}: arms differ beyond baEnabled")
        if off_config != reference_off or on_config != reference_on:
            raise SystemExit(f"repetition {repetition}: configuration changed across seeds")
        if off_ba != "false" or on_ba != "true":
            raise SystemExit(f"repetition {repetition}: invalid baEnabled treatment assignment")

    print("config,runs,generated,acked,attempts,expired,degradations,sensor,"
          "ba_activations,validated_final_positions,recovered_during_movement,"
          "failed_repositions,actual_distance_m,commanded_distance_m,"
          "predictions,prediction_age_mean_s,"
          "prediction_age_max_s,max_hop_count")
    for config, values, hop_values in rows:
        hop_text = f"{max(hop_values):g}" if hop_values else "NA"
        print(
            f"{config},5,{values['uniqueAlertsGenerated']:g},"
            f"{values['uniqueAlertsAcked']:g},{values['alertAttemptsSent']:g},"
            f"{values['alertsExpired']:g},{values['degradationIndications']:g},"
            f"{values['sensorConfirmations']:g},{values['baActivations']:g},"
            f"{values['successfulRepositions']:g},"
            f"{values['repositionAckedBeforeValidation']:g},"
            f"{values['failedRepositions']:g},{values['baDistance']:g},"
            f"{values['commandedBaDistance']:g},"
            f"{values['predictedTeamPositions']:g},"
            f"{values['teamPredictionAgeSum'] / values['predictedTeamPositions'] if values['predictedTeamPositions'] else 0:g},"
            f"{values['teamPredictionAgeMax']:g},{hop_text}"
        )

    off, on = (row[1] for row in rows)
    if off["baActivations"] != 0 or on["baActivations"] <= 0:
        raise SystemExit("invalid paired control: BA activation does not follow baEnabled")
    if min(on["degradationIndications"], on["sensorConfirmations"]) <= 0:
        raise SystemExit("natural obstacle did not exercise degradation and sensing")
    recovered = on["uniqueAlertsAcked"] > off["uniqueAlertsAcked"]
    on_hops = rows[1][2]
    if recovered and (not on_hops or max(on_hops) != 0):
        raise SystemExit("pilot recovery was not exclusively direct-hop")
    print(f"recovery_improvement={'yes' if recovered else 'no'}")

    by_arm = {
        config: {int(row["repetition"]): int(row["acked"]) for row in run_records if row["config"] == config}
        for config in CONFIGS
    }
    improvements = sum(
        by_arm[CONFIGS[0]][seed] == 0 and by_arm[CONFIGS[1]][seed] == 1
        for seed in range(5)
    )
    regressions = sum(
        by_arm[CONFIGS[0]][seed] == 1 and by_arm[CONFIGS[1]][seed] == 0
        for seed in range(5)
    )
    discordant = improvements + regressions
    if discordant:
        tail = sum(math.comb(discordant, i) for i in range(min(improvements, regressions) + 1))
        mcnemar_p = min(1.0, 2 * tail / (2 ** discordant))
    else:
        mcnemar_p = 1.0
    print(f"paired_improvements={improvements}")
    print(f"paired_regressions={regressions}")
    print(f"exact_mcnemar_two_sided_p={mcnemar_p:g}")

    runs_path = Path("simulations/results/pilot_runs.csv")
    with runs_path.open("w", newline="", encoding="utf-8") as output:
        writer = csv.DictWriter(output, fieldnames=list(run_records[0]))
        writer.writeheader()
        writer.writerows(sorted(run_records, key=lambda row: (row["config"], row["repetition"])))

    inputs = [
        Path("simulations/omnetpp.ini"),
        Path("simulations/hypothesis-pilot-obstacle.xml"),
        Path("simulations/hypothesis-pilot-team.xml"),
    ]
    commit = subprocess.run(
        ["git", "rev-parse", "HEAD"], capture_output=True, text=True, check=False
    ).stdout.strip()
    dirty = bool(subprocess.run(
        ["git", "status", "--porcelain"], capture_output=True, text=True, check=False
    ).stdout.strip())
    manifest = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "git_commit": commit or None,
        "git_worktree_dirty": dirty,
        "configs": list(CONFIGS),
        "repetitions": list(range(5)),
        "paired_contract_verified": True,
        "direct_hop_verified": recovered,
        "paired_improvements": improvements,
        "paired_regressions": regressions,
        "exact_mcnemar_two_sided_p": mcnemar_p,
        "input_sha256": {str(path): sha256(path) for path in inputs},
        "result_sha256": {
            str(path): sha256(path)
            for config in CONFIGS
            for path in sorted(RESULTS.glob(f"{config}-*.sca"))
        },
        "runs_csv_sha256": sha256(runs_path),
    }
    manifest_path = Path("simulations/results/pilot_manifest.json")
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"manifest={manifest_path}")
    print(f"runs={runs_path}")


if __name__ == "__main__":
    main()
