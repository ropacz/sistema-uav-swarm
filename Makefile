-include .env
WORKSPACE ?= $(abspath ..)
INET_VERSION ?= inet-4.5.4
INET_ROOT := $(WORKSPACE)/$(INET_VERSION)
ANALYSIS_SCRIPTS := analysis/core/process_results.py \
	analysis/core/experiment_metrics.py \
	analysis/core/write_manifest.py \
	analysis/reports/figures.py \
	analysis/reports/alert_sheet.py \
	analysis/reports/mechanism_summary.py \
	analysis/reports/formula_workbook.py \
	analysis/reports/scavetool_figures.py \
	analysis/validation/validate_ba_smoke_test.py \
	analysis/validation/validate_alert_lifecycle_smoke_test.py \
	analysis/validation/validate_connectivity_smoke_test.py \
	analysis/validation/validate_obstacle_smoke_test.py \
	analysis/validation/validate_sensor_range_smoke_test.py \
	analysis/validation/validate_reposition_interrupted_smoke_test.py \
	analysis/validation/validate_multihop_smoke_test.py \
	analysis/validation/validate_no_known_team_smoke_test.py \
	analysis/validation/validate_obstruction_indication_smoke_test.py

.PHONY: all clean cleanall clean-results makefiles checkmakefiles check analysis-tests \
	experiment main-experiment robustness-experiment reproduce \
	alert-sheet scavetool-check \
	ba-smoke-test alert-lifecycle-smoke-test connectivity-smoke-test \
	obstacle-smoke-test reposition-interrupted-smoke-test multihop-smoke-test \
	sensor-range-smoke-test no-known-team-smoke-test \
	obstruction-indication-smoke-test manifest

all: checkmakefiles
	cd src && $(MAKE) MODE=debug

clean: checkmakefiles
	cd src && $(MAKE) MODE=debug clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	$(MAKE) clean-results

# Remove somente artefatos reproduzíveis; código, configuração e referências ficam.
clean-results:
	rm -rf "$(CURDIR)/simulations/results" "$(CURDIR)/analysis/figures" \
	       "$(CURDIR)/analysis/tables"

makefiles:
	cd src && opp_makemake -f --deep -O out \
	  -I. \
	  -I$(INET_ROOT)/src \
	  -L$(INET_ROOT)/src \
	  -lINET_dbg

checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi

check:
	bash -n run.sh
	python3 -m py_compile $(ANALYSIS_SCRIPTS)
	python3 -c "import analysis.core.experiment_metrics, analysis.core.process_results, analysis.reports.figures, analysis.reports.alert_sheet, analysis.reports.mechanism_summary, analysis.reports.formula_workbook, analysis.reports.scavetool_figures, analysis.validation.validate_ba_smoke_test, analysis.validation.validate_alert_lifecycle_smoke_test, analysis.validation.validate_connectivity_smoke_test, analysis.validation.validate_obstacle_smoke_test, analysis.validation.validate_sensor_range_smoke_test, analysis.validation.validate_reposition_interrupted_smoke_test, analysis.validation.validate_multihop_smoke_test, analysis.validation.validate_no_known_team_smoke_test, analysis.validation.validate_obstruction_indication_smoke_test"
	@! grep -rEn "HypothesisPilot|pilot_experiment|hypothesis-pilot" \
		README.md docs run.sh simulations analysis --exclude-dir=results

analysis-tests: check
	python3 -m unittest discover -s analysis/tests -v

# Verifica integração do gatilho e BA; não produz evidência científica.
ba-smoke-test:
	./run.sh -c BA_SmokeTest -r 0
	python3 analysis/validation/validate_ba_smoke_test.py

# Verifica ciclos periódicos sem alertas concorrentes para a mesma vítima.
alert-lifecycle-smoke-test:
	./run.sh -c AlertLifecycle_SmokeTest -r 0
	python3 analysis/validation/validate_alert_lifecycle_smoke_test.py

# Verifica descoberta local e preservação de ao menos um vizinho conhecido.
connectivity-smoke-test:
	./run.sh -c Connectivity_SmokeTest -r 0
	python3 analysis/validation/validate_connectivity_smoke_test.py

