"""Testes das estatísticas calculadas entre execuções independentes."""

from __future__ import annotations

import math
import sys
import unittest
from pathlib import Path

import pandas as pd

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from analysis.pcap.pcap_batch_to_spreadsheet import (  # noqa: E402
    Capture,
    compare_group,
    summarize_multiseed,
)


class MultiseedStatisticsTests(unittest.TestCase):
    def test_multihop_forward_is_not_counted_as_new_application_send(self) -> None:
        base = {
            "message_type": "VictimAlert", "source_ip": "10.0.0.1",
            "destination_ip": "10.0.0.5", "ip_id": 1,
            "source_port": 5000, "destination_port": 5000,
            "udp_payload_bytes": 320, "wire_magic": "ECHO",
            "wire_version": 1, "message_id": "alert-attempt-1",
            "sequence_number": 1, "attempt_number": 1,
            "time_seconds": 2.0,
        }
        origin = Capture(Path("origin.pcap"), "Demo", "0", "drone[0]",
                         "10.0.0.1", [{**base, "direction": "outbound"}])
        relay = Capture(Path("relay.pcap"), "Demo", "0", "drone[1]",
                        "10.0.0.2", [{**base, "direction": "outbound"}])
        destination = Capture(
            Path("team.pcap"), "Demo", "0", "team[0]", "10.0.0.5",
            [{**base, "direction": "inbound"}],
        )
        rows = compare_group([origin, relay, destination])
        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0]["received"], "Sim")

    def test_mean_sample_std_median_and_student_interval(self) -> None:
        runs = pd.DataFrame(
            {
                "configuration": ["Demo"] * 4,
                "run": ["0", "1", "2", "3"],
                "message_type": ["PositionUpdate"] * 4,
                "metric_scope": ["broadcast_reception_opportunity"] * 4,
                "sent": [10] * 4,
                "received": [10, 8, 6, 4],
                "lost": [0, 2, 4, 6],
                "pdr": [1.0, 0.8, 0.6, 0.4],
                "loss_rate": [0.0, 0.2, 0.4, 0.6],
            }
        )
        row = summarize_multiseed(runs).iloc[0]
        self.assertEqual(row["n_seeds"], 4)
        self.assertEqual(row["sent_total"], 40)
        self.assertAlmostEqual(row["pdr_mean"], 0.7)
        self.assertAlmostEqual(row["pdr_median"], 0.7)
        self.assertAlmostEqual(row["pdr_std"], math.sqrt(0.2 / 3))
        self.assertLess(row["pdr_ci95_lower"], row["pdr_mean"])
        self.assertGreater(row["pdr_ci95_upper"], row["pdr_mean"])

    def test_one_seed_does_not_invent_confidence_interval(self) -> None:
        runs = pd.DataFrame(
            [{
                "configuration": "Demo", "run": "0",
                "message_type": "VictimAck",
                "metric_scope": "unicast_packet_delivery",
                "sent": 1, "received": 1, "lost": 0,
                "pdr": 1.0, "loss_rate": 0.0,
            }]
        )
        row = summarize_multiseed(runs).iloc[0]
        self.assertTrue(math.isnan(row["pdr_std"]))
        self.assertTrue(math.isnan(row["pdr_ci95_lower"]))


if __name__ == "__main__":
    unittest.main()
