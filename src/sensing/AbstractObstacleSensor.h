#pragma once

#include <string>

#include "inet/common/ModuleRefByPar.h"
#include "inet/environment/contract/IPhysicalEnvironment.h"
#include "omnetpp.h"

namespace echosar {

struct ObstacleObservation {
    bool confirmed = false;
    /// Primeiro ponto da superfície a partir do drone: onde a linha de visada
    /// começa a ser bloqueada. É o único ponto que o BA consome.
    inet::Coord nearestSurfacePoint;
    /// Distância drone-superfície. Registrada sempre; só limita a confirmação
    /// quando o sensor tem alcance máximo configurado.
    double distance = NAN;
    /// Motivo da confirmação ou da rejeição, contabilizado pela aplicação.
    std::string reason;
};

class AbstractObstacleSensor : public omnetpp::cSimpleModule
{
  protected:
    inet::ModuleRefByPar<inet::physicalenvironment::IPhysicalEnvironment> environment;
    double minimumRange = 0.7;
    /// Alcance máximo do sensor. Negativo significa sem limite: o mecanismo
    /// científico avalia a obstrução geométrica do enlace inteiro, e não a
    /// faixa de um sensor de prevenção de colisões.
    double maximumRange = -1;

    /// Obtém o ambiente físico e valida os limites de alcance configurados.
    virtual void initialize() override;
    /// Descarta mensagens porque o sensor é consultado sincronamente por inspect().
    virtual void handleMessage(omnetpp::cMessage *message) override { delete message; }

  public:
    /// Inspeciona a LOS drone–equipe e descreve o primeiro obstáculo observável.
    ObstacleObservation inspect(const inet::Coord& dronePosition,
                                const inet::Coord& teamPosition) const;
    /// Retorna se o segmento informado cruza qualquer objeto físico do cenário.
    bool intersectsAnyObstacle(const inet::Coord& a, const inet::Coord& b) const;
};

} // namespace echosar
