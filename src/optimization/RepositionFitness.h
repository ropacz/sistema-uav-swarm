#pragma once

#include <optional>
#include <vector>

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
    // Distância mínima entre um candidato e a única superfície observada pela
    // câmera. É uma repulsão local em torno desse ponto, não prova de que a
    // trajetória inteira esteja livre — o sensor só enxerga até maximumRange.
    double obstacleSafetyMargin = 1;
    // Distância de referência para normalizar o custo de enlace.
    double linkNormalizationDistance = 1000;
    // Alcance geométrico aproximado para preservar ao menos um vizinho conhecido.
    double communicationRange = 0;
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
/// pilha de rede montada.
///
/// **Modelo híbrido, deliberado.** A *detecção* que ativa o reposicionamento
/// respeita o alcance físico da câmera (30 m, via `inspect()`); a *avaliação
/// de posições candidatas* usa um predicado geométrico idealizado do simulador
/// (`intersectsAnyObstacleGroundTruth()`), autorizado pelo §14 da diretriz e
/// declarado em D4/E4. O objetivo é isolar a análise da política de
/// reposicionamento das incertezas do planejamento de trajetória. Não há mapa
/// persistente. A aptidão nunca consulta o rádio nem conhece o RSSI futuro.
class RepositionFitness
{
  public:
    RepositionFitness(const FitnessParameters& parameters,
                      const AbstractObstacleSensor *sensor,
                      const inet::Coord& current,
                      const inet::Coord& teamPosition,
                      const std::optional<inet::Coord>& obstaclePoint,
                      const std::vector<inet::Coord>& neighborPositions,
                      bool preserveConnectivity,
                      omnetpp::simtime_t now);

    /// Custo normalizado do candidato: enlace, obstáculo e deslocamento.
    /// Só é avaliado depois de feasible(); por isso não repete o raycasting
    /// que aquela já fez.
    double cost(const inet::Coord& candidate) const;
    /// Limites de área, altitude, alcance, margem local, autonomia e — pelo
    /// avaliador geométrico idealizado — trajeto livre e linha de visada até
    /// a equipe estimada.
    bool feasible(const inet::Coord& candidate) const;
    /// Só área e altitude — sem geometria de obstáculo. Barato o bastante para
    /// o gerador de candidatos do BA descartar amostras triviais antes de
    /// gastar uma avaliação completa de feasible() nelas.
    bool inDomain(const inet::Coord& candidate) const;
    /// Tempo de percurso entre dois pontos, com o eixo mais lento decidindo.
    double travelTime(const inet::Coord& from, const inet::Coord& to) const;

  private:
    const FitnessParameters& parameters;
    /// Usado só por feasible(), e só pelo predicado ground truth.
    const AbstractObstacleSensor *sensor;
    inet::Coord current;
    inet::Coord teamPosition;
    /// Única superfície observada pela câmera nesta ativação, se houve alguma.
    std::optional<inet::Coord> obstaclePoint;
    std::vector<inet::Coord> neighborPositions;
    bool preserveConnectivity;
    omnetpp::simtime_t now;

    bool predictsNeighbor(const inet::Coord& candidate) const;
};

} // namespace echosar
