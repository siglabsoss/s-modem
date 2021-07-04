.PHONY: all test test_integration test_int

all:
	@echo ""
	@echo "JS folder"

# see package.json for the behavior of test
test:
	npm run test




INTEGRATION_TESTS = \
integration/test_smodem_start_stop.js \
integration/test_smodem_txmode.js

define invoke_int_test
: $(foreach ttest,$(INTEGRATION_TESTS), && ./node_modules/.bin/mocha $(ttest) )
endef


test_integration2:
	./node_modules/.bin/mocha $(INTEGRATION_TESTS)

test_integration:
	$(call invoke_int_test)

# Alias
test_int: test_integration