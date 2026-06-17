import inet.common.packet.chunk.Chunk;

namespace echosar;

class TeamUpdateChunk extends inet::FieldsChunk
{
    string teamId;
    string ipAddress;
    double posX;        // coordenada cartesiana X no playground (m)
    double posY;        // coordenada cartesiana Y no playground (m)
    bool available;
}
