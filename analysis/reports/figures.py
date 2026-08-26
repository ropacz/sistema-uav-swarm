#!/usr/bin/env python3
"""Gera as figuras do trabalho a partir das tabelas já produzidas pelos relatórios.

As figuras saem em PDF vetorial, sem título embutido: na ABNT o título é legenda
acima da figura e a fonte vem abaixo, ambos escritos no LaTeX. Os rótulos ficam
em português e a paleta é legível em tons de cinza, para não depender de cor na
versão impressa.

Não recalcula nada. Se um número aparece numa figura, ele veio de um CSV gerado
por `report_main_experiment.py` ou `report_robustness.py`.
"""

from __future__ import annotations

from pathlib import Path
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import pandas as pd  # noqa: E402

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

TABLES = REPOSITORY_ROOT / "analysis/tables/detalhado"
OUTPUT = REPOSITORY_ROOT / "analysis/figures/detalhado"
# As figuras de atendimento e perda acompanham a planilha simples.
SIMPLE_OUTPUT = REPOSITORY_ROOT / "analysis/figures"

# Largura útil de uma página A4 com margens ABNT (3 cm / 2 cm): ~16 cm.
TEXT_WIDTH_IN = 6.3
INK = "#1a1a1a"
MUTED = "#6e6e6e"
GRID = "#d9d9d9"

# Nomes de exibição. Apenas as métricas que a figura realmente usa: métricas em
# unidades diferentes não podem dividir o mesmo eixo sem induzir comparação
# indevida, então atrasos, retransmissões e saltos ficam nas tabelas.
PERCENTAGE_METRICS = {
    "alert_pdr_pct": "Entrega de alertas",
    "appack_pct": "Confirmação de alertas",
    "attempt_pdr_pct": "Entrega de tentativas",
    "multi_hop_delivery_rate_pct": "Entregas multissalto",
}
PRIMARY_METRIC = "alert_pdr_pct"

FUNNEL_STAGES = [
    ("reposition_triggers", "Gatilho de reposicionamento"),
    ("obstacles_detected", "Obstáculo detectado"),
    ("ba_activations", "Algoritmo do Morcego acionado"),
    ("repositions_started", "Deslocamento iniciado"),
    ("repositions_completed", "Deslocamento concluído"),
]


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
    directory = OUTPUT if directory is None else directory
    directory.mkdir(parents=True, exist_ok=True)
    path = directory / f"{name}.pdf"
    figure.savefig(path)
    plt.close(figure)
    return path


ARMS = [(False, "BA desligado", "#e0e0e0", "///"),
        (True, "BA ligado", "#9e9e9e", "")]


