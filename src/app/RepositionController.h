#pragma once

#include <string>

namespace echosar {

/// Estado do reposicionamento de um drone.
///
///     IDLE --begin--> MOVING --release--> IDLE
///
/// Um drone conduz no máximo um reposicionamento por vez; os demais alertas
/// continuam pendentes e seguem o retry normal.
class RepositionController
{
  public:
    /// Nenhum reposicionamento em curso.
    bool idle() const { return activeAlertId.empty(); }
    /// O drone está a caminho da posição escolhida pelo Bat Algorithm.
    bool moving() const { return !idle(); }
    /// Este alerta é o que conduz o reposicionamento em curso.
    bool owns(const std::string& alertId) const
    {
        return !activeAlertId.empty() && activeAlertId == alertId;
    }
    /// Assume o alerta e inicia o deslocamento.
    void begin(const std::string& alertId)
    {
        activeAlertId = alertId;
    }
    /// Encerra o ciclo, por confirmação ou por expiração do alerta.
    void release()
    {
        activeAlertId.clear();
    }

  private:
    std::string activeAlertId;
};

} // namespace echosar
