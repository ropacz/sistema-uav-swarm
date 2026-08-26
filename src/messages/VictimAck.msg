import inet.common.packet.chunk.Chunk;

namespace echosar;

class VictimAckChunk extends inet::FieldsChunk
{
    string alertId;
    string messageId;
    string victimId;
    string sourceDroneId;
    string teamId;
    simtime_t ackTimestamp;
}
