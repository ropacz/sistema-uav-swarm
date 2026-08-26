#pragma once

#include <set>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/TeamUpdate_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAck_m.h"

namespace echosar {

class TeamApp : public inet::ApplicationBase, public inet::UdpSocket::ICallback
{
  protected:
    inet::UdpSocket socket;
    omnetpp::cMessage *updateTimer = nullptr;
    std::string teamId;
    omnetpp::simtime_t updateInterval;
    omnetpp::simtime_t initialJitter;
    omnetpp::simtime_t ackStartTime;
    int appPort = 5000;
    int applicationIpTtl = 32;
    int64_t teamUpdatePayloadBytes = 160;
    int64_t victimAckPayloadBytes = 96;
    int64_t updateSequence = 0;
    std::set<std::string> receivedAttempts;

    omnetpp::simsignal_t alertDeliveredSignal = SIMSIGNAL_NULL;

    /// Cancela o timer periódico e encerra o socket da aplicação.
    virtual ~TeamApp();
    /// Informa ao OMNeT++ os estágios de inicialização utilizados pelo INET.
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    /// Valida parâmetros e configura identidade, socket UDP e timer de posição.
    virtual void initialize(int stage) override;
    /// Processa o timer de TeamUpdate e as indicações recebidas pelo socket.
    virtual void handleMessageWhenUp(omnetpp::cMessage *message) override;
    /// Hooks vazios: os experimentos não param nem reiniciam a aplicação em runtime.
    virtual void handleStartOperation(inet::LifecycleOperation *) override {}
    virtual void handleStopOperation(inet::LifecycleOperation *) override {}
    virtual void handleCrashOperation(inet::LifecycleOperation *) override {}
    /// Entrega pacotes VictimAlert ao tratador e descarta tipos desconhecidos.
    virtual void socketDataArrived(inet::UdpSocket *, inet::Packet *packet) override;
    /// Consome indicações de erro e fechamento exigidas pelo callback UDP.
    virtual void socketErrorArrived(inet::UdpSocket *, inet::Indication *indication) override { delete indication; }
    virtual void socketClosed(inet::UdpSocket *) override {}

    /// Publica a posição atual da equipe por broadcast UDP.
    void sendTeamUpdate();
    /// Valida e deduplica a tentativa, registra a entrega e envia VictimAck.
    void handleVictimAlert(inet::Packet *packet);
};

} // namespace echosar
