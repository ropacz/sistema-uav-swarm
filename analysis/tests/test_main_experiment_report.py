import unittest
import sys
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from analysis.reports.report_main_experiment import (  # noqa: E402
    require_informative_treatment,
)


class MainExperimentContractTests(unittest.TestCase):
    def test_rejects_treatment_without_ba_exposure(self):
        frame = pd.DataFrame([{
            "degradation_indications": 1,
            "sensor_confirmations": 0,
            "ba_activations": 0,
            "repositions_started": 0,
        }])
        with self.assertRaisesRegex(ValueError, "uninformative treatment"):
            require_informative_treatment(frame)

    def test_accepts_exercised_treatment(self):
        frame = pd.DataFrame([{
            "degradation_indications": 1,
            "sensor_confirmations": 1,
            "ba_activations": 1,
            "repositions_started": 1,
        }])
        require_informative_treatment(frame)


if __name__ == "__main__":
    unittest.main()
