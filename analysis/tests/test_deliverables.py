#!/usr/bin/env python3
"""Valida figuras e planilha sem depender de uma campanha executada.

O objetivo é pegar quebra de contrato entre os relatórios e os entregáveis antes
de gastar horas de simulação: se um relatório renomear uma coluna, a figura que
a consome falha aqui, e não no fim da campanha.

Os dados são sintéticos e as colunas vêm das constantes dos próprios relatórios,
de modo que uma mudança de esquema é detectada em vez de ser duplicada no teste.
"""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

import pandas as pd  # noqa: E402

from analysis.reports import figures, verification_sheet  # noqa: E402
from analysis.reports.report_main_experiment import (  # noqa: E402
    EXPOSURE_METRICS, METRIC_SPECS,
)
from analysis.reports.report_robustness import METRICS as ROBUSTNESS_METRICS  # noqa: E402

SEEDS = 30


def main_summary() -> pd.DataFrame:
    """Mesmas colunas que report_main_experiment.summarize() escreve."""
    return pd.DataFrame([{
        "metric": metric,
        "role": role,
        "favorable_direction": direction,
        "paired_n": SEEDS,
        "control_mean": 80.0,
        "treatment_mean": 84.0,
        "effect_on_minus_off": 4.0,
        "effect_std": 11.0,
        "effect_ci95_half_width": 4.2,
    } for metric, (role, direction) in METRIC_SPECS.items()])


def exposure_frames() -> tuple[pd.DataFrame, pd.DataFrame]:
    per_run = pd.DataFrame({
        "seed": range(SEEDS),
        **{metric: [i % 3 for i in range(SEEDS)] for metric in EXPOSURE_METRICS},
    })
    summary = pd.DataFrame([{
        "treatment_runs": SEEDS,
        "runs_with_ba_activation": 20,
        "ba_activations": 35,
        "repositions_started": 20,
        "repositions_completed": 19,
        "exposure_status": "observed",
    }])
    return summary, per_run


def robustness_summary() -> pd.DataFrame:
    """Mesmas colunas que report_robustness.summarize_effects() escreve."""
    rows = []
    for scenario in ("Scenario1_OneVictim", "Scenario1_TwoVictims"):
        for teams in (1, 5, 10, 15):
            for metric in ROBUSTNESS_METRICS:
                rows.append({
                    "scenario": scenario, "teams": teams, "metric": metric,
                    "paired_n": SEEDS, "effect_mean": 2.5,
                    "effect_median": 2.0, "effect_std": 9.0,
                    "effect_ci95_half_width": 3.1,
                })
    return pd.DataFrame(rows)


class FigureTests(unittest.TestCase):
    """As três figuras precisam sair de tabelas com o esquema real."""

    def setUp(self) -> None:
        figures.configure_style()
        self.directory = tempfile.TemporaryDirectory()
        self.output = Path(self.directory.name)
        self._original = figures.OUTPUT
        figures.OUTPUT = self.output
        self.addCleanup(self.directory.cleanup)
        self.addCleanup(setattr, figures, "OUTPUT", self._original)

    def assertUsableFigure(self, path: Path) -> None:
        self.assertTrue(path.exists(), f"{path.name} não foi gerado")
        content = path.read_bytes()
        self.assertTrue(content.startswith(b"%PDF"), "a figura não é PDF vetorial")
        # Um PDF vazio de matplotlib tem poucos KB; abaixo disso não há desenho.
        self.assertGreater(len(content), 2000, f"{path.name} saiu praticamente vazio")

    def test_paired_effect_figure_uses_only_comparable_units(self):
        path = figures.paired_effect_figure(main_summary())
        self.assertUsableFigure(path)
        # Misturar segundos e pontos percentuais no mesmo eixo induziria
        # comparação visual indevida entre métricas incomparáveis.
        for metric in figures.PERCENTAGE_METRICS:
            self.assertIn("pct", metric)
        self.assertIn(figures.PRIMARY_METRIC, figures.PERCENTAGE_METRICS)

    def test_exposure_funnel_figure(self):
        summary, per_run = exposure_frames()
        self.assertUsableFigure(figures.exposure_funnel_figure(summary, per_run))

    def test_robustness_figure(self):
        self.assertUsableFigure(figures.robustness_figure(robustness_summary()))

    def test_every_metric_a_figure_needs_is_produced_by_the_reports(self):
        """Contrato entre relatórios e figuras, verificado nos dois sentidos."""
        for metric in figures.PERCENTAGE_METRICS:
            self.assertIn(metric, METRIC_SPECS, f"{metric} sumiu do relatório principal")
            self.assertIn(metric, ROBUSTNESS_METRICS, f"{metric} sumiu da robustez")
        for column, _ in figures.FUNNEL_STAGES:
            self.assertIn(column, EXPOSURE_METRICS,
                          f"{column} não é exportado como diagnóstico de exposição")


