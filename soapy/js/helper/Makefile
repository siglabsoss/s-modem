all: out/NapiHelper.hpp

.PHONY: copy

out/NapiHelper.hpp: templates/gen* generate.js
	node generate.js


copy:
	cp out/NapiHelper.hpp ../../src/bindjs/NapiHelper.hpp