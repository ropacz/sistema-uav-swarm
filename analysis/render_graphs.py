"""Renderiza gráficos de métricas a partir do cache JSON.

Uso:
    python3 analysis/render_graphs.py            # usa metrics_cache.json padrão
    python3 analysis/render_graphs.py --cache outro.json

Edite as seções LABELS, COLORS e CONFIGS abaixo para personalizar sem re-rodar simulações.
"""

import json
import os
import argparse

import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

# ── Configurações editáveis ───────────────────────────────────────────────────

# Rótulos dos cenários nos gráficos
CONFIG_LABELS = {
    "Cenario_SemObstaculos": "Sem Obstáculos",
    "Cenario_ComObstaculos":  "Com Obstáculos",
}

# Cores das barras (uma por cenário, na ordem de CONFIG_LABELS)
COLORS = ["#4878CF", "#D65F5F"]

# Métricas: (chave_no_json, título_do_gráfico, label_eixo_y, é_percentual)
METRICS = [
    ("m5_appack",   "Taxa de Confirmação\n(AppACK)",           "AppACK (%)",           True),
    ("m2_e2e",      "Delay\n(drone → equipe)",                 "Delay (s)",            False),
    ("m3_retries",  "Retransmissões\npor Alerta",              "Retransmissões",       False),
    ("m4_overhead", "Overhead de Alerta\n(msgs / alerta)",     "Overhead (msgs)",      False),
    ("m1_pdr",      "PDR\n(alertas recebidos / gerados)",      "PDR (%)",              True),
    ("m6_availrate","Alertas Recebidos com\nEquipe Disponível","Alertas Disponível (%)",True),
]

METRIC_SLUGS = {
    "m5_appack":   "appack",
    "m2_e2e":      "atraso",
    "m3_retries":  "retransmissoes",
    "m4_overhead": "overhead",
    "m1_pdr":      "pdr",
    "m6_availrate":"disponibilidade",
}

FIGURES_DIR = os.path.join(os.path.dirname(__file__), "figures")
CACHE_DEFAULT = os.path.join(os.path.dirname(__file__), "metrics_cache.json")

# ── Renderização ──────────────────────────────────────────────────────────────

def plot_metric(data, configs, col, title, ylabel, is_pct, colors):
    x_labels = [f"{CONFIG_LABELS.get(c, c)}\n(n={data[c]['n_runs']})" for c in configs]
    means = [data[c][f"{col}_mean"] for c in configs]
    stds  = [data[c][f"{col}_std"]  for c in configs]

    fig, ax = plt.subplots(figsize=(7, 5))
    bars = ax.bar(x_labels, means, yerr=stds, capsize=5,
                  color=colors[:len(configs)], edgecolor="black",
                  linewidth=0.7, width=0.5)

    ax.set_title(title, fontsize=12, fontweight="bold", pad=10)
    ax.set_ylabel(ylabel, fontsize=11, labelpad=8)
    ax.tick_params(axis="x", rotation=0, labelsize=10)
    ax.tick_params(axis="y", labelsize=9)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)

    if is_pct:
        ax.set_ylim(0, 100)
        ax.yaxis.set_major_formatter(mticker.PercentFormatter(xmax=100))

    for bar, mean, std in zip(bars, means, stds):
        label = f"{mean:.1f}±{std:.1f}%" if is_pct else f"{mean:.3g}±{std:.2g}"
        ax.text(bar.get_x() + bar.get_width() / 2,
                bar.get_height() + std + ax.get_ylim()[1] * 0.01,
                label, ha="center", va="bottom", fontsize=9, fontweight="bold")

    plt.tight_layout()
    return fig


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--cache", default=CACHE_DEFAULT)
    args = parser.parse_args()

    with open(args.cache) as f:
        data = json.load(f)

    configs = list(data.keys())
    os.makedirs(FIGURES_DIR, exist_ok=True)

    for col, title, ylabel, is_pct in METRICS:
        fig = plot_metric(data, configs, col, title, ylabel, is_pct, COLORS)
        slug = METRIC_SLUGS[col]
        for ext in ("pdf", "png"):
            out = os.path.join(FIGURES_DIR, f"metric_{slug}.{ext}")
            fig.savefig(out, bbox_inches="tight", dpi=150)
            print(f"Salvo: {out}")
        plt.close(fig)


if __name__ == "__main__":
    main()
