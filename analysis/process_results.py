"""Pós-processamento dos resultados da simulação ECHOSAR-Net.

Uso:
    python3 analysis/process_results.py

Calcula as 5 métricas por seed (run) e agrega por configuração como
média ± desvio-padrão entre seeds — não soma todos os runs num único pool.

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
    """Lê um arquivo .sca e devolve DataFrame [config, run, module, name, value].

    `run` é extraído do nome do arquivo (ex.: BasicTest-3.sca → run="BasicTest-3"),
    não do conteúdo — garante uma chave estável mesmo que o atributo `run` interno
    do .sca mude de formato.
    """
    config = "unknown"
    run = os.path.splitext(os.path.basename(path))[0]
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
                    'run':    run,
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


# ── Cálculo das 5 métricas por seed (run) ────────────────────────────────────
#
# alertsReceived (equipe recebeu VictimAlert) e alertsAcked (drone origem
# recebeu VictimAck) são conceitos distintos: o primeiro mede entrega até a
# equipe, o segundo mede o ciclo completo confirmado fim a fim. O PDR e o
# overhead usam alertsAcked como critério de sucesso para manter os dois
# coerentes entre si.

def compute_run_metrics(df):
    rows = []
    for (config, run), g in df.groupby(['config', 'run']):
        def s(name):
            return g[g['name'] == name]['value'].sum()

        def avg(name):
            vals = g[(g['name'] == name) & (g['value'] >= 0)]['value']
            return vals.mean() if len(vals) else float('nan')

        generated = s('alertsGenerated')
        acked     = s('alertsAcked')
        expired   = s('alertsExpired')
        retries   = s('totalRetries')
        # alertsSentDirect/alertsSentRelay já contam toda transmissão de
        # VictimAlert (originada OU relayed) — forwardAlertOnce() incrementa
        # um dos dois em toda chamada, seja por detectVictim() ou por
        # handleVictimAlertRelay(). Não somar alertsRelayed aqui (duplicaria).
        sent      = s('alertsSentDirect') + s('alertsSentRelay')
        received  = s('alertsReceived')
        # Atraso de entrega (1 via): soma bruta no lado da equipe / nº recebidos.
        # Média global PONDERADA do run (não média de médias por módulo).
        delivDelay = s('totalDeliveryDelay')
        # Alertas que chegaram a equipe DISPONÍVEL vs OCUPADA
        availRecv = s('alertsReceivedAvailable')
        busyRecv  = s('alertsReceivedBusy')

        rows.append({
            'config': config,
            'run':    run,
            # m1: PDR canônico — alertas chegaram ao destino (equipe/embarcação recebeu)
            # Alinhado à definição da dissertação: "recebidas no destino / enviadas"
            'm1_pdr':       (received / generated * 100) if generated else 0.0,
            # m2: atraso de entrega fim-a-fim de 1 via (drone → equipe), ponderado
            'm2_e2e':       (delivDelay / received) if received else float('nan'),
            'm3_retries':   (retries / acked) if acked else 0.0,
            'm4_overhead':  (sent / acked) if acked else 0.0,
            # m5: taxa de sucesso do ciclo completo (drone recebeu VictimAck de volta)
            # Era a definição anterior de PDR; renomeado para distinguir dos dois sentidos
            'm5_appack':    (acked / generated * 100) if generated else 0.0,
            # m6: dos alertas entregues, fração que encontrou equipe DISPONÍVEL
            'm6_availrate': (availRecv / received * 100) if received else 0.0,
            'alertsGenerated': generated,
            'alertsAcked':     acked,
            'alertsExpired':   expired,
            'totalRetries':    retries,
            'alertsSent':      sent,
            'alertsReceived':  received,
            'availRecv':       availRecv,
            'busyRecv':        busyRecv,
        })
    return pd.DataFrame(rows)


# ── Agregação entre seeds: média ± desvio-padrão (não soma tudo num pool) ───

METRIC_COLS = ['m1_pdr', 'm2_e2e', 'm3_retries', 'm4_overhead', 'm5_appack', 'm6_availrate']
COUNT_COLS  = ['alertsGenerated', 'alertsAcked', 'alertsExpired',
               'totalRetries', 'alertsSent', 'alertsReceived',
               'availRecv', 'busyRecv']


def aggregate_metrics(run_df):
    agg = run_df.groupby('config')[METRIC_COLS + COUNT_COLS].agg(['mean', 'std'])
    agg.columns = ['_'.join(c) for c in agg.columns]
    agg['n_runs'] = run_df.groupby('config')['run'].nunique()
    # std é NaN com n=1 run — trata como 0 para não quebrar barras de erro
    agg = agg.fillna(0.0)
    return agg


# ── Gráficos ──────────────────────────────────────────────────────────────────

METRICS = [
    ('m1_pdr',      'PDR\n(alertas recebidos / gerados)',     '%',              True),
    ('m2_e2e',      'Atraso de Entrega\n1 via (drone→equipe)','s',              False),
    ('m3_retries',  'Retransmissões\npor Alerta Confirmado',  'tentativas',     False),
    ('m4_overhead', 'Overhead de\nAlerta (msgs/confirmado)',  'msgs / confirm.', False),
    ('m5_appack',   'AppACK\n(ciclo completo confirmado)',    '%',              True),
    ('m6_availrate','Alertas a Equipe\nDISPONÍVEL (/ recebidos)','%',            True),
]


def plot_metrics(agg_df):
    configs = agg_df.index.tolist()
    colors  = [PALETTE[i % len(PALETTE)] for i in range(len(configs))]

    fig, axes = plt.subplots(2, 3, figsize=(16, 9))
    axes_flat = axes.flatten()

    for ax_idx, (col, title, ylabel, is_pct) in enumerate(METRICS):
        ax = axes_flat[ax_idx]
        means = [agg_df.loc[c, f'{col}_mean'] for c in configs]
        stds  = [agg_df.loc[c, f'{col}_std']  for c in configs]

        bars = ax.bar(configs, means, yerr=stds, capsize=4,
                       color=colors, edgecolor='black', linewidth=0.6, width=0.5)
        ax.set_title(title, fontsize=11, fontweight='bold', pad=8)
        ax.set_ylabel(ylabel, fontsize=9)
        ax.tick_params(axis='x', rotation=20, labelsize=8)

        if is_pct:
            ax.set_ylim(0, 110)
            ax.yaxis.set_major_formatter(mticker.PercentFormatter(xmax=100))

        for bar, mean, std in zip(bars, means, stds):
            if pd.notna(mean):
                label = f'{mean:.1f}±{std:.1f}%' if is_pct else f'{mean:.3g}±{std:.2g}'
                ax.text(bar.get_x() + bar.get_width() / 2,
                        bar.get_height() + std + ax.get_ylim()[1] * 0.01,
                        label, ha='center', va='bottom', fontsize=7.5, fontweight='bold')

    n_runs = agg_df['n_runs'].iloc[0] if len(agg_df) else 0
    fig.suptitle(f'ECHOSAR-Net — Métricas de Comunicação SAR (média ± desvio, n={n_runs} seeds)',
                 fontsize=13, fontweight='bold', y=1.01)
    plt.tight_layout()
    return fig


# ── Tabela resumo ─────────────────────────────────────────────────────────────

def print_summary(agg_df):
    print("\n╔══ Métricas por configuração (média ± desvio entre seeds) ══════════╗")
    for config in agg_df.index:
        row = agg_df.loc[config]
        n = int(row['n_runs'])
        print(f"\n  [{config}]  n={n} seeds")
        print(f"    PDR (%)                {row['m1_pdr_mean']:.3f} ± {row['m1_pdr_std']:.3f}")
        print(f"    Atraso entrega 1via(s) {row['m2_e2e_mean']:.3f} ± {row['m2_e2e_std']:.3f}")
        print(f"    Retries/confirmado     {row['m3_retries_mean']:.3f} ± {row['m3_retries_std']:.3f}")
        print(f"    Overhead               {row['m4_overhead_mean']:.3f} ± {row['m4_overhead_std']:.3f}")
        print(f"    AppACK (%)             {row['m5_appack_mean']:.3f} ± {row['m5_appack_std']:.3f}")
        print(f"    Alerta→disponível (%)  {row['m6_availrate_mean']:.3f} ± {row['m6_availrate_std']:.3f}")
        print(f"    Gerados (total)    {row['alertsGenerated_mean']:.1f} ± {row['alertsGenerated_std']:.1f}")
        print(f"    Confirmados        {row['alertsAcked_mean']:.1f} ± {row['alertsAcked_std']:.1f}")
        print(f"    Expirados          {row['alertsExpired_mean']:.1f} ± {row['alertsExpired_std']:.1f}")
        print(f"    →equipe disponível {row['availRecv_mean']:.1f} ± {row['availRecv_std']:.1f}  "
              f"→ocupada {row['busyRecv_mean']:.1f} ± {row['busyRecv_std']:.1f}")
    print("\n╚═══════════════════════════════════════════════════════════════════╝\n")


# ── Main ──────────────────────────────────────────────────────────────────────

def main():
    print(f"Lendo resultados de: {os.path.abspath(RESULTS_DIR)}")
    try:
        df = load_all_scalars()
    except FileNotFoundError as e:
        print(f"Erro: {e}\nExecute as simulações antes de gerar os gráficos.")
        sys.exit(1)

    n_runs = df.groupby('config')['run'].nunique()
    print(f"  {len(df)} escalares carregados de {df['config'].nunique()} config(s): "
          f"{', '.join(df['config'].unique())}  |  seeds por config: {dict(n_runs)}")

    run_df = compute_run_metrics(df)
    agg_df = aggregate_metrics(run_df)
    print_summary(agg_df)

    fig = plot_metrics(agg_df)

    for ext in ('pdf', 'png'):
        out = os.path.join(FIGURES_DIR, f'metrics.{ext}')
        fig.savefig(out, bbox_inches='tight', dpi=150)
        print(f"Figura salva: {out}")

    plt.close(fig)


if __name__ == '__main__':
    main()
