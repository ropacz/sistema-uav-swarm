-include .env
WORKSPACE ?= $(abspath ..)
INET_VERSION ?= inet-4.5.4
INET_ROOT := $(WORKSPACE)/$(INET_VERSION)
VALIDATION_CONFIGS := Validation_Direct Validation_Multihop Validation_Clear_Rssi \
	Validation_Obstacle_Rssi Validation_Obstacle_BaOff Validation_BaOn \
	Validation_Sensor_RejectRange Validation_TwoVictims
ANALYSIS_SCRIPTS := analysis/process_results.py analysis/validate_results.py \
	analysis/network_metrics.py analysis/pcap_batch_to_spreadsheet.py analysis/pcap_core.py

.PHONY: all clean cleanall makefiles checkmakefiles check analysis-tests validate \
	validate-results experiment analyze network-metrics reproduce

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
	@! grep -rEn "ports\.h|simulations/run|Cenario_ComObstaculos|flightTimeLimit = 900s" \
		README.md docs run.sh simulations src

analysis-tests: check
	python3 -m unittest discover -s analysis/tests -v

validate: check
	@for config in $(VALIDATION_CONFIGS); do \
		./run.sh -c $$config -r 0 || exit 1; \
	done
	$(MAKE) validate-results

validate-results:
	python3 analysis/validate_results.py

# Experimento científico: os dois braços pareados, com as mesmas seeds.
# A análise falha se algum braço estiver incompleto ou se os dois diferirem
# por qualquer parâmetro além de baEnabled.
experiment:
	./run.sh -c Experiment_Control_BaOff
	./run.sh -c Experiment_Proposed_BaOn
	$(MAKE) analyze

analyze:
	python3 analysis/process_results.py

# Métricas de rede diagnósticas: escalares do INET agregados por seed, e
# auditoria de nível de fio a partir das capturas PCAPNG.
network-metrics:
	./run.sh -c Network_Realistic_Evaluation
	python3 analysis/network_metrics.py
	python3 analysis/pcap_batch_to_spreadsheet.py simulations/results/pcap \
		--configuration Network_Realistic_Evaluation \
		--output simulations/results/spreadsheets/metricas-rede.xlsx

# Caminho único do código-fonte às tabelas do artigo.
reproduce: all analysis-tests validate experiment
