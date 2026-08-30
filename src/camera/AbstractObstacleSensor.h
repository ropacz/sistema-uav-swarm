#pragma once

#include <cmath>
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
    /// Distância drone-superfície, quando há obstáculo observado.
    double distance = NAN;
    /// Desfecho da inspeção. Distingue os dois sentidos de "sem obstáculo":
    /// `clearToTarget` (o segmento inteiro até o alvo foi observado e está
    /// livre) e `clearWithinSensorRange` (só o trecho dentro do alcance foi
    /// observado; o restante permanece desconhecido, não "livre").
    std::string reason;
};

class AbstractObstacleSensor : public omnetpp::cSimpleModule
{
  protected:
    inet::ModuleRefByPar<inet::physicalenvironment::IPhysicalEnvironment> environment;
    double minimumRange = 0.7;
    /// Alcance máximo do modelo visual (padrão: 30 m, Phantom 4 Pro).
    /// Negativo desabilita o limite — modo oráculo geométrico idealizado,
    /// disponível para análises que queiram isolar o mecanismo do funil de
    /// detecção (ver docs/desvios_e_extensoes.md, D4), mas não é mais o
    /// comportamento padrão.
    double maximumRange = 30;

    /// Obtém o ambiente físico e valida os parâmetros configurados.
    ///
    /// `fieldOfViewHorizontal`, `fieldOfViewVertical` e `measurementFrequency`
    /// são validados aqui mas não são guardados: documentam a câmera de
    /// referência do Phantom 4 Pro e ainda não recortam a linha de visada
    /// avaliada (ver docs/desvios_e_extensoes.md, D4 e a lacuna L2). Viram
    /// membros quando o campo de visão passar a ser aplicado de fato.
    virtual void initialize() override;
    /// Descarta mensagens porque o sensor é consultado sincronamente por inspect().
    virtual void handleMessage(omnetpp::cMessage *message) override { delete message; }

  public:
    /// Inspeciona a LOS entre o drone e um alvo (equipe, candidato do BA ou
    /// qualquer outro ponto) e descreve o primeiro obstáculo observável. No
    /// modo sensor físico (maximumRange >= 0), o segmento avaliado é
    /// truncado no alcance máximo: nada além dele é visível à câmera.
    ObstacleObservation inspect(const inet::Coord& dronePosition,
                                const inet::Coord& inspectionTarget) const;

    /// **Não é uma capacidade da câmera.** Predicado geométrico idealizado do
    /// simulador ("ground truth"): responde se o segmento informado cruza
    /// algum objeto físico, sem limite de alcance e sem campo de visão.
    ///
    /// Existe para avaliar a viabilidade de posições candidatas durante o
    /// reposicionamento — uso que o §14 da diretriz autoriza explicitamente,
    /// desde que declarado como "conhecimento geométrico idealizado
    /// disponibilizado ao mecanismo de reposicionamento" (ver
    /// docs/desvios_e_extensoes.md, D4 e E4). Não use para detecção: essa
    /// respeita o alcance físico e passa por inspect().
    bool intersectsAnyObstacleGroundTruth(const inet::Coord& a, const inet::Coord& b) const;
};

} // namespace echosar
