#!/usr/bin/env python3
"""Planilha de atendimento: uma linha por alerta, duas taxas por execução.

A simulação grava um CSV por execução com uma linha por `alertId`. Este script
apenas junta esses arquivos com o contexto da execução lido do `.sca`
correspondente (seed, quantidade de equipes, política ligada ou desligada).

A deduplicação é responsabilidade do coletor dentro da simulação: um mesmo
`alertId` recebido por várias equipes ou retransmitido várias vezes já chega
aqui como uma única linha.

    Atendimento(%) = alertId com pelo menos um ACK / alertId únicos gerados
    Perda(%)       = alertId sem entrega a nenhuma equipe / alertId únicos gerados
"""

from __future__ import annotations

from pathlib import Path
import re
import sys

import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.process_results import parse_sca  # noqa: E402
from analysis.reports import figures  # noqa: E402

RESULTS = REPOSITORY_ROOT / "simulations/results/omnetpp"
OUTPUT = REPOSITORY_ROOT / "analysis/tables"

COLUMNS = [
    "seed", "numTeams", "baEnabled", "alertId", "victimId", "droneId",
    "generationTime", "delivered", "receivingTeamId", "acknowledged",
    "ackTeamId", "retryCount",
]


def run_context(alerts_path: Path) -> dict:
    """Lê seed, equipes e política do .sca da mesma execução."""
    scalar_path = alerts_path.with_name(alerts_path.name.replace("-alerts.csv", ".sca"))
    if not scalar_path.exists():
        raise SystemExit(f"{scalar_path.name} ausente para {alerts_path.name}")
    attributes, _, parameters = parse_sca(str(scalar_path))

    ba_values = {value.strip().strip('"').lower()
                 for key, value in parameters.items() if key.endswith(" baEnabled")}
    if len(ba_values) != 1:
        raise SystemExit(f"{scalar_path.name}: baEnabled inconsistente {ba_values}")

    teams = next((value for key, value in parameters.items()
                  if key.endswith("BasicNetwork numTeams")), None)
    return {
        "config": attributes.get("configname", "?"),
        "seed": int(float(attributes.get("seedset", attributes.get("repetition", 0)))),
        "numTeams": int(float(teams)) if teams is not None else -1,
        "baEnabled": ba_values.pop() == "true",
    }


def load() -> pd.DataFrame:
    paths = sorted(RESULTS.glob("*-alerts.csv"))
    if not paths:
        raise SystemExit(
            f"nenhum registro de alertas em {RESULTS}.\n"
            "Reconstrua a simulação e execute um cenário antes de gerar a planilha.")
    frames = []
    for path in paths:
        alerts = pd.read_csv(path, dtype={"receivingTeamId": str, "ackTeamId": str})
        context = run_context(path)
        for key, value in context.items():
            alerts[key] = value
        frames.append(alerts)
    table = pd.concat(frames, ignore_index=True)
    return table.fillna({"receivingTeamId": "", "ackTeamId": ""})


def summarize(table: pd.DataFrame) -> pd.DataFrame:
    """Uma linha por célula experimental, com as duas taxas pedidas."""
    rows = []
    for (config, teams, enabled), group in table.groupby(
            ["config", "numTeams", "baEnabled"], sort=True):
        generated = len(group)
        acknowledged = int(group["acknowledged"].sum())
        undelivered = int((group["delivered"] == 0).sum())
        rows.append({
            "config": config,
            "numTeams": teams,
            "baEnabled": enabled,
            "execucoes": group["seed"].nunique(),
            "alertas_gerados": generated,
            "alertas_entregues": int(group["delivered"].sum()),
            "alertas_confirmados": acknowledged,
            "alertas_sem_entrega": undelivered,
            "atendimento_pct": 100.0 * acknowledged / generated if generated else float("nan"),
            "perda_pct": 100.0 * undelivered / generated if generated else float("nan"),
        })
    return pd.DataFrame(rows)


def main() -> None:
    table = load()
    # Um alertId é único dentro da execução; a chave global inclui o contexto.
    duplicated = table.duplicated(subset=["config", "seed", "alertId"]).sum()
    if duplicated:
        raise SystemExit(f"{duplicated} alertId repetidos na mesma execução")

    sheet = table[COLUMNS].sort_values(
        ["baEnabled", "numTeams", "seed", "generationTime"]).reset_index(drop=True)
    summary = summarize(table)

    OUTPUT.mkdir(parents=True, exist_ok=True)
    workbook = OUTPUT / "atendimento.xlsx"
    with pd.ExcelWriter(workbook, engine="openpyxl") as writer:
        sheet.to_excel(writer, sheet_name="Alertas", index=False)
        summary.to_excel(writer, sheet_name="Resumo", index=False)
    sheet.to_csv(OUTPUT / "atendimento_alertas.csv", index=False)
    summary.to_csv(OUTPUT / "atendimento_resumo.csv", index=False)

    figures.configure_style()
    charts = figures.attendance_figures(summary)

    print(f"gerado: {workbook.relative_to(REPOSITORY_ROOT)} "
          f"({len(sheet)} alertas, {summary['execucoes'].sum()} execuções)")
    for chart in charts:
        print(f"gerado: {chart.relative_to(REPOSITORY_ROOT)}")
    print()
    print(summary.to_string(index=False, float_format=lambda value: f"{value:.1f}"))


if __name__ == "__main__":
    main()
