"""Fail-fast checks for the deterministic dissertation validation scenarios."""

from __future__ import annotations

import math
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
RESULTS = (
    Path(sys.argv[1]) if len(sys.argv) > 1
    else ROOT / "simulations" / "results" / "omnetpp"
)


def scalars(config: str) -> list[tuple[str, str, float]]:
    path = RESULTS / f"{config}-0.sca"
    if not path.exists():
        raise AssertionError(f"missing deterministic result: {path}")
    rows = []
    pattern = re.compile(r'scalar (\S+) "?([^"\n]+?)"? ([^\s]+)$')
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        match = pattern.match(line)
        if match:
            try:
                rows.append((match.group(1), match.group(2), float(match.group(3))))
            except ValueError:
                pass
    return rows


def total(rows: list[tuple[str, str, float]], name: str) -> float:
    return sum(value for _, scalar_name, value in rows if scalar_name == name and math.isfinite(value))


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def main() -> None:
    direct = scalars("Validation_Direct")
    multihop = scalars("Validation_Multihop")
    clear = scalars("Validation_Clear_Rssi")
    obstructed = scalars("Validation_Obstacle_Rssi")
    retry = scalars("Validation_Obstacle_BaOff")
    ba = scalars("Validation_BaOn")
    range_reject = scalars("Validation_Sensor_RejectRange")
    two_victims = scalars("Validation_TwoVictims")

    require(total(direct, "uniqueAlertsAcked") == 1, "direct delivery did not receive AppACK")
    require(total(multihop, "uniqueAlertsAcked") == 1, "multi-hop alert was not acknowledged")
    require(total(multihop, "hopCount:mean") >= 1, "multi-hop scenario used no forwarding node")

    clear_rssi = total(clear, "positionUpdateRssi:mean")
    obstacle_rssi = total(obstructed, "positionUpdateRssi:mean")
    require(clear_rssi > obstacle_rssi + 10, "obstacle did not create a measurable RSSI penalty")
    require(total(obstructed, "Obstacle loss intersection count") > 0,
            "DielectricObstacleLoss reported no geometric intersection")

    require(total(retry, "alertAttemptsSent") >= 2, "alert was not retried")
    require(total(retry, "alertsExpired") == 1, "unacknowledged alert did not expire")
    require(total(ba, "baActivations") >= 1, "BA was not activated")
    require(total(ba, "successfulRepositions") == 1, "post-BA validation was not acknowledged")
    require(total(ba, "recoverySamples") == 1, "communication recovery time was not recorded")

    require(total(range_reject, "baActivations") == 0 and total(range_reject, "sensorRejections") >= 1,
            "sensor did not reject obstacle outside visual range")
    require(total(two_victims, "uniqueAlertsGenerated") == 2 and
            total(two_victims, "uniqueAlertsAcked") == 2,
            "two-victim scenario did not generate and acknowledge two unique alerts")

    print("All deterministic ECHOSAR-Net validation checks passed.")


if __name__ == "__main__":
    main()
