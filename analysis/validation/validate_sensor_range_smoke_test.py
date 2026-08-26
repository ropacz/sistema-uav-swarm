"""Validate rejection of an obstacle outside the configured visual range."""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402

SCA = REPOSITORY_ROOT / (
    "simulations/results/omnetpp/SensorOutOfRange_SmokeTest-0.sca"
)


def main() -> None:
    if not SCA.exists():
        raise SystemExit(f"resultado ausente: {SCA}")
    row = collect(str(SCA))
    expected = {
        "reposition_triggers": 1,
        "obstacles_detected": 0,
        "ba_activations": 0,
        "repositions_started": 0,
    }
    failures = [
        f"{name}: esperado {value}, obtido {row[name]}"
        for name, value in expected.items()
        if row[name] != value
    ]
    if failures:
        raise SystemExit(
            "Sensor range smoke test FALHOU:\n- " + "\n- ".join(failures)
        )

    print("Sensor range smoke test OK")
    print("  obstáculo na linha de visada, mas além do alcance configurado")
    print("  sensor rejeita a detecção e o BA não é ativado")


if __name__ == "__main__":
    main()
