import inet.common.packet.chunk.Chunk;

namespace echosar;

class VictimAlertChunk extends inet::FieldsChunk
{
    string droneId;
    string msgId;
    string originIp;  // IP do drone que originou o alerta (rota o ACK de volta ao criador)
    double posX;      // coordenada cartesiana X da vítima no playground (m)
    double posY;      // coordenada cartesiana Y da vítima no playground (m)
    simtime_t sentAt;
}
