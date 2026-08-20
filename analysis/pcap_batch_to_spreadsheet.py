#!/usr/bin/env python3
"""Consolida todas as capturas PCAPNG do ECHOSAR-Net em uma planilha.

O script descobre automaticamente arquivos com nomes como::

    Validation_Direct-BasicNetwork.team[0].pcap
    Validation_Direct-BasicNetwork.drone[0].pcap

Para cada cenário, ele lê o IP gravado no Interface Description Block do
PCAPNG e usa a direção registrada pelo INET (inbound/outbound). Cada transmissão
unicast é comparada com a captura do IP de destino. Cada broadcast é comparado
com todos os outros nós capturados, produzindo uma oportunidade de entrega por
receptor.

Exemplo::

    python3 analysis/pcap_batch_to_spreadsheet.py simulations/results \
      -o simulations/results/metricas-rede.xlsx

As mensagens atuais são identificadas pela assinatura ``ECHO`` e pelo código
explícito do tipo. Para capturas antigas, permanece o fallback pelos tamanhos:
96 bytes = VictimAck, 160 = PositionUpdate e 320 = VictimAlert. UDP/654 é AODV.
"""

from __future__ import annotations

import argparse
import re
import struct
from collections import Counter
from dataclasses import dataclass
from pathlib import Path

import pandas as pd

from pcap_to_spreadsheet import (
    format_workbook,
    is_group_destination,
    load_capture,
    packet_identity_method,
    packet_key,
)


CAPTURE_NAME = re.compile(
    r"^(?P<prefix>.+)-BasicNetwork\.(?P<node>(?:drone|team)\[\d+\])$"
)


@dataclass
class Capture:
    """Metadados e eventos pertencentes à captura de um único nó."""

    path: Path
    configuration: str
    run: str
    node: str
    interface_ip: str
    events: list[dict]


def parse_capture_name(path: Path) -> tuple[str, str, str] | None:
    """Extrai configuração, execução e nó a partir do nome do arquivo."""

    match = CAPTURE_NAME.match(path.stem)
    if match is None:
        return None
    prefix = match.group("prefix")
    run_match = re.match(r"^(?P<config>.+)-(?P<run>\d+)$", prefix)
    if run_match:
        return run_match.group("config"), run_match.group("run"), match.group("node")
    return prefix, "não informado", match.group("node")


def discover_captures(directory: Path) -> tuple[list[Capture], list[dict]]:
    """Encontra PCAP/PCAPNG válidos e cria o inventário da coleta."""

    captures = []
    inventory = []
    paths = sorted(set(directory.rglob("*.pcap")) | set(directory.rglob("*.pcapng")))
    for path in paths:
        parsed_name = parse_capture_name(path)
        if parsed_name is None:
            inventory.append(
                {
                    "file": str(path),
                    "status": "ignorado",
                    "detail": "nome não contém -BasicNetwork.drone[n]/team[n]",
                }
            )
            continue

        configuration, run, node = parsed_name
        try:
            events = load_capture(path, node)
        except (OSError, ValueError, struct.error) as error:
            inventory.append(
                {"file": str(path), "status": "erro", "detail": str(error)}
            )
            continue

        interface_ips = [
            event["interface_ip"] for event in events if event.get("interface_ip")
        ]
        if not interface_ips:
            inventory.append(
                {
                    "file": str(path),
                    "status": "ignorado",
                    "detail": "IP da interface não encontrado no PCAPNG",
                }
            )
            continue
        interface_ip = Counter(interface_ips).most_common(1)[0][0]
        captures.append(
            Capture(path, configuration, run, node, interface_ip, events)
        )
        inventory.append(
            {
                "file": str(path),
                "configuration": configuration,
                "run": run,
                "node": node,
                "interface_ip": interface_ip,
                "udp_events": len(events),
                "status": "processado",
                "detail": "",
            }
        )
    return captures, inventory


