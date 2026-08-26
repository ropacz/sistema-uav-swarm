"""Contract tests for the normative, central scalar reader."""

from __future__ import annotations

import math
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from analysis.core.experiment_metrics import collect  # noqa: E402


SCALARS = {
    "alertsGenerated": 2,
    "alertsDelivered": 1,
    "alertsConfirmed": 1,
    "alertsExpired": 1,
    "alertAttemptsSent": 5,
    "attemptsReceived": 3,
    "applicationRetries": 3,
    "deliveryDelaySum": 4,
    "deliveryDelayCount": 1,
    "confirmationDelaySum": 5,
    "confirmationDelayCount": 1,
    "hopCountSum": 2,
    "hopCountCount": 1,
    "multiHopDeliveries": 1,
    "intermediateForwardings": 1,
    "neverKnownTeamSelectionEvents": 4,
    "expiredKnownTeamSelectionEvents": 0,
    "knownTeamNoAckTimeoutEvents": 0,
    "alertsWithoutKnownTeam": 1,
    "repositionTriggers": 2,
    "obstaclesDetected": 1,
    "baActivations": 1,
    "repositionsStarted": 1,
    "repositionsCompleted": 1,
    "repositionDistanceSum": 12.5,
    "repositionDurationSum": 3.5,
}


def write_sca(path: Path, overrides: dict[str, float] | None = None) -> None:
    values = {**SCALARS, **(overrides or {})}
    lines = ['attr configname "Demo"', "attr seedset 7"]
    lines.extend(
        f"scalar BasicNetwork.experimentMetrics {name} {value}"
        for name, value in values.items()
    )
    path.write_text("\n".join(lines), encoding="utf-8")


class ExperimentMetricsReaderTests(unittest.TestCase):
    def test_derives_only_auditable_run_outcomes(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run.sca"
            write_sca(path)
            row = collect(str(path))
        self.assertEqual(row["seed"], 7)
        self.assertEqual(row["alert_pdr_pct"], 50)
        self.assertEqual(row["alert_loss_pct"], 50)
        self.assertEqual(row["appack_pct"], 50)
        self.assertEqual(row["delivery_delay_mean_s"], 4)
        self.assertEqual(row["confirmation_delay_mean_s"], 5)
        self.assertEqual(row["retries_per_alert"], 1.5)
        self.assertEqual(row["attempt_pdr_pct"], 60)
        self.assertEqual(row["attempt_loss_pct"], 40)
        self.assertEqual(row["mean_hop_count"], 2)
        self.assertEqual(row["multi_hop_delivery_rate_pct"], 100)
        self.assertEqual(row["reposition_distance_mean_m"], 12.5)
        self.assertEqual(row["reposition_duration_mean_s"], 3.5)

    def test_zero_generated_alerts_produce_undefined_ratios(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run.sca"
            write_sca(path, {"alertsGenerated": 0})
            row = collect(str(path))
        self.assertTrue(math.isnan(row["alert_pdr_pct"]))
        self.assertTrue(math.isnan(row["alert_loss_pct"]))
        self.assertTrue(math.isnan(row["retries_per_alert"]))

    def test_missing_central_scalar_is_a_contract_failure(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "run.sca"
            values = dict(SCALARS)
            del values["alertsConfirmed"]
            write_sca(path, values)
            # write_sca merges defaults, so create the malformed file explicitly.
            path.write_text(
                'attr configname "Demo"\nattr seedset 7\n'
                'scalar BasicNetwork.experimentMetrics alertsGenerated 1\n',
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "alertsDelivered"):
                collect(str(path))


if __name__ == "__main__":
    unittest.main()
