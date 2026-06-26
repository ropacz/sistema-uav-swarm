import inet.common.packet.chunk.Chunk;

namespace echosar;

class ProbeAckChunk extends inet::FieldsChunk
{
    string droneId;
    string teamId;
    simtime_t sentAt;   // sentAt do DroneStatus original — chave de correlação ARR
}
