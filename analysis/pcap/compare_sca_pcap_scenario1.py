#!/usr/bin/env python3
"""Compara métricas do Cenário 1 obtidas dos escalares e das PCAPNG.

A unidade experimental é uma execução/seed. Atendimento é calculado com
``alertId`` único; perdas usam cada ``messageId`` de VictimAlert, incluindo
retransmissões. Assim, a semântica reproduz as métricas registradas pela
aplicação sem confundir quadros de PositionUpdate ou AODV.
"""

from pathlib import Path
import sys

import matplotlib.pyplot as plt
import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.pcap.pcap_batch_to_spreadsheet import (  # noqa: E402
    compare_group, discover_captures,
)

ROOT = REPOSITORY_ROOT
CONFIG = "Scenario1_Line1_900s10_BaOn"
PCAP_DIR = ROOT / "simulations/results/pcap"
SCALAR_CSV = ROOT / "analysis/figures/professor_runs.csv"
OUTPUT_DIR = ROOT / "analysis/figures"
RUNS_CSV = OUTPUT_DIR / "scenario1_line1_sca_pcap_runs.csv"
SUMMARY_CSV = OUTPUT_DIR / "scenario1_line1_sca_pcap_summary.csv"
PCAP_GRAPH = OUTPUT_DIR / "scenario1_line1_atendimento_perdas_pcap.png"
EXPECTED = {1: 10, 5: 10, 10: 10, 15: 10}


def pcap_metrics() -> pd.DataFrame:
    captures, inventory = discover_captures(PCAP_DIR)
    invalid = [row for row in inventory if row.get("status") != "processado"
               and CONFIG in row.get("file", "")]
    if invalid:
        raise ValueError(f"capturas inválidas do cenário: {invalid[:3]}")

    selected = [capture for capture in captures
                if capture.configuration == CONFIG]
    grouped = {}
    for capture in selected:
        # A comparação científica solicitada só usa o fluxo de alerta/ACK.
        capture.events = [event for event in capture.events
                          if event["message_type"] in {"VictimAlert", "VictimAck"}]
        grouped.setdefault(capture.run, []).append(capture)

    rows = []
    for run, run_captures in grouped.items():
        comparisons = pd.DataFrame(compare_group(run_captures))
        alerts = comparisons[comparisons["message_type"] == "VictimAlert"]
        acks = comparisons[comparisons["message_type"] == "VictimAck"]
        generated_ids = set(alerts["alert_id"].dropna().astype(str))
        acked_ids = set(
            acks.loc[acks["received"] == "Sim", "alert_id"].dropna().astype(str)
        )
        # O PcapRecorder observa retransmissões MAC da mesma tentativa. A
        # aplicação, porém, incrementa alertAttemptsSent uma vez por messageId.
        # Reduza as várias cópias a uma tentativa e marque entrega quando ao
        # menos uma cópia válida aparece na captura do destino.
        attempts_by_message = alerts.groupby("message_id", as_index=False).agg(
            received=("received", lambda values: (values == "Sim").any())
        )
        attempts = len(attempts_by_message)
        received = int(attempts_by_message["received"].sum())
        rows.append({
            "run": int(run),
            "pcap_alerts_generated": len(generated_ids),
            "pcap_alerts_acked": len(generated_ids & acked_ids),
            "pcap_attempts_sent": attempts,
            "pcap_attempts_received": received,
            "pcap_attendance_pct": (
                100 * len(generated_ids & acked_ids) / len(generated_ids)
                if generated_ids else float("nan")
            ),
            "pcap_attempt_loss_pct": (
                100 * (1 - received / attempts) if attempts else float("nan")
            ),
        })
    return pd.DataFrame(rows)


def plot_pcap(runs: pd.DataFrame) -> None:
    summary = runs.groupby("teams")[[
        "pcap_attendance_pct", "pcap_attempt_loss_pct"
    ]].agg(["mean", "std"])
    teams = summary.index.to_list()
    x = range(len(teams))
    width = 0.36
    figure, axis = plt.subplots(figsize=(9, 5.4))
    axis.bar([value - width / 2 for value in x],
             summary[("pcap_attendance_pct", "mean")], width,
             yerr=summary[("pcap_attendance_pct", "std")], capsize=4,
             label="Atendimento Drone → equipe (%)", color="#2878B5")
    axis.bar([value + width / 2 for value in x],
             summary[("pcap_attempt_loss_pct", "mean")], width,
             yerr=summary[("pcap_attempt_loss_pct", "std")], capsize=4,
             label="Perdas na entrega pelo enlace (%)", color="#D9534F")
    axis.set_xticks(list(x), [str(team) for team in teams])
    axis.set_xlabel("Número de equipes")
    axis.set_ylabel("Percentual (%)")
    axis.set_ylim(0, 105)
    axis.set_title(
        "Cenário 1 — métricas obtidas das PCAPNG\n"
        "4 drones, 1 vítima, 2 obstáculos, 900 s, 10 seeds"
    )
    axis.grid(axis="y", alpha=0.25)
    axis.legend(loc="upper right")
    figure.tight_layout()
    figure.savefig(PCAP_GRAPH, dpi=180)
    plt.close(figure)


def main() -> None:
    scalar = pd.read_csv(SCALAR_CSV)
    scalar = scalar[scalar["config"] == CONFIG].copy()
    scalar["run"] = scalar["run_file"].str.extract(r"-(\d+)\.sca$")[0].astype(int)
    pcap = pcap_metrics()
    runs = scalar.merge(pcap, on="run", how="outer", validate="one_to_one")

    counts = runs.groupby("teams")["run"].nunique().to_dict()
    if counts != EXPECTED or len(pcap) != sum(EXPECTED.values()):
        raise ValueError(
            f"esperadas 40 seeds e {EXPECTED}; obtido {len(pcap)} e {counts}"
        )

    runs["attendance_difference_pp"] = (
        runs["pcap_attendance_pct"] - runs["attendance_pct"]
    )
    runs["loss_difference_pp"] = (
        runs["pcap_attempt_loss_pct"] - runs["attempt_loss_pct"]
    )
    summary = runs.groupby("teams").agg(
        seeds=("run", "nunique"),
        scalar_attendance_mean=("attendance_pct", "mean"),
        pcap_attendance_mean=("pcap_attendance_pct", "mean"),
        attendance_max_abs_difference_pp=("attendance_difference_pp", lambda x: x.abs().max()),
        scalar_loss_mean=("attempt_loss_pct", "mean"),
        pcap_loss_mean=("pcap_attempt_loss_pct", "mean"),
        loss_max_abs_difference_pp=("loss_difference_pp", lambda x: x.abs().max()),
    )

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    runs.to_csv(RUNS_CSV, index=False)
    summary.to_csv(SUMMARY_CSV)
    plot_pcap(runs)
    print(summary.to_string(float_format=lambda value: f"{value:.6f}"))
    print(f"\nComparação por seed: {RUNS_CSV}")
    print(f"Gráfico PCAP: {PCAP_GRAPH}")


if __name__ == "__main__":
    main()
