import inet.common.packet.chunk.Chunk;

namespace echosar;

class VictimAlertChunk extends inet::FieldsChunk
{
    string droneId;
    string msgId;
    double lat;
    double lon;
    simtime_t sentAt;
}
