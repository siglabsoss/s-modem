HIGGS_ROOT=../../
Q_ENGINE_REPO=$(HIGGS_ROOT)/libs/q-engine
IP_LIBRARY_REPO=$(HIGGS_ROOT)/libs/ip-library
RISCV_BASEBAND_REPO=$(HIGGS_ROOT)/libs/riscv-baseband
DATAPATH_REPO=$(HIGGS_ROOT)/libs/datapath
SMODEM_REPO=$(HIGGS_ROOT)/libs/s-modem


.PHONY: documentation open_docs dox show_dox all hconst
.PHONY: soapy/js/mock/hconst.js


all:
	@echo ""
	@echo "Has make targets for generationg doxygen output:"
	@echo "  make dox"
	@echo "  make show_dox"
	@echo "  make hconst"
	@echo ""


dox: documentation

show_dox: open_docs



documentation:
	doxygen docs/s-modem-doxygen

hconst: soapy/js/mock/hconst.js

# we could list each file, but it's so cheap to run this
# we can just make it phony and run it each time
soapy/js/mock/hconst.js:
	@echo "Before:"
	@md5sum soapy/js/mock/hconst.js || true
	@echo ""
	cd soapy/const_parser/ && node const_parser.js
	@echo ""
	@echo "After:"
	@md5sum soapy/js/mock/hconst.js

open_docs:
ifneq (, $(shell which chromium-browser))
	chromium-browser docs/html/index.html
else ifneq (, $(shell which chromium))
	chromium docs/html/index.html
else ifneq (, $(shell which firefox))
	firefox docs/html/index.html
endif
