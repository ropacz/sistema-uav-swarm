"""Validate that the visual range limit constrains detection, not BA activation.

O obstáculo está na linha de visada, mas além do alcance configurado do sensor.
Duas coisas devem valer ao mesmo tempo:

- o sensor físico rejeita a detecção (`obstacles_detected == 0`), respeitando o
  alcance nominal da câmera;
- o Bat Algorithm é acionado assim mesmo, porque quem decide *quando*
  reposicionar é a degradação da rede, não a câmera. Não observar um obstáculo
  significa apenas que nenhum obstáculo estava dentro do alcance visual — não
  que o enlace esteja íntegro.

Ver docs/desvios_e_extensoes.md, D4 e E4.
"""

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
        # O sensor físico rejeita o obstáculo fora de alcance.
        "obstacles_detected": 0,
        # E ainda assim a degradação da rede aciona o BA, que encontra e inicia
        # um reposicionamento sem nenhum ponto de obstáculo na aptidão.
        "ba_activations": 1,
        "repositions_started": 1,
        # A execução termina antes de o deslocamento concluir.
        "repositions_completed": 0,
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
    print("  sensor rejeita a detecção (obstaclesDetected = 0)")
    print("  degradação da rede aciona o BA assim mesmo, sem obstáculo visto")


if __name__ == "__main__":
    main()
