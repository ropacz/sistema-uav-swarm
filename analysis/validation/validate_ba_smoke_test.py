"""Validate the complete, minimal BA control chain with central scalars."""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402

SCA = REPOSITORY_ROOT / "simulations/results/omnetpp/BA_SmokeTest-0.sca"


def main() -> None:
    if not SCA.exists():
        raise SystemExit(f"resultado ausente: {SCA}")
    row = collect(str(SCA))
    expected = {
        "alerts_generated": 1,
        "alerts_delivered": 1,
        "alerts_confirmed": 1,
        "alerts_expired": 0,
        "alert_attempts_sent": 2,
        "application_retries": 1,
        "reposition_triggers": 1,
        "obstacles_detected": 1,
        "ba_activations": 1,
        "repositions_started": 1,
        "repositions_completed": 1,
        "reposition_distance_count": 1,
    }
    failures = [
        f"{name}: esperado {value}, obtido {row[name]}"
        for name, value in expected.items()
        if row[name] != value
    ]
    if row["reposition_distance_sum_m"] <= 0:
        failures.append(
            "reposition_distance_sum_m deve ser positivo, obtido "
            f"{row['reposition_distance_sum_m']}"
        )
    if row["alert_pdr_pct"] != 100 or row["appack_pct"] != 100:
        failures.append(
            "PDR e confirmação devem ser 100%; obtido "
            f"{row['alert_pdr_pct']}%/{row['appack_pct']}%"
        )
    if failures:
        raise SystemExit("BA smoke test FALHOU:\n- " + "\n- ".join(failures))

    print("BA smoke test OK")
    print("  timeout sem ACK -> sensor binário -> BA -> movimento: confirmado")
    print("  tentativa imediata na chegada -> ACK: confirmado")
    print(f"  distância real: {row['reposition_distance_sum_m']:.2f} m")


if __name__ == "__main__":
    main()
