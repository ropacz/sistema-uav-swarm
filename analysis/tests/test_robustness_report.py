"""Contract tests for paired robustness analysis."""

from pathlib import Path
import sys
import unittest

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from analysis.reports.report_robustness import METRICS, pair_runs  # noqa: E402


def run(enabled: bool, seed: int, pdr: float) -> dict:
    row = {metric: 0.0 for metric in METRICS}
    row.update({
        "scenario": "Scenario1_OneVictim",
        "teams": 1,
        "seed": seed,
        "ba_enabled": enabled,
        "result_path": f"run-{enabled}-{seed}.sca",
        "alert_pdr_pct": pdr,
    })
    return row


class RobustnessPairingTests(unittest.TestCase):
    def test_pairs_each_seed_and_calculates_treatment_effect(self) -> None:
        runs = pd.DataFrame([
            run(False, 0, 40), run(True, 0, 55),
            run(False, 1, 60), run(True, 1, 70),
        ])
        paired = pair_runs(runs, check_parameters=False)
        self.assertEqual(len(paired), 2)
        self.assertEqual(
            sorted(paired["alert_pdr_pct_effect"].tolist()), [10, 15]
        )

    def test_rejects_an_unpaired_seed(self) -> None:
        runs = pd.DataFrame([run(False, 0, 40), run(True, 1, 55)])
        with self.assertRaisesRegex(ValueError, "sem par"):
            pair_runs(runs, check_parameters=False)


if __name__ == "__main__":
    unittest.main()
