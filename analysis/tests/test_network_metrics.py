import math
import sys
import unittest
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from analysis.core.network_metrics import ratio, sum_where  # noqa: E402


class NetworkDiagnosticsTests(unittest.TestCase):
    def test_sums_only_the_requested_module_layer(self):
        frame = pd.DataFrame([
            {"module": "node.wlan[0].mac", "name": "sent", "value": 2},
            {"module": "node.wlan[0].mac.dcf", "name": "sent", "value": 3},
        ])
        self.assertEqual(sum_where(frame, "sent", r"\.mac$"), 2)

    def test_zero_denominator_is_undefined(self):
        self.assertTrue(math.isnan(ratio(0, 0)))


if __name__ == "__main__":
    unittest.main()
