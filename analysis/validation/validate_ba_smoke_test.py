"""Validate the complete, minimal BA control chain with central scalars.

Cobre o caminho *com* obstáculo visível: a câmera observa a superfície dentro
do alcance e o ponto observado entra no termo de obstáculo da aptidão. O
caminho complementar — degradação da rede aciona o BA *sem* obstáculo visto —
é coberto por validate_sensor_range_smoke_test.py.
"""

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
        "attempts_received": 2,
        "application_retries": 1,
        "hop_count_sum": 1,
        "hop_count_count": 1,
        "multi_hop_deliveries": 0,
        "intermediate_forwardings": 0,
        "never_known_team_selection_events": 0,
        "expired_known_team_selection_events": 0,
        "alerts_without_known_team": 0,
        "confirmation_delay_count": 1,
        "reposition_triggers": 1,
        "obstacles_detected": 1,
        "ba_activations": 1,
        "repositions_started": 1,
        "repositions_completed": 1,
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
    if row["confirmation_delay_sum_s"] <= 0:
        failures.append(
            "confirmation_delay_sum_s deve ser positivo, obtido "
            f"{row['confirmation_delay_sum_s']}"
        )
    if row["reposition_duration_sum_s"] <= 0:
        failures.append(
            "reposition_duration_sum_s deve ser positivo, obtido "
            f"{row['reposition_duration_sum_s']}"
        )
    if row["alert_pdr_pct"] != 100 or row["appack_pct"] != 100:
        failures.append(
            "PDR e confirmação devem ser 100%; obtido "
            f"{row['alert_pdr_pct']}%/{row['appack_pct']}%"
        )
    if failures:
        raise SystemExit("BA smoke test FALHOU:\n- " + "\n- ".join(failures))

    print("BA smoke test OK")
    print("  timeout sem ACK -> BA -> movimento: confirmado")
    print("  obstáculo observado dentro do alcance entra na aptidão")
    print("  tentativa imediata na chegada -> ACK: confirmado")
    print(f"  distância real: {row['reposition_distance_sum_m']:.2f} m")
    print(f"  duração do reposicionamento: {row['reposition_duration_sum_s']:.2f} s")


if __name__ == "__main__":
    main()
