-include .env
WORKSPACE ?= $(abspath ..)
INET_VERSION ?= inet-4.5.4
INET_ROOT := $(WORKSPACE)/$(INET_VERSION)
ANALYSIS_SCRIPTS := analysis/process_results.py \
	analysis/network_metrics.py analysis/pcap_batch_to_spreadsheet.py analysis/pcap_core.py \
	analysis/report_hypothesis_pilot.py

.PHONY: all clean cleanall makefiles checkmakefiles check analysis-tests \
	experiment analyze network-metrics reproduce hypothesis-pilot

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
	PYTHONPATH=analysis python3 -c "import network_metrics, pcap_batch_to_spreadsheet, process_results, report_hypothesis_pilot"
	@! grep -rEn "Professor_|Validation_|DissertationBase|professor-scenarios" \
		README.md docs run.sh simulations/omnetpp.ini simulations/*.xml src

analysis-tests: check
	python3 -m unittest discover -s analysis/tests -v

# Experimento causal pareado do artigo.
hypothesis-pilot:
	./run.sh -c HypothesisPilot_BaOff
	./run.sh -c HypothesisPilot_BaOn
	python3 analysis/report_hypothesis_pilot.py

experiment: hypothesis-pilot

analyze:
	python3 analysis/process_results.py

network-metrics:
	python3 analysis/network_metrics.py simulations/results/omnetpp \
		--configs HypothesisPilot_BaOff HypothesisPilot_BaOn

# Caminho único do código-fonte ao resultado do piloto.
reproduce:
	./run.sh --build -c HypothesisPilot_BaOff -r 0
	$(MAKE) analysis-tests
	$(MAKE) hypothesis-pilot
	$(MAKE) network-metrics