# Compara o mesmo enlace com e sem atenuação pelo obstáculo concreto.
obstacle-smoke-test:
	./run.sh -c ObstacleClear_SmokeTest -r 0
	./run.sh -c ObstacleBlocked_SmokeTest -r 0
	python3 analysis/validation/validate_obstacle_smoke_test.py

# Verifica que uma interseção geométrica fora do alcance não ativa o sensor.
sensor-range-smoke-test:
	./run.sh -c SensorOutOfRange_SmokeTest -r 0
	python3 analysis/validation/validate_sensor_range_smoke_test.py

# Verifica que movimento ainda em curso não entra nas métricas de concluídos.
reposition-interrupted-smoke-test:
	./run.sh -c RepositionInterrupted_SmokeTest -r 0
	python3 analysis/validation/validate_reposition_interrupted_smoke_test.py

# Verifica hop count real e encaminhamento por um drone intermediário.
multihop-smoke-test:
	./run.sh -c Multihop_SmokeTest -r 0
	python3 analysis/validation/validate_multihop_smoke_test.py

# Verifica falhas operacionais quando nenhuma equipe foi descoberta.
no-known-team-smoke-test:
	./run.sh -c NoKnownTeam_SmokeTest -r 0
	python3 analysis/validation/validate_no_known_team_smoke_test.py

# Verifica a indicação de possível obstrução (S_ij) ramo a ramo: enlace limpo,
# atenuação de 10 dB e perda das recepções diretas por distância.
obstruction-indication-smoke-test:
	./run.sh -c ObstructionClear_SmokeTest -r 0
	./run.sh -c ObstructionSensitive_SmokeTest -r 0
	./run.sh -c ObstructionDegraded_SmokeTest -r 0
	./run.sh -c ObstructionSilent_SmokeTest -r 0
	python3 analysis/validation/validate_obstruction_indication_smoke_test.py

# Experimento confirmatório: um único contraste pareado e uma pergunta central.
# A análise produz atendimento/perda e o efeito BA-On − BA-Off pareado por seed.
main-experiment: manifest
	./run.sh -c MainExperiment_BaOff
	./run.sh -c MainExperiment_BaOn
	$(MAKE) alert-sheet

# Robustez: varia equipes e vítimas, mas preserva o contraste BA Off/On.
robustness-experiment:
	./run.sh -c Scenario1_OneVictim_BaOff
	./run.sh -c Scenario1_OneVictim_BaOn
	./run.sh -c Scenario1_TwoVictims_BaOff
	./run.sh -c Scenario1_TwoVictims_BaOn
	$(MAKE) alert-sheet

experiment: main-experiment

manifest:
	opp_env run $(INET_VERSION) -w $(WORKSPACE) --no-isolated \
		-c "cd '$(CURDIR)' && INET_VERSION='$(INET_VERSION)' \
		python3 analysis/core/write_manifest.py"

# Planilha e figuras de atendimento e perda, uma linha por alerta.
alert-sheet:
	python3 analysis/reports/alert_sheet.py

# Segunda via de atendimento/perda: exporta os .sca da campanha oficial com a
# ferramenta padrão do OMNeT++ (opp_scavetool) e recalcula a partir desse CSV,
# sem tocar no caminho de leitura que alert-sheet usa. Serve para confirmar,
# com uma ferramenta de terceiros, que parse_sca() não introduz erro.
scavetool-check:
	opp_env run $(INET_VERSION) -w $(WORKSPACE) --no-isolated \
		-c "cd '$(CURDIR)/simulations' && opp_scavetool x results/omnetpp/MainExperiment_*.sca \
		    -o results/campanha_scavetool.csv"
	python3 analysis/reports/scavetool_figures.py simulations/results/campanha_scavetool.csv

# Reprodução confirmatória. A robustez possui alvo próprio.
reproduce:
	./run.sh --build -c MainExperiment_BaOff -r 0
	$(MAKE) analysis-tests
	$(MAKE) main-experiment
