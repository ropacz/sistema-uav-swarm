#pragma once

#include <string>

namespace echosar {

/// Estado do reposicionamento de um drone.
///
/// Antes, `repositionState` e `activeRepositionAlertId` eram mutados em doze
/// pontos espalhados por quatro métodos do DroneApp, com as transições
/// implícitas no meio da lógica de cada um. Reunidos aqui, a máquina inteira
/// cabe numa tela e cada transição tem nome.
///
///     IDLE --begin--> MOVING --arrived--> AWAITING_VALIDATION
///       ^                                          |
///       +---------------- release() ---------------+
///
/// Um drone conduz no máximo um reposicionamento por vez; os demais alertas
/// continuam pendentes e seguem o retry normal.
class RepositionController
{
  public:
    /// Nenhum reposicionamento em curso.
    bool idle() const { return state == State::IDLE; }
    /// O drone está a caminho da posição escolhida pelo Bat Algorithm.
    bool moving() const { return state == State::MOVING; }
    /// Este alerta é o que conduz o reposicionamento em curso.
    bool owns(const std::string& alertId) const
    {
        return !activeAlertId.empty() && activeAlertId == alertId;
    }
    /// Há um reposicionamento em curso, conduzido por um alerta diferente.
    bool busyWithOther(const std::string& alertId) const
    {
        return !activeAlertId.empty() && activeAlertId != alertId;
    }
    /// O drone chegou e a próxima tentativa deste alerta valida a posição.
    bool awaitingValidationOf(const std::string& alertId) const
    {
        return owns(alertId) && state == State::AWAITING_VALIDATION;
    }

    /// Assume o alerta e inicia o deslocamento.
    void begin(const std::string& alertId)
    {
        activeAlertId = alertId;
        state = State::MOVING;
    }
    /// Chegada à posição candidata; passa a aguardar a próxima tentativa.
    void arrived() { state = State::AWAITING_VALIDATION; }
    /// Encerra o ciclo, por confirmação ou por expiração do alerta.
    void release()
    {
        activeAlertId.clear();
        state = State::IDLE;
    }

  private:
    enum class State { IDLE, MOVING, AWAITING_VALIDATION };
    State state = State::IDLE;
    std::string activeAlertId;
};

} // namespace echosar
