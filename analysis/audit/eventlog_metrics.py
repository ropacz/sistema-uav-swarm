#!/usr/bin/env python3
"""Reconstrói atendimento e perda a partir do .elog, sem tocar nos sinais da app.

Por que o .elog e não o payload binário ou o PCAP: o event log do OMNeT++ já
registra, de graça, a criação/envio/entrega/descarte de cada mensagem no nível
do kernel de simulação — sem exigir um dissector de wire format. A única
informação que faltava era a identidade do alerta, então o nome do pacote passou
a carregar "VictimAlert:<alertId>" e "VictimAck:<alertId>" (ver DroneApp.cc e
TeamApp.cc) — sem tocar no formato serializado, é só um metadado do simulador.

O sinal usado é a linha `DM` (dispose message): quando um módulo processa e
descarta o pacote. Isso é o que o log bruto consegue ver — "o pacote chegou e
foi consumido por este módulo" — e é exatamente o limite deste método: o C++ da
aplicação também descarta pacotes que REJEITA (TTL vencido, equipe errada, ACK
que não bate com a tentativa). O event log não distingue aceitar de rejeitar;
só os sinais da aplicação (usados no .sca) sabem essa diferença. É essa lacuna
que a comparação com o .sca deste script existe para expor.

Formato das linhas: código de duas letras seguido de pares "chave valor"
separados por espaço, em ordem NÃO fixa (ex.: módulos raiz não têm campo
`pid`, os demais têm `pid` antes ou depois de `n` dependendo do módulo). Por
isso o parser abaixo é por par chave-valor, não por posição.
"""

from __future__ import annotations

import re
from pathlib import Path

# Token: palavra sem espaço, ou string entre aspas (pode conter espaço/escape).
TOKEN = re.compile(r'(\S+?)="((?:[^"\\]|\\.)*)"|(\S+)')

ALERT_PREFIX = "VictimAlert:"
ACK_PREFIX = "VictimAck:"


def fields(line: str) -> dict[str, str]:
    """Extrai pares chave-valor de uma linha do .elog, ignorando o código inicial."""
    parts = line.rstrip("\n").split(" ")
    result: dict[str, str] = {}
    i = 1  # parts[0] é o código de duas letras (MC, DM, E, ...).
    while i < len(parts) - 1:
        key, value = parts[i], parts[i + 1]
        if value.startswith('"') and not value.endswith('"'):
            # Valor com espaço, entre aspas (ex.: display string). Rejunta até
            # achar o fechamento; não é usado pelos campos que interessam aqui,
            # mas não pode confundir a contagem de tokens dos campos seguintes.
            j = i + 2
            while j < len(parts) and not parts[j].endswith('"'):
                j += 1
            i = j + 1
            continue
        result[key] = value.strip('"')
        i += 2
    return result


class Module:
    __slots__ = ("cls", "name", "parent")

    def __init__(self, cls: str, name: str, parent: int | None):
        self.cls = cls
        self.name = name
        self.parent = parent


def build_module_index(path: Path) -> dict[int, Module]:
    modules: dict[int, Module] = {}
    with open(path, encoding="utf-8", errors="replace") as source:
        for line in source:
            if not line.startswith("MC "):
                continue
            f = fields(line)
            if "id" not in f or "c" not in f or "n" not in f:
                continue
            modules[int(f["id"])] = Module(
                f["c"], f["n"], int(f["pid"]) if "pid" in f else None)
    return modules


def full_path(modules: dict[int, Module], module_id: int | None) -> str:
    parts = []
    while module_id is not None and module_id in modules:
        module = modules[module_id]
        parts.append(module.name)
        module_id = module.parent
    return ".".join(reversed(parts))


def is_class(modules: dict[int, Module], module_id: int | None, class_name: str) -> bool:
    return module_id is not None and module_id in modules and modules[module_id].cls == class_name


def parse_alert_id(name: str, prefix: str) -> str:
    return name[len(prefix):]


def blank_record(alert_id: str, generation_time: str) -> dict:
    parts = alert_id.split("-")
    return {
        "alertId": alert_id,
        "victimId": parts[1] if len(parts) >= 2 else "",
        "droneId": parts[0] if len(parts) >= 1 else "",
        "generationTime": generation_time,
        "delivered": 0, "receivingTeamId": "",
        "acknowledged": 0, "ackTeamId": "",
    }


def reconstruct(path: Path) -> dict[str, dict]:
    """Uma linha por alertId, com os mesmos campos que ExperimentMetrics grava."""
    modules = build_module_index(path)
    alerts: dict[str, dict] = {}
    attempts: dict[str, int] = {}
    current_time = "0"

    with open(path, encoding="utf-8", errors="replace") as source:
        for line in source:
            if line.startswith("E # "):
                match = re.match(r"^E # \d+ t (\S+)", line)
                if match:
                    current_time = match.group(1)
                continue
            if line.startswith("CM ") and ALERT_PREFIX in line:
                f = fields(line)
                name = f.get("n", "")
                if not name.startswith(ALERT_PREFIX):
                    continue
                alert_id = parse_alert_id(name, ALERT_PREFIX)
                attempts[alert_id] = attempts.get(alert_id, 0) + 1
                alerts.setdefault(alert_id, blank_record(alert_id, current_time))
                continue
            if not line.startswith("DM "):
                continue
            if ALERT_PREFIX not in line and ACK_PREFIX not in line:
                continue
            f = fields(line)
            name = f.get("n", "")
            consumer_id = int(f["m"]) if "m" in f else None
            if name.startswith(ALERT_PREFIX):
                alert_id = parse_alert_id(name, ALERT_PREFIX)
                # "Chegou e foi consumido por um módulo TeamApp" — o log bruto
                # não sabe se handleVictimAlert aceitou ou rejeitou o pacote.
                if is_class(modules, consumer_id, "echosar::TeamApp"):
                    record = alerts.setdefault(alert_id, blank_record(alert_id, current_time))
                    if not record["delivered"]:
                        record["delivered"] = 1
                        team_path = full_path(modules, consumer_id)
                        segments = team_path.split(".")
                        record["receivingTeamId"] = segments[1] if len(segments) > 1 else team_path
            elif name.startswith(ACK_PREFIX):
                alert_id = parse_alert_id(name, ACK_PREFIX)
                # Mesma lacuna do outro lado: chegou ao DroneApp de origem, mas
                # o log não sabe se handleVictimAck validou teamId/endereço.
                if is_class(modules, consumer_id, "echosar::DroneApp"):
                    record = alerts.setdefault(alert_id, blank_record(alert_id, current_time))
                    if not record["acknowledged"]:
                        record["acknowledged"] = 1
                        record["ackTeamId"] = record["receivingTeamId"]

    for alert_id, record in alerts.items():
        record["retryCount"] = max(0, attempts.get(alert_id, 1) - 1)
    return alerts


def rates(alerts: dict[str, dict]) -> dict:
    generated = len(alerts)
    acknowledged = sum(1 for record in alerts.values() if record["acknowledged"])
    undelivered = sum(1 for record in alerts.values() if not record["delivered"])
    return {
        "alertas_gerados": generated,
        "alertas_entregues": sum(1 for record in alerts.values() if record["delivered"]),
        "alertas_confirmados": acknowledged,
        "alertas_sem_entrega": undelivered,
        "atendimento_pct": 100.0 * acknowledged / generated if generated else float("nan"),
        "perda_pct": 100.0 * undelivered / generated if generated else float("nan"),
    }
