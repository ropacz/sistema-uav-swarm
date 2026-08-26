"""Validate sequential periodic alert cycles for one active victim."""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402

SCA = (
    REPOSITORY_ROOT
    / "simulations/results/omnetpp/AlertLifecycle_SmokeTest-0.sca"
)


def main() -> None:
    if not SCA.exists():
        raise SystemExit(f"resultado ausente: {SCA}")
    row = collect(str(SCA))
    expected = {
        "alerts_generated": 7,
        "alerts_delivered": 7,
        "alerts_confirmed": 7,
        "alerts_expired": 0,
        "alert_attempts_sent": 7,
        "attempts_received": 7,
        "application_retries": 0,
        "hop_count_sum": 7,
        "hop_count_count": 7,
        "multi_hop_deliveries": 0,
        "intermediate_forwardings": 0,
        "no_known_team_failures": 0,
        "alerts_without_known_team": 0,
        "confirmation_delay_count": 7,
    }
    failures = [
        f"{name}: esperado {value}, obtido {row[name]}"
        for name, value in expected.items()
        if row[name] != value
    ]
    if failures:
        raise SystemExit(
            "Alert lifecycle smoke test FALHOU:\n- " + "\n- ".join(failures)
        )
    if row["confirmation_delay_sum_s"] <= 0:
        raise SystemExit(
            "Alert lifecycle smoke test FALHOU:\n- "
            "confirmation_delay_sum_s deve ser positivo"
        )

    print("Alert lifecycle smoke test OK")
    print("  1 vítima ativa -> 7 alertas sequenciais confirmados")
    print("  nenhum alerta expirado ou retransmitido")


if __name__ == "__main__":
    main()
