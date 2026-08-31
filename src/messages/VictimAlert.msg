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
    // Anexo de imagem: miniatura da vítima capturada junto com o alerta. Um
    // photoId vazio e photoData de tamanho zero descrevem um alerta sem foto,
    // que é o formato usado nos cenários anteriores à câmera.
    string photoId;
    // Dimensões declaradas da miniatura, em pixels. São metadados: quem mede o
    // custo de transmissão usa o tamanho de photoData, não estes campos.
    uint16_t photoWidth;
    uint16_t photoHeight;
    simtime_t photoCaptureTime;
    // Conteúdo opaco da imagem. O modelo não decodifica a miniatura; ela existe
    // para que o alerta ocupe no enlace os bytes que uma foto real ocuparia.
    uint8_t photoData[];
}
