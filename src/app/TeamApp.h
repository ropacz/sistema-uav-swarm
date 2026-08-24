#pragma once

#include <set>
#include <string>

#include "inet/applications/base/ApplicationBase.h"
#include "inet/common/lifecycle/LifecycleOperation.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "messages/PositionUpdate_m.h"
#include "messages/VictimAlert_m.h"
#include "messages/VictimAck_m.h"

namespace echosar {

class TeamApp : public inet::ApplicationBase, public inet::UdpSocket::ICallback
{
  protected:
    inet::UdpSocket socket;
    omnetpp::cMessage *updateTimer = nullptr;
    std::string teamId;
    std::string ipAddress;
    omnetpp::simtime_t updateInterval;
    omnetpp::simtime_t initialJitter;
    omnetpp::simtime_t ackStartTime;
    int appPort = 5000;
    int64_t positionUpdatePayloadBytes = 160;
    int64_t victimAckPayloadBytes = 96;
    int64_t updateSequence = 0;
    std::set<std::string> attendedAlerts;
    std::set<std::string> receivedAttempts;

    int positionUpdatesSent = 0;
    int uniqueAlertsReceived = 0;
    int attemptsReceived = 0;
    int duplicatePackets = 0;
    int applicationAcksSent = 0;
    omnetpp::simtime_t totalDeliveryDelay = 0;
    omnetpp::simtime_t totalAlertAgeAtReception = 0;
    omnetpp::simsignal_t deliveryDelaySignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t hopCountSignal = SIMSIGNAL_NULL;
    omnetpp::simsignal_t alertDeliveredSignal = SIMSIGNAL_NULL;

    /// Cancela o timer periódico e encerra o socket da aplicação.
    virtual ~TeamApp();
    /// Informa ao OMNeT++ os estágios de inicialização utilizados pelo INET.
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    /// Valida parâmetros e configura identidade, socket UDP e timer de posição.
    virtual void initialize(int stage) override;
    /// Processa o timer de PositionUpdate e as indicações recebidas pelo socket.
    virtual void handleMessageWhenUp(omnetpp::cMessage *message) override;
    /// Registra contadores e tempos acumulados como escalares OMNeT++.
    virtual void finish() override;
    /// Hooks vazios: os experimentos não param nem reiniciam a aplicação em runtime.
    virtual void handleStartOperation(inet::LifecycleOperation *) override {}
    virtual void handleStopOperation(inet::LifecycleOperation *) override {}
    virtual void handleCrashOperation(inet::LifecycleOperation *) override {}
    /// Entrega pacotes VictimAlert ao tratador e descarta tipos desconhecidos.
    virtual void socketDataArrived(inet::UdpSocket *, inet::Packet *packet) override;
    /// Consome indicações de erro e fechamento exigidas pelo callback UDP.
    virtual void socketErrorArrived(inet::UdpSocket *, inet::Indication *indication) override { delete indication; }
    virtual void socketClosed(inet::UdpSocket *) override {}

    /// Publica a posição atual da equipe por broadcast UDP de um salto.
    void sendPositionUpdate();
    /// Valida e deduplica o alerta, registra métricas e envia VictimAck.
    void handleVictimAlert(inet::Packet *packet);
};

} // namespace echosar
