import inet.common.packet.chunk.Chunk;

namespace echosar;

class PositionUpdateChunk extends inet::FieldsChunk
{
    string messageId;
    string senderId;
    string senderType;
    string ipAddress;
    int waypointId;
    double positionX;
    double positionY;
    double positionZ;
    int64 sequenceNumber;
    simtime_t timestamp;
    string operationalState;
}
