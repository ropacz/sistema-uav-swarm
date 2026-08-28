#!/usr/bin/env python3
"""Funções compartilhadas para leitura e estatística de escalares OMNeT++."""

import re

import pandas as pd


def parse_sca(path: str) -> tuple[dict[str, str], pd.DataFrame, dict[str, str]]:
    """Return attributes, scalar rows and recorded parameters from one SCA."""
    attrs: dict[str, str] = {}   # metadados da execução: configname, seedset, iterationvars...
    params: dict[str, str] = {}  # parâmetros de módulo resolvidos: "modulo paramName" -> valor
    rows = []                    # linhas de escalar: (módulo, nome, valor)
    # Cada regex casa exatamente um tipo de linha do .sca:
    #   attr configname Scenario1_OneVictim_BaOff
    attr_pattern = re.compile(r'attr (\S+) "?([^"\n]+)"?')
    #   scalar BasicNetwork.experimentMetrics alertsGenerated 24
    scalar_pattern = re.compile(r'scalar (\S+) "?([^"\n]+?)"? ([^\s]+)$')
    #   par BasicNetwork.experimentMetrics requireClosedAlerts true
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
                    # chave composta "modulo paramName" — é assim que outros scripts
                    # buscam um parâmetro específico depois, ex.: parameters[key].endswith(" baEnabled")
                    params[f"{match.group(1)} {match.group(2)}"] = match.group(3).strip()
            else:
                # Cai aqui toda linha que não é "attr " nem "par " — inclusive "config ...",
                # "version 3", "run ...", "statistic ...:histogram" e "field mean ...". Só as
                # que começam literalmente com "scalar " batem no regex; o resto (config e
                # histogramas de framework) é silenciosamente ignorado por este parser.
                match = scalar_pattern.match(line)
                if match:
                    try:
                        # valor de escalar é sempre numérico; se não for, descarta a linha
                        # em vez de quebrar o parsing do arquivo inteiro
                        rows.append((match.group(1), match.group(2).strip(), float(match.group(3))))
                    except ValueError:
                        pass
    attrs.setdefault("configname", "unknown")
    # seedset nem sempre é gravado como attr próprio — cai pro repetition quando falta
    # (caso padrão; só Calibration_Exposure desloca seed-set = repetition+100)
    attrs.setdefault("seedset", attrs.get("repetition", "0"))
    return attrs, pd.DataFrame(rows, columns=["module", "name", "value"]), params
