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

from analysis.reports import alert_sheet, figures, scavetool_figures  # noqa: E402


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

    def test_paired_effects_use_seed_level_differences(self):
        rows = []
        outcomes = {
            (False, 0): [(1, 1), (0, 0), (0, 0), (1, 0)],
            (True, 0): [(1, 1), (1, 1), (1, 1), (0, 0)],
            (False, 1): [(1, 1), (1, 1), (1, 0), (0, 0)],
            (True, 1): [(1, 1), (1, 1), (1, 1), (1, 0)],
        }
        for (enabled, seed), values in outcomes.items():
            config = f"Main_Ba{'On' if enabled else 'Off'}"
            for index, (delivered, acknowledged) in enumerate(values):
                rows.append({
                    "config": config, "numTeams": 1, "baEnabled": enabled,
                    "seed": seed, "alertId": f"{enabled}-{seed}-{index}",
                    "delivered": delivered, "acknowledged": acknowledged,
                })

        effect = alert_sheet.paired_effects(pd.DataFrame(rows)).iloc[0]
        self.assertEqual(effect["pares"], 2)
        self.assertAlmostEqual(effect["efeito_atendimento_pp"], 37.5)
        self.assertAlmostEqual(effect["efeito_perda_pp"], -25.0)
        self.assertLessEqual(effect["efeito_atendimento_ic95_inf_pp"], 37.5)
        self.assertGreaterEqual(effect["efeito_atendimento_ic95_sup_pp"], 37.5)


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


class ScavetoolFiguresTests(unittest.TestCase):
    """Segunda via de atendimento/perda: lê o CSV do opp_scavetool, não o .sca."""

    def write_scavetool_csv(self, path: Path) -> None:
        # Duas execuções (uma por braço), quatro drones cada — mesmo esquema
        # de módulo que o scavetool real produz: "BasicNetwork.drone[N].app[0]",
        # não "BasicNetwork.drone[N].app". O sufixo errado já causou um bug
        # real aqui (o filtro batia zero linhas e "baEnabled" ficava vazio).
        rows = ["run,type,module,name,attrname,attrvalue,value,count,sumweights,"
                "mean,stddev,min,max,underflows,overflows,binedges,binvalues"]
        for run, config, enabled in (("r0", "C_BaOff", "false"), ("r1", "C_BaOn", "true")):
            rows.append(f'{run},runattr,,,configname,{config},,,,,,,,,,,')
            for drone in range(2):
                rows.append(
                    f'{run},param,BasicNetwork.drone[{drone}].app[0],baEnabled,,,'
                    f'{enabled},,,,,,,,,,')
            rows.append(f'{run},param,BasicNetwork,numTeams,,,1,,,,,,,,,,')
            for name, value in (("alertsGenerated", 10), ("alertsDelivered", 8),
                                ("alertsConfirmed", 6 if enabled == "false" else 9)):
                rows.append(
                    f'{run},scalar,BasicNetwork.experimentMetrics,{name},,,'
                    f'{value},,,,,,,,,,')
        path.write_text("\n".join(rows), encoding="utf-8")

    def test_matches_the_official_pipeline_on_a_synthetic_campaign(self):
        with tempfile.TemporaryDirectory() as directory:
            csv_path = Path(directory) / "scavetool.csv"
            self.write_scavetool_csv(csv_path)
            runs = scavetool_figures.load_runs(csv_path)
            summary = scavetool_figures.summarize(runs)

        off = summary[~summary["baEnabled"]].iloc[0]
        on = summary[summary["baEnabled"]].iloc[0]
        # Mesma fórmula que alert_sheet.py: confirmados / gerados.
        self.assertAlmostEqual(off["atendimento_pct"], 60.0)
        self.assertAlmostEqual(on["atendimento_pct"], 90.0)
        self.assertAlmostEqual(off["perda_pct"], 20.0)  # (10-8)/10

    def test_module_scoped_params_use_the_real_scavetool_suffix(self):
        # BasicNetwork.drone[0].app[0], não .app — regressão do bug real desta
        # sessão: o filtro antigo (".app" sem índice) não batia nada e o
        # conjunto de baEnabled ficava vazio, abortando com "inconsistente".
        with tempfile.TemporaryDirectory() as directory:
            csv_path = Path(directory) / "scavetool.csv"
            self.write_scavetool_csv(csv_path)
            runs = scavetool_figures.load_runs(csv_path)
        for run_values in runs.values():
            self.assertIn("baEnabled", run_values,
                          "baEnabled não foi capturado — regex de módulo quebrou")


if __name__ == "__main__":
    unittest.main(verbosity=2)
