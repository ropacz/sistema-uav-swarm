import inet.common.packet.chunk.Chunk;

namespace echosar;

class VictimAlertChunk extends inet::FieldsChunk
{
    string alertId;
    string messageId;
    string victimId;
    string originDroneId;
    string originDroneAddress;
    double victimPositionX;
    double victimPositionY;
    double victimPositionZ;
    double dronePositionX;
    double dronePositionY;
    double dronePositionZ;
    int waypointId;
    int64 sequenceNumber;
    int attemptNumber;
    simtime_t generationTimestamp;
    simtime_t transmissionTimestamp;
    simtime_t timeToLive;
}
