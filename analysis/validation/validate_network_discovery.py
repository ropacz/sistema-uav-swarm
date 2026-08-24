#!/usr/bin/env python3
"""Valida entrega direta e o efeito operacional do broadcast com TTL 1."""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.experiment_metrics import collect  # noqa: E402

ROOT = REPOSITORY_ROOT
RESULTS = ROOT / "simulations/results/omnetpp"


def main() -> None:
    direct_path = RESULTS / "DiscoveryValidation_Direct-0.sca"
    relay_path = RESULTS / "DiscoveryValidation_RemoteViaRelay-0.sca"
    for path in (direct_path, relay_path):
        if not path.exists():
            raise SystemExit(f"resultado ausente: {path}")

    direct = collect(str(direct_path))
    if direct["alerts_confirmed"] != 1:
        raise SystemExit(
            "direto: esperado um alerta confirmado, obtido "
            f"{direct['alerts_confirmed']}"
        )

    relay = collect(str(relay_path))
    if relay["alerts_confirmed"] != 0 or relay["alert_attempts_sent"] != 0:
        raise SystemExit(
            "origem não deve alcançar equipe conhecida apenas pelo relay; "
            f"obtido ACK={relay['alerts_confirmed']}, "
            f"tentativas={relay['alert_attempts_sent']}"
        )
    print("direto: um alerta confirmado")
    print("remoto via relay: equipe não descoberta, tentativas=0 (TTL 1 confirmado)")


if __name__ == "__main__":
    main()
