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


def attendance_figures(summary: pd.DataFrame) -> list[Path]:
    """As duas taxas pedidas, cada uma em sua figura para caber uma legenda."""
    return [
        rate_figure(summary, "atendimento_pct",
                    "Atendimento (%)", "atendimento"),
        rate_figure(summary, "perda_pct",
                    "Perda (%)", "perda"),
    ]
