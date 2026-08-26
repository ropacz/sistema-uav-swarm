#pragma once

#include <map>
#include <string>

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp/simtime.h"

namespace echosar {

/** Estado de entrega e recuperação mantido para cada alerta ainda sem ACK. */
struct PendingVictimAlert {
    std::string alertId;
    std::string victimId;
    inet::Coord victimPosition;
    omnetpp::simtime_t creationTime;
    omnetpp::simtime_t ackDeadline = -1;
    omnetpp::simtime_t nextAttempt = -1;
    int attempts = 0;
    std::string targetTeamId;
    // Destino imutável de cada tentativa. A validação do ACK não pode depender
    // de a entrada dinâmica da equipe ainda existir quando a resposta chegar.
    std::map<std::string, std::string> attemptTeamIds;
    std::map<std::string, std::string> attemptTeamAddresses;
    // O limiar de tentativas sem ACK produz uma única decisão por alerta.
    bool repositionDecisionMade = false;
    inet::Coord repositionOrigin;
};

} // namespace echosar
