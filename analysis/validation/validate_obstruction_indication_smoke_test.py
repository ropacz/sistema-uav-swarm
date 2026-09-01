"""Validate the possible-obstruction indication (S_ij) branch by branch.

Quatro execuções isolam cada caminho da indicação:

* Clear      — enlace limpo a 100 m: a atenuação excedente fica em torno de
               zero e nenhuma indicação sobe.
* Sensitive  — o mesmo enlace com o limiar em 0,5 dB. Continuar em zero prova
               que a referência implementada reproduz o canal do simulador
               dentro dessa margem.
* Degraded   — a mesma geometria com 10 dB a menos de potência e a referência
               inalterada: a atenuação excedente é exatamente 10 dB e sobe pelo
               ramo do RSSI, com as recepções diretas continuando.
* Silent     — a equipe se afasta para fora de alcance: a indicação sobe pelo
               ramo do prazo sem recepção direta, sem passar pelo RSSI.
* Walled     — uma parede de concreto entre os nós: a degradação vem de
               obstrução de fato, e não de um degrau de potência, com as
               recepções diretas continuando.
"""

from pathlib import Path
import sys

REPOSITORY_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPOSITORY_ROOT))

from analysis.core.process_results import parse_sca  # noqa: E402

RESULTS = REPOSITORY_ROOT / "simulations/results/omnetpp"
DRONE = ".drone[0].app[0]"

# Uma equipe emitindo a cada 0,5 s dá 60 recepções diretas em 30 s. No cenário
# Silent as recepções param quando a equipe sai de alcance, restando 30.
# Média de Delta esperada em cada cenário, com a tolerância admitida. Clear e
# Silent têm enlace limpo, então Delta é zero a menos do arredondamento da
# referência. Degraded reduz a potência em exatamente 10 dB e mantém a
# referência, então Delta é o próprio degrau: é a verificação da equação da
# atenuação excedente contra um valor fechado, sem depender de material algum.
EXPECTED_MEAN_EXCESS_LOSS = {
    "ObstructionClear_SmokeTest": 0.0,
    "ObstructionSensitive_SmokeTest": 0.0,
    "ObstructionDegraded_SmokeTest": 10.0,
    "ObstructionSilent_SmokeTest": 0.0,
}
EXCESS_LOSS_TOLERANCE_DB = 0.5

# A atenuação da parede não é um valor fechado: sai do modelo dielétrico do
# INET, não de aritmética do cenário. O que precisa valer é o enquadramento —
# acima do limiar de 6 dB, e longe o bastante do orçamento de enlace para que a
# recepção continue. Fixar o valor exato prenderia o teste à tabela de
# materiais do INET sem ganho de garantia.
EXPECTED_EXCESS_LOSS_RANGE = {
    "ObstructionWalled_SmokeTest": (6.0, 15.0),
}

EXPECTED = {
    "ObstructionClear_SmokeTest": {
        "directRssiSamples": 60,
        "forwardedTeamUpdatesIgnoredForRssi": 0,
        "rssiDegradationIndications": 0,
        "directUpdateTimeoutIndications": 0,
        "possibleObstructionIndications": 0,
    },
    "ObstructionSensitive_SmokeTest": {
        "directRssiSamples": 60,
        "forwardedTeamUpdatesIgnoredForRssi": 0,
        "rssiDegradationIndications": 0,
        "directUpdateTimeoutIndications": 0,
        "possibleObstructionIndications": 0,
    },
    "ObstructionDegraded_SmokeTest": {
        "directRssiSamples": 60,
        "forwardedTeamUpdatesIgnoredForRssi": 0,
        "rssiDegradationIndications": 1,
        "directUpdateTimeoutIndications": 0,
        "possibleObstructionIndications": 1,
    },
    "ObstructionSilent_SmokeTest": {
        "directRssiSamples": 30,
        "forwardedTeamUpdatesIgnoredForRssi": 0,
        "rssiDegradationIndications": 0,
        "directUpdateTimeoutIndications": 1,
        "possibleObstructionIndications": 1,
    },
    # A parede atenua sem derromper: as 60 recepções continuam, e a indicação
    # vem do RSSI, não da ausência delas. É o que separa este caso do Silent.
    "ObstructionWalled_SmokeTest": {
        "directRssiSamples": 60,
        "forwardedTeamUpdatesIgnoredForRssi": 0,
        "rssiDegradationIndications": 1,
        "directUpdateTimeoutIndications": 0,
        "possibleObstructionIndications": 1,
    },
}


def scalar(frame, module_suffix: str, name: str) -> float:
    selected = frame.loc[
        frame["module"].str.endswith(module_suffix) & (frame["name"] == name),
        "value",
    ]
    if len(selected) != 1:
        raise ValueError(
            f"expected one {module_suffix}.{name} scalar, found {len(selected)}"
        )
    return float(selected.iloc[0])


def main() -> None:
    failures = []
    for config, expected in EXPECTED.items():
        path = RESULTS / f"{config}-0.sca"
        if not path.exists():
            raise SystemExit(f"resultado ausente: {path}")
        _, frame, _ = parse_sca(str(path))
        for name, value in expected.items():
            observed = scalar(frame, DRONE, name)
            if observed != value:
                failures.append(
                    f"{config}.{name}: esperado {value}, obtido {observed:g}"
                )
        # A indicação nunca aciona o reposicionamento por conta própria: com
        # requireObstructionIndication falso o gate não barra nada, e com o BA
        # desligado nenhum destes cenários pode mover o drone.
        samples = scalar(frame, DRONE, "directRssiSamples")
        mean_excess = scalar(frame, DRONE, "directRssiExcessLossSum") / samples
        if config in EXPECTED_EXCESS_LOSS_RANGE:
            low, high = EXPECTED_EXCESS_LOSS_RANGE[config]
            if not low < mean_excess < high:
                failures.append(
                    f"{config}: Delta médio esperado entre {low:g} e {high:g} dB, "
                    f"obtido {mean_excess:.4f} dB"
                )
        else:
            target = EXPECTED_MEAN_EXCESS_LOSS[config]
            if abs(mean_excess - target) > EXCESS_LOSS_TOLERANCE_DB:
                failures.append(
                    f"{config}: Delta médio esperado {target:g} dB "
                    f"(+-{EXCESS_LOSS_TOLERANCE_DB:g}), obtido {mean_excess:.4f} dB"
                )
        suppressions = scalar(frame, DRONE, "obstructionGateSuppressions")
        if suppressions != 0:
            failures.append(
                f"{config}.obstructionGateSuppressions: esperado 0, "
                f"obtido {suppressions:g}"
            )
    if failures:
        raise SystemExit(
            "Obstruction indication smoke test FALHOU:\n- " + "\n- ".join(failures)
        )

    print("Obstruction indication smoke test OK")
    print("  enlace limpo: atenuação excedente abaixo de 0,5 dB, S = 0")
    print("  degrau de 10 dB: Delta médio reproduz o valor analítico, S = 1")
    print("  parede de concreto: degradação por obstrução real, enlace vivo, S = 1")
    print("  equipe fora de alcance: indicação pelo prazo sem recepção, S = 1")
    print("  repasses não entram na janela de RSSI e o eco local não é contado")


if __name__ == "__main__":
    main()
