import unittest
import sys
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from analysis.core.network_metrics import (  # noqa: E402
    global_or_legacy, global_scalar,
)


class GlobalMetricsTests(unittest.TestCase):
    def test_prefers_central_scalar_over_local_counters(self):
        frame = pd.DataFrame([
            {"module": "BasicNetwork.drone[0].app[0]",
             "name": "uniqueAlertsAcked", "value": 1},
            {"module": "BasicNetwork.drone[1].app[0]",
             "name": "uniqueAlertsAcked", "value": 1},
            {"module": "BasicNetwork.experimentMetrics",
             "name": "alertsConfirmed", "value": 1},
        ])

        self.assertEqual(
            global_or_legacy(frame, "alertsConfirmed", "uniqueAlertsAcked"), 1)

    def test_falls_back_for_results_created_before_central_collector(self):
        frame = pd.DataFrame([
            {"module": "BasicNetwork.drone[0].app[0]",
             "name": "uniqueAlertsAcked", "value": 1},
            {"module": "BasicNetwork.drone[1].app[0]",
             "name": "uniqueAlertsAcked", "value": 2},
        ])

        self.assertEqual(
            global_or_legacy(frame, "alertsConfirmed", "uniqueAlertsAcked"), 3)
        self.assertTrue(pd.isna(global_scalar(frame, "alertsConfirmed")))


if __name__ == "__main__":
    unittest.main()
