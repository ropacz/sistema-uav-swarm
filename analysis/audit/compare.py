#!/usr/bin/env python3
"""Compara atendimento/perda calculado por duas fontes independentes.

Fonte 1 — sinais da aplicação (.sca + "-alerts.csv" que ExperimentMetrics.cc
grava): sabe a semântica do protocolo — TTL, endereço da tentativa, validade
da equipe. É a fonte usada em `analysis/reports/alert_sheet.py`, a que alimenta
a planilha e as figuras oficiais.

Fonte 2 — event log do kernel (.elog, via `eventlog_metrics.reconstruct`): só
sabe "um pacote com este nome foi consumido por aquele módulo". Não enxerga a
decisão de aceitar ou rejeitar dentro da aplicação, e não enxerga um alerta que
o app considerou "gerado" mas para o qual nunca chegou a existir um pacote na
rede (equipe ficou sem entrada válida em toda oportunidade de envio).

Este script roda as duas fontes sobre os MESMOS arquivos de uma mesma execução
e reporta onde concordam e onde divergem — e por quê.
"""

from __future__ import annotations

import csv
from pathlib import Path
import sys

REPO = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO))

from analysis.audit.eventlog_metrics import rates, reconstruct  # noqa: E402

RAW = REPO / "analysis/audit/raw"
OUTPUT = REPO / "analysis/audit/comparison"


def load_ground_truth(path: Path) -> dict[str, dict]:
    with open(path, newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    return {row["alertId"]: row for row in rows}


def diff_run(config: str, seed: int) -> dict:
    truth = load_ground_truth(RAW / f"{config}-{seed}-alerts.csv")
    elog = reconstruct(RAW / f"{config}-{seed}.elog")

    only_in_truth = sorted(set(truth) - set(elog))
    only_in_elog = sorted(set(elog) - set(truth))
    common = sorted(set(truth) & set(elog))
    mismatched = [
        alert_id for alert_id in common
        if int(truth[alert_id]["delivered"]) != elog[alert_id]["delivered"]
        or int(truth[alert_id]["acknowledged"]) != elog[alert_id]["acknowledged"]
    ]

    truth_rates = {
        "alertas_gerados": len(truth),
        "alertas_entregues": sum(int(r["delivered"]) for r in truth.values()),
        "alertas_confirmados": sum(int(r["acknowledged"]) for r in truth.values()),
        "alertas_sem_entrega": sum(1 for r in truth.values() if r["delivered"] == "0"),
    }
    truth_rates["atendimento_pct"] = (
        100.0 * truth_rates["alertas_confirmados"] / truth_rates["alertas_gerados"]
        if truth_rates["alertas_gerados"] else float("nan"))
    truth_rates["perda_pct"] = (
        100.0 * truth_rates["alertas_sem_entrega"] / truth_rates["alertas_gerados"]
        if truth_rates["alertas_gerados"] else float("nan"))

    return {
        "config": config, "seed": seed,
        "sca_alertas": truth_rates["alertas_gerados"],
        "sca_atendimento_pct": round(truth_rates["atendimento_pct"], 2),
        "sca_perda_pct": round(truth_rates["perda_pct"], 2),
        "elog_alertas": len(elog),
        "elog_atendimento_pct": round(rates(elog)["atendimento_pct"], 2),
        "elog_perda_pct": round(rates(elog)["perda_pct"], 2),
        "so_no_sca": len(only_in_truth),
        "so_no_elog": len(only_in_elog),
        "delivered_ou_acked_diferente": len(mismatched),
        "so_no_sca_ids": ";".join(only_in_truth),
        "mismatch_ids": ";".join(mismatched),
    }


def main() -> None:
    runs = []
    for config in ("MainExperiment_BaOff", "MainExperiment_BaOn"):
        for seed in range(5):
            sca_path = RAW / f"{config}-{seed}-alerts.csv"
            elog_path = RAW / f"{config}-{seed}.elog"
            if not sca_path.exists() or not elog_path.exists():
                print(f"pulando {config}-{seed}: arquivo ausente")
                continue
            runs.append(diff_run(config, seed))

    if not runs:
        raise SystemExit("nenhuma execução de auditoria encontrada em analysis/audit/raw/")

    OUTPUT.mkdir(parents=True, exist_ok=True)
    fieldnames = list(runs[0].keys())
    with open(OUTPUT / "comparativo.csv", "w", newline="", encoding="utf-8") as out:
        writer = csv.DictWriter(out, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(runs)

    print(f"{'config':<22}{'seed':<6}{'sca_ger':<9}{'elog_ger':<10}"
          f"{'sca_atend%':<12}{'elog_atend%':<13}{'só_no_sca':<11}{'diferentes':<11}")
    for row in runs:
        print(f"{row['config']:<22}{row['seed']:<6}{row['sca_alertas']:<9}"
              f"{row['elog_alertas']:<10}{row['sca_atendimento_pct']:<12}"
              f"{row['elog_atendimento_pct']:<13}{row['so_no_sca']:<11}"
              f"{row['delivered_ou_acked_diferente']:<11}")

    total_sca = sum(r["sca_alertas"] for r in runs)
    total_elog = sum(r["elog_alertas"] for r in runs)
    total_only_sca = sum(r["so_no_sca"] for r in runs)
    total_mismatch = sum(r["delivered_ou_acked_diferente"] for r in runs)
    print(f"\nagregado: {total_sca} alertas (.sca) vs {total_elog} alertas (.elog) "
          f"| {total_only_sca} só no .sca | {total_mismatch} delivered/acked divergentes "
          f"entre os {min(total_sca, total_elog)} alertas em comum")
    print(f"gerado: {(OUTPUT / 'comparativo.csv').relative_to(REPO)}")


if __name__ == "__main__":
    main()
