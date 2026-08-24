import math
import unittest
import sys
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from analysis.core.network_metrics import (  # noqa: E402
    global_or_legacy, global_scalar, pooled_statistic_mean,
    received_power_mean_dbm,
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

    def test_pools_signal_sum_and_count_instead_of_module_means(self):
        frame = pd.DataFrame([
            {"module": "node[0].app[0]", "name": "hopCount:sum", "value": 1},
            {"module": "node[0].app[0]", "name": "hopCount:count", "value": 1},
            {"module": "node[1].app[0]", "name": "hopCount:sum", "value": 9},
            {"module": "node[1].app[0]", "name": "hopCount:count", "value": 3},
        ])
        self.assertEqual(pooled_statistic_mean(frame, "hopCount"), 2.5)

    def test_averages_power_before_converting_to_dbm(self):
        frame = pd.DataFrame([
            {"module": "drone[0].app[0]",
             "name": "positionUpdatePowerMilliwatt:sum", "value": 1.0},
            {"module": "drone[0].app[0]",
             "name": "positionUpdatePowerMilliwatt:count", "value": 1},
            {"module": "drone[1].app[0]",
             "name": "positionUpdatePowerMilliwatt:sum", "value": 0.003},
            {"module": "drone[1].app[0]",
             "name": "positionUpdatePowerMilliwatt:count", "value": 3},
        ])
        self.assertAlmostEqual(received_power_mean_dbm(frame),
                               10 * math.log10(1.003 / 4))


if __name__ == "__main__":
    unittest.main()
