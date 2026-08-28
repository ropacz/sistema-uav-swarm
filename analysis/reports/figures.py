#!/usr/bin/env python3
"""Gera as figuras de atendimento e perda a partir da planilha de alertas.

As figuras saem em PDF vetorial, sem título embutido: na ABNT o título é legenda
acima da figura e a fonte vem abaixo, ambos escritos no LaTeX. Os rótulos ficam
em português e a paleta é legível em tons de cinza, para não depender de cor na
versão impressa.

Não recalcula nada. Todo número vem do resumo que `alert_sheet.py` já produziu.
"""

from __future__ import annotations

from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import pandas as pd  # noqa: E402

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]

# As duas figuras acompanham a planilha simples, na raiz de analysis/figures/.
SIMPLE_OUTPUT = REPOSITORY_ROOT / "analysis/figures"

TEXT_WIDTH_IN = 6.3  # Largura útil de uma página A4 com margens ABNT (~16 cm).
INK = "#1a1a1a"
GRID = "#d9d9d9"

ARMS = [(False, "BA desligado", "#e0e0e0", "///"),
        (True, "BA ligado", "#9e9e9e", "")]


def configure_style() -> None:
    plt.rcParams.update({
        "font.family": "serif",
        "font.serif": ["DejaVu Serif", "Times New Roman", "STIXGeneral"],
        "font.size": 9,
        "axes.labelsize": 9,
        "axes.titlesize": 9,
        "xtick.labelsize": 8,
        "ytick.labelsize": 8,
        "legend.fontsize": 8,
        "axes.edgecolor": INK,
        "axes.labelcolor": INK,
        "text.color": INK,
        "xtick.color": INK,
        "ytick.color": INK,
        "axes.spines.top": False,
        "axes.spines.right": False,
        "figure.dpi": 200,
        "savefig.bbox": "tight",
        "savefig.pad_inches": 0.02,
    })


def save(figure: plt.Figure, name: str, directory: Path | None = None) -> Path:
    directory = SIMPLE_OUTPUT if directory is None else directory
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f"{name}.pdf"
    figure.savefig(path)
    plt.close(figure)
    return path


def rate_figure(summary: pd.DataFrame, column: str, axis_label: str,
                name: str) -> Path:
    """Barras agrupadas de uma taxa, uma cor por braço e uma legenda.

    O eixo horizontal é a quantidade de equipes, de modo que a mesma figura
    serve a uma única célula (uma barra por braço) ou a uma matriz maior
    (várias quantidades de equipe lado a lado), sem trocar de formato.
    """
    teams = sorted(summary["numTeams"].unique())
    figure, axes = plt.subplots(figsize=(TEXT_WIDTH_IN, 2.7))
    # Com poucas categorias, barras largas dominam a figura sem acrescentar
    # informação; a largura acompanha a quantidade de níveis no eixo.
    width = 0.34 if len(teams) > 2 else 0.20
    for offset, (enabled, label, color, hatch) in zip((-width / 2, width / 2), ARMS):
        rows = summary[summary["baEnabled"] == enabled].set_index("numTeams")
        values = [rows.loc[team, column] if team in rows.index else float("nan")
                  for team in teams]
        positions = [index + offset for index in range(len(teams))]
        axes.bar(positions, values, width=width, label=label,
                 color=color, edgecolor=INK, linewidth=0.8, hatch=hatch)
        for position, value in zip(positions, values):
            if value == value:
                axes.text(position, value + 1.5, f"{value:.1f}",
                          ha="center", fontsize=8, color=INK)
    axes.set_xticks(range(len(teams)))
    axes.set_xticklabels([int(team) for team in teams])
    axes.set_xlabel("Quantidade de equipes")
    axes.set_ylabel(axis_label)
    axes.set_xlim(-0.6, len(teams) - 0.4)
    axes.set_ylim(0, 105)
    # Legenda acima da área do gráfico: não disputa espaço com os rótulos das
    # barras nem obriga a deixar um vazio artificial no topo.
    axes.legend(frameon=False, ncols=2, loc="lower center",
                bbox_to_anchor=(0.5, 1.01))
    axes.grid(axis="y", color=GRID, linewidth=0.6, zorder=0)
    axes.set_axisbelow(True)
    return save(figure, name)


def attendance_figures(summary: pd.DataFrame, suffix: str = "") -> list[Path]:
    """As duas taxas pedidas, cada uma em sua figura para caber uma legenda.

    `suffix` distingue os arquivos quando o resumo cobre várias famílias de
    cenário (ex.: "_OneVictim", "_TwoVictims") — sem isso, `rate_figure`
    indexaria por numTeams com valores repetidos entre cenários e o mesmo
    nome de arquivo seria sobrescrito por cenário.
    """
    return [
        rate_figure(summary, "atendimento_pct",
                    "Atendimento (%)", f"atendimento{suffix}"),
        rate_figure(summary, "perda_pct",
                    "Perda (%)", f"perda{suffix}"),
    ]


def effect_figure(paired: pd.DataFrame, column: str, ic_low: str, ic_high: str,
                   axis_label: str, name: str) -> Path:
    """Efeito pareado (BA-On − BA-Off) com IC 95% bootstrap, um painel por
    cenário lado a lado — mesma ideia do `rate_figure`, mas com barra de erro
    em vez de barras, porque aqui a grandeza é uma diferença, não uma taxa.
    """
    scenarios = sorted(paired["cenario"].unique())
    figure, axes_list = plt.subplots(
        1, len(scenarios), figsize=(TEXT_WIDTH_IN, 2.8), sharey=True)
    if len(scenarios) == 1:
        axes_list = [axes_list]

    for axes, scenario in zip(axes_list, scenarios):
        cell = paired[paired["cenario"] == scenario].sort_values("numTeams")
        teams = cell["numTeams"].to_numpy()
        positions = range(len(teams))
        low = cell[column] - cell[ic_low]
        high = cell[ic_high] - cell[column]

        axes.errorbar(positions, cell[column], yerr=[low, high], fmt="o",
                      color=INK, capsize=3, linewidth=0.8, markersize=4)
        # Efeito zero é a hipótese nula: cruzar essa linha é o que decide se
        # há evidência de diferença naquela célula.
        axes.axhline(0, color=GRID, linewidth=0.8, zorder=0)
        axes.set_xticks(list(positions))
        axes.set_xticklabels([int(value) for value in teams])
        axes.set_xlabel("Quantidade de equipes")
        axes.set_title(scenario, fontsize=8)
        axes.grid(axis="y", color=GRID, linewidth=0.6, zorder=0)
        axes.set_axisbelow(True)

    axes_list[0].set_ylabel(axis_label)
    return save(figure, name)


def effect_figures(paired: pd.DataFrame) -> list[Path]:
    """Efeito pareado (BA-On − BA-Off) com IC bootstrap, atendimento e perda."""
    return [
        effect_figure(paired, "efeito_atendimento_pp",
                      "efeito_atendimento_ic95_inf_pp", "efeito_atendimento_ic95_sup_pp",
                      "Efeito no atendimento (p.p.)", "efeito_atendimento"),
        effect_figure(paired, "efeito_perda_pp",
                      "efeito_perda_ic95_inf_pp", "efeito_perda_ic95_sup_pp",
                      "Efeito na perda (p.p.)", "efeito_perda"),
    ]
