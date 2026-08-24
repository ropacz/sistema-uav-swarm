#pragma once

#include <cmath>
#include <cstdint>
#include <map>
#include <set>
#include <string>

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp/simtime.h"

namespace echosar {

/** Estado de entrega e recuperação mantido para cada alerta ainda sem ACK. */
struct PendingVictimAlert {
    std::string alertId;
    std::string victimId;
    inet::Coord victimPosition;
    omnetpp::simtime_t generationTime;
    omnetpp::simtime_t ackDeadline = -1;
    omnetpp::simtime_t nextAttempt = -1;
    int attempts = 0;
    int64_t sequence = 0;
    std::string targetTeamId;
    std::map<std::string, omnetpp::simtime_t> attemptSentTimes;
    // Destino imutável de cada tentativa. A validação do ACK não pode depender
    // de a entrada dinâmica da equipe ainda existir quando a resposta chegar.
    std::map<std::string, std::string> attemptTeamIds;
    std::map<std::string, std::string> attemptTeamAddresses;
    bool degradationEvaluated = false;
    int baCycles = 0;
    std::set<std::string> testedPositions;
    // Cada movimento iniciado recebe uma identidade própria. Isso impede que um
    // ACK tardio da posição anterior seja atribuído ao candidato corrente.
    int repositionCycle = 0;
    std::string activeRepositionCycleId;
    std::string validationMessageId;
    std::string validationCycleId;
    omnetpp::simtime_t repositionStart = -1;
    inet::Coord repositionOrigin;
    bool repositionDistanceRecorded = false;
    double preRepositionPdr = NAN;
    double preRepositionRssi = NAN;
};

} // namespace echosar
