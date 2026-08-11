#!/usr/bin/env python3
"""Parseia log Cmdenv (--msglog/--info) em eventos de troca de mensagens.

Lê linhas "[DRONE x] ..." / "[TEAM y] ..." emitidas via EV_INFO/EV_WARN em
SimpleDroneApp.cc e SimpleTeamApp.cc e escreve CSV em formato longo (1 linha
por evento), com o simtime resolvido a partir do cabeçalho de evento mais
recente ("** Event #N  t=<time> ..."), já que a linha de log em si não traz
timestamp.

Uso:
  ./run.sh --msglog -c BasicTest_Piloto -r 0 > /tmp/run.log
  python3 analysis/parse_message_log.py /tmp/run.log > /tmp/messages.csv

Ou via pipe:
  ./run.sh --msglog -c BasicTest_Piloto -r 0 | python3 analysis/parse_message_log.py > /tmp/messages.csv

Ver analysis/export_messages_parquet.sh para converter direto pra parquet.
"""
import csv
import re
import sys

EVENT_RE = re.compile(r'^\*\* Event #\d+\s+t=([0-9.eE+-]+)\s')
LOG_RE = re.compile(r'^\[(?:INFO|WARN)\]\t(.*)$')
ANSI_RE = re.compile(r'\x1b\[[0-9;]*m')

FIELDS = ["time", "event", "drone_id", "team_id", "msg_id", "peer_id",
          "ip", "x", "y", "z", "delay", "retries", "max_retries"]

# (nome_evento, regex, campos capturados na ordem dos grupos)
PATTERNS = [
    ("team_table_update", re.compile(
        r'^\[DRONE (\S+)\] tabela: (\S+) ip=(\S+)$'),
     ["drone_id", "team_id", "ip"]),
    ("alert_generated", re.compile(
        r'^\[DRONE (\S+)\] alerta sintético gerado → (\S+)$'),
     ["drone_id", "msg_id"]),
    ("alert_sent_direct", re.compile(
        r'^\[DRONE (\S+)\] VictimAlert (\S+) → (\S+) \((\S+)\)$'),
     ["drone_id", "msg_id", "team_id", "ip"]),
    ("alert_sent_relay", re.compile(
        r'^\[DRONE (\S+)\] relay: (\S+) → broadcast \(sem equipe elegível\)$'),
     ["drone_id", "msg_id"]),
    ("alert_dedup_drone", re.compile(
        r'^\[DRONE (\S+)\] dedup: descartando (\S+)$'),
     ["drone_id", "msg_id"]),
    ("alert_relay_received", re.compile(
        r'^\[DRONE (\S+)\] relay recebido: (\S+) de (\S+)$'),
     ["drone_id", "msg_id", "peer_id"]),
    ("ack_received", re.compile(
        r'^\[DRONE (\S+)\] VictimAck recebido para (\S+) de (\S+)$'),
     ["drone_id", "msg_id", "team_id"]),
    ("alert_expired", re.compile(
        r'^\[DRONE (\S+)\] store-forward: descartando (\S+) após (\d+) tentativas$'),
     ["drone_id", "msg_id", "max_retries"]),
    ("retry", re.compile(
        r'^\[DRONE (\S+)\] store-forward: retry (\d+)/(\d+) para (\S+)$'),
     ["drone_id", "retries", "max_retries", "msg_id"]),
    ("team_timeout", re.compile(
        r'^\[DRONE (\S+)\] timeout: removendo (\S+)$'),
     ["drone_id", "team_id"]),

    ("team_update_sent", re.compile(
        r'^\[TEAM (\S+)\] TeamUpdate broadcast \(ip=(\S+)\)$'),
     ["team_id", "ip"]),
    ("drone_status_received", re.compile(
        r'^\[TEAM (\S+)\] DroneStatus de (\S+) pos=\(([-0-9.]+),([-0-9.]+),([-0-9.]+)\) '
        r'RTT=([0-9.eE+-]+)s$'),
     ["team_id", "peer_id", "x", "y", "z", "delay"]),
    ("alert_dedup_team", re.compile(
        r'^\[TEAM (\S+)\] dedup: (\S+) — reenviando ACK idempotente$'),
     ["team_id", "msg_id"]),
    ("alert_received", re.compile(
        r'^\[TEAM (\S+)\] \*\*\* ALERTA de (\S+) msgId=(\S+) vitima em '
        r'\(([-0-9.]+),([-0-9.]+)\) delay=([0-9.eE+-]+)s$'),
     ["team_id", "peer_id", "msg_id", "x", "y", "delay"]),
    ("ack_sent", re.compile(
        r'^\[TEAM (\S+)\] VictimAck (\S+) → drone origem (\S+)$'),
     ["team_id", "msg_id", "ip"]),
    ("unexpected_packet", re.compile(
        r'^\[TEAM (\S+)\] pacote inesperado descartado$'),
     ["team_id"]),
]


def parse(lines):
    t = 0.0
    for raw in lines:
        line = ANSI_RE.sub('', raw.rstrip('\n'))

        m = EVENT_RE.match(line)
        if m:
            t = float(m.group(1))
            continue

        m = LOG_RE.match(line)
        if not m:
            continue
        msg = m.group(1)
        if not (msg.startswith('[DRONE') or msg.startswith('[TEAM')):
            continue

        for name, rx, fields in PATTERNS:
            mm = rx.match(msg)
            if mm:
                row = {f: None for f in FIELDS}
                row["time"] = t
                row["event"] = name
                for field, value in zip(fields, mm.groups()):
                    row[field] = value
                yield row
                break
        else:
            sys.stderr.write(f"WARN: linha não reconhecida: {msg!r}\n")


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else None
    src = open(path, encoding='utf-8', errors='replace') if path else sys.stdin

    writer = csv.DictWriter(sys.stdout, fieldnames=FIELDS)
    writer.writeheader()
    n = 0
    for row in parse(src):
        writer.writerow(row)
        n += 1

    if path:
        src.close()
    sys.stderr.write(f">>> {n} eventos parseados\n")


if __name__ == "__main__":
    main()
