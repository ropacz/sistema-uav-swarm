#!/usr/bin/env python3
"""Estatísticas de tráfego e do mecanismo BA para complementar o texto do artigo.

Não recalcula nada a partir dos pacotes: soma os escalares que
`ExperimentMetrics` já grava por execução (ver `src/metrics/ExperimentMetrics.cc`,
`recordScalar`) e agrega por cenário e braço (BA ligado/desligado), somando
todos os valores de `numTeams` — é a mesma pergunta de `alert_sheet.py`
("quantas mensagens, qual taxa"), só que nos escalares de mecanismo/roteamento
em vez dos alertas individuais.
"""

from __future__ import annotations

import re
from pathlib import Path

import pandas as pd

from analysis.core.process_results import parse_sca
from analysis.reports.alert_sheet import ARM_PATTERN, EXCLUDED_SUFFIX

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
RESULTS = REPOSITORY_ROOT / "simulations/results/omnetpp"
OUTPUT = REPOSITORY_ROOT / "analysis/tables"

# Nomes gravados por ExperimentMetrics::finish() (ExperimentMetrics.cc).
SCALARS = [
    "alertsGenerated", "alertsDelivered", "alertsConfirmed", "alertsExpired",
    "alertAttemptsSent", "attemptsReceived", "applicationRetries",
    "confirmationDelaySum", "confirmationDelayCount",
    "hopCountSum", "hopCountCount",
    "multiHopDeliveries", "intermediateForwardings",
    "neverKnownTeamSelectionEvents", "expiredKnownTeamSelectionEvents",
    "alertsWithoutKnownTeam",
    "repositionTriggers", "sensorEvaluations", "obstaclesDetected",
    "baActivations", "repositionsStarted", "repositionsCompleted",
    "repositionDistanceSum", "repositionDurationSum", "effectiveRepositions",
    "recoveryProbeChecks", "recoveryProbesSent", "recoveryProbesConfirmed",
    "recoveryProbesFailed", "recoveryProbesUnreachable",
    "recoveryProbesAbandoned",
]


def load_totals() -> pd.DataFrame:
    """Um DataFrame longo (scenario, arm, name, value) com um escalar por linha."""
    frames = []
    for path in sorted(RESULTS.glob("*.sca")):
        if EXCLUDED_SUFFIX in path.name:
            continue
        attrs, scalars, _ = parse_sca(str(path))
        if scalars.empty:
            continue
        match = re.match(ARM_PATTERN, attrs.get("configname", "?"))
        if not match:
            continue
        scalars = scalars.copy()
        scalars["scenario"] = match.group("scenario")
        scalars["arm"] = match.group("arm")
        frames.append(scalars)
    if not frames:
        raise SystemExit(
            f"nenhum .sca em {RESULTS}.\n"
            "Rode a campanha (make experiment / make robustness-experiment) antes.")
    return pd.concat(frames, ignore_index=True)


def summarize(all_scalars: pd.DataFrame) -> pd.DataFrame:
    """Uma linha por (cenário, braço), com totais e taxas derivadas."""
    subset = all_scalars[all_scalars["name"].isin(SCALARS)]
    totals = (subset.groupby(["scenario", "arm", "name"])["value"]
              .sum().unstack("name").reindex(columns=SCALARS))

    completed = totals["repositionsCompleted"].replace(0, pd.NA)
    table = pd.DataFrame({
        "alertasGerados": totals["alertsGenerated"],
        "alertasEntregues": totals["alertsDelivered"],
        "alertasConfirmados": totals["alertsConfirmed"],
        "alertasExpirados": totals["alertsExpired"],
        "tentativasEnviadas": totals["alertAttemptsSent"],
        "retriesAplicacao": totals["applicationRetries"],
        "hopCountMedio": totals["hopCountSum"] / totals["hopCountCount"],
        "entregaMultihopPct":
            100.0 * totals["multiHopDeliveries"] / totals["alertsDelivered"],
        "encaminhamentosIntermediarios": totals["intermediateForwardings"],
        "atrasoConfirmacaoMedioS":
            totals["confirmationDelaySum"] / totals["confirmationDelayCount"],
        "alertasSemEquipeConhecida": totals["alertsWithoutKnownTeam"],
        "selecaoComEquipeExpirada": totals["expiredKnownTeamSelectionEvents"],
        "obstaculosDetectados": totals["obstaclesDetected"],
        "baAtivacoes": totals["baActivations"],
        "reposicionamentosIniciados": totals["repositionsStarted"],
        "reposicionamentosCompletados": totals["repositionsCompleted"],
        "reposicionamentoDistanciaMediaM":
            totals["repositionDistanceSum"] / completed,
        "reposicionamentoDuracaoMediaS":
            totals["repositionDurationSum"] / completed,
    })
    return table.reset_index().sort_values(["scenario", "arm"])


def main() -> None:
    table = summarize(load_totals())
    OUTPUT.mkdir(parents=True, exist_ok=True)
    path = OUTPUT / "mecanismo_resumo.csv"
    table.to_csv(path, index=False)
    print(f"gerado: {path.relative_to(REPOSITORY_ROOT)}")
    print()
    print(table.to_string(index=False, float_format=lambda value: f"{value:.2f}"))


if __name__ == "__main__":
    main()
