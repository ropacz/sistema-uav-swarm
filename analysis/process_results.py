"""Pós-processamento dos resultados da simulação ECHOSAR-Net.

Uso:
    python3 analysis/process_results.py

Saída: analysis/figures/metrics.pdf  +  analysis/figures/metrics.png
"""

import re
import sys
import os
import glob

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker

RESULTS_DIR = os.path.join(os.path.dirname(__file__), '..', 'simulations', 'results')
FIGURES_DIR = os.path.join(os.path.dirname(__file__), 'figures')
os.makedirs(FIGURES_DIR, exist_ok=True)

PALETTE = plt.cm.tab10.colors


# ── Parser de .sca ────────────────────────────────────────────────────────────

def parse_sca(path):
    """Lê um arquivo .sca e devolve DataFrame [config, module, name, value]."""
    config = "unknown"
    rows = []
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = re.match(r'attr configname "?([^"\n]+)"?', line)
            if m:
                config = m.group(1).strip()
                continue
            m = re.match(r'scalar (\S+) "?([^"]+)"? ([\d.eE+\-nan]+)', line)
            if m:
                try:
                    value = float(m.group(3))
                except ValueError:
                    continue
                rows.append({
                    'config': config,
                    'module': m.group(1),
                    'name':   m.group(2),
                    'value':  value,
                })
    return pd.DataFrame(rows)


def load_all_scalars():
    """Lê todos os .sca em RESULTS_DIR e concatena."""
    files = glob.glob(os.path.join(RESULTS_DIR, '*.sca'))
    if not files:
        raise FileNotFoundError(f"Nenhum arquivo .sca encontrado em {RESULTS_DIR}")
    frames = [parse_sca(f) for f in files]
    df = pd.concat(frames, ignore_index=True)
    # remove valores inválidos (-1 = sem dados)
    df = df[df['value'] >= 0]
    return df


# ── Cálculo das 5 métricas por config ────────────────────────────────────────

def compute_metrics(df):
    results = {}
    for config, g in df.groupby('config'):
        def s(name):
            return g[g['name'] == name]['value'].sum()

        def avg(name):
            vals = g[(g['name'] == name) & (g['value'] >= 0)]['value']
            return vals.mean() if len(vals) else float('nan')

        generated = s('alertsGenerated')
        acked     = s('alertsAcked')
        expired   = s('alertsExpired')
        retries   = s('totalRetries')
        sent      = s('alertsSentDirect') + s('alertsSentRelay')
        received  = s('alertsReceived')

        results[config] = {
            'm1_pdr':      (acked / generated * 100) if generated else 0.0,
            'm2_e2e':      avg('meanE2EDelay'),
            'm3_retries':  (retries / acked) if acked else 0.0,
            'm4_overhead': (sent / received) if received else 0.0,
            'm5_ack_rate': (acked / (acked + expired) * 100) if (acked + expired) else 0.0,
            # contadores brutos (para tabela)
            'alertsGenerated': generated,
            'alertsAcked':     acked,
            'alertsExpired':   expired,
            'totalRetries':    retries,
            'alertsSent':      sent,
            'alertsReceived':  received,
        }
    return pd.DataFrame(results).T


# ── Gráficos ──────────────────────────────────────────────────────────────────

METRICS = [
    ('m1_pdr',      'Taxa de Entrega\n(PDR)',               '%',              True),
    ('m2_e2e',      'Atraso Fim a Fim\nMédio',              's',              False),
    ('m3_retries',  'Retransmissões\npor Alerta Entregue',  'tentativas',     False),
    ('m4_overhead', 'Overhead de\nComunicação',             'msgs / alerta',  False),
    ('m5_ack_rate', 'Taxa de Sucesso\npor ACK de Aplicação','%',              True),
]


def plot_metrics(metrics_df):
    configs = metrics_df.index.tolist()
    colors  = [PALETTE[i % len(PALETTE)] for i in range(len(configs))]

    fig, axes = plt.subplots(2, 3, figsize=(16, 9))
    axes_flat = axes.flatten()

    for ax_idx, (col, title, ylabel, is_pct) in enumerate(METRICS):
        ax = axes_flat[ax_idx]
        vals = [metrics_df.loc[c, col] for c in configs]

        bars = ax.bar(configs, vals, color=colors, edgecolor='black', linewidth=0.6, width=0.5)
        ax.set_title(title, fontsize=11, fontweight='bold', pad=8)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.tick_params(axis='x', rotation=20, labelsize=8)

        if is_pct:
            ax.set_ylim(0, 110)
            ax.yaxis.set_major_formatter(mticker.PercentFormatter(xmax=100))

        for bar, val in zip(bars, vals):
            if pd.notna(val):
                label = f'{val:.1f}%' if is_pct else f'{val:.3g}'
                ax.text(bar.get_x() + bar.get_width() / 2,
                        bar.get_height() + ax.get_ylim()[1] * 0.01,
                        label, ha='center', va='bottom', fontsize=8, fontweight='bold')

    # remove 6º subplot vazio
    axes_flat[5].set_visible(False)

    fig.suptitle('ECHOSAR-Net — Métricas de Comunicação SAR', fontsize=13, fontweight='bold', y=1.01)
    plt.tight_layout()
    return fig


# ── Tabela resumo ─────────────────────────────────────────────────────────────

def print_summary(metrics_df):
    print("\n╔══ Métricas por configuração ══════════════════════════════════════╗")
    display = metrics_df[[
        'm1_pdr', 'm2_e2e', 'm3_retries', 'm4_overhead', 'm5_ack_rate',
        'alertsGenerated', 'alertsAcked', 'alertsExpired', 'totalRetries',
    ]].rename(columns={
        'm1_pdr':           'PDR (%)',
        'm2_e2e':           'Atraso E2E (s)',
        'm3_retries':       'Retries/entrega',
        'm4_overhead':      'Overhead',
        'm5_ack_rate':      'Taxa ACK (%)',
        'alertsGenerated':  'Gerados',
        'alertsAcked':      'Confirmados',
        'alertsExpired':    'Expirados',
        'totalRetries':     'Total retries',
    })
    pd.set_option('display.float_format', '{:.3f}'.format)
    print(display.to_string())
    print("╚═══════════════════════════════════════════════════════════════════╝\n")


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    print(f"Lendo resultados de: {os.path.abspath(RESULTS_DIR)}")
    try:
        df = load_all_scalars()
    except FileNotFoundError as e:
        print(f"Erro: {e}\nExecute as simulações antes de gerar os gráficos.")
        sys.exit(1)

    print(f"  {len(df)} escalares carregados de {df['config'].nunique()} config(s): "
          f"{', '.join(df['config'].unique())}")

    metrics_df = compute_metrics(df)
    print_summary(metrics_df)

    fig = plot_metrics(metrics_df)

    for ext in ('pdf', 'png'):
        out = os.path.join(FIGURES_DIR, f'metrics.{ext}')
        fig.savefig(out, bbox_inches='tight', dpi=150)
        print(f"Figura salva: {out}")

    plt.close(fig)


if __name__ == '__main__':
    main()
