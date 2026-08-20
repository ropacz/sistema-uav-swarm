# Formato binário ECHOSAR para auditoria PCAP

As mensagens UDP da aplicação são serializadas em ordem de rede (big endian).
Todo payload começa com o cabeçalho comum:

| Campo | Tamanho | Valor |
|---|---:|---|
| magic | 4 bytes | ASCII `ECHO` |
| version | 1 byte | `1` |
| message type | 1 byte | `1`, `2` ou `3` |
| encoded body length | 2 bytes | tamanho dos campos significativos |

Os códigos de mensagem são:

| Código | Mensagem |
|---:|---|
| 1 | `PositionUpdate` |
| 2 | `VictimAlert` |
| 3 | `VictimAck` |

Strings são codificadas como tamanho `uint16` seguido por UTF-8. Inteiros e
doubles usam big endian; timestamps são doubles em segundos. Após os campos, o
payload recebe zeros até atingir o tamanho experimental configurado.

## Campos auditáveis

- `PositionUpdate`: `messageId`, remetente, IP anunciado, waypoint, posição,
  sequência, timestamp e estado operacional.
- `VictimAlert`: `alertId`, `messageId`, vítima, drone de origem, posições,
  waypoint, sequência, tentativa e timestamps.
- `VictimAck`: `alertId`, mensagem recebida, vítima, equipe, drone de origem e
  timestamps de recepção e confirmação.

O analisador prioriza esse cabeçalho. A identificação por tamanho existe apenas
para ler capturas antigas, anteriores à versão 1 do formato.
