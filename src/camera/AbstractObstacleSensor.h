#pragma once

#include <cmath>
#include <string>

#include "inet/common/ModuleRefByPar.h"
#include "inet/environment/contract/IPhysicalEnvironment.h"
#include "omnetpp.h"

namespace echosar {

/// Resultado de uma verificação geométrica direcional da obstrução.
///
/// `detected` e `distanceValid` são independentes de propósito: uma superfície
/// pode ser notada sem que sua distância seja utilizável. Colapsar os dois em
/// um único booleano foi um erro anterior deste módulo, que devolvia a
/// distância exata calculada pelo simulador junto com a alegação de que ela
/// não era mensurável.
struct ObstacleObservation {
    /// Uma superfície foi observada na direção inspecionada, dentro da faixa
    /// em que este modelo se declara válido.
    bool detected = false;
    /// `distance` e `nearestSurfacePoint` podem ser consumidos. Falso sempre
    /// que nada foi observado ou que a observação caiu fora da faixa
    /// calibrada; nesses casos `distance` é NaN e `nearestSurfacePoint`
    /// permanece indefinido, e nenhum consumidor pode substituí-los pela
    /// geometria exata do simulador.
    bool distanceValid = false;
    /// O segmento inteiro até o alvo coube no alcance. Falso significa que o
    /// trecho além do alcance não foi observado — não que esteja livre.
    bool fullyObserved = false;
    /// Primeiro ponto da superfície a partir do drone: onde a linha de visada
    /// começa a ser bloqueada. Só definido quando `distanceValid`.
    inet::Coord nearestSurfacePoint;
    /// Distância drone-superfície. NaN quando não é utilizável.
    double distance = NAN;
    /// Desfecho da verificação, para auditoria. Distingue os dois sentidos de
    /// "sem obstáculo" — `clearToTarget` (segmento inteiro observado e livre)
    /// e `clearWithinSensorRange` (só o trecho dentro do alcance foi
    /// observado) — e nomeia a recusa por faixa (`outsideCalibratedRange`).
    std::string reason;
};

/// VERIFICADOR GEOMÉTRICO DIRECIONAL DA OBSTRUÇÃO, com faixa de validade
/// declarada.
///
/// Responde a uma única pergunta: **existe uma superfície bloqueando a linha
/// de visada entre estes dois pontos?** Faz isso por raycast sobre um segmento
/// infinitamente fino, e a faixa 0,7-30 m declara em que distâncias o
/// resultado é assumido válido.
///
/// **Não é o modelo de uma câmera.** Uma câmera observa um volume definido por
/// orientação e campo de visão, a uma taxa de amostragem finita, e enxerga
/// obstáculos que aparecem na imagem sem cruzar exatamente o eixo inspecionado.
/// Nada disso existe aqui: a verificação é instantânea, omnidirecional e
/// restrita à linha central. Não descreva este módulo como "detecção por
/// câmera" — nem no código, nem no texto. Ver docs/desvios_e_extensoes.md, D4.
class AbstractObstacleSensor : public omnetpp::cSimpleModule
{
  protected:
    inet::ModuleRefByPar<inet::physicalenvironment::IPhysicalEnvironment> environment;
    /// Limite inferior da faixa de validade declarada. Abaixo dele o modelo
    /// não afirma nada: nem presença, nem distância.
    double minimumRange = 0.7;
    /// Limite superior da faixa de validade declarada (padrão: 30 m).
    /// Negativo desabilita o limite — modo oráculo geométrico idealizado,
    /// disponível para análises que queiram isolar o mecanismo do funil de
    /// detecção (ver docs/desvios_e_extensoes.md, D4), mas não é o padrão.
    double maximumRange = 30;

    /// Obtém o ambiente físico e valida a faixa configurada.
    virtual void initialize() override;
    /// Descarta mensagens porque o módulo é consultado sincronamente por inspect().
    virtual void handleMessage(omnetpp::cMessage *message) override { delete message; }

  public:
    /// Verifica a linha de visada entre o drone e um alvo (equipe, candidato do
    /// BA ou qualquer outro ponto) e descreve a primeira superfície observável.
    /// Com `maximumRange >= 0` o segmento avaliado é truncado no alcance: nada
    /// além dele é observado.
    ObstacleObservation inspect(const inet::Coord& dronePosition,
                                const inet::Coord& inspectionTarget) const;

    /// **Não é uma capacidade observável.** Predicado geométrico idealizado do
    /// simulador ("ground truth"): responde se o segmento informado cruza
    /// algum objeto físico, sem limite de alcance e sem faixa de validade.
    ///
    /// Existe para avaliar a viabilidade de posições candidatas durante o
    /// reposicionamento — uso que o §14 da diretriz autoriza explicitamente,
    /// desde que declarado como "conhecimento geométrico idealizado
    /// disponibilizado ao mecanismo de reposicionamento" (ver
    /// docs/desvios_e_extensoes.md, D4 e E4). Não use para detecção: essa
    /// respeita a faixa declarada e passa por inspect().
    bool intersectsAnyObstacleGroundTruth(const inet::Coord& a, const inet::Coord& b) const;

    /// Mesma natureza idealizada do predicado acima, mas para um corredor de
    /// raio `clearance` em torno do segmento, em vez de sua linha central.
    ///
    /// Um drone não é um ponto: uma trajetória cujo eixo passa rente a uma
    /// parede é livre pela linha central e ainda assim colide. `clearance`
    /// deve cobrir o raio físico do drone mais a margem de segurança desejada.
    ///
    /// Aproximação declarada: o corredor é amostrado pelo eixo mais quatro
    /// raios paralelos deslocados de `clearance` em duas direções
    /// perpendiculares entre si. Um obstáculo fino o bastante para passar
    /// entre os cinco raios escapa; um cilindro exato exigiria consulta de
    /// distância ponto-superfície, que a interface de formas do INET não expõe.
    bool clearCorridorGroundTruth(const inet::Coord& a, const inet::Coord& b,
                                  double clearance) const;
};

} // namespace echosar
