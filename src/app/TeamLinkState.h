#pragma once

#include <cstdint>
#include <string>

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp/simtime.h"

namespace echosar {

/**
 * Entrada temporária aprendida exclusivamente pelos broadcasts PositionUpdate.
 * Não representa um diretório pré-configurado de equipes.
 */
struct TeamLinkState {
    std::string ipAddress;
    inet::Coord position;
    omnetpp::simtime_t lastSeen = -1;
    int64_t lastSequence = -1;
};

} // namespace echosar
