#pragma once
namespace echosar {
    static const int ALERT_PORT        = 5000;  // VictimAlert  drone  → equipe
    static const int TEAM_UPDATE_PORT  = 5001;  // TeamUpdate   equipe → broadcast
    static const int ACK_PORT          = 5002;  // VictimAck    equipe → drone origem
    static const int DRONE_STATUS_PORT = 5003;  // DroneStatus  drone  → equipe
    static const int RELAY_PORT        = 5004;  // VictimAlert  drone  → drone (relay)
    static const int PROBE_ACK_PORT   = 5005;  // ProbeAck     equipe → drone (ARR + validação)
}
