"""Validate hop accounting from the IPv4 HopLimitInd received by TeamApp."""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.process_results import parse_sca  # noqa: E402
from analysis.core.experiment_metrics import collect  # noqa: E402

SCA = REPOSITORY_ROOT / "simulations/results/omnetpp/Multihop_SmokeTest-0.sca"


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
    row = collect(str(SCA))
    _, frame, _ = parse_sca(str(SCA))
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
    # Cada drone ouve o repasse do outro, e nenhum conta o eco local do próprio
    # repasse: os dois contadores separam o que é evidência do enlace direto do
    # que chegou por salto intermediário.
    for drone in (".drone[0].app[0]", ".drone[1].app[0]"):
        forwarded = scalar(frame, drone, "forwardedTeamUpdatesIgnoredForRssi")
        direct = scalar(frame, drone, "directRssiSamples")
        if forwarded <= 0:
            failures.append(
                f"{drone}: repasses recebidos devem ficar fora da janela de RSSI"
            )
        if direct <= 0:
            failures.append(
                f"{drone}: a equipe também é ouvida diretamente neste cenário"
            )
    if failures:
        raise SystemExit("Multihop smoke test FALHOU:\n- " + "\n- ".join(failures))

    print("Multihop smoke test OK")
    print("  VictimAlert entregue em 2 saltos por um drone intermediário")
    print("  HopLimitInd e encaminhamento intermediário contabilizados")
    print("  repasses separados das recepções diretas na janela de RSSI")


if __name__ == "__main__":
    main()
