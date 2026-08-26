#pragma once

#include <cstdint>
#include <string>

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp/simtime.h"

namespace echosar {

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
};

} // namespace echosar
