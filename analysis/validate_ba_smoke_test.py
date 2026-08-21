"""Falha se o teste mínimo não percorrer toda a cadeia de recuperação do BA."""

from pathlib import Path

from process_results import parse_sca

ROOT = Path(__file__).resolve().parents[1]
SCA = ROOT / "simulations/results/omnetpp/BA_SmokeTest-0.sca"


def total(frame, name):
    return float(frame.loc[frame["name"] == name, "value"].sum())


def main():
    if not SCA.exists():
        raise SystemExit(f"resultado ausente: {SCA}")
    _, frame, _ = parse_sca(str(SCA))
    required = {
        "uniqueAlertsGenerated": 1,
        "uniqueAlertsAcked": 1,
        "alertsExpired": 0,
        "degradationIndications": 1,
        "sensorConfirmations": 1,
        "baActivations": 1,
    }
    observed = {name: total(frame, name) for name in required}
    failures = [f"{name}: esperado {expected}, obtido {observed[name]}"
                for name, expected in required.items() if observed[name] != expected]
    distance = total(frame, "repositionDistance:sum")
    attempts = total(frame, "alertAttemptsSent")
    validated = total(frame, "successfulRepositions")
    during_movement = total(frame, "repositionAckedBeforeValidation")
    if distance <= 0:
        failures.append(f"repositionDistance:sum deve ser > 0, obtido {distance}")
    if attempts < 1:
        failures.append(f"alertAttemptsSent deve ser >= 1, obtido {attempts}")
    if validated + during_movement != 1:
        failures.append("a recuperação deve ocorrer na posição final ou durante o movimento; "
                        f"obtido {validated}+{during_movement}")
    if failures:
        raise SystemExit("BA smoke test FALHOU:\n- " + "\n- ".join(failures))
    print("BA smoke test OK")
    print(f"  tentativas: {attempts:.0f}")
    print(f"  distância real de reposicionamento: {distance:.2f} m")
    mode = "posição final" if validated else "durante o movimento (pacote em fila no AODV)"
    print(f"  recuperação: {mode}")
    print("  degradação → sensor → BA → movimento → AppACK: confirmado")


if __name__ == "__main__":
    main()
