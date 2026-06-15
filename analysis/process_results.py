"""Pós-processamento dos resultados da simulação ECHOSAR-Net.

Uso:
    python3 process_results.py

Saída: analysis/figures/comparison.pdf
"""

import sys
import os
import pandas as pd
import matplotlib.pyplot as plt

try:
    from omnetpp.scave import results
    USE_SCAVE = True
except ImportError:
    USE_SCAVE = False
    print("[AVISO] omnetpp.scave não disponível. Usando modo de leitura CSV de fallback.")


RESULTS_DIR = os.path.join(os.path.dirname(__file__), '..', 'simulations', 'results')
FIGURES_DIR = os.path.join(os.path.dirname(__file__), 'figures')
os.makedirs(FIGURES_DIR, exist_ok=True)

CONFIGS     = ['Baseline_FixedPower', 'Baseline_NoCooperation', 'ECHOSAR']
LABELS      = ['Potência Fixa',       'Sem Cooperação',          'ECHOSAR-Net']
COLORS      = ['#999999',             '#bbbbbb',                  '#3a7fc1']


def load_scalars():
    if USE_SCAVE:
        pattern = os.path.join(RESULTS_DIR, '*.sca')
        return results.read_scalars(pattern)
    # Fallback: espera CSVs exportados manualmente pelo IDE OMNeT++
    csvs = [f for f in os.listdir(RESULTS_DIR) if f.endswith('.csv')]
    if not csvs:
        raise FileNotFoundError(f"Nenhum arquivo .sca ou .csv em {RESULTS_DIR}")
    frames = [pd.read_csv(os.path.join(RESULTS_DIR, f)) for f in csvs]
    return pd.concat(frames, ignore_index=True)


def compute_pdr(df):
    """Taxa de entrega de alertas às equipes (%)."""
    sent = (df[df['name'] == 'alertSent:sum']
              .groupby('configname')['value'].sum())
    recv = (df[df['name'] == 'alertReceived:sum']
              .groupby('configname')['value'].sum())
    return (recv / sent * 100).rename('PDR (%)')


def compute_latency(df):
    """Latência média ± desvio-padrão de entrega às equipes (s)."""
    lat = df[df['name'] == 'alertDeliveryDelay:mean']
    return lat.groupby('configname')['value'].agg(['mean', 'std'])


def compute_energy(df):
    """Energia residual média ± desvio-padrão ao fim da simulação (J)."""
    energy = df[df['name'] == 'residualEnergyCapacity:last']
    return energy.groupby('configname')['value'].agg(['mean', 'std'])


def plot_bar(ax, series, yerr=None, title='', ylabel=''):
    vals   = [series.get(c, float('nan')) for c in CONFIGS]
    errors = [yerr.get(c, 0) if yerr is not None else 0 for c in CONFIGS] if yerr is not None else None
    bars = ax.bar(LABELS, vals, yerr=errors, color=COLORS, capsize=5, edgecolor='black', linewidth=0.5)
    ax.set_title(title, fontsize=11, fontweight='bold')
    ax.set_ylabel(ylabel)
    ax.tick_params(axis='x', rotation=15)
    for bar, val in zip(bars, vals):
        if not pd.isna(val):
            ax.text(bar.get_x() + bar.get_width() / 2, bar.get_height() * 1.01,
                    f'{val:.1f}', ha='center', va='bottom', fontsize=8)


def main():
    print(f"Lendo resultados de: {RESULTS_DIR}")
    try:
        df = load_scalars()
    except FileNotFoundError as e:
        print(f"Erro: {e}")
        print("Execute as simulações antes de gerar os gráficos.")
        sys.exit(1)

    pdr    = compute_pdr(df)
    lat    = compute_latency(df)
    energy = compute_energy(df)

    fig, axes = plt.subplots(1, 3, figsize=(14, 5))

    plot_bar(axes[0], pdr,
             title='Taxa de Entrega (PDR)',
             ylabel='%')

    plot_bar(axes[1],
             lat['mean'],
             yerr=lat['std'],
             title='Latência Média de Entrega',
             ylabel='s')

    plot_bar(axes[2],
             energy['mean'],
             yerr=energy['std'],
             title='Energia Residual Média',
             ylabel='J')

    plt.tight_layout()
    out = os.path.join(FIGURES_DIR, 'comparison.pdf')
    plt.savefig(out, bbox_inches='tight')
    print(f"Figura salva em: {out}")

    # Tabela resumo no terminal
    print("\n=== Resumo dos Resultados ===")
    summary = pd.DataFrame({
        'PDR (%)':           pdr,
        'Latência média (s)': lat['mean'],
        'Latência std (s)':   lat['std'],
        'Energia média (J)':  energy['mean'],
        'Energia std (J)':    energy['std'],
    })
    print(summary.to_string())


if __name__ == '__main__':
    main()
