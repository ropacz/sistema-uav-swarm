#pragma once

#include <cstdint>
#include <deque>
#include <string>

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp/simtime.h"

namespace echosar {

/// Uma recepção direta de TeamUpdate. Guarda a distância medida no instante da
/// recepção porque tanto o drone quanto a equipe se movem: recalculá-la depois,
/// com as posições atuais, compararia o RSSI a uma geometria que já mudou.
struct RssiSample {
    omnetpp::simtime_t receptionTime;
    double rssiDbm = 0;
    double distanceMeters = 0;
};

/**
 * Entrada temporária aprendida exclusivamente pelos broadcasts TeamUpdate.
 * Não representa um diretório pré-configurado de equipes.
 */
struct TeamLinkState {
    std::string ipAddress;
    inet::Coord position;
    omnetpp::simtime_t lastUpdateTime = -1;
    int64_t lastSequence = -1;
    /// A entrada operacional expirou, mas a última posição continua retida
    /// para o mecanismo de recuperação. Não serve para transmissão normal.
    bool stale = false;

    /// Última recepção vinda da própria equipe (hopCount zero). Repasses de
    /// outros drones atualizam a posição, mas não renovam este instante.
    omnetpp::simtime_t lastDirectUpdateTime = -1;
    /// A equipe já foi ouvida por enlace direto alguma vez. Separa "nunca
    /// houve enlace direto" de "havia e parou", que é o que indica obstrução.
    bool hasDirectReception = false;
    /// Janela deslizante das recepções diretas, usada na atenuação excedente.
    std::deque<RssiSample> rssiSamples;
    /// Sub-condições e resultado de S_ij retidos entre avaliações. O timer de
    /// manutenção reavalia a cada maintenanceInterval; sem o valor anterior os
    /// contadores mediriam avaliações em vez de episódios.
    bool rssiDegraded = false;
    bool directUpdateTimedOut = false;
    bool possibleObstruction = false;
};

} // namespace echosar
