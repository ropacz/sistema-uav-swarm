import unittest
import sys
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from analysis.reports.report_main_experiment import (  # noqa: E402
    summarize_exposure,
)


class MainExperimentContractTests(unittest.TestCase):
    def test_reports_treatment_without_ba_exposure(self):
        frame = pd.DataFrame([{
            "ba_activations": 0,
            "repositions_started": 0,
        }])
        summary = summarize_exposure(frame).iloc[0]
        self.assertEqual(summary["exposure_status"], "not_observed")
        self.assertEqual(summary["treatment_runs"], 1)
        self.assertEqual(summary["runs_with_ba_activation"], 0)

    def test_reports_exposure_without_filtering_runs(self):
        frame = pd.DataFrame([
            {"ba_activations": 0, "repositions_started": 0},
            {"ba_activations": 2, "repositions_started": 1},
        ])
        summary = summarize_exposure(frame).iloc[0]
        self.assertEqual(summary["exposure_status"], "observed")
        self.assertEqual(summary["treatment_runs"], 2)
        self.assertEqual(summary["runs_with_ba_activation"], 1)
        self.assertEqual(summary["ba_activations"], 2)


if __name__ == "__main__":
    unittest.main()
