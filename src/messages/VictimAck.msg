import inet.common.packet.chunk.Chunk;

namespace echosar;

class VictimAckChunk extends inet::FieldsChunk
{
    string alertId;
    string receivedMessageId;
    string victimId;
    string teamId;
    string originDroneId;
}