def compare_group(captures: list[Capture]) -> list[dict]:
    """Compara todas as transmissões de uma configuração/execução."""

    by_ip = {capture.interface_ip: capture for capture in captures}
    inbound = {
        capture.node: Counter(
            packet_key(event)
            for event in capture.events
            if event["direction"] == "inbound"
        )
        for capture in captures
    }
    comparisons = []

    for source in captures:
        outgoing = [
            event for event in source.events if event["direction"] == "outbound"
        ]
        for event in outgoing:
            if is_group_destination(event["destination_ip"]):
                destinations: list[Capture | None] = [
                    capture for capture in captures if capture.node != source.node
                ]
            else:
                destinations = [by_ip.get(event["destination_ip"])]

            # Mantém na auditoria unicasts cujo destino não possui captura.
            if not destinations:
                destinations = [None]

            for destination in destinations:
                key = packet_key(event)
                delivered = (
                    destination is not None and inbound[destination.node][key] > 0
                )
                if delivered:
                    inbound[destination.node][key] -= 1
                comparisons.append(
                    {
                        "configuration": source.configuration,
                        "run": source.run,
                        "message_type": event["message_type"],
                        "metric_scope": (
                            "broadcast_reception_opportunity"
                            if is_group_destination(event["destination_ip"])
                            else "unicast_packet_delivery"
                        ),
                        "expected_receiver_basis": (
                            "all_other_captured_nodes"
                            if is_group_destination(event["destination_ip"])
                            else "destination_ip"
                        ),
                        "identity_method": packet_identity_method(event),
                        "source_node": source.node,
                        "source_ip": source.interface_ip,
                        "destination_node": (
                            destination.node
                            if destination is not None
                            else "captura ausente"
                        ),
                        "destination_ip": event["destination_ip"],
                        "ip_id": event["ip_id"],
                        "source_port": event["source_port"],
                        "destination_port": event["destination_port"],
                        "udp_payload_bytes": event["udp_payload_bytes"],
                        "wire_magic": event.get("wire_magic", ""),
                        "wire_version": event.get("wire_version", ""),
                        "message_id": event.get("message_id", ""),
                        "received_message_id": event.get(
                            "received_message_id", ""
                        ),
                        "alert_id": event.get("alert_id", ""),
                        "sequence_number": event.get("sequence_number", ""),
                        "attempt_number": event.get("attempt_number", ""),
                        "sent_time_seconds": event["time_seconds"],
                        "received": "Sim" if delivered else "Não",
                        "lost": 0 if delivered else 1,
                    }
                )
    return comparisons


def summarize_by_link(comparisons: pd.DataFrame) -> pd.DataFrame:
    """Calcula métricas por configuração, execução, tipo e enlace."""

    columns = [
        "configuration", "run", "message_type", "metric_scope", "source_node",
        "destination_node", "sent", "received", "lost", "pdr", "loss_rate",
    ]
    if comparisons.empty:
        return pd.DataFrame(columns=columns)
    group_columns = columns[:6]
    summary = comparisons.groupby(group_columns, as_index=False).agg(
        sent=("lost", "size"), lost=("lost", "sum")
    )
    summary["received"] = summary["sent"] - summary["lost"]
    summary["pdr"] = summary["received"] / summary["sent"]
    summary["loss_rate"] = summary["lost"] / summary["sent"]
    return summary[columns]


def summarize_runs(comparisons: pd.DataFrame) -> pd.DataFrame:
    """Calcula métricas agregadas por configuração, execução e mensagem."""

    columns = [
        "configuration", "run", "message_type", "metric_scope", "sent",
        "received", "lost", "pdr", "loss_rate",
    ]
    if comparisons.empty:
        return pd.DataFrame(columns=columns)
    summary = comparisons.groupby(columns[:4], as_index=False).agg(
        sent=("lost", "size"), lost=("lost", "sum")
    )
    summary["received"] = summary["sent"] - summary["lost"]
    summary["pdr"] = summary["received"] / summary["sent"]
    summary["loss_rate"] = summary["lost"] / summary["sent"]
    return summary[columns]


def summarize_configurations(comparisons: pd.DataFrame) -> pd.DataFrame:
    """Agrega execuções sem misturar configurações experimentais distintas."""

    columns = [
        "configuration", "message_type", "metric_scope", "sent", "received",
        "lost", "pdr", "loss_rate",
    ]
    if comparisons.empty:
        return pd.DataFrame(columns=columns)
    summary = comparisons.groupby(columns[:3], as_index=False).agg(
        sent=("lost", "size"), lost=("lost", "sum")
    )
    summary["received"] = summary["sent"] - summary["lost"]
    summary["pdr"] = summary["received"] / summary["sent"]
    summary["loss_rate"] = summary["lost"] / summary["sent"]
    return summary[columns]


