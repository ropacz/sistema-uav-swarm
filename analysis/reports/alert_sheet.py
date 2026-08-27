#!/usr/bin/env python3
"""Planilha de atendimento e efeito pareado: uma linha por alerta.

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
import sys

import numpy as np
import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.process_results import parse_sca  # noqa: E402
from analysis.reports import figures  # noqa: E402

RESULTS = REPOSITORY_ROOT / "simulations/results/omnetpp"
OUTPUT = REPOSITORY_ROOT / "analysis/tables"

# Smoke tests (validation/) usam baEnabled misto por desenho de teste — ex.:
# Connectivity_SmokeTest liga a política só no drone que a testa. Não são
# execuções da campanha e não têm lugar nesta planilha.
EXCLUDED_SUFFIX = "_SmokeTest"
ARM_PATTERN = r"^(?P<scenario>.+)_Ba(?P<arm>Off|On)$"
BOOTSTRAP_RESAMPLES = 10_000

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
    paths = sorted(
        path for path in RESULTS.glob("*-alerts.csv")
        if EXCLUDED_SUFFIX not in path.name
    )
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
    """Uma linha por célula experimental, com as duas taxas pedidas.

    Atendimento e perda não são complementares: `confirmado` é subconjunto de
    `entregue` (um alerta pode chegar à equipe e o ACK se perder na volta), e
    isso não é bug — as duas taxas medem coisas diferentes (confirmação
    fim-a-fim vs. entrega). A terceira categoria abaixo torna essa diferença
    explícita para quem só olhar as duas figuras lado a lado.
    """
    rows = []
    for (config, teams, enabled), group in table.groupby(
            ["config", "numTeams", "baEnabled"], sort=True):
        generated = len(group)
        delivered = int(group["delivered"].sum())
        acknowledged = int(group["acknowledged"].sum())
        undelivered = int((group["delivered"] == 0).sum())
        delivered_unacknowledged = delivered - acknowledged
        rows.append({
            "config": config,
            "numTeams": teams,
            "baEnabled": enabled,
            "execucoes": group["seed"].nunique(),
            "alertas_gerados": generated,
            "alertas_entregues": delivered,
            "alertas_confirmados": acknowledged,
            "alertas_sem_entrega": undelivered,
            "alertas_entregues_sem_confirmacao": delivered_unacknowledged,
            "atendimento_pct": 100.0 * acknowledged / generated if generated else float("nan"),
            "perda_pct": 100.0 * undelivered / generated if generated else float("nan"),
            "entregue_sem_confirmacao_pct":
                100.0 * delivered_unacknowledged / generated if generated else float("nan"),
        })
    return pd.DataFrame(rows)


def build_pairs(table: pd.DataFrame) -> pd.DataFrame:
    """Uma linha por (cenário, numTeams, seed), com o efeito BA-On − BA-Off.

    Reaproveitada por `paired_effects()` (IC bootstrap) e por
    `paired_effect_ttest.py` (IC Student-t) — a construção do par por seed é
    a mesma nos dois; só o método de intervalo de confiança muda.
    """
    arm = table["config"].str.extract(ARM_PATTERN)
    selected = table.loc[arm["scenario"].notna()].copy()
    if selected.empty:
        return pd.DataFrame()
    selected["scenario"] = arm.loc[selected.index, "scenario"]
    selected["declaredEnabled"] = arm.loc[selected.index, "arm"].eq("On")
    if not (selected["declaredEnabled"] == selected["baEnabled"]).all():
        raise ValueError("nome do braço e baEnabled são inconsistentes")

    runs = selected.groupby(
        ["scenario", "numTeams", "seed", "baEnabled"], sort=True
    ).agg(
        generated=("alertId", "size"),
        acknowledged=("acknowledged", "sum"),
        undelivered=("delivered", lambda values: int((values == 0).sum())),
    ).reset_index()
    runs["attendancePct"] = 100.0 * runs["acknowledged"] / runs["generated"]
    runs["lossPct"] = 100.0 * runs["undelivered"] / runs["generated"]

    keys = ["scenario", "numTeams", "seed"]
    off = runs.loc[~runs["baEnabled"], keys + ["attendancePct", "lossPct"]].rename(
        columns={"attendancePct": "attendanceOff", "lossPct": "lossOff"})
    on = runs.loc[runs["baEnabled"], keys + ["attendancePct", "lossPct"]].rename(
        columns={"attendancePct": "attendanceOn", "lossPct": "lossOn"})
    pairs = off.merge(on, on=keys, how="inner", validate="one_to_one")
    if len(pairs) != len(off) or len(pairs) != len(on):
        raise ValueError("a campanha contém seeds sem o respectivo braço pareado")
    pairs["attendanceEffect"] = pairs["attendanceOn"] - pairs["attendanceOff"]
    pairs["lossEffect"] = pairs["lossOn"] - pairs["lossOff"]
    return pairs


def paired_effects(table: pd.DataFrame) -> pd.DataFrame:
    """Resume diferenças BA-On − BA-Off por seed, com IC bootstrap de 95%."""
    pairs = build_pairs(table)
    if pairs.empty:
        return pd.DataFrame()

    rng = np.random.default_rng(20260826)

    def interval(values: pd.Series) -> tuple[float, float]:
        data = values.to_numpy(dtype=float)
        if len(data) == 1:
            return data[0], data[0]
        means = rng.choice(
            data, size=(BOOTSTRAP_RESAMPLES, len(data)), replace=True
        ).mean(axis=1)
        return tuple(np.quantile(means, [0.025, 0.975]))

    rows = []
    for (scenario, teams), group in pairs.groupby(
            ["scenario", "numTeams"], sort=True):
        attendance_low, attendance_high = interval(group["attendanceEffect"])
        loss_low, loss_high = interval(group["lossEffect"])
        rows.append({
            "cenario": scenario,
            "numTeams": teams,
            "pares": len(group),
            "atendimento_ba_off_pct": group["attendanceOff"].mean(),
            "atendimento_ba_on_pct": group["attendanceOn"].mean(),
            "efeito_atendimento_pp": group["attendanceEffect"].mean(),
            "efeito_atendimento_ic95_inf_pp": attendance_low,
            "efeito_atendimento_ic95_sup_pp": attendance_high,
            "perda_ba_off_pct": group["lossOff"].mean(),
            "perda_ba_on_pct": group["lossOn"].mean(),
            "efeito_perda_pp": group["lossEffect"].mean(),
            "efeito_perda_ic95_inf_pp": loss_low,
            "efeito_perda_ic95_sup_pp": loss_high,
        })
    return pd.DataFrame(rows)


def main() -> None:
    table = load()
    # Um alertId é único dentro da execução; a chave global inclui o contexto.
    # numTeams entra na chave porque os cenários de robustez reaproveitam os
    # mesmos 30 seeds em cada valor de numTeams (${teams=1,5,10,15}) — sem
    # numTeams, alertIds deterministicos (ex.: "drone2-victim0-alert-1") do
    # mesmo seed em execuções distintas seriam falsamente sinalizados como
    # duplicados.
    duplicated = table.duplicated(subset=["config", "numTeams", "seed", "alertId"]).sum()
    if duplicated:
        raise SystemExit(f"{duplicated} alertId repetidos na mesma execução")

    sheet = table[COLUMNS].sort_values(
        ["baEnabled", "numTeams", "seed", "generationTime"]).reset_index(drop=True)
    summary = summarize(table)
    paired = paired_effects(table)

    OUTPUT.mkdir(parents=True, exist_ok=True)
    workbook = OUTPUT / "atendimento.xlsx"
    with pd.ExcelWriter(workbook, engine="openpyxl") as writer:
        sheet.to_excel(writer, sheet_name="Alertas", index=False)
        summary.to_excel(writer, sheet_name="Resumo", index=False)
        if not paired.empty:
            paired.to_excel(writer, sheet_name="EfeitoPareado", index=False)
    sheet.to_csv(OUTPUT / "atendimento_alertas.csv", index=False)
    summary.to_csv(OUTPUT / "atendimento_resumo.csv", index=False)
    if not paired.empty:
        paired.to_csv(OUTPUT / "efeito_pareado.csv", index=False)

    figures.configure_style()
    # rate_figure indexa por numTeams dentro de um braço; se o resumo cobrir
    # mais de uma família de cenário (ex.: OneVictim e TwoVictims, que
    # compartilham os mesmos valores de numTeams), o índice fica duplicado e
    # o gráfico quebra. Uma figura por família evita a mistura.
    scenario = summary["config"].str.extract(ARM_PATTERN)["scenario"]
    charts = []
    for name in sorted(scenario.dropna().unique()):
        charts += figures.attendance_figures(
            summary[scenario == name], suffix=f"_{name}")
    unmatched = summary[scenario.isna()]
    if not unmatched.empty:
        charts += figures.attendance_figures(unmatched)

    print(f"gerado: {workbook.relative_to(REPOSITORY_ROOT)} "
          f"({len(sheet)} alertas, {summary['execucoes'].sum()} execuções)")
    for chart in charts:
        print(f"gerado: {chart.relative_to(REPOSITORY_ROOT)}")
    print()
    print(summary.to_string(index=False, float_format=lambda value: f"{value:.1f}"))
    if not paired.empty:
        print()
        print("Efeito pareado BA-On − BA-Off por seed (pontos percentuais):")
        print(paired.to_string(index=False, float_format=lambda value: f"{value:.2f}"))


if __name__ == "__main__":
    main()
