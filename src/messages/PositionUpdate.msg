import inet.common.packet.chunk.Chunk;

namespace echosar;

class PositionUpdateChunk extends inet::FieldsChunk
{
    string senderId;
    double positionX;
    double positionY;
    double positionZ;
    int64 sequenceNumber;
}
