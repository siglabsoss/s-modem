all: generate


.PHONY: generate all clean

generate: ../src/gen/higgsdriverjsonserver.h


../src/gen/higgsdriverjsonserver.h: spec.json
	jsonrpcstub spec.json --cpp-server=HiggsDriverJsonServer --cpp-server-file=../src/gen/higgsdriverjsonserver.h --cpp-client=SideControlJsonClient --cpp-client-file=../src/gen/sidecontroljsonclient.h


clean:
	rm -f ../src/gen/higgsdriverjsonserver.h ../src/gen/sidecontroljsonclient.h
