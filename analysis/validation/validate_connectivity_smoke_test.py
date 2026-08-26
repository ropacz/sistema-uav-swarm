"""Validate DroneStatus discovery and BA neighbor preservation."""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.process_results import parse_sca  # noqa: E402
from analysis.core.experiment_metrics import collect  # noqa: E402

SCA = (
    REPOSITORY_ROOT
    / "simulations/results/omnetpp/Connectivity_SmokeTest-0.sca"
)


def scalar(frame, module_suffix: str, name: str) -> float:
    selected = frame.loc[
        frame["module"].str.endswith(module_suffix) & (frame["name"] == name),
        "value",
    ]
    if len(selected) != 1:
        raise ValueError(
            f"expected one {module_suffix}.{name} scalar, found {len(selected)}"
        )
    return float(selected.iloc[0])


def main() -> None:
    if not SCA.exists():
        raise SystemExit(f"resultado ausente: {SCA}")
    _, frame, _ = parse_sca(str(SCA))
    outcomes = collect(str(SCA))
    drone0 = ".drone[0].app[0]"
    drone1 = ".drone[1].app[0]"
    values = {
        "drone0_updates": scalar(frame, drone0, "droneStatusUpdatesAccepted"),
        "drone1_updates": scalar(frame, drone1, "droneStatusUpdatesAccepted"),
        "constraints": scalar(frame, drone0, "connectivityConstraintsApplied"),
        "preserved": scalar(frame, drone0, "connectivityPreservedSelections"),
    }
    failures = []
    if values["drone0_updates"] <= 0 or values["drone1_updates"] <= 0:
        failures.append("os dois drones devem receber DroneStatus do vizinho")
    if values["constraints"] != 1:
        failures.append(
            f"esperada uma restrição de conectividade, obtido {values['constraints']}"
        )
    if values["preserved"] != 1:
        failures.append(
            f"esperada uma seleção conectada, obtido {values['preserved']}"
        )
    if outcomes["repositions_completed"] != 1:
        failures.append("o reposicionamento conectado deve ser concluído")
    if outcomes["alerts_confirmed"] != 1:
        failures.append("a rede deve continuar disponível para alerta e ACK")
    if failures:
        raise SystemExit("Connectivity smoke test FALHOU:\n- " + "\n- ".join(failures))

    print("Connectivity smoke test OK")
    print("  descoberta mútua por DroneStatus: confirmada")
    print("  restrição aplicada e vizinho preservado: confirmado")
    print("  comunicação após o movimento: confirmada")


if __name__ == "__main__":
    main()
