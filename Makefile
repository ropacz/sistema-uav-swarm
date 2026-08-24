-include .env
WORKSPACE ?= $(abspath ..)
INET_VERSION ?= inet-4.5.4
INET_ROOT := $(WORKSPACE)/$(INET_VERSION)
ANALYSIS_SCRIPTS := analysis/core/process_results.py \
	analysis/core/experiment_metrics.py analysis/core/network_metrics.py \
	analysis/core/write_manifest.py \
	analysis/pcap/pcap_batch_to_spreadsheet.py \
	analysis/pcap/pcap_core.py analysis/reports/report_robustness.py \
	analysis/reports/report_main_experiment.py \
	analysis/validation/validate_ba_smoke_test.py \
	analysis/validation/validate_network_discovery.py

.PHONY: all clean cleanall makefiles checkmakefiles check analysis-tests \
	experiment main-experiment robustness-experiment optional-multihop \
	optional-pcap analyze network-metrics reproduce professor-pcap \
	ba-smoke-test manifest

.PHONY: network-discovery-validation

all: checkmakefiles
	cd src && $(MAKE) MODE=debug

clean: checkmakefiles
	cd src && $(MAKE) MODE=debug clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	rm -f src/Makefile

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
	python3 -c "import analysis.core.experiment_metrics, analysis.core.network_metrics, analysis.core.process_results, analysis.pcap.pcap_batch_to_spreadsheet, analysis.reports.report_main_experiment, analysis.reports.report_robustness, analysis.validation.validate_ba_smoke_test"
	@! grep -rEn "HypothesisPilot|pilot_experiment|hypothesis-pilot" \
		README.md docs run.sh simulations analysis --exclude-dir=results

analysis-tests: check
	python3 -m unittest discover -s analysis/tests -v

# Verifica integração do gatilho e BA; não produz evidência científica.
ba-smoke-test:
	./run.sh -c BA_SmokeTest -r 0
	python3 analysis/validation/validate_ba_smoke_test.py

network-discovery-validation:
	./run.sh -c DiscoveryValidation_Direct -r 0
	./run.sh -c DiscoveryValidation_RemoteViaRelay -r 0
	python3 analysis/validation/validate_network_discovery.py

# Experimento confirmatório: um único contraste pareado e uma pergunta central.
main-experiment: manifest
	./run.sh -c MainExperiment_BaOff
	./run.sh -c MainExperiment_BaOn
	python3 analysis/reports/report_main_experiment.py

# Robustez: varia equipes e vítimas, mas preserva o contraste BA Off/On.
robustness-experiment:
	./run.sh -c Scenario1_OneVictim_BaOff
	./run.sh -c Scenario1_OneVictim_BaOn
	./run.sh -c Scenario1_TwoVictims_BaOff
	./run.sh -c Scenario1_TwoVictims_BaOn
	python3 analysis/reports/report_robustness.py --configs \
		Scenario1_OneVictim_BaOff Scenario1_OneVictim_BaOn \
		Scenario1_TwoVictims_BaOff Scenario1_TwoVictims_BaOn

# Referência opcional: roteamento multihop responde a uma pergunta diferente.
optional-multihop:
	./run.sh -c Scenario1_OneVictim_Multihop
	./run.sh -c Scenario1_TwoVictims_Multihop
	python3 analysis/reports/report_robustness.py --configs \
		Scenario1_OneVictim_Multihop Scenario1_TwoVictims_Multihop

# Captura todos os nós em todas as seeds preliminares.
professor-pcap:
	./run.sh -c Scenario1_OneVictim_BaOff --pcap
	./run.sh -c Scenario1_OneVictim_BaOn --pcap
	./run.sh -c Scenario1_OneVictim_Multihop --pcap
	./run.sh -c Scenario1_TwoVictims_BaOff --pcap
	./run.sh -c Scenario1_TwoVictims_BaOn --pcap
	./run.sh -c Scenario1_TwoVictims_Multihop --pcap
	python3 analysis/pcap/pcap_batch_to_spreadsheet.py simulations/results/pcap \
		-o simulations/results/spreadsheets/professor-all-seeds.xlsx

optional-pcap: professor-pcap

experiment: main-experiment

manifest:
	opp_env run $(INET_VERSION) -w $(WORKSPACE) --no-isolated \
		-c "cd '$(CURDIR)' && INET_VERSION='$(INET_VERSION)' \
		python3 analysis/core/write_manifest.py"

analyze:
	python3 analysis/reports/report_main_experiment.py

network-metrics:
	python3 analysis/core/network_metrics.py simulations/results/omnetpp \
		--configs Scenario1_OneVictim_BaOff Scenario1_OneVictim_BaOn \
			Scenario1_OneVictim_Multihop Scenario1_TwoVictims_BaOff \
			Scenario1_TwoVictims_BaOn Scenario1_TwoVictims_Multihop

# Reprodução confirmatória. PCAP e robustez possuem alvos próprios.
reproduce:
	./run.sh --build -c MainExperiment_BaOff -r 0
	$(MAKE) analysis-tests
	$(MAKE) main-experiment
