#!/usr/bin/env python3
"""Efeito pareado (BA-On − BA-Off) com IC 95% por Student-t, ao lado do
bootstrap oficial — para comparar os dois métodos.

`alert_sheet.py::paired_effects()` usa bootstrap percentil (ver
`docs/metricas_e_arquivos_de_resultado.md`, seção 6). Este script usa a
mesma construção de pares (`build_pairs`), mas troca o método de intervalo
de confiança por `ci95()` (`analysis/core/process_results.py`), a
aproximação de Student-t que existe no projeto mas não é usada em nenhum
outro lugar. Não substitui a planilha oficial — é uma segunda via, para
decidir com números na mão se os dois métodos concordam.
"""

from __future__ import annotations

from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import pandas as pd  # noqa: E402

from analysis.core.process_results import ci95  # noqa: E402
from analysis.reports import figures  # noqa: E402
from analysis.reports.alert_sheet import build_pairs, load, paired_effects  # noqa: E402

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
OUTPUT = REPOSITORY_ROOT / "analysis/tables"
FIGURES_OUTPUT = REPOSITORY_ROOT / "analysis/figures"


def student_t_effects(pairs: pd.DataFrame) -> pd.DataFrame:
    """Uma linha por (cenário, numTeams): efeito médio ± IC 95% via ci95()."""
    rows = []
    for (scenario, teams), group in pairs.groupby(
            ["scenario", "numTeams"], sort=True):
        attendance_half = ci95(group["attendanceEffect"])
        loss_half = ci95(group["lossEffect"])
        attendance_mean = group["attendanceEffect"].mean()
        loss_mean = group["lossEffect"].mean()
        rows.append({
            "cenario": scenario,
            "numTeams": teams,
            "pares": len(group),
            "efeito_atendimento_pp": attendance_mean,
            "efeito_atendimento_t_ic95_inf_pp": attendance_mean - attendance_half,
            "efeito_atendimento_t_ic95_sup_pp": attendance_mean + attendance_half,
            "efeito_perda_pp": loss_mean,
            "efeito_perda_t_ic95_inf_pp": loss_mean - loss_half,
            "efeito_perda_t_ic95_sup_pp": loss_mean + loss_half,
        })
    return pd.DataFrame(rows)


def compare(bootstrap: pd.DataFrame, student_t: pd.DataFrame) -> pd.DataFrame:
    """Uma linha por célula, os dois métodos lado a lado, larguras dos dois IC."""
    keys = ["cenario", "numTeams"]
    # "pares" e o efeito médio são idênticos nos dois métodos (mesmos pares de
    # seed) — só os limites do IC diferem. Descartar do lado direito evita
    # colunas duplicadas (ex.: "efeito_atendimento_pp_t") sem informação nova.
    only_ci = student_t.drop(columns=["pares", "efeito_atendimento_pp", "efeito_perda_pp"])
    merged = bootstrap.merge(only_ci, on=keys, validate="one_to_one")
    merged["atendimento_largura_bootstrap_pp"] = (
        merged["efeito_atendimento_ic95_sup_pp"] - merged["efeito_atendimento_ic95_inf_pp"])
    merged["atendimento_largura_t_pp"] = (
        merged["efeito_atendimento_t_ic95_sup_pp"] - merged["efeito_atendimento_t_ic95_inf_pp"])
    merged["perda_largura_bootstrap_pp"] = (
        merged["efeito_perda_ic95_sup_pp"] - merged["efeito_perda_ic95_inf_pp"])
    merged["perda_largura_t_pp"] = (
        merged["efeito_perda_t_ic95_sup_pp"] - merged["efeito_perda_t_ic95_inf_pp"])
    return merged


