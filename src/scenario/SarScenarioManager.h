#pragma once

#include <map>
#include <string>

#include "omnetpp.h"

namespace echosar {

class SarScenarioManager : public omnetpp::cSimpleModule
{
  protected:
    std::map<omnetpp::cMessage *, omnetpp::cModule *> detections;
    /// Cancela eventos de detecção que ainda não ocorreram.
    virtual ~SarScenarioManager();
    /// Solicita inicialização em estágios compatível com os módulos INET.
    virtual int numInitStages() const override;
    /// Valida IDs e agenda o evento abstrato de detecção de cada vítima.
    virtual void initialize(int stage) override;
    /// Seleciona o drone mais próximo e envia diretamente a associação da vítima.
    virtual void handleMessage(omnetpp::cMessage *message) override;
};

} // namespace echosar
