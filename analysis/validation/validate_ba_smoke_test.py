"""Falha se o teste mínimo não percorrer toda a cadeia de recuperação do BA."""

from pathlib import Path
import math
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.process_results import parse_sca  # noqa: E402

ROOT = REPOSITORY_ROOT
SCA = ROOT / "simulations/results/omnetpp/BA_SmokeTest-0.sca"


def total(frame, name, module_suffix=None):
    selected = frame["name"] == name
    if module_suffix:
        selected &= frame["module"].str.endswith(module_suffix)
    return float(frame.loc[selected, "value"].sum())


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
    observed = {
        name: total(frame, name, ".drone[0].app[0]")
        for name in required
    }
    failures = [f"{name}: esperado {expected}, obtido {observed[name]}"
                for name, expected in required.items() if observed[name] != expected]
    distance = total(frame, "repositionDistance:sum")
    attempts = total(frame, "alertAttemptsSent", ".drone[0].app[0]")
    validated = total(frame, "successfulRepositions", ".drone[0].app[0]")
    unvalidated_local = total(
        frame, "repositionAckedBeforeValidation", ".drone[0].app[0]")
    rssi_available = total(frame, "rssiSamplesAvailable", ".drone[0].app[0]")
    rssi_missing = total(frame, "rssiSamplesMissing", ".drone[0].app[0]")
    pdr = total(frame, "linkWindowPdr:mean")
    if distance <= 0:
        failures.append(f"repositionDistance:sum deve ser > 0, obtido {distance}")
    if attempts < 1:
        failures.append(f"alertAttemptsSent deve ser >= 1, obtido {attempts}")
    if validated + unvalidated_local != 1:
        failures.append("a recuperação deve ser validada ou ocorrer sem validação final; "
                        f"obtido {validated}+{unvalidated_local}")
    if rssi_available <= 0 or rssi_missing != 0:
        failures.append(
            "SignalPowerInd deve estar disponível no smoke test; "
            f"obtido disponíveis={rssi_available}, ausentes={rssi_missing}"
        )
    if pdr >= 0.8:
        failures.append(f"PDR deve cruzar o limiar 0.8, obtido {pdr}")
    global_expected = {
        "alertsGenerated": 1,
        "alertsDelivered": 1,
        "alertsConfirmed": 1,
        "alertsExpired": 0,
        "pdr": 1,
        "packetLossRate": 0,
        "confirmationRate": 1,
        "alertAttemptsDelivered": 1,
        "alertAttemptsLost": 0,
        "attemptDeliveryRate": 1,
        "attemptLossRate": 0,
        "baActivations": 1,
        "repositionsStarted": 1,
        "repositionsValidated": 0,
        "repositionsRecoveredDuringMovement": 0,
        "repositionsRecoveredAfterArrival": 1,
        "repositionsRecoveredWithoutValidation": 1,
        "successfulRepositions": 1,
        "operationallySuccessfulRepositions": 1,
        "repositionSuccessRate": 1,
        "operationalRepositionRecoveryRate": 1,
        # O ACK pertence à tentativa anterior ao movimento. Recuperação
        # operacional não pode ser promovida a validação causal da posição BA.
        "repositionValidationRate": 0,
        "recoveryTimeCount": 1,
        "operationalRecoveryTimeCount": 1,
        "validatedRecoveryTimeCount": 0,
        "repositionDistanceCount": 1,
    }
    for name, expected in global_expected.items():
        value = total(frame, name, ".experimentMetrics")
        if value != expected:
            failures.append(
                f"ExperimentMetrics.{name}: esperado {expected}, obtido {value}"
            )

    central_distance = total(frame, "repositionDistanceSum", ".experimentMetrics")
    commanded_distance = total(
        frame, "commandedRepositionDistanceSum", ".experimentMetrics")
    if not math.isclose(central_distance, distance, rel_tol=1e-9):
        failures.append(
            "distância central deve reproduzir a soma do sinal local; "
            f"obtido central={central_distance}, local={distance}")
    if commanded_distance + 1e-9 < central_distance:
        failures.append(
            "distância comandada não pode ser menor que a efetivamente percorrida; "
            f"obtido comandada={commanded_distance}, real={central_distance}")

    operational_time = total(
        frame, "operationalRecoveryTimeSum", ".experimentMetrics")
    validated_time = total(
        frame, "validatedRecoveryTimeSum", ".experimentMetrics")
    if operational_time <= 0 or validated_time != 0:
        failures.append(
            "recuperação por tentativa antiga deve ter tempo operacional positivo "
            f"e tempo causal zero; obtido {operational_time} e {validated_time}")

    local_validation_samples = total(
        frame, "repositionValidationSamples", ".drone[0].app[0]")
    pre_pdr_sum = total(frame, "preRepositionPdrSum", ".drone[0].app[0]")
    post_pdr_sum = total(frame, "postRepositionPdrSum", ".drone[0].app[0]")
    if local_validation_samples != 0 or pre_pdr_sum != 0 or post_pdr_sum != 0:
        failures.append(
            "ACK não causal não pode alimentar comparação pré/pós; obtido "
            f"amostras={local_validation_samples}, pré={pre_pdr_sum}, pós={post_pdr_sum}")

    power_samples = total(frame, "positionUpdatePowerMilliwatt:count")
    hop_samples = total(frame, "hopCount:count")
    hop_sum = total(frame, "hopCount:sum")
    if power_samples != rssi_available:
        failures.append(
            "cada RSSI disponível deve possuir uma amostra de potência linear; "
            f"obtido potência={power_samples}, RSSI={rssi_available}")
    if hop_samples != 1 or hop_sum != 0:
        failures.append(
            "entrega direta deve registrar uma amostra com zero intermediários; "
            f"obtido count={hop_samples}, sum={hop_sum}")
    if failures:
        raise SystemExit("BA smoke test FALHOU:\n- " + "\n- ".join(failures))
    print("BA smoke test OK")
    print(f"  tentativas: {attempts:.0f}")
    print(f"  distância real de reposicionamento: {distance:.2f} m")
    recovered_in_transit = total(
        frame, "repositionsRecoveredDuringMovement", ".experimentMetrics")
    recovered_after_arrival = total(
        frame, "repositionsRecoveredAfterArrival", ".experimentMetrics")
    if validated:
        mode = "posição final validada"
    elif recovered_in_transit:
        mode = "durante o movimento"
    elif recovered_after_arrival:
        mode = "após a chegada, por tentativa anterior"
    else:
        mode = "sem classificação"
    print(f"  recuperação: {mode}")
    print(f"  RSSI: {rssi_available:.0f} amostras disponíveis, {rssi_missing:.0f} ausentes")
    print(f"  PDR temporal no gatilho: {pdr:.2f}")
    print("  degradação → sensor → BA → movimento → AppACK: confirmado")


if __name__ == "__main__":
    main()
