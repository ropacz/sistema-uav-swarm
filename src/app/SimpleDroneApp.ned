package echosar.app;

import inet.applications.contract.IApp;

simple SimpleDroneApp like IApp
{
    parameters:
        @class(echosar::SimpleDroneApp);
        @display("i=misc/drone");

        // ── Missão ──────────────────────────────────────────────────────────
        string myDroneId          = default("");
        double victimInterval @unit(s) = default(40s);
        double teamTimeout    @unit(s) = default(30s);
        double retryInterval  @unit(s) = default(10s);
        int    maxRetries             = default(5);

        // ── Algoritmo do Morcego ─────────────────────────────────────────────
        bool   enableBatReposition  = default(false);

        // Janela de monitoramento de enlace
        int    windowSize           = default(20);   // N_W — qtd de mensagens na janela
        int    maxLosses            = default(2);    // limiar de perdas → θ_ARR = 1 - maxLosses/N_W

        // Parâmetros do modelo de propagação para θ_RSSI
        double rssiRef     @unit(dBm) = default(-30dBm);  // RSSI(d0)
        double refDistance @unit(m)   = default(1m);       // d0
        double pathLossExp            = default(2.0);      // η
        double rssiMargin  @unit(dBm) = default(6dBm);    // Δ de margem

        // Validação pós-reposicionamento
        double probeTimeout     @unit(s) = default(5s);
        int    maxRepositionTries        = default(3);

        // Bat Algorithm (função de aptidão e espaço de busca)
        int    batPop          = default(40);    // N_b — tamanho da população
        int    batIters        = default(30);    // I_max — iterações máximas
        int    batStagnation   = default(8);     // G — iterações sem melhora → parada antecipada
        double wRepulsion      = default(0.6);   // w1 — peso da repulsão de obstáculo
        double wDisplacement   = default(0.4);   // w2 — peso do deslocamento do drone
        double sigmaRho @unit(m)  = default(150m);  // σ_ρ — alcance de decaimento da repulsão
        double dMaxReposition @unit(m) = default(300m);  // raio de busca ao redor de p_drone
        double zMinReposition @unit(m) = default(80m);   // altitude mínima dos candidatos
        double zMaxReposition @unit(m) = default(120m);  // altitude máxima dos candidatos
        double batAlpha        = default(0.9);   // α — decaimento de loudness
        double batGamma        = default(0.9);   // γ — incremento de pulse rate

    gates:
        input  socketIn  @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}
