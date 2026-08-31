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
    // O limiar de tentativas sem ACK produz uma única decisão por alerta. A
    // decisão só é registrada quando é definitiva: uma recusa temporária
    // (equipe momentaneamente não retida, drone ocupado com outro alerta) deixa
    // o alerta elegível para tentar de novo no próximo ciclo sem ACK.
    bool repositionDecisionMade = false;
    // O gatilho é contabilizado uma única vez, mesmo que a decisão exija várias
    // oportunidades até ser definitiva.
    bool repositionTriggerRecorded = false;
    inet::Coord repositionOrigin;
    // Miniatura capturada uma única vez, no início do ciclo. Identificador e
    // instante ficam fixos para que toda tentativa reenvie a mesma imagem, e
    // não uma foto nova a cada retransmissão. Vazio quando a captura está
    // desabilitada.
    std::string photoId;
    omnetpp::simtime_t photoCaptureTime = -1;
    // Sondagem de recuperação em curso. Enquanto pendente, o alerta não é
    // transmitido: o objetivo é justamente não gastar o pacote pesado antes de
    // saber se o reposicionamento restabeleceu o enlace.
    bool probePending = false;
    std::string probeId;
    int probeAttempts = 0;
    omnetpp::simtime_t probeDeadline = -1;
};

} // namespace echosar
