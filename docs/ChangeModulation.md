Modulation
====
Changing modulation schemes is rather involves.

AirPacket
====
* set_subcarrier_allocation
* demodulateSlicedHeader


Edit TxSchedule.cpp
and set air params


Edit DemodThread.cpp
and set air params



Change modulation from QPSK to QAM16
====
* edit mover_9/cs21/main.c
* add a new define at the top (i used `MODE_QAM_16_640_LIN`)

Part 1
-----
Choose type of mover
```c
#ifdef MODE_QAM_16_640_LIN
#define DISABLE_SLICE_MOVER
#endif
```


Part 2
----
in `pet_mover`
```c
#ifdef MODE_QAM_16_640_LIN
mover_640_lin(input_row, DST_ROW_REV2);
#endif
```


Part 3
----
in `pet_mover`
```c
#ifdef MODE_QAM_16_640_LIN
demod_body_length = 80;
slice_input_length = 640;
#endif
```


Part 4
----
in `pet_mover`
```c
#ifdef MODE_QAM_16_640_LIN
  demod_mode = FEEDBACK_MAPMOV_QAM16;
#endif
```

Part 5 (AirPacket.cpp)
----
```c
        if( modulation_schema == FEEDBACK_MAPMOV_QAM16 ) {
            if( subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {
                packed = _demodulateSlicedHeaderHelper<TransformWords128Qam16>(as_iq, transformQam16, effective_sliced_word_count, i, sync_word, modulation_schema);
            } else {
                // std::cout << "here effective_sliced_word_count " << effective_sliced_word_count << "\n";
                packed = _demodulateSlicedHeaderHelperUntransformed(as_iq, effective_sliced_word_count, i, sync_word, modulation_schema);
            }
        } else {
            if( subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
                packed = _demodulateSlicedHeaderHelperUntransformed(as_iq, effective_sliced_word_count, i, sync_word, modulation_schema);
            } else if ( subcarrier_allocation == MAPMOV_SUBCARRIER_640_LIN || subcarrier_allocation == MAPMOV_SUBCARRIER_512_LIN ) {
                packed = _demodulateSlicedHeaderHelperUntransformed(as_iq, effective_sliced_word_count, i, sync_word, modulation_schema);
            } else {
                packed = _demodulateSlicedHeaderHelper<TransformWords128>(as_iq, transform, effective_sliced_word_count, i, sync_word, modulation_schema);
            }
        }
```

Part 6 (AirPacket.cpp)
---
```c
    } else if ( modulation_schema == FEEDBACK_MAPMOV_QPSK && subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {
        for(unsigned i = start_at; i < stop_at; i++) {

            const auto optional_result = transform.t(i);

            const auto lookup_i = optional_result.second;

            const auto w2 = bitMangleSliced(as_iq[1][lookup_i]);
            dout.push_back(w2);
        }
    } else {
```



Steps To Full Verilator Test
====
* edit TxSchedule to enable print after feedback_vector_mapmov_scheduled_sized()
* `sjs.txSchedule.doWarmup = true`
* `sjs.txSchedule.print1 = true`
* `sjs.txSchedule.debug(620)`
* Copy into `mapmov_640_lin_1.hex`
* `mapmov.c` was adjusted (guess and check)
* `test_tx_9/test_11.cpp`
  * ringbus adjusted for new mode
  * Filename copied from above
  * `TEST_SELECT=11 make quick`
  * Edit cs11/main.c and enable `ENABLE_TB_DEBUG` to verify that the mapmov happens and finishes
  * disable `ENABLE_TB_DEBUG` to get the final output at "full speed"
  * Also run with `make quick` and `make show` to verify that the mapmov is finished writing settings, before the first word of the feedback bus
* cs11_in.hex opened and copy all lines past mapmov header (first 16) to `mapmov_640_lin_1_mapped.hex`
* test_mover_9/test_5.cpp
  * edit file name
  * runs first part with `exit(0)`
    * fftAirSubcarrierTransform
    * fftAirSubcarrierTransform
  * `TEST_SELECT=5 make quick`
  * output of this is then pasted into test_mover_9/override/fpgas/cs/cs22/c/src/debug_640_lin4.h as a VMEM_SECTION array
* `test_mover_9/cs22/main.c`:
  * copy `test_mover_9/override/fpgas/cs/cs22/c/src/debug_new_modulation.txt` over main
  * edited to include correct file from previous step
* test_5.cpp edited to remove `exit(0)` and run. output is final output of sliced data
* `cs22/main.c` can add or remove DONT_SLICE_DATA to verify order of output
  * Result is copied into `_sliced.hex`
* edit `test_air_packet_6.cpp`
  * set the path
  * make a run the test, see README to learn how to enable unit tests for cmake





Qam16 notes
===
The vertial subcarrier loader seems to be always using 16 frames for sync
and 16 wide for data.  This means that for qam16 we are under-packing the header
just to maintain compatiblity with the same symbol per frame wide