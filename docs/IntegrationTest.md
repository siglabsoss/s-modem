# Integration Test

## Get Started

### Integration Test w/ Old Frame Structure

Open two terminals. From the first terminal, execute the following:
```bash
cd higgs_sdr_rev2/libs/s-modem/soapy/productionjs
reset && make run_pc
```

Fromt the second terminal, execute the following:
```bash
cd higgs_sdr_rev2/libs/s-modem/soapy/build
make -j
reset && ./test_client
```