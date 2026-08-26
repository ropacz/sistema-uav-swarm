"""Validate operational failures when an active victim has no known team."""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402

SCA = (
    REPOSITORY_ROOT
    / "simulations/results/omnetpp/NoKnownTeam_SmokeTest-0.sca"
)


def main() -> None:
    if not SCA.exists():
        raise SystemExit(f"resultado ausente: {SCA}")
    row = collect(str(SCA))
    expected = {
        "alerts_generated": 2,
        "alerts_delivered": 0,
        "alerts_confirmed": 0,
        "alerts_expired": 2,
        "alert_attempts_sent": 0,
        "attempts_received": 0,
        "no_known_team_failures": 10,
        "alerts_without_known_team": 2,
        "confirmation_delay_count": 0,
    }
    failures = [
        f"{name}: esperado {value}, obtido {row[name]}"
        for name, value in expected.items()
        if row[name] != value
    ]
    if failures:
        raise SystemExit(
            "No-known-team smoke test FALHOU:\n- " + "\n- ".join(failures)
        )

    print("No-known-team smoke test OK")
    print("  oportunidades sem destino registradas como falhas operacionais")
    print("  nenhuma tentativa de rede foi inventada")


if __name__ == "__main__":
    main()
