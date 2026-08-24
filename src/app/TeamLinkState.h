#pragma once

#include <cmath>
#include <cstdint>
#include <deque>
#include <string>

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp/simtime.h"

namespace echosar {

/** Amostra recebida de um broadcast periódico de posição da equipe. */
struct LinkSample {
    int64_t sequence = 0;
    omnetpp::simtime_t receptionTime;
    double rssiDbm = NAN;
};

/**
 * Entrada temporária aprendida exclusivamente pelos broadcasts PositionUpdate.
 * Não representa um diretório pré-configurado de equipes.
 */
struct TeamLinkState {
    std::string ipAddress;
    inet::Coord position;
    inet::Coord velocity;
    omnetpp::simtime_t positionTime = -1;
    omnetpp::simtime_t lastSeen = -1;
    int64_t lastSequence = -1;
    bool velocityValid = false;
    // Início da observação local. Antes deste instante, beacons ausentes não são
    // perdas observáveis e não devem reduzir o PDR de uma equipe recém-descoberta.
    omnetpp::simtime_t observationStart = -1;
    std::deque<LinkSample> samples;
};

} // namespace echosar
