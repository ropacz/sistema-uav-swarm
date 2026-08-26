#!/usr/bin/env python3
"""Planilha de verificação da configuração efetivamente executada.

Os valores não são digitados aqui: são lidos dos parâmetros que o OMNeT++ grava
no arquivo `.sca` de uma execução real. É isso que torna a planilha uma
verificação e não uma cópia da documentação — se o `.ini` divergir do que a
dissertação afirma, a divergência aparece.

A coluna de referência traz o valor pedido pela diretriz normativa, para que
cada linha possa ser classificada como conforme, desvio ou não especificada.
Os desvios estão justificados em `docs/desvios_e_extensoes.md`.
"""

from __future__ import annotations

from pathlib import Path
import re
import sys

import pandas as pd

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.process_results import parse_sca  # noqa: E402

RESULTS = REPOSITORY_ROOT / "simulations/results/omnetpp"
OUTPUT = REPOSITORY_ROOT / "analysis/tables"
REFERENCE_RUN = "MainExperiment_BaOn-0.sca"

NOT_SPECIFIED = "não especificado"

# Desvios conhecidos e justificados em docs/desvios_e_extensoes.md. Sem esta
# marcação a comparação textual classificaria RandomWaypoint como "Random Walk"
# conforme, escondendo justamente o que a planilha existe para revelar.
DEVIATIONS = {
    ("Equipe", "Modelo de mobilidade"): "D1",
    ("Ciclo do alerta", "Intervalo entre alertas"): "D3",
    ("Sensor", "Alcance máximo"): "D4",
}

# (grupo, rótulo, sufixo do parâmetro no .sca, valor da diretriz, seção)
# O sufixo casa com o final da chave "<módulo> <parâmetro>"; o primeiro módulo
# que casar é usado, porque os parâmetros são idênticos entre drones.
PARAMETERS = [
    ("Cenário", "Quantidade de drones", "BasicNetwork numDrones", "4", "§3"),
    ("Cenário", "Quantidade de vítimas", "BasicNetwork numVictims", "1 ou 2", "§3"),
    ("Cenário", "Quantidade de equipes", "BasicNetwork numTeams", "1, 5, 10 ou 15", "§3"),
    ("Cenário", "Limite da área em X", "physicalEnvironment spaceMaxX", "1000 m", "§3"),
    ("Cenário", "Limite da área em Y", "physicalEnvironment spaceMaxY", "1000 m", "§3"),
    ("Cenário", "Teto do espaço físico", "physicalEnvironment spaceMaxZ", "20 m", "§3"),

    ("VANT", "Modelo de mobilidade", "drone[0].mobility typename",
     "Gauss–Markov tridimensional", "§3"),
    ("VANT", "Velocidade média", "drone[0].mobility speed", "13 m/s", "§3"),
    ("VANT", "Altitude mínima de voo", "drone[0].app[0] minimumAltitude",
     NOT_SPECIFIED, "§17"),
    ("VANT", "Altitude máxima de voo", "drone[0].app[0] maximumAltitude", "20 m", "§3"),
    ("VANT", "Velocidade horizontal no reposicionamento",
     "drone[0].app[0] horizontalSpeed", NOT_SPECIFIED, "§18"),
    ("VANT", "Velocidade de subida", "drone[0].app[0] climbSpeed", NOT_SPECIFIED, "§18"),
    ("VANT", "Velocidade de descida", "drone[0].app[0] descentSpeed", NOT_SPECIFIED, "§18"),
    ("VANT", "Tempo operacional máximo", "drone[0].app[0] flightTimeLimit",
     "900 s (15 min)", "§3"),

    ("Equipe", "Modelo de mobilidade", "team[0].mobility typename",
     "Random Walk", "§3"),

    ("Rede", "Tecnologia sem fio", "wlan[0] opMode",
     "b (IEEE 802.11 DSSS)", "§3"),
    ("Rede", "Taxa física", "wlan[0] bitrate", "1 Mbit/s", "§3"),
    ("Rede", "Potência de transmissão", "wlan[0].radio.transmitter power",
     NOT_SPECIFIED, "—"),
    ("Rede", "Sensibilidade do receptor", "wlan[0].radio.receiver sensitivity",
     NOT_SPECIFIED, "—"),
    ("Rede", "Frequência da portadora", "wlan[0].radio carrierFrequency",
     NOT_SPECIFIED, "—"),
    ("Rede", "TTL da aplicação", "drone[0].app[0] applicationIpTtl",
     "> 1 (múltiplos saltos)", "§5"),

    ("Ciclo do alerta", "Intervalo entre alertas", "drone[0].app[0] alertInterval",
     "30 s (§7.3) ou 10 s (§13)", "§7.3"),
    ("Ciclo do alerta", "Validade do alerta", "drone[0].app[0] alertTtl", "25 s", "§13"),
    ("Ciclo do alerta", "Espera por confirmação", "drone[0].app[0] ackTimeout",
     "2 s", "§13"),
    ("Ciclo do alerta", "Intervalo entre retransmissões",
     "drone[0].app[0] retryInterval", "3 s", "§13"),
    ("Ciclo do alerta", "Máximo de tentativas", "drone[0].app[0] maxAttempts",
     "4", "§13"),
    ("Ciclo do alerta", "Limiar de tentativas sem confirmação",
     "drone[0].app[0] repositionAfterUnackedAttempts", "2", "§12"),
    ("Ciclo do alerta", "Período de manutenção interna",
     "drone[0].app[0] maintenanceInterval", "0,1 s", "§13"),

    ("Descoberta", "Validade da entrada de equipe",
     "drone[0].app[0] teamEntryLifetime", "30 s", "§13"),
    ("Descoberta", "Retenção da última posição conhecida",
     "drone[0].app[0] lastKnownTeamRetention", NOT_SPECIFIED, "§8"),
    ("Descoberta", "Saltos máximos do TeamUpdate",
     "drone[0].app[0] teamUpdateMaxHops", NOT_SPECIFIED, "§7.1"),
    ("Descoberta", "Jitter de repasse", "drone[0].app[0] teamUpdateForwardJitter",
     NOT_SPECIFIED, "§7.1"),

    ("Sensor", "Alcance mínimo", "drone[0].obstacleSensor minimumRange",
     NOT_SPECIFIED, "§14"),
    ("Sensor", "Alcance máximo", "drone[0].obstacleSensor maximumRange",
     "configurável, exemplo 30 m", "§14"),

    ("Reposicionamento", "Política habilitada", "drone[0].app[0] baEnabled",
     "true no tratamento, false no controle", "§15"),
    ("Reposicionamento", "Peso da proximidade da equipe", "drone[0].app[0] wLink",
     "0,60", "§16"),
    ("Reposicionamento", "Peso do afastamento do obstáculo",
     "drone[0].app[0] wObstacle", "0,25", "§16"),
    ("Reposicionamento", "Peso do deslocamento", "drone[0].app[0] wMove",
     "0,15", "§16"),
    ("Reposicionamento", "Distância máxima de reposicionamento",
     "drone[0].app[0] maximumRepositionDistance", NOT_SPECIFIED, "§17"),
    ("Reposicionamento", "Margem de segurança do obstáculo",
     "drone[0].app[0] obstacleSafetyMargin", NOT_SPECIFIED, "§17"),
    ("Reposicionamento", "Influência do obstáculo na aptidão",
     "drone[0].app[0] obstacleSigma", NOT_SPECIFIED, "§16.2"),
    ("Reposicionamento", "População de morcegos", "drone[0].app[0] batPopulation",
     NOT_SPECIFIED, "§6.6"),
    ("Reposicionamento", "Iterações do algoritmo", "drone[0].app[0] batIterations",
     NOT_SPECIFIED, "§6.6"),

]

