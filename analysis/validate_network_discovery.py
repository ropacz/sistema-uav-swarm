#!/usr/bin/env python3
"""Valida entrega direta e o limite da descoberta local por broadcast."""

from pathlib import Path

from network_metrics import APP, mean_where, sum_where
from process_results import parse_sca


ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "simulations/results/omnetpp"


def main() -> None:
    direct_path = RESULTS / "DiscoveryValidation_Direct-0.sca"
    relay_path = RESULTS / "DiscoveryValidation_RemoteViaRelay-0.sca"
    for path in (direct_path, relay_path):
        if not path.exists():
            raise SystemExit(f"resultado ausente: {path}")

    _, direct, _ = parse_sca(str(direct_path))
    direct_acked = sum_where(direct, "uniqueAlertsAcked", APP)
    direct_hops = mean_where(direct, "hopCount:mean")
    if direct_acked != 1 or direct_hops != 0:
        raise SystemExit(
            f"direto: esperado ACK=1/hopCount=0, obtido {direct_acked}/{direct_hops}"
        )

    _, relay, _ = parse_sca(str(relay_path))
    relay_acked = sum_where(relay, "uniqueAlertsAcked", APP)
    relay_attempts = sum_where(relay, "alertAttemptsSent", APP)
    discovered = relay[relay["name"] == "teamEntriesDiscovered"]
    origin_discovered = float(discovered.loc[
        discovered["module"].str.contains(r"\.drone\[0\]\.app\[0\]$", regex=True),
        "value",
    ].sum())
    relay_discovered = float(discovered.loc[
        discovered["module"].str.contains(r"\.drone\[1\]\.app\[0\]$", regex=True),
        "value",
    ].sum())
    if (relay_acked != 0 or relay_attempts != 0 or
            origin_discovered != 0 or relay_discovered != 1):
        raise SystemExit(
            "equipe remota não deve ser conhecida sem encaminhamento de descoberta; "
            f"obtido ACK={relay_acked}, tentativas={relay_attempts}, "
            f"origem={origin_discovered}, relay={relay_discovered}"
        )
    print("direto: ACK=1, intermediários=0")
    print("remoto via relay: equipe não descoberta, tentativas=0 (TTL 1 confirmado)")


if __name__ == "__main__":
    main()
