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
    """Return raw counters and the four outcomes for one experimental run."""
    attrs, frame, _ = parse_sca(path)
    raw_names = {
        "alerts_generated": "alertsGenerated",
        "alerts_delivered": "alertsDelivered",
        "alerts_confirmed": "alertsConfirmed",
        "alerts_expired": "alertsExpired",
        "alert_attempts_sent": "alertAttemptsSent",
        "application_retries": "applicationRetries",
        "delivery_delay_sum_s": "deliveryDelaySum",
        "delivery_delay_count": "deliveryDelayCount",
        "reposition_triggers": "repositionTriggers",
        "obstacles_detected": "obstaclesDetected",
        "ba_activations": "baActivations",
        "repositions_started": "repositionsStarted",
        "repositions_completed": "repositionsCompleted",
        "reposition_distance_sum_m": "repositionDistanceSum",
        "reposition_distance_count": "repositionDistanceCount",
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
        "appack_pct": ratio(values["alerts_confirmed"], generated, 100),
        "delivery_delay_mean_s": ratio(
            values["delivery_delay_sum_s"], values["delivery_delay_count"]
        ),
        "retries_per_alert": ratio(values["application_retries"], generated),
    })
    return values
