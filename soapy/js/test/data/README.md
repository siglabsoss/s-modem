# Test folder
Read the `.hex` with `mockutils.js->readHexFile` but make sure there is no trailing `\n`

# mapmov_in0.hex mapmov_out0.hex
This was generated from a random run of `higgs_sdr_rev2/libs/s-modem/soapy/src/test/test_transform.cpp` with some extra prints uncommented.  This is pure data without any feedback bus

# mapmovfb_in1.hex mapmovfb_out1.hex
Generated from `higgs_sdr_rev2/sim/verilator/test_tx_7/tb.cpp` `test0()`.  This has a single mapmov feedback bus packet.

# mapmovfb_in2.hex
Same as previous but with a preceding and trailing feedback bus packet

# coarse_sync0.hex
Various vectors output from coarse_sync process.  new entries are marked with 1,2,3

# sc_16_144_880_1008_bpsk_mod.hex
Was created from `sc_16_144_880_1008_bpsk.hex` but for every value a repeating sequence from 0->33 was added to make sure each ofdm frame in the file is unique.

# coarse_log.csv
Created using `debug_coarse` to try and find a correlation betwen offset to the input data and the coarse sync result

# coarse_sync0.hex
Created using a lost modificiation of `debug_coarse`.  Contains `input_conj_multi_location_0` `input_conj_multi_location_1` `output_conj_multi_location`
Used in a test

# coarse_sync1.hex
Created using a lost modificiation of `debug_coarse`.  Contains `permutation_address_onetone_calculation_location` `bank_address_onetone_calculation_location` `output_onetone_calculation_location` `exp_data_location` `fake_input_location` 


# coarse_sync2.hex
Created using a lost modificiation of `debug_coarse`.  Contains `input_conj_multi_location_0` `input_conj_multi_location_1` `output_conj_multi_location` `exp_data` 


# long_cs21_out.hex
4000us of output from test_mover_8 with a modification so that all_sc feedback frame came more ofen

# native_fft_256_cp.hex
Generated from output of this, passed through complexToIShortMulti
```javascript
let theta = 0.1;
let chunk = 1024;
let addOutputCp = 256;
let removeInputCp = 0;
let source = new streamers.CF64StreamSin({}, {theta:theta,count:1,chunk:chunk} );
dut = new nativeWrap.WrapNativeTransformStream({},{print:print,args:[removeInputCp,addOutputCp]}, fftChain, afterPipe, afterShutdown);
```

# sin_1024.hex
Generated from the output of this, should be the same as the input to the fft for `native_fft_256_cp.hex`
```javascript
let theta = 0.1;
let chunk = 1024;
let source = new streamers.CF64StreamSin({}, {theta:theta,count:1,chunk:chunk} );
```

# reverse_mover_input.hex
Extrated from the middle of `emulateHiggsToHiggs()`.  Feed this into the reverse mover + slicer and the expected output is:
```c++
std::vector<uint32_t> input_data;
for(int i = 0; i < 32; i++) {
  const uint32_t a_word = (((i*2598234492)^(i*20423)) + 0x42)&0xffffffff;
  input_data.push_back(a_word);
}
```
Note that the loop above only produces valid results for the last 32 words.  the file has a preamble sequence as well.

# cooked_cs10_out.hex
FFT of cooked data captured at output of cs10.  Will use to test rx chain


# crash_mapmov_1.raw or crash_mapmov_1.hex
Taken with code a few comments earler than `66d485f1aa57d4b1a852abae07df11496e0067b8`.  Ran tx and rx radios. crashed after 8 or 10 mapmov packets.  I was able to cut this down and re-send it to the board to create the same crash.  Near the end of the packet there is a mapmov for epoc second 2100.  `.raw` version is in raw bytes. load as Buffer and convert to ishort

# crash_mapmov_2.hex
Taken from crash_mapmov_1.raw and converted to hex.

# packet_320_qam16_2.hex
Input to mapmov as sent from s-modem.  but with         `new_subcarrier_data_sync(&output_vector,header_word,enabled_subcarriers*2);`