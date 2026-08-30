package echosar.camera;

// Câmera de visão frontal/traseira do DJI Phantom 4 Pro (sensoriamento de
// obstáculos). Sensor visual físico idealizado: respeita o alcance nominal
// (0,7-30 m) por padrão, mas o reconhecimento de obstáculo em si (visão
// computacional/ML) é abstraído por verificação geométrica — ver inspect()
// em AbstractObstacleSensor.cc e D4 em docs/desvios_e_extensoes.md. Um modo
// oráculo sem limite de alcance continua disponível via maximumRange < 0.
simple AbstractObstacleSensor
{
    parameters:
        string physicalEnvironmentModule = default("^.^.physicalEnvironment");
        double minimumRange @unit(m) = default(0.7m);
        // Alcance máximo do sensor visual do Phantom 4 Pro. Padrão: 30 m
        // (comportamento de sensor físico). Negativo desabilita o limite —
        // modo oráculo geométrico idealizado, disponível para análises que
        // queiram isolar o mecanismo do funil de detecção (ver
        // docs/desvios_e_extensoes.md, D4), mas não é mais o padrão.
        double maximumRange @unit(m) = default(30m);
        // Campo de visão e frequência da câmera frontal/traseira do Phantom 4
        // Pro V2.0 (especificação DJI, ver
        // docs/references/especificacoes_dji_phantom_4_pro_v2.docx).
        //
        // DECLARATIVOS: validados na inicialização, mas ainda não aplicados —
        // a inspeção é omnidirecional e não recorta pelo cone de visão real
        // nem amostra a 10 Hz. Ver D4 e a lacuna L2 em
        // docs/desvios_e_extensoes.md.
        double fieldOfViewHorizontal @unit(deg) = default(60deg);
        double fieldOfViewVertical @unit(deg) = default(54deg);
        double measurementFrequency @unit(Hz) = default(10Hz);
        @class(echosar::AbstractObstacleSensor);
        @display("i=block/camera;is=s");
}