class VerificationSheetTests(unittest.TestCase):
    """A planilha precisa ler o .sca, e não repetir a documentação."""

    def write_sca(self, overrides: dict[str, str] | None = None) -> Path:
        overrides = overrides or {}
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        path = Path(directory.name) / "run.sca"
        lines = ["attr configname Test", "attr repetition 0"]
        for _, label, suffix, _, _ in verification_sheet.PARAMETERS:
            module, name = suffix.rsplit(" ", 1)
            value = overrides.get(label, "42")
            lines.append(f"par BasicNetwork.{module} {name} {value}")
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        return path

    def test_values_come_from_the_sca(self):
        path = self.write_sca({"Quantidade de drones": "7"})
        parameters, _ = verification_sheet.build(path)
        row = parameters.set_index("Parâmetro").loc["Quantidade de drones"]
        # 7 contraria a diretriz (4 drones): o valor tem de vir do arquivo, e a
        # divergência tem de aparecer. Se viesse da documentação, diria 4.
        self.assertEqual(row["Valor executado"], "7")
        self.assertEqual(row["Situação"], "Conferir")

    def test_conforming_value_is_recognised(self):
        path = self.write_sca({"Quantidade de drones": "4"})
        parameters, _ = verification_sheet.build(path)
        row = parameters.set_index("Parâmetro").loc["Quantidade de drones"]
        self.assertEqual(row["Situação"], "Conforme")

    def test_known_deviations_are_never_reported_as_conforming(self):
        # RandomWaypointMobility contém "Random", e a comparação textual o daria
        # como conforme com "Random Walk" — escondendo o desvio D1.
        path = self.write_sca({"Modelo de mobilidade": '"RandomWaypointMobility"'})
        parameters, _ = verification_sheet.build(path)
        deviating = parameters[parameters["Grupo"] == "Equipe"]
        self.assertTrue(
            deviating["Situação"].str.startswith("Desvio").all(),
            "um desvio conhecido foi classificado como conforme")

    def test_decimal_notation_does_not_create_false_divergence(self):
        # O .sca grava 0.6; a diretriz escreve 0,60. É o mesmo valor.
        path = self.write_sca({"Peso da proximidade da equipe": "0.6"})
        parameters, _ = verification_sheet.build(path)
        row = parameters.set_index("Parâmetro").loc["Peso da proximidade da equipe"]
        self.assertEqual(row["Situação"], "Conforme")

    def test_messages_sheet_covers_the_four_message_types(self):
        _, messages = verification_sheet.build(self.write_sca())
        self.assertEqual(
            set(messages["Mensagem"]),
            {"TeamUpdate", "DroneStatus", "VictimAlert", "VictimAck"})
        self.assertTrue(messages["Campos mínimos da diretriz"].str.len().gt(20).all())


if __name__ == "__main__":
    unittest.main(verbosity=2)