# Campos mínimos exigidos pelo §7; os acrescentados aparecem marcados.
MESSAGES = [
    ("TeamUpdate", "Equipe → drones (difusão, repassada pela FANET)",
     "teamId, messageId, sequenceNumber, position, timestamp",
     "teamAddress, hopCount",
     "team[0].app[0] teamUpdateInterval", "team[0].app[0] teamUpdatePayloadBytes",
     "§7.1"),
    ("DroneStatus", "Drone → drones (difusão de presença)",
     "droneId, messageId, sequenceNumber, position, timestamp",
     "—",
     "drone[0].app[0] droneStatusInterval", "drone[0].app[0] droneStatusPayloadBytes",
     "§7.2"),
    ("VictimAlert", "Drone → equipe (unicast, roteado pelo AODV)",
     "alertId, messageId, victimId, sourceDroneId, targetTeamId, "
     "victimPosition, creationTime, attemptNumber",
     "timeToLive",
     "drone[0].app[0] alertInterval", "drone[0].app[0] victimAlertPayloadBytes",
     "§7.3"),
    ("VictimAck", "Equipe → drone (confirmação)",
     "alertId, messageId, victimId, sourceDroneId, teamId, ackTimestamp",
     "—",
     "sob demanda", "team[0].app[0] victimAckPayloadBytes",
     "§7.4"),
]


def clean(value: str) -> str:
    """O OMNeT++ grava strings entre aspas escapadas no .sca."""
    return value.strip().replace('\\"', "").replace('"', "").strip()


def lookup(parameters: dict[str, str], suffix: str) -> str:
    """Primeiro parâmetro cujo 'módulo nome' termina no sufixo informado."""
    for key in sorted(parameters):
        if key.endswith(suffix):
            return clean(parameters[key])
    return "—"


def numbers(text: str) -> list[float]:
    return [float(match.replace(",", "."))
            for match in re.findall(r"-?\d+(?:[.,]\d+)?", text)]


