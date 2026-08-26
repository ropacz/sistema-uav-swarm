#!/usr/bin/env python3
"""Segunda via de atendimento/perda: fonte é o CSV do `opp_scavetool`, não os
arquivos `.sca`/`-alerts.csv` que `alert_sheet.py` lê diretamente.

`opp_scavetool` é a ferramenta padrão do OMNeT++ para exportar resultados —
achata todos os `.sca` de uma campanha num único CSV em formato longo
(`run, type, module, name, ..., value`). Aqui a fonte não é o pacote-a-pacote
(como no `analysis/audit/`, que reconstrói do `.elog`), é o mesmo agregado por
execução que a planilha oficial usa — só lido por um caminho de leitura
diferente e com uma ferramenta de terceiros, o que serve como confirmação
independente de que a leitura de `parse_sca()` não introduz erro.

Uso:
    opp_scavetool x simulations/results/omnetpp/MainExperiment_*.sca \
        -o simulations/results/campanha_scavetool.csv
    python3 analysis/reports/scavetool_figures.py \
        simulations/results/campanha_scavetool.csv
"""

from __future__ import annotations

import csv
import re
import sys
from collections import defaultdict
from pathlib import Path

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

import pandas as pd  # noqa: E402

from analysis.reports import figures  # noqa: E402

OUTPUT = REPOSITORY_ROOT / "analysis/tables"
FIGURES_OUTPUT = REPOSITORY_ROOT / "analysis/figures"

# Escalares que ExperimentMetrics.cc grava e que bastam para atendimento/perda
# agregados por braço — os mesmos nomes que analysis/core/experiment_metrics.py
# usa, aqui lidos do formato longo do scavetool em vez de parse_sca().
REQUIRED_SCALARS = {"alertsGenerated", "alertsDelivered", "alertsConfirmed"}


def load_runs(csv_path: Path) -> dict[str, dict]:
    """Uma entrada por execução: baEnabled, numTeams e os escalares centrais."""
    runs: dict[str, dict] = defaultdict(dict)
    with open(csv_path, newline="", encoding="utf-8", errors="replace") as source:
        for row in csv.DictReader(source):
            run = row["run"]
            if row["type"] == "param" and row["name"] == "baEnabled" \
                    and re.search(r"\.app\[\d+\]$", row["module"]):
                runs[run].setdefault("baEnabled", set()).add(row["value"])
            elif row["type"] == "param" and row["name"] == "numTeams":
                runs[run]["numTeams"] = row["value"]
            elif row["type"] == "scalar" and row["module"].endswith(".experimentMetrics") \
                    and row["name"] in REQUIRED_SCALARS:
                runs[run][row["name"]] = float(row["value"])
            elif row["type"] == "runattr" and row["attrname"] == "configname":
                runs[run]["config"] = row["attrvalue"]
    return runs


def summarize(runs: dict[str, dict]) -> pd.DataFrame:
    rows = []
    for run, values in runs.items():
        missing = REQUIRED_SCALARS - values.keys()
        if missing:
            raise SystemExit(f"{run}: escalares ausentes no CSV do scavetool: {missing}")
        flags = values.get("baEnabled", set())
        if len(flags) != 1:
            raise SystemExit(f"{run}: baEnabled inconsistente entre drones: {flags}")
        rows.append({
            "run": run,
            "config": values.get("config", "?"),
            "numTeams": int(float(values.get("numTeams", -1))),
            "baEnabled": flags.pop() == "true",
            "alertsGenerated": values["alertsGenerated"],
            "alertsDelivered": values["alertsDelivered"],
            "alertsConfirmed": values["alertsConfirmed"],
        })
    table = pd.DataFrame(rows)

    summary = table.groupby(["config", "numTeams", "baEnabled"], as_index=False).agg(
        execucoes=("run", "count"),
        alertas_gerados=("alertsGenerated", "sum"),
        alertas_entregues=("alertsDelivered", "sum"),
        alertas_confirmados=("alertsConfirmed", "sum"),
    )
    summary["alertas_sem_entrega"] = summary["alertas_gerados"] - summary["alertas_entregues"]
    summary["atendimento_pct"] = 100.0 * summary["alertas_confirmados"] / summary["alertas_gerados"]
    summary["perda_pct"] = 100.0 * summary["alertas_sem_entrega"] / summary["alertas_gerados"]
    return summary


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit(
            "uso: python3 analysis/reports/scavetool_figures.py <csv exportado pelo scavetool>")
    csv_path = Path(sys.argv[1])
    if not csv_path.exists():
        raise SystemExit(f"arquivo não encontrado: {csv_path}\n"
                         "gere com: opp_scavetool x <arquivos .sca> -o <csv>")

    runs = load_runs(csv_path)
    if not runs:
        raise SystemExit(f"nenhuma execução encontrada em {csv_path}")
    summary = summarize(runs)

    OUTPUT.mkdir(parents=True, exist_ok=True)
    table_path = OUTPUT / "atendimento_via_scavetool.csv"
    summary.to_csv(table_path, index=False)

    figures.configure_style()
    charts = [
        figures.rate_figure(summary, "atendimento_pct", "Atendimento (%)",
                            "atendimento_via_scavetool"),
        figures.rate_figure(summary, "perda_pct", "Perda (%)",
                            "perda_via_scavetool"),
    ]

    print(f"gerado: {table_path.relative_to(REPOSITORY_ROOT)}")
    for chart in charts:
        print(f"gerado: {chart.relative_to(REPOSITORY_ROOT)}")
    print()
    print(summary.to_string(index=False, float_format=lambda v: f"{v:.1f}"))


if __name__ == "__main__":
    main()
