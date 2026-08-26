"""Validate hop accounting from the IPv4 HopLimitInd received by TeamApp."""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402

SCA = REPOSITORY_ROOT / "simulations/results/omnetpp/Multihop_SmokeTest-0.sca"


def main() -> None:
    if not SCA.exists():
        raise SystemExit(f"resultado ausente: {SCA}")
    row = collect(str(SCA))
    expected = {
        "alerts_generated": 1,
        "alerts_delivered": 1,
        "alerts_confirmed": 1,
        "alert_attempts_sent": 1,
        "attempts_received": 1,
        "hop_count_sum": 2,
        "hop_count_count": 1,
        "multi_hop_deliveries": 1,
        "intermediate_forwardings": 1,
        "never_known_team_selection_events": 0,
        "expired_known_team_selection_events": 0,
        "confirmation_delay_count": 1,
    }
    failures = [
        f"{name}: esperado {value}, obtido {row[name]}"
        for name, value in expected.items()
        if row[name] != value
    ]
    if failures:
        raise SystemExit("Multihop smoke test FALHOU:\n- " + "\n- ".join(failures))

    print("Multihop smoke test OK")
    print("  VictimAlert entregue em 2 saltos por um drone intermediário")
    print("  HopLimitInd e encaminhamento intermediário contabilizados")


if __name__ == "__main__":
    main()