def main() -> None:
    """Descobre as capturas e grava todas as visões na planilha consolidada."""

    parser = argparse.ArgumentParser(
        description="Consolida automaticamente os PCAPNG do ECHOSAR-Net."
    )
    parser.add_argument(
        "directory", type=Path, nargs="?", default=Path("simulations/results"),
        help="diretório pesquisado recursivamente (padrão: simulations/results)",
    )
    parser.add_argument(
        "-o", "--output", type=Path,
        default=Path("simulations/results/metricas-rede.xlsx"),
        help="planilha XLSX de saída",
    )
    parser.add_argument(
        "--configuration",
        help="processa somente esta configuração; demais arquivos ficam no inventário",
    )
    parser.add_argument(
        "--run",
        help="processa somente esta execução (por exemplo, 0)",
    )
    args = parser.parse_args()

    captures, inventory = discover_captures(args.directory)
    captures = [
        capture for capture in captures
        if (args.configuration is None or capture.configuration == args.configuration)
        and (args.run is None or capture.run == args.run)
    ]
    grouped: dict[tuple[str, str], list[Capture]] = {}
    for capture in captures:
        grouped.setdefault((capture.configuration, capture.run), []).append(capture)

    comparisons = []
    for group_captures in grouped.values():
        comparisons.extend(compare_group(group_captures))

    events = []
    for capture in captures:
        for event in capture.events:
            events.append(
                {
                    "configuration": capture.configuration,
                    "run": capture.run,
                    "node": capture.node,
                    **event,
                }
            )

    comparisons_df = pd.DataFrame(comparisons)
    events_df = pd.DataFrame(events)
    inventory_df = pd.DataFrame(inventory)
    links_df = summarize_by_link(comparisons_df)
    runs_df = summarize_runs(comparisons_df)
    general_df = summarize_configurations(comparisons_df)
    methodology_df = pd.DataFrame(
        [
            {
                "item": "Identidade primária",
                "definition": (
                    "Campos ECHO v1: tipo, messageId/receivedMessageId, "
                    "sequência e tentativa; IPs preservam o fluxo"
                ),
            },
            {
                "item": "Fallback",
                "definition": (
                    "Cabeçalhos IP/UDP e tamanho apenas para tráfego sem ECHO "
                    "ou capturas legadas"
                ),
            },
            {
                "item": "Unicast",
                "definition": "uma transmissão esperada na captura do IP de destino",
            },
            {
                "item": "Broadcast/multicast",
                "definition": (
                    "uma oportunidade de recepção por cada outro nó capturado "
                    "na mesma configuração e execução"
                ),
            },
            {
                "item": "Limite da métrica broadcast",
                "definition": (
                    "mede alcance entre nós capturados; não afirma que todos "
                    "eram vizinhos de rádio ou destinatários funcionais"
                ),
            },
            {
                "item": "PDR e perda",
                "definition": "PDR=recebidos/enviados; perda=perdidos/enviados",
            },
        ]
    )

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with pd.ExcelWriter(args.output, engine="openpyxl") as writer:
        general_df.to_excel(writer, sheet_name="Resumo geral", index=False)
        runs_df.to_excel(writer, sheet_name="Por execução", index=False)
        links_df.to_excel(writer, sheet_name="Por enlace", index=False)
        comparisons_df.to_excel(writer, sheet_name="Comparação", index=False)
        events_df.to_excel(writer, sheet_name="Eventos", index=False)
        inventory_df.to_excel(writer, sheet_name="Inventário", index=False)
        methodology_df.to_excel(writer, sheet_name="Metodologia", index=False)
    format_workbook(args.output)

    print(f"Capturas processadas: {len(captures)}")
    print(f"Grupos configuração/execução: {len(grouped)}")
    print(f"Planilha criada: {args.output}")
    if not general_df.empty:
        print(general_df.to_string(index=False))


if __name__ == "__main__":
    main()
