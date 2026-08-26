import inet.common.packet.chunk.Chunk;

namespace echosar;

class DroneStatusChunk extends inet::FieldsChunk
{
    string droneId;
    string messageId;
    int64 sequenceNumber;
    double positionX;
    double positionY;
    double positionZ;
    simtime_t timestamp;
}