def comparison_figure(merged: pd.DataFrame, column: str, ic_bootstrap: tuple[str, str],
                       ic_t: tuple[str, str], axis_label: str, name: str) -> Path:
    """Um painel por cenário: efeito ± IC, bootstrap e t lado a lado por numTeams."""
    scenarios = sorted(merged["cenario"].unique())
    figure, axes_list = plt.subplots(
        1, len(scenarios), figsize=(figures.TEXT_WIDTH_IN, 2.8), sharey=True)
    if len(scenarios) == 1:
        axes_list = [axes_list]

    for axes, scenario in zip(axes_list, scenarios):
        cell = merged[merged["cenario"] == scenario].sort_values("numTeams")
        teams = cell["numTeams"].to_numpy()
        positions = range(len(teams))
        width = 0.32

        bootstrap_low = cell[column] - cell[ic_bootstrap[0]]
        bootstrap_high = cell[ic_bootstrap[1]] - cell[column]
        t_low = cell[column] - cell[ic_t[0]]
        t_high = cell[ic_t[1]] - cell[column]

        axes.errorbar([p - width / 2 for p in positions], cell[column],
                      yerr=[bootstrap_low, bootstrap_high], fmt="o",
                      color=figures.INK, capsize=3, label="bootstrap")
        axes.errorbar([p + width / 2 for p in positions], cell[column],
                      yerr=[t_low, t_high], fmt="s", color="#9e9e9e",
                      capsize=3, label="Student-t")
        axes.axhline(0, color=figures.GRID, linewidth=0.8, zorder=0)
        axes.set_xticks(list(positions))
        axes.set_xticklabels([int(value) for value in teams])
        axes.set_xlabel("Quantidade de equipes")
        axes.set_title(scenario, fontsize=8)
        axes.grid(axis="y", color=figures.GRID, linewidth=0.6, zorder=0)
        axes.set_axisbelow(True)

    axes_list[0].set_ylabel(axis_label)
    axes_list[-1].legend(frameon=False, loc="upper right", fontsize=7)
    return figures.save(figure, name, directory=FIGURES_OUTPUT)


def main() -> None:
    table = load()
    pairs = build_pairs(table)
    if pairs.empty:
        raise SystemExit("nenhum par BA-Off/BA-On encontrado — rode a campanha antes.")

    bootstrap = paired_effects(table)
    student_t = student_t_effects(pairs)
    merged = compare(bootstrap, student_t)

    OUTPUT.mkdir(parents=True, exist_ok=True)
    workbook = OUTPUT / "efeito_pareado_bootstrap_vs_t.xlsx"
    with pd.ExcelWriter(workbook, engine="openpyxl") as writer:
        merged.to_excel(writer, sheet_name="BootstrapVsT", index=False)
        bootstrap.to_excel(writer, sheet_name="Bootstrap", index=False)
        student_t.to_excel(writer, sheet_name="StudentT", index=False)
    print(f"gerado: {workbook.relative_to(REPOSITORY_ROOT)}")

    figures.configure_style()
    attendance_chart = comparison_figure(
        merged, "efeito_atendimento_pp",
        ("efeito_atendimento_ic95_inf_pp", "efeito_atendimento_ic95_sup_pp"),
        ("efeito_atendimento_t_ic95_inf_pp", "efeito_atendimento_t_ic95_sup_pp"),
        "Efeito no atendimento (p.p.)", "efeito_atendimento_bootstrap_vs_t")
    loss_chart = comparison_figure(
        merged, "efeito_perda_pp",
        ("efeito_perda_ic95_inf_pp", "efeito_perda_ic95_sup_pp"),
        ("efeito_perda_t_ic95_inf_pp", "efeito_perda_t_ic95_sup_pp"),
        "Efeito na perda (p.p.)", "efeito_perda_bootstrap_vs_t")
    for chart in (attendance_chart, loss_chart):
        print(f"gerado: {chart.relative_to(REPOSITORY_ROOT)}")

    print()
    print(merged[["cenario", "numTeams", "pares",
                  "atendimento_largura_bootstrap_pp", "atendimento_largura_t_pp",
                  "perda_largura_bootstrap_pp", "perda_largura_t_pp"]]
          .to_string(index=False, float_format=lambda value: f"{value:.2f}"))


if __name__ == "__main__":
    main()
