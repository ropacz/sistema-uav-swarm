import inet.common.packet.chunk.Chunk;

namespace echosar;

class TeamUpdateChunk extends inet::FieldsChunk
{
    string teamId;
    string messageId;
    int64 sequenceNumber;
    double positionX;
    double positionY;
    double positionZ;
    simtime_t timestamp;
}
