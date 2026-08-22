#pragma once

#include "inet/common/geometry/common/Coord.h"
#include "omnetpp.h"

namespace echosar {

class AbstractObstacleSensor;

/// Parâmetros da aptidão e das restrições de viabilidade do reposicionamento.
/// Todos vêm da configuração e permanecem constantes durante uma ativação.
struct FitnessParameters {
    // Pesos da soma ponderada; devem ser não negativos e somar 1.
    double wLink = 0.60;
    double wObstacle = 0.25;
    double wMove = 0.15;
    // Escala do decaimento exponencial da penalidade de proximidade.
    double obstacleSigma = 10;
    // Distância mínima entre um candidato e a superfície detectada.
    double obstacleSafetyMargin = 1;
    // Distância de referência para normalizar o custo de enlace.
    double linkNormalizationDistance = 1000;
    // Raio da região de busca ao redor da posição atual.
    double maximumRepositionDistance = 25;
    double areaMinX = 0, areaMaxX = 1000, areaMinY = 0, areaMaxY = 1000;
    double minimumAltitude = 6, maximumAltitude = 20;
    double horizontalSpeed = 13, climbSpeed = 5, descentSpeed = 3;
    omnetpp::simtime_t flightTimeLimit = 1800;
};

/// Avalia posições candidatas para uma ativação do Bat Algorithm.
///
/// Separada da aplicação porque responde a uma pergunta própria — quanto custa
/// e se é alcançável uma posição — e porque assim pode ser exercitada sem uma
/// pilha de rede montada. A geometria vem do sensor; a aptidão nunca consulta
/// o rádio nem conhece o RSSI futuro.
class RepositionFitness
{
  public:
    RepositionFitness(const FitnessParameters& parameters,
                      const AbstractObstacleSensor *sensor,
                      const inet::Coord& current,
                      const inet::Coord& teamPosition,
                      const inet::Coord& obstaclePoint,
                      omnetpp::simtime_t now,
                      bool useObstacleModel = true);

    /// Custo normalizado do candidato: enlace, obstáculo e deslocamento.
    double cost(const inet::Coord& candidate) const;
    /// Limites de área, altitude, alcance, margem, trajeto livre e autonomia.
    bool feasible(const inet::Coord& candidate) const;
    /// Tempo de percurso entre dois pontos, com o eixo mais lento decidindo.
    double travelTime(const inet::Coord& from, const inet::Coord& to) const;

  private:
    const FitnessParameters& parameters;
    const AbstractObstacleSensor *sensor;
    bool useObstacleModel;
    inet::Coord current;
    inet::Coord teamPosition;
    inet::Coord obstaclePoint;
    omnetpp::simtime_t now;
};

} // namespace echosar