def rate_figure(summary: pd.DataFrame, column: str, axis_label: str,
                name: str) -> Path:
    """Barras agrupadas de uma taxa, uma cor por braço e uma legenda.

    O eixo horizontal é a quantidade de equipes, de modo que a mesma figura
    serve ao experimento principal (uma única quantidade) e à matriz de
    robustez, sem trocar de formato entre os dois.
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
    return save(figure, name, SIMPLE_OUTPUT)


def attendance_figures(summary: pd.DataFrame) -> list[Path]:
    """As duas taxas pedidas, cada uma em sua figura para caber uma legenda."""
    return [
        rate_figure(summary, "atendimento_pct",
                    "Atendimento (%)", "atendimento"),
        rate_figure(summary, "perda_pct",
                    "Perda (%)", "perda"),
    ]


def paired_effect_figure(summary: pd.DataFrame) -> Path:
    """Efeito pareado das métricas em pontos percentuais, com IC de 95%."""
    data = summary[summary["metric"].isin(PERCENTAGE_METRICS)].copy()
    data["label"] = data["metric"].map(PERCENTAGE_METRICS)
    # Primária no topo; as demais abaixo, na ordem em que foram declaradas.
    order = [PRIMARY_METRIC] + [m for m in PERCENTAGE_METRICS if m != PRIMARY_METRIC]
    data = data.set_index("metric").loc[order].reset_index()

    figure, axes = plt.subplots(figsize=(TEXT_WIDTH_IN, 2.2))
    positions = range(len(data))[::-1]
    for position, row in zip(positions, data.itertuples()):
        primary = row.metric == PRIMARY_METRIC
        axes.errorbar(
            row.effect_on_minus_off, position,
            xerr=row.effect_ci95_half_width,
            fmt="o" if primary else "s",
            markersize=6 if primary else 4.5,
            markerfacecolor=INK if primary else "white",
            markeredgecolor=INK, color=INK,
            elinewidth=1.1, capsize=3.5,
        )
    axes.axvline(0, color=MUTED, linewidth=0.9, linestyle="--", zorder=0)
    axes.set_yticks(list(positions))
    labels = [f"$\\bf{{{r.label.replace(' ', chr(92) + ' ')}}}$" if r.metric == PRIMARY_METRIC
              else r.label for r in data.itertuples()]
    axes.set_yticklabels(labels)
    axes.set_xlabel("Diferença pareada, BA ligado − BA desligado (pontos percentuais)")
    axes.set_ylim(-0.6, len(data) - 0.4)
    axes.grid(axis="x", color=GRID, linewidth=0.6, zorder=0)
    axes.set_axisbelow(True)
    return save(figure, "efeito_pareado")


def exposure_funnel_figure(exposure: pd.DataFrame, per_run: pd.DataFrame) -> Path:
    """Quantas execuções alcançaram cada etapa do mecanismo avaliado."""
    total = int(exposure.iloc[0]["treatment_runs"])
    counts, events, labels = [], [], []
    for column, label in FUNNEL_STAGES:
        counts.append(int((per_run[column] > 0).sum()))
        events.append(int(per_run[column].sum()))
        labels.append(label)

    figure, axes = plt.subplots(figsize=(TEXT_WIDTH_IN, 2.4))
    positions = list(range(len(labels)))[::-1]
    axes.barh(positions, counts, height=0.6,
              color="#bdbdbd", edgecolor=INK, linewidth=0.8)
    for position, count, event in zip(positions, counts, events):
        axes.text(count + total * 0.015, position,
                  f"{count}/{total}  ({event} eventos)",
                  va="center", fontsize=8, color=INK)
    axes.set_yticks(positions)
    axes.set_yticklabels(labels)
    axes.set_xlabel(f"Execuções que alcançaram a etapa (de {total})")
    axes.set_xlim(0, total * 1.42)
    axes.grid(axis="x", color=GRID, linewidth=0.6, zorder=0)
    axes.set_axisbelow(True)
    return save(figure, "funil_exposicao")


def robustness_figure(summary: pd.DataFrame) -> Path:
    """Efeito na entrega de alertas conforme a quantidade de equipes."""
    data = summary[summary["metric"] == PRIMARY_METRIC].copy()
    names = {"Scenario1_OneVictim": "Uma vítima",
             "Scenario1_TwoVictims": "Duas vítimas"}
    markers = {"Scenario1_OneVictim": "o", "Scenario1_TwoVictims": "s"}

    figure, axes = plt.subplots(figsize=(TEXT_WIDTH_IN, 2.6))
    offsets = {"Scenario1_OneVictim": -0.10, "Scenario1_TwoVictims": 0.10}
    for scenario, group in data.groupby("scenario"):
        group = group.sort_values("teams")
        # Deslocamento horizontal pequeno evita sobreposição das barras de erro.
        x = [i + offsets.get(scenario, 0) for i in range(len(group))]
        axes.errorbar(
            x, group["effect_mean"], yerr=group["effect_ci95_half_width"],
            fmt=markers.get(scenario, "^"), markersize=5,
            markerfacecolor="white" if scenario.endswith("TwoVictims") else INK,
            markeredgecolor=INK, color=INK,
            elinewidth=1.1, capsize=3.5, linestyle="none",
            label=names.get(scenario, scenario),
        )
        axes.set_xticks(range(len(group)))
        axes.set_xticklabels([int(t) for t in group["teams"]])
    axes.axhline(0, color=MUTED, linewidth=0.9, linestyle="--", zorder=0)
    axes.set_xlabel("Quantidade de equipes")
    axes.set_ylabel("Diferença pareada na\nentrega de alertas (p.p.)")
    axes.legend(frameon=False, loc="best")
    axes.grid(axis="y", color=GRID, linewidth=0.6, zorder=0)
    axes.set_axisbelow(True)
    return save(figure, "efeito_por_equipes")


def main() -> None:
    configure_style()
    produced, skipped = [], []

    main_dir = TABLES / "main_experiment"
    if (main_dir / "summary.csv").exists():
        produced.append(paired_effect_figure(pd.read_csv(main_dir / "summary.csv")))
        produced.append(exposure_funnel_figure(
            pd.read_csv(main_dir / "exposure_summary.csv"),
            pd.read_csv(main_dir / "ba_exposure.csv")))
    else:
        skipped.append("experimento principal (rode `make experiment`)")

    robustness = TABLES / "robustness/summary.csv"
    if robustness.exists():
        produced.append(robustness_figure(pd.read_csv(robustness)))
    else:
        skipped.append("robustez (rode `make robustness-experiment`)")

    for path in produced:
        print(f"gerado: {path.relative_to(REPOSITORY_ROOT)}")
    for item in skipped:
        print(f"ignorado, sem dados: {item}")
    if not produced:
        raise SystemExit("nenhuma tabela disponível para gerar figuras")


if __name__ == "__main__":
    main()