def words(text: str) -> list[str]:
    return [token for token in re.findall(r"[a-zà-ú]+", text.lower())
            if len(token) >= 4]


def classify(executed: str, reference: str) -> str:
    """Compara o valor executado com o da diretriz, sem inventar equivalência."""
    if reference == NOT_SPECIFIED:
        return "Não especificado pela diretriz"
    executed = clean(executed)
    if executed == "—":
        return "Conferir"

    executed_numbers = numbers(executed)
    reference_numbers = numbers(reference)
    if executed_numbers and reference_numbers:
        value = executed_numbers[0]
        if reference.strip().startswith(">"):
            return "Conforme" if value > reference_numbers[0] else "Conferir"
        # 0.6 e "0,60" são o mesmo valor; a comparação é numérica, não textual.
        if any(abs(value - expected) < 1e-9 for expected in reference_numbers):
            return "Conforme"
        return "Conferir"

    normalized = re.sub(r"[^a-zà-ú]", "", executed.lower())
    if any(word in normalized for word in words(reference)):
        return "Conforme"
    if executed.lower() in [token.strip().lower()
                            for token in re.split(r"[,()]", reference)]:
        return "Conforme"
    return "Conferir"


def build(reference_path: Path) -> tuple[pd.DataFrame, pd.DataFrame]:
    _, _, parameters = parse_sca(str(reference_path))
    rows = []
    for group, label, suffix, reference, section in PARAMETERS:
        executed = lookup(parameters, suffix)
        deviation = DEVIATIONS.get((group, label))
        rows.append({
            "Grupo": group,
            "Parâmetro": label,
            "Valor executado": executed,
            "Valor da diretriz": reference,
            "Seção": section,
            "Situação": (f"Desvio justificado ({deviation})" if deviation
                         else classify(executed, reference)),
            "Chave no .sca": suffix,
        })
    messages = []
    for name, direction, required, added, period_key, size_key, section in MESSAGES:
        messages.append({
            "Mensagem": name,
            "Sentido": direction,
            "Campos mínimos da diretriz": required,
            "Campos acrescentados": added,
            "Periodicidade": (period_key if period_key == "sob demanda"
                              else lookup(parameters, period_key)),
            "Payload": lookup(parameters, size_key),
            "Seção": section,
        })
    return pd.DataFrame(rows), pd.DataFrame(messages)


def main() -> None:
    reference_path = RESULTS / REFERENCE_RUN
    if not reference_path.exists():
        raise SystemExit(
            f"{REFERENCE_RUN} não encontrado em {RESULTS}. "
            "Rode `make experiment` antes de gerar a planilha.")

    parameters, messages = build(reference_path)
    executed_runs = len(list(RESULTS.glob(REFERENCE_RUN.rsplit("-", 1)[0] + "-*.sca")))
    parameters.loc[len(parameters)] = {
        "Grupo": "Experimento",
        "Parâmetro": "Repetições executadas por braço",
        "Valor executado": str(executed_runs),
        "Valor da diretriz": "pelo menos 30",
        "Seção": "§3",
        "Situação": "Conforme" if executed_runs >= 30 else "Conferir",
        "Chave no .sca": f"contagem de {REFERENCE_RUN.rsplit('-', 1)[0]}-*.sca",
    }
    OUTPUT.mkdir(parents=True, exist_ok=True)
    workbook = OUTPUT / "verificacao.xlsx"
    with pd.ExcelWriter(workbook, engine="openpyxl") as writer:
        parameters.to_excel(writer, sheet_name="Parâmetros", index=False)
        messages.to_excel(writer, sheet_name="Mensagens", index=False)
        for sheet, frame in (("Parâmetros", parameters), ("Mensagens", messages)):
            worksheet = writer.sheets[sheet]
            for index, column in enumerate(frame.columns, start=1):
                width = max(len(str(column)),
                            *(len(str(value)) for value in frame[column]))
                worksheet.column_dimensions[
                    worksheet.cell(row=1, column=index).column_letter
                ].width = min(width + 2, 60)
    parameters.to_csv(OUTPUT / "verificacao_parametros.csv", index=False)
    messages.to_csv(OUTPUT / "verificacao_mensagens.csv", index=False)

    print(f"referência: {reference_path.name}")
    print(f"gerado: {workbook.relative_to(REPOSITORY_ROOT)} "
          f"({len(parameters)} parâmetros, {len(messages)} mensagens)")
    print()
    print(parameters["Situação"].value_counts().to_string())
    pending = parameters[parameters["Situação"] == "Conferir"]
    if not pending.empty:
        print("\nLinhas sem correspondência automática, para conferência manual:")
        print(pending[["Grupo", "Parâmetro", "Valor executado",
                       "Valor da diretriz"]].to_string(index=False))


if __name__ == "__main__":
    main()
