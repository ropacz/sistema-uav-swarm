#pragma once

#include <cstdint>
#include <string>

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp/simtime.h"

namespace echosar {

/** Vítima que continua ativa e pode originar alertas lógicos periódicos. */
struct ActiveVictim {
    std::string victimId;
    inet::Coord position;
    omnetpp::simtime_t nextAlertTime;
    int64_t alertSequence = 0;
    std::string pendingAlertId;
};

} // namespace echosar
