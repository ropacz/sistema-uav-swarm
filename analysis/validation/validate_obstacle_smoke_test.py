"""Validate packet delivery before and after enabling obstacle attenuation."""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402

RESULTS = REPOSITORY_ROOT / "simulations/results/omnetpp"


def main() -> None:
    paths = {
        "clear": RESULTS / "ObstacleClear_SmokeTest-0.sca",
        "blocked": RESULTS / "ObstacleBlocked_SmokeTest-0.sca",
    }
    missing = [str(path) for path in paths.values() if not path.exists()]
    if missing:
        raise SystemExit("resultados ausentes: " + ", ".join(missing))

    clear = collect(str(paths["clear"]))
    blocked = collect(str(paths["blocked"]))
    failures = []
    if clear["alerts_delivered"] != 1 or clear["alerts_confirmed"] != 1:
        failures.append("o enlace sem atenuação deve entregar e confirmar o alerta")
    if blocked["alert_attempts_sent"] <= 0:
        failures.append("o enlace obstruído deve realizar tentativa de transmissão")
    if blocked["attempts_received"] != 0 or blocked["alerts_delivered"] != 0:
        failures.append("o obstáculo deve impedir todas as tentativas no receptor")
    if blocked["alerts_expired"] != 1:
        failures.append("o alerta obstruído deve terminar por expiração")
    if failures:
        raise SystemExit("Obstacle smoke test FALHOU:\n- " + "\n- ".join(failures))

    print("Obstacle smoke test OK")
    print("  enlace livre: alerta entregue e confirmado")
    print("  mesmo enlace com atenuação: tentativas perdidas e alerta expirado")


if __name__ == "__main__":
    main()
