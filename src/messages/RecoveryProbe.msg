import inet.common.packet.chunk.Chunk;

namespace echosar;

/// Sondagem leve enviada após um reposicionamento, antes de gastar a
/// transmissão do VictimAlert completo. Um único formato serve à ida e à
/// volta, como no eco do ICMP: a equipe devolve o mesmo probeId com
/// reply=true. Manter um só tipo evita duplicar a validação de identidade em
/// dois codecs para um par de mensagens que sempre andam juntas.
class RecoveryProbeChunk extends inet::FieldsChunk
{
    string probeId;
    // O alerta que aguarda o resultado da sondagem. É o que permite ao drone
    // reencontrar o alerta certo quando a resposta chega.
    string alertId;
    string sourceDroneId;
    string targetTeamId;
    // Instante da ida, copiado na volta. Mede o RTT sem exigir que a equipe
    // tenha relógio comum com o drone.
    simtime_t sendTime;
    bool reply;
}
