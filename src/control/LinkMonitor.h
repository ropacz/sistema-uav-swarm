#pragma once
#include <deque>
#include <cmath>
#include <omnetpp/simtime.h>

namespace echosar {

// Janela deslizante de N_W mensagens por enlace drone_i ↔ equipe_j.
// Alimentada em SimpleDroneApp:
//   pushRssi()     a cada TeamUpdate recebido (sentido equipe→drone)
//   pushAttempt()  a cada DroneStatus enviado (sentido drone→equipe); acked=false inicialmente
//   markAck()      quando ProbeAck chega — localiza a tentativa por sentAt e marca acked=true
//
// Avalia Γ_ij (Eq. gamma da dissertação) quando a janela de ARR está cheia.
struct LinkMonitor {
    struct Attempt {
        omnetpp::simtime_t sentAt;
        bool acked = false;
    };

    int N_W = 20;
    std::deque<double>  rssiWindow;    // RSSI (dBm) recebidos, ≤ N_W entradas
    std::deque<Attempt> ackWindow;     // tentativas de DroneStatus, = N_W entradas quando cheia

    // ── Alimentação ──────────────────────────────────────────────────────────
    void pushRssi(double dBm) {
        rssiWindow.push_back(dBm);
        if ((int)rssiWindow.size() > N_W)
            rssiWindow.pop_front();
    }

    void pushAttempt(omnetpp::simtime_t sentAt) {
        ackWindow.push_back({sentAt, false});
        if ((int)ackWindow.size() > N_W)
            ackWindow.pop_front();
    }

    // Marca a tentativa correspondente a sentAt como confirmada.
    void markAck(omnetpp::simtime_t sentAt) {
        for (auto& a : ackWindow)
            if (a.sentAt == sentAt) { a.acked = true; return; }
    }

    // ── Métricas ─────────────────────────────────────────────────────────────
    bool windowComplete() const { return (int)ackWindow.size() == N_W; }

    // Média dos RSSI recebidos (dBm). Retorna NaN se n==0.
    double rssiAvg() const {
        if (rssiWindow.empty()) return std::numeric_limits<double>::quiet_NaN();
        double sum = 0;
        for (double v : rssiWindow) sum += v;
        return sum / rssiWindow.size();
    }

    int rssiCount() const { return (int)rssiWindow.size(); }

    // ARR = N_ACK / N_W
    double arr() const {
        if (ackWindow.empty()) return 0.0;
        int acked = 0;
        for (const auto& a : ackWindow) if (a.acked) acked++;
        return (double)acked / N_W;
    }

    // ── Indicador Γ_ij(t) ────────────────────────────────────────────────────
    // Avaliado apenas quando windowComplete(). Parâmetros do modelo log-distância.
    bool gamma(double rssiRef_dBm, double refDist_m, double pathLossExp,
               double margin_dB, double dist_m, double thetaARR) const {
        if (!windowComplete()) return false;
        double thetaRssi = rssiRef_dBm
            - 10.0 * pathLossExp * std::log10(dist_m / refDist_m)
            - margin_dB;
        int n = rssiCount();
        if (n >= 1)
            return rssiAvg() < thetaRssi && arr() < thetaARR;
        else
            return arr() < thetaARR;  // n==0: RSSI indefinido, usa só ARR
    }

    void reset() { rssiWindow.clear(); ackWindow.clear(); }
};

} // namespace echosar
