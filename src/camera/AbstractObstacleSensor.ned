package echosar.camera;

// VERIFICADOR GEOMÉTRICO DIRECIONAL DA OBSTRUÇÃO, com faixa de validade
// declarada (0,7-30 m por padrão, a faixa nominal da visão frontal/traseira
// do DJI Phantom 4 Pro que motivou os limites).
//
// Responde a uma pergunta só: existe superfície bloqueando a linha de visada
// entre dois pontos? Por raycast sobre um segmento infinitamente fino.
//
// NÃO MODELA UMA CÂMERA. Não há orientação, campo de visão, taxa de
// amostragem, iluminação, ruído nem falsos negativos; a verificação é
// instantânea, omnidirecional e restrita à linha central, então não nota
// obstáculo que apareceria na imagem sem cruzar o eixo inspecionado. Os
// parâmetros declarativos de FOV e frequência foram removidos justamente por
// não recortarem nada — declarar especificação que o modelo não aplica
// sugeria um realismo inexistente. Ver D4 em docs/desvios_e_extensoes.md.
//
// Um modo sem faixa de validade continua disponível via maximumRange < 0.
simple AbstractObstacleSensor
{
    parameters:
        string physicalEnvironmentModule = default("^.^.physicalEnvironment");
        // Abaixo deste limite o modelo não afirma nada: nem presença, nem
        // distância. A observação é recusada com reason
        // "outsideCalibratedRange" em vez de devolver a geometria exata do
        // simulador sob o rótulo de medição.
        double minimumRange @unit(m) = default(0.7m);
        // Limite superior da faixa de validade. Negativo desabilita o limite —
        // modo oráculo geométrico idealizado, disponível para análises que
        // queiram isolar o mecanismo do funil de detecção (ver
        // docs/desvios_e_extensoes.md, D4), mas não é o padrão.
        double maximumRange @unit(m) = default(30m);
        @class(echosar::AbstractObstacleSensor);
        @display("i=block/circle;is=s");
}
