"""Validate that unfinished movement is excluded from completed costs."""

from pathlib import Path
import math
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402

SCA = (
    REPOSITORY_ROOT
    / "simulations/results/omnetpp/RepositionInterrupted_SmokeTest-0.sca"
)


def main() -> None:
    if not SCA.exists():
        raise SystemExit(f"resultado ausente: {SCA}")
    row = collect(str(SCA))
    expected = {
        "repositions_started": 1,
        "repositions_completed": 0,
        "reposition_distance_sum_m": 0,
        "reposition_duration_sum_s": 0,
    }
    failures = [
        f"{name}: esperado {value}, obtido {row[name]}"
        for name, value in expected.items()
        if row[name] != value
    ]
    for name in ("reposition_distance_mean_m", "reposition_duration_mean_s"):
        if not math.isnan(row[name]):
            failures.append(f"{name}: esperado indefinido, obtido {row[name]}")
    if failures:
        raise SystemExit(
            "Reposition interrupted smoke test FALHOU:\n- "
            + "\n- ".join(failures)
        )

    print("Reposition interrupted smoke test OK")
    print("  movimento iniciado e não concluído ao término da execução")
    print("  distância e duração parciais excluídas das somas")


if __name__ == "__main__":
    main()
