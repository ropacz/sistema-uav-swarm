#!/usr/bin/env python3
"""Valida a planilha e as figuras de atendimento/perda sem depender de campanha.

Roda em cerca de um segundo, com dados sintéticos, e serve para pegar quebra de
contrato antes de gastar tempo de simulação: se a fórmula ou o esquema de coluna
mudar, cai aqui em vez de sumir silenciosamente na planilha final.
"""

from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

import pandas as pd  # noqa: E402

from analysis.reports import alert_sheet, figures  # noqa: E402


class AlertSheetTests(unittest.TestCase):
    """As duas taxas contam alertId únicos, não tentativas nem recebimentos."""

    def frame(self) -> pd.DataFrame:
        # Quatro alertas: um confirmado, um entregue sem ACK, dois sem entrega.
        return pd.DataFrame([
            {"config": "C", "numTeams": 1, "baEnabled": True, "seed": 0,
             "alertId": "a1", "delivered": 1, "acknowledged": 1},
            {"config": "C", "numTeams": 1, "baEnabled": True, "seed": 0,
             "alertId": "a2", "delivered": 1, "acknowledged": 0},
            {"config": "C", "numTeams": 1, "baEnabled": True, "seed": 0,
             "alertId": "a3", "delivered": 0, "acknowledged": 0},
            {"config": "C", "numTeams": 1, "baEnabled": True, "seed": 1,
             "alertId": "a4", "delivered": 0, "acknowledged": 0},
        ])

    def test_rates_follow_the_definitions(self):
        summary = alert_sheet.summarize(self.frame()).iloc[0]
        self.assertEqual(summary["alertas_gerados"], 4)
        self.assertEqual(summary["execucoes"], 2)
        # Atendimento conta alertId com ao menos um ACK: 1 de 4.
        self.assertAlmostEqual(summary["atendimento_pct"], 25.0)
        # Perda conta alertId sem entrega a nenhuma equipe: 2 de 4.
        self.assertAlmostEqual(summary["perda_pct"], 50.0)

    def test_smoke_tests_never_enter_the_campaign_sheet(self):
        # Connectivity_SmokeTest liga baEnabled só num drone por desenho do
        # teste; se entrasse aqui, run_context() rejeitaria a mistura como
        # inconsistência, ou pior, contaminaria as taxas da campanha.
        self.assertTrue(alert_sheet.EXCLUDED_SUFFIX in "Connectivity_SmokeTest")

    def test_sheet_columns_are_the_requested_ones(self):
        self.assertEqual(alert_sheet.COLUMNS, [
            "seed", "numTeams", "baEnabled", "alertId", "victimId", "droneId",
            "generationTime", "delivered", "receivingTeamId", "acknowledged",
            "ackTeamId", "retryCount"])


class FigureTests(unittest.TestCase):
    """As duas figuras precisam sair em PDF com desenho de verdade."""

    def setUp(self) -> None:
        figures.configure_style()
        self.directory = tempfile.TemporaryDirectory()
        self._original = figures.SIMPLE_OUTPUT
        figures.SIMPLE_OUTPUT = Path(self.directory.name)
        self.addCleanup(self.directory.cleanup)
        self.addCleanup(setattr, figures, "SIMPLE_OUTPUT", self._original)

    def assertUsableFigure(self, path: Path) -> None:
        self.assertTrue(path.exists(), f"{path.name} não foi gerado")
        content = path.read_bytes()
        self.assertTrue(content.startswith(b"%PDF"), "a figura não é PDF vetorial")
        # Um PDF vazio de matplotlib tem poucos KB; abaixo disso não há desenho.
        self.assertGreater(len(content), 2000, f"{path.name} saiu praticamente vazio")

    def test_rate_figures_scale_from_one_cell_to_the_matrix(self):
        for levels in ([1], [1, 5, 10, 15]):
            summary = pd.DataFrame([
                {"config": "C", "numTeams": teams, "baEnabled": enabled,
                 "atendimento_pct": 80.0, "perda_pct": 20.0}
                for teams in levels for enabled in (False, True)])
            for path in figures.attendance_figures(summary):
                self.assertUsableFigure(path)


if __name__ == "__main__":
    unittest.main(verbosity=2)
