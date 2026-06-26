#pragma once
#include <optional>
#include "inet/common/geometry/common/Coord.h"
#include "inet/environment/contract/IPhysicalEnvironment.h"

namespace echosar {

// Sensor simulado de obstáculo.
// Acionado apenas quando Γ_ij = 1 (custo sob demanda).
// Faz ray-cast na LoS entre drone e equipe usando o PhysicalEnvironment do OMNeT++/INET.
// Abstrai a camada de processamento sensorial (LiDAR/câmera) — consulta a ground-truth
// do ambiente de simulação, conforme descrito na dissertação.
//
// Retorna:
//   p_obs (Coord) — centro do obstáculo mais próximo do drone que intercepta a LoS
//   nullopt       — sem obstáculo na LoS (degradação atribuída à mobilidade da equipe)
std::optional<inet::Coord> senseObstacleOnLoS(
    const inet::Coord& dronePos,
    const inet::Coord& teamPos,
    inet::physicalenvironment::IPhysicalEnvironment* physEnv);

} // namespace echosar
