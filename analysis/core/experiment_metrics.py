"""Read the small, normative ExperimentMetrics contract from one SCA file."""

from __future__ import annotations

import math
from pathlib import Path

import pandas as pd

from analysis.core.process_results import parse_sca

METRICS_MODULE_SUFFIX = ".experimentMetrics"


def ratio(numerator: float, denominator: float, scale: float = 1.0) -> float:
    """Return a ratio without inventing a value when no alert was generated."""
    return scale * numerator / denominator if denominator else math.nan


def central_scalar(frame: pd.DataFrame, name: str) -> float:
    """Read exactly one scalar from the central collector.

    Missing or duplicate rows are contract violations. Silently replacing them
    with zero could turn a broken instrumentation run into a scientific result.
    """
    selected = frame.loc[
        (frame["name"] == name)
        & frame["module"].str.endswith(METRICS_MODULE_SUFFIX),
        "value",
    ]
    if len(selected) != 1:
        raise ValueError(
            f"expected one ExperimentMetrics.{name} scalar, found {len(selected)}"
        )
    return float(selected.iloc[0])


def collect(path: str) -> dict:
    """Return raw counters and auditable outcomes for one experimental run."""
    attrs, frame, _ = parse_sca(path)
    raw_names = {
        "alerts_generated": "alertsGenerated",
        "alerts_delivered": "alertsDelivered",
        "alerts_confirmed": "alertsConfirmed",
        "alerts_expired": "alertsExpired",
        "alert_attempts_sent": "alertAttemptsSent",
        "attempts_received": "attemptsReceived",
        "application_retries": "applicationRetries",
        "confirmation_delay_sum_s": "confirmationDelaySum",
        "confirmation_delay_count": "confirmationDelayCount",
        "hop_count_sum": "hopCountSum",
        "hop_count_count": "hopCountCount",
        "multi_hop_deliveries": "multiHopDeliveries",
        "intermediate_forwardings": "intermediateForwardings",
        "never_known_team_selection_events": "neverKnownTeamSelectionEvents",
        "expired_known_team_selection_events": "expiredKnownTeamSelectionEvents",
        "alerts_without_known_team": "alertsWithoutKnownTeam",
        "reposition_triggers": "repositionTriggers",
        "sensor_evaluations": "sensorEvaluations",
        "obstacles_detected": "obstaclesDetected",
        "ba_activations": "baActivations",
        "repositions_started": "repositionsStarted",
        "repositions_completed": "repositionsCompleted",
        "reposition_distance_sum_m": "repositionDistanceSum",
        "reposition_duration_sum_s": "repositionDurationSum",
        "effective_repositions": "effectiveRepositions",
        "recovery_probe_checks": "recoveryProbeChecks",
        "recovery_probes_sent": "recoveryProbesSent",
        "recovery_probes_confirmed": "recoveryProbesConfirmed",
        "recovery_probes_failed": "recoveryProbesFailed",
        "recovery_probes_unreachable": "recoveryProbesUnreachable",
        "recovery_probes_abandoned": "recoveryProbesAbandoned",
    }
    values = {
        output_name: central_scalar(frame, scalar_name)
        for output_name, scalar_name in raw_names.items()
    }
    generated = values["alerts_generated"]
    values.update({
        "config": attrs["configname"],
        "seed": int(float(attrs["seedset"])),
        "run_file": Path(path).name,
        # Recalculadas a partir dos contadores para manter o relatório auditável.
        "alert_pdr_pct": ratio(values["alerts_delivered"], generated, 100),
        "alert_loss_pct": ratio(
            generated - values["alerts_delivered"], generated, 100
        ),
        "appack_pct": ratio(values["alerts_confirmed"], generated, 100),
        "confirmation_delay_mean_s": ratio(
            values["confirmation_delay_sum_s"],
            values["confirmation_delay_count"],
        ),
        "retries_per_alert": ratio(values["application_retries"], generated),
        "attempt_pdr_pct": ratio(
            values["attempts_received"], values["alert_attempts_sent"], 100
        ),
        "attempt_loss_pct": ratio(
            values["alert_attempts_sent"] - values["attempts_received"],
            values["alert_attempts_sent"],
            100,
        ),
        "mean_hop_count": ratio(
            values["hop_count_sum"], values["hop_count_count"]
        ),
        "multi_hop_delivery_rate_pct": ratio(
            values["multi_hop_deliveries"], values["alerts_delivered"], 100
        ),
        "reposition_distance_mean_m": ratio(
            values["reposition_distance_sum_m"],
            values["repositions_completed"],
        ),
        "reposition_duration_mean_s": ratio(
            values["reposition_duration_sum_s"],
            values["repositions_completed"],
        ),
        # Fração das verificações que liberaram o alerta. O denominador são as
        # verificações abertas, não as sondagens emitidas: o que interessa é
        # quantos reposicionamentos o portão conseguiu confirmar.
        "recovery_probe_confirm_pct": ratio(
            values["recovery_probes_confirmed"],
            values["recovery_probe_checks"],
            100,
        ),
        # Sondagens gastas por verificação: acima de 1 indica reenvio, e é o
        # custo que o mecanismo cobra para poupar o alerta pesado.
        "recovery_probes_per_check": ratio(
            values["recovery_probes_sent"], values["recovery_probe_checks"]
        ),
    })
    return values
