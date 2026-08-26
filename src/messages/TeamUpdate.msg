import inet.common.packet.chunk.Chunk;

namespace echosar;

class TeamUpdateChunk extends inet::FieldsChunk
{
    string teamId;
    string messageId;
    int64 sequenceNumber;
    double positionX;
    double positionY;
    double positionZ;
    simtime_t timestamp;
    // Endereço da equipe observado no primeiro salto; preservado nos repasses
    // para que drones distantes possam endereçar o alerta em unicast.
    string teamAddress;
    // Saltos de aplicação já percorridos. A equipe emite com zero.
    int hopCount = 0;
}
