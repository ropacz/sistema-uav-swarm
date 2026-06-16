import inet.common.packet.chunk.Chunk;

namespace echosar;

class VictimAlertChunk extends inet::FieldsChunk
{
    string droneId;
    string msgId;
    string originIp;  // IP do drone que originou o alerta (rota o ACK de volta ao criador)
    double lat;
    double lon;
    simtime_t sentAt;
}
