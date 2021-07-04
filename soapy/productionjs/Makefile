.PHONY: all help

all:
	@echo "Things you can do"
	@echo "   - make run_nuc           - Run modem_main from nuc"
	@echo "   - make run_pc            - Run modem_main from your PC"
	@echo "   - make rc                - run repl-client"
	@echo "   - make arb               - run arbiter"
	@echo "   - make hconst            - regenerate the hconst file"
	@echo "   - make test_ring         - same as running this from the root of higgs_sdr"
	@echo "   - make test_eth          - same as running this from the root of higgs_sdr"
	@echo "   - make svf               - will run svf2higgs.sh to program bitfiles"
	@echo ""
	@echo "   - make help              - Show this message"
	@echo ""

help: all


HIGGS_ROOT=../../../..
Q_ENGINE_REPO=$(HIGGS_ROOT)/libs/q-engine
IP_LIBRARY_REPO=$(HIGGS_ROOT)/libs/ip-library
RISCV_BASEBAND_REPO=$(HIGGS_ROOT)/libs/riscv-baseband
DATAPATH_REPO=$(HIGGS_ROOT)/libs/datapath
SMODEM_REPO=$(HIGGS_ROOT)/libs/s-modem





.PHONY: hconst run run_pc run_nuc arb deps0 clean0 deps1 deps2 clean2
.PHONY: run_deps phony pre_work rc clean_all clean run_deps_all run_deps_debug
.PHONY: sudo_deps

hconst:
	@make --no-print-directory -C ../../ hconst

run:
	@echo "Use   make run_pc"
	@echo "or    make run_nuc"

clean:
	@echo "Use   make clean_all"
	@echo "or    make clean0"
	@echo "or    make clean2"

clean_all: clean0 clean2 vclean

../build:
	mkdir $@

../build/CMakeLists.txt:
	cd ../build && cmake ../

deps00: ../build ../build/Makefile

deps0:
	$(MAKE) -C../build HiggsDriver


clean0:
	$(MAKE) -C../build clean


deps1:
	npm run check_nvm

# if you see package.json this already has a j flag of 8 specified in there
deps2:
	npm run build

clean2:
	npm run clean

# debug mode does not bootload or touch hardware
# it does however launch s-modem
# so these deps should skip vall but still build s-modem

run_deps_debug:
	make deps00
	make -j6 deps0
	make deps1

# deps0 is built with the $(MAKE) and without -j
# this means that when we call deps0 we can pass our own -j count
# note that $(MAKE)  and not make must be used for this to work

run_deps:
	make deps00
	make -j6 deps0
	make deps1
	make deps2

sudo_deps:
	sudo whoami

# same as run deps but also adds vall
run_deps_all: run_deps
	make vall


pre_work: run_deps_all

NODE_VERSION=$(shell node -v)

# assumes you are a user with a /home directory
# this is needed to allow sudo
run_pc: sudo_deps run_deps
	make vall
	@echo "${USER}"
	sudo -E "/home/${USER}/.nvm/versions/node/$(NODE_VERSION)/bin/node" ./modem_main.js

debug_pc: run_deps_debug
	SMODEM_START_DEBUG=1 sudo -E "/home/${USER}/.nvm/versions/node/$(NODE_VERSION)/bin/node" ./modem_main.js

# assumes you are root
run_nuc: run_deps
	make vall
	node modem_main

debug_nuc: run_deps_debug
	SMODEM_START_DEBUG=1 node modem_main


# assumes you are root
arb: run_deps
	node arb.js

rc:
	sudo chmod 777 /tmp/repl/sjs.sock; node_modules/.bin/rc /tmp/repl/sjs.sock

rc2:
	sudo chmod 777 /tmp/repl/sjs2.sock; node_modules/.bin/rc /tmp/repl/sjs2.sock

###################
#
#  Mirror targets
#

.PHONY: svf test_ring test_eth vall vclean

# this will force run svf
svf:
	$(HIGGS_ROOT)/svf2higgs.sh


test_ring:
	make -C $(HIGGS_ROOT) $@

test_eth:
	make -C $(HIGGS_ROOT) $@


vall:
	make -C $(HIGGS_ROOT) $@
	make -C $(HIGGS_ROOT)/sim/verilator/test_narrow_1  $@

vclean:
	make -C $(HIGGS_ROOT) $@
	make -C $(HIGGS_ROOT)/sim/verilator/test_narrow_1  $@

