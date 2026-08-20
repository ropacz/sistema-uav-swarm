"""Testes determinísticos da decodificação e da auditoria de PCAP.

Não exigem uma simulação nem dependem do Wireshark: os payloads binários ECHO
são construídos aqui como exemplos mínimos do formato usado pelo projeto.
"""

from __future__ import annotations

import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from pcap_core import (  # noqa: E402
    compare_direction,
    decode_echosar_payload,
    packet_identity_method,
    packet_key,
)


def text(value: str) -> bytes:
    encoded = value.encode("utf-8")
    return struct.pack("!H", len(encoded)) + encoded


def wire(message_code: int, body: bytes, payload_length: int) -> bytes:
    header = b"ECHO" + struct.pack("!BBH", 1, message_code, len(body))
    return (header + body).ljust(payload_length, b"\0")


def position_update_payload() -> bytes:
    body = b"".join(
        [
            text("team0-pos-7"), text("team0"), text("team"), text("10.0.0.2"),
            struct.pack("!i", -1), struct.pack("!ddd", 540.0, 500.0, 1.5),
            struct.pack("!q", 7), struct.pack("!d", 6.5), text("mobile"),
        ]
    )
    return wire(1, body, 160)


def victim_alert_payload() -> bytes:
    body = b"".join(
        [
            text("alert-1"), text("victim0-event-attempt-2"), text("victim0"),
            text("drone0"), text("10.0.0.1"),
            struct.pack("!dddddd", 10.0, 20.0, 0.0, 11.0, 21.0, 30.0),
            struct.pack("!iqi", 3, 9, 2),
            struct.pack("!ddd", 1.0, 1.1, 30.0),
        ]
    )
    return wire(2, body, 320)


def victim_ack_payload() -> bytes:
    body = b"".join(
        [
            text("alert-1"), text("victim0-event-attempt-2"), text("victim0"),
            text("team0"), text("drone0"), struct.pack("!dd", 1.2, 1.3),
        ]
    )
    return wire(3, body, 96)


def event(**overrides) -> dict:
    base = {
        "message_type": "PositionUpdate",
        "source_ip": "10.0.0.2",
        "destination_ip": "255.255.255.255",
        "ip_id": 10,
        "source_port": 5000,
        "destination_port": 5000,
        "udp_payload_bytes": 160,
        "wire_magic": "ECHO",
        "wire_version": 1,
        "message_id": "team0-pos-7",
        "sequence_number": 7,
        "direction": "outbound",
        "time_seconds": 6.5,
    }
    base.update(overrides)
    return base


class EchoDecodingTests(unittest.TestCase):
    def test_decodes_position_update_example(self) -> None:
        decoded = decode_echosar_payload(position_update_payload())
        self.assertEqual(decoded["message_type"], "PositionUpdate")
        self.assertEqual(decoded["message_id"], "team0-pos-7")
        self.assertEqual(decoded["sequence_number"], 7)
        self.assertEqual(decoded["position_z"], 1.5)

    def test_decodes_alert_and_ack_link(self) -> None:
        alert = decode_echosar_payload(victim_alert_payload())
        ack = decode_echosar_payload(victim_ack_payload())
        self.assertEqual(alert["attempt_number"], 2)
        self.assertEqual(alert["message_id"], ack["received_message_id"])
        self.assertEqual(alert["alert_id"], ack["alert_id"])


class PacketAuditTests(unittest.TestCase):
    def test_echo_identity_does_not_depend_on_ipv4_id(self) -> None:
        sent = event(ip_id=10)
        observed = event(ip_id=99, direction="inbound")
        self.assertEqual(packet_key(sent), packet_key(observed))
        self.assertEqual(packet_identity_method(sent), "echo_fields")

    def test_legacy_identity_uses_network_headers(self) -> None:
        legacy = event(wire_magic="", message_id="")
        changed = {**legacy, "ip_id": 11}
        self.assertNotEqual(packet_key(legacy), packet_key(changed))
        self.assertEqual(packet_identity_method(legacy), "network_headers")

    def test_comparison_reports_one_delivery_and_one_loss(self) -> None:
        sent_7 = event()
        sent_8 = event(message_id="team0-pos-8", sequence_number=8, ip_id=11)
        received_7 = event(direction="inbound", ip_id=90)
        rows = compare_direction([sent_7, sent_8], [received_7], "10.0.0.2")
        self.assertEqual([row["lost"] for row in rows], [0, 1])
        self.assertTrue(all(row["identity_method"] == "echo_fields" for row in rows))
        self.assertTrue(
            all(row["metric_scope"] == "broadcast_reception_opportunity" for row in rows)
        )


if __name__ == "__main__":
    unittest.main()
