#pragma once

#include <cstdint>

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp/simtime.h"

namespace echosar {

/** Posição de vizinho aprendida exclusivamente por DroneStatus recebido. */
struct DroneLinkState {
    inet::Coord position;
    omnetpp::simtime_t lastUpdateTime = -1;
    int64_t lastSequence = -1;
};

} // namespace echosar
