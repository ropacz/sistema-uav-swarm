import inet.common.packet.chunk.Chunk;

namespace echosar;

class TeamUpdateChunk extends inet::FieldsChunk
{
    string teamId;
    string ipAddress;
    double lat;
    double lon;
    bool available;
}
