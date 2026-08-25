import inet.common.packet.chunk.Chunk;

namespace echosar;

class VictimAlertChunk extends inet::FieldsChunk
{
    string alertId;
    string messageId;
    string victimId;
    string originDroneId;
    double victimPositionX;
    double victimPositionY;
    double victimPositionZ;
    int attemptNumber;
    simtime_t generationTimestamp;
    simtime_t timeToLive;
}
