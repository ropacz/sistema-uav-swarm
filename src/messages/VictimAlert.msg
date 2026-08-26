import inet.common.packet.chunk.Chunk;

namespace echosar;

class VictimAlertChunk extends inet::FieldsChunk
{
    string alertId;
    string messageId;
    string victimId;
    string sourceDroneId;
    string targetTeamId;
    double victimPositionX;
    double victimPositionY;
    double victimPositionZ;
    simtime_t creationTime;
    int attemptNumber;
    // Extensão operacional: permite ao receptor rejeitar alertas expirados.
    simtime_t timeToLive;
}
