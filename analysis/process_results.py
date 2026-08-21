#!/usr/bin/env python3
"""Shared scalar helpers and entry point for the hypothesis pilot."""

import math
import re
from statistics import NormalDist

import pandas as pd


def parse_sca(path: str) -> tuple[dict[str, str], pd.DataFrame, dict[str, str]]:
    """Return attributes, scalar rows and recorded parameters from one SCA."""
    attrs: dict[str, str] = {}
    params: dict[str, str] = {}
    rows = []
    attr_pattern = re.compile(r'attr (\S+) "?([^"\n]+)"?')
    scalar_pattern = re.compile(r'scalar (\S+) "?([^"\n]+?)"? ([^\s]+)$')
    param_pattern = re.compile(r'par (\S+) (\S+) (.*)$')
    with open(path, encoding="utf-8", errors="replace") as source:
        for line in source:
            if line.startswith("attr "):
                match = attr_pattern.match(line)
                if match:
                    attrs[match.group(1)] = match.group(2).strip()
            elif line.startswith("par "):
                match = param_pattern.match(line.rstrip("\n"))
                if match:
                    params[f"{match.group(1)} {match.group(2)}"] = match.group(3).strip()
            else:
                match = scalar_pattern.match(line)
                if match:
                    try:
                        rows.append((match.group(1), match.group(2).strip(), float(match.group(3))))
                    except ValueError:
                        pass
    attrs.setdefault("configname", "unknown")
    attrs.setdefault("seedset", attrs.get("repetition", "0"))
    return attrs, pd.DataFrame(rows, columns=["module", "name", "value"]), params


def ci95(values: pd.Series) -> float:
    """Student-t approximate 95% confidence-interval half-width for a mean."""
    clean = values.dropna()
    if len(clean) < 2:
        return math.nan
    degrees = len(clean) - 1
    z = NormalDist().inv_cdf(0.975)
    critical = z + (z**3 + z) / (4 * degrees) + \
        (5 * z**5 + 16 * z**3 + 3 * z) / (96 * degrees**2)
    return critical * clean.std(ddof=1) / math.sqrt(len(clean))


def main() -> None:
    from report_hypothesis_pilot import main as report_main
    report_main()


if __name__ == "__main__":
    main()
