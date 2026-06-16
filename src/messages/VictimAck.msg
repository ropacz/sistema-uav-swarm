import inet.common.packet.chunk.Chunk;

namespace echosar;

class VictimAckChunk extends inet::FieldsChunk
{
    string msgId;    // msgId do VictimAlert confirmado
    string teamId;
    simtime_t sentAt;
}
