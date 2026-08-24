"""Gera o gráfico solicitado para Scenario1_Line1_900s10_BaOn."""

from pathlib import Path

import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parents[2]
INPUT = ROOT / "analysis/figures/professor_runs.csv"
OUTPUT = ROOT / "analysis/figures/scenario1_line1_atendimento_perdas.png"
CONFIG = "Scenario1_Line1_900s10_BaOn"


def main():
    runs = pd.read_csv(INPUT)
    selected = runs[runs["config"] == CONFIG]
    counts = selected.groupby("teams")["seed"].nunique()
    if counts.to_dict() != {1: 10, 5: 10, 10: 10, 15: 10}:
        raise SystemExit(f"esperadas 10 seeds por equipe; obtido {counts.to_dict()}")

    summary = selected.groupby("teams")[["attendance_pct", "attempt_loss_pct"]].agg(["mean", "std"])
    teams = summary.index.to_list()
    x = range(len(teams))
    width = 0.36
    figure, axis = plt.subplots(figsize=(9, 5.4))
    axis.bar([value - width / 2 for value in x],
             summary[("attendance_pct", "mean")], width,
             yerr=summary[("attendance_pct", "std")], capsize=4,
             label="Atendimento Drone → equipe (%)", color="#2878B5")
    axis.bar([value + width / 2 for value in x],
             summary[("attempt_loss_pct", "mean")], width,
             yerr=summary[("attempt_loss_pct", "std")], capsize=4,
             label="Perdas até aceitação na aplicação (%)", color="#D9534F")
    axis.set_xticks(list(x), [str(team) for team in teams])
    axis.set_xlabel("Número de equipes")
    axis.set_ylabel("Percentual (%)")
    axis.set_ylim(0, 105)
    axis.set_title("Cenário 1 — métricas da aplicação (.sca)\n"
                   "4 drones, 1 vítima, 2 obstáculos, 900 s, 10 seeds")
    axis.grid(axis="y", alpha=0.25)
    axis.legend(loc="upper right")
    figure.tight_layout()
    OUTPUT.parent.mkdir(exist_ok=True)
    figure.savefig(OUTPUT, dpi=180)
    plt.close(figure)

    table = selected.groupby("teams").agg(
        seeds=("seed", "nunique"),
        atendimento_media=("attendance_pct", "mean"),
        atendimento_dp=("attendance_pct", "std"),
        perdas_media=("attempt_loss_pct", "mean"),
        perdas_dp=("attempt_loss_pct", "std"),
    )
    print(table.to_string(float_format=lambda value: f"{value:.2f}"))
    print(f"\nGráfico: {OUTPUT}")


if __name__ == "__main__":
    main()
