package echosar.app;

import inet.applications.contract.IApp;

simple DroneApp like IApp
{
    parameters:
        @class(echosar::DroneApp);

        // Identidade e formato da aplicação
        // Identificador único do drone; vazio usa o nome do módulo.
        string droneId = default("");
        // Porta UDP compartilhada por alertas, ACKs e atualizações de posição.
        int appPort = default(5000);
        // Tamanho experimental do payload VictimAlert, sem cabeçalhos da pilha.
        int victimAlertPayloadBytes = default(320);
        // Bytes da miniatura anexada ao alerta, somados ao payload acima. Zero
        // envia o alerta sem foto e reproduz os cenários anteriores à câmera.
        // O teto prático não é o MTU e sim a duração da transmissão: no PHY
        // 802.11b a 1 Mbps do cenário, acima de ~768 B o quadro ultrapassa o
        // maxTransmissionDuration do RadioMedium e a simulação aborta.
        int victimAlertPhotoBytes = default(0);
        // Dimensões declaradas da miniatura. São metadados do anexo: o custo no
        // enlace vem de victimAlertPhotoBytes, não daqui.
        int victimAlertPhotoWidth = default(160);
        int victimAlertPhotoHeight = default(120);
        // Formato e validade dos anúncios usados para conhecer drones vizinhos.
        int droneStatusPayloadBytes;
        double droneStatusInterval @unit(s);
        double droneStatusInitialOffset @unit(s);
        double droneEntryLifetime @unit(s);
        // Alcance geométrico aproximado usado apenas na previsão de conectividade.
        double communicationRange @unit(m);

        // Ciclo do alerta
        // Intervalo entre duas tentativas da aplicação para o mesmo alerta.
        double retryInterval @unit(s) = default(30s);
        // Tempo sem VictimAck antes de avaliar a degradação do enlace.
        double ackTimeout @unit(s) = default(5s);
        // Número máximo de tentativas de aplicação por alerta.
        int maxAttempts = default(5);
        // Após esta quantidade de tentativas sem ACK, o sensor é consultado uma vez.
        int repositionAfterUnackedAttempts = default(2);
        // Validade total do alerta desde sua geração.
        double alertTtl @unit(s) = default(180s);
        // Intervalo entre alertas lógicos sucessivos da mesma vítima ativa.
        double alertInterval @unit(s);
        // Último instante permitido para gerar um alerta. Negativo mantém a
        // geração sem corte, como nos cenários técnicos deliberadamente curtos.
        double alertGenerationEndTime @unit(s) = default(-1s);

        // Descoberta da equipe
        // Tempo sem broadcast após o qual a equipe deixa de ser operacional.
        double teamEntryLifetime @unit(s) = default(30s);
        // Após expirar, a última posição conhecida continua retida por este
        // período. Fica marcada como desatualizada e só o mecanismo de
        // recuperação pode usá-la; a transmissão normal exige entrada válida.
        double lastKnownTeamRetention @unit(s) = default(120s);
        // Saltos de aplicação que um TeamUpdate pode percorrer pela FANET.
        // Zero desabilita o repasse e mantém a descoberta em um salto.
        int teamUpdateMaxHops = default(3);
        // Espera aleatória antes de repassar, para dispersar as retransmissões.
        double teamUpdateForwardJitter @unit(s) = default(20ms);
        // Período do timer que verifica alertas, ACKs, TTL e reposicionamento.
        double maintenanceInterval @unit(s) = default(0.1s);

        // Controle e limites do reposicionamento
        // Ativa ou desativa somente o reposicionamento pelo BA.
        bool baEnabled = default(true);
        // Raio máximo da região de busca ao redor da posição atual.
        double maximumRepositionDistance @unit(m) = default(25m);
        // Menor altitude permitida para uma posição candidata.
        double minimumAltitude @unit(m) = default(6m);
        // Maior altitude experimental permitida para o drone.
        double maximumAltitude @unit(m) = default(20m);
        // Limite inferior da área de busca no eixo X.
        double areaMinX @unit(m) = default(0m);
        // Limite superior da área de busca no eixo X.
        double areaMaxX @unit(m) = default(1000m);
        // Limite inferior da área de busca no eixo Y.
        double areaMinY @unit(m) = default(0m);
        // Limite superior da área de busca no eixo Y.
        double areaMaxY @unit(m) = default(1000m);
        // Velocidade horizontal usada no deslocamento comandado pelo BA.
        double horizontalSpeed @unit(mps) = default(13mps);
        // Velocidade vertical máxima durante subida.
        double climbSpeed @unit(mps) = default(5mps);
        // Velocidade vertical máxima durante descida.
        double descentSpeed @unit(mps) = default(3mps);
        // Instante limite de voo usado para rejeitar deslocamentos tardios.
        double flightTimeLimit @unit(s) = default(1800s);
        // TTL IPv4 aplicado aos pacotes UDP da aplicação.
        int applicationIpTtl = default(32);

        // Sondagem de recuperação
        // Após concluir o deslocamento, confirma o enlace com um pacote leve
        // antes de gastar a transmissão do alerta completo. Falso mantém o
        // comportamento anterior: o alerta é enviado direto na chegada.
        bool recoveryProbeEnabled = default(false);
        // Payload da sondagem, sem cabeçalhos da pilha. Só faz sentido se for
        // muito menor que o alerta que ela protege. O piso não é livre: o
        // corpo codificado carrega probeId, alertId, drone e equipe, e o
        // serializer aborta se não couber — 128 B dão folga para os
        // identificadores dos cenários atuais.
        int recoveryProbePayloadBytes = default(128);
        // Espera pela resposta antes de contar a sondagem como perdida.
        double recoveryProbeTimeout @unit(s) = default(1s);
        // Sondagens sem resposta antes de desistir da verificação. Esgotado o
        // limite, o alerta volta ao retry normal e é transmitido mesmo sem
        // confirmação: a sondagem economiza transmissões, não bloqueia entrega.
        int recoveryProbeMaxAttempts = default(2);

        // Indicação de possível obstrução (S_ij)
        // Janela de observação das amostras de RSSI do enlace direto.
        double rssiWindow @unit(s) = default(3s);
        // Tempo sem TeamUpdate recebido diretamente da equipe antes de a
        // ausência valer como indicação. Repasses de outros drones não o
        // reiniciam: eles não observam o enlace direto.
        double directUpdateTimeout @unit(s) = default(3s);
        // RSSI de referência a rssiReferenceDistance sem obstrução. O padrão
        // corresponde a 10 mW, 2,4 GHz, 1 m e antenas sem ganho: 10 dBm menos
        // os 40,05 dB de FSPL do primeiro metro.
        double rssiReferenceDbm = default(-30.05);
        double rssiReferenceDistance @unit(m) = default(1m);
        // Expoente de perda de percurso admitido em linha de visada.
        double losPathLossExponent = default(2.0);
        // Margem admitida para as variações do canal. Acima dela a atenuação
        // excedente conta como degradação.
        double excessLossThresholdDb = default(6.0);
        // Exige a indicação antes de consultar o sensor de obstáculos. Falso
        // preserva o mecanismo atual e apenas registra S_ij; a indicação
        // sozinha nunca aciona o reposicionamento em nenhum dos dois modos.
        bool requireObstructionIndication = default(false);

        // Função de aptidão: os três pesos devem somar 1
        // Peso do custo estimado de comunicação com a equipe.
        double wLink = default(0.60);
        // Peso da penalidade de proximidade e obstrução.
        double wObstacle = default(0.25);
        // Peso da penalidade pela distância de reposicionamento.
        double wMove = default(0.15);
        // Escala espacial do decaimento exponencial da penalidade do obstáculo.
        double obstacleSigma @unit(m) = default(10m);
        // Folga exigida em torno da trajetória e do obstáculo observado.
        double obstacleSafetyMargin @unit(m) = default(1m);
        // Raio físico do drone, somado à margem acima para formar o corredor
        // que a trajetória precisa ter livre. Padrão: 0,35 m, cobrindo a
        // envergadura com hélices de um quadricóptero da classe do Phantom 4.
        double droneRadius @unit(m) = default(0.35m);
        // Distância de referência para normalizar o custo de enlace.
        double linkNormalizationDistance @unit(m) = default(1000m);

        // Parâmetros do Bat Algorithm
        // Quantidade de morcegos virtuais, isto é, candidatos simultâneos.
        int batPopulation = default(20);
        // Número de iterações de otimização por ciclo do BA.
        int batIterations = default(50);
        // Tentativas para inicializar cada morcego em uma posição viável.
        int batInitializationAttempts = default(100);
        // Limite inferior da frequência usada na atualização da velocidade.
        double batFrequencyMin = default(0);
        // Limite superior da frequência usada na atualização da velocidade.
        double batFrequencyMax = default(2);
        // Amplitude inicial, que controla a aceitação de novas soluções.
        double batInitialAmplitude = default(0.9);
        // Taxa inicial de pulsos, que controla busca global versus local.
        double batInitialPulseRate = default(0.5);
        // Fator multiplicativo de redução da amplitude apó aceitação.
        double batAmplitudeDecay = default(0.9);
        // Taxa de crescimento da emissão de pulsos ao longo das iterações.
        double batPulseGrowth = default(0.9);
        // Fração do raio de busca usada para perturbações locais.
        double batLocalSearchScale = default(0.1);

        // Eventos consumidos exclusivamente pelo coletor central.
        @signal[victimAlertGenerated](type=echosar::AlertMetricEvent);
        @signal[victimAlertAttemptSent](type=echosar::AlertMetricEvent);
        @signal[victimAlertConfirmed](type=echosar::AlertMetricEvent);
        @signal[victimAlertExpired](type=echosar::AlertMetricEvent);
        @signal[victimAlertOperationalFailure](type=echosar::AlertMetricEvent);
        @signal[victimRepositionTriggered](type=echosar::AlertMetricEvent);
        @signal[victimSensorEvaluated](type=echosar::AlertMetricEvent);
        @signal[victimBaActivated](type=echosar::AlertMetricEvent);
        @signal[victimRepositionEvent](type=echosar::AlertMetricEvent);
        @signal[victimRecoveryProbe](type=echosar::AlertMetricEvent);
        @display("i=misc/drone");
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
        input assignmentIn;
}
