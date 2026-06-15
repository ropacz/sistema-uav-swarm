import inet.common.packet.chunk.Chunk;

namespace echosar;

class DroneStatusChunk extends inet::FieldsChunk
{
    string droneId;
    double posX;
    double posY;
    double posZ;
    simtime_t sentAt;
}
