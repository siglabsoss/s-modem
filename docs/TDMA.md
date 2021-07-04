# TDMA Details
This system is how we syncronise two transmitters to be on exactly the same OFDM frame.  A single subcarrier is modulated.  cs20 is in charge of modulation, mover mapper is not used.  This is using QPSK however it's a different bit->symbol mapping than mover mapper.

# Modes
A mode is set from RadioEstimate to the code in cs20.  This is done with `TDMA_CMD` ringbus command.

* 4  - First attempt to find counters, initially had a bug. I belive this mode can be skipped
* 6  - beamform
* 7  - Canned data will only send 1 of the 5 canned vectors.
* 20 - disabled
* 21 - reset 



# RadioDemodTDMA class
This class was extrated from RadioEstimate.

* A word is sent.  We check all combinations of QPSK phases until we find this word
* Once this word is matched, we set tdma_phase.
  * tmda_phase is measured against RX frame counter however:
  * This tdma_phase is relative both TX frame counter, RX frame counter. 
* Once tdma_phase is set, we begin "slicing" QPSK points into 32 bit words and pusing them to demod_words
* Seems like code around demod_words uses the member HiggsTDMA[] which is over parallel (we only use 1)
* 




# Behavior internal to cs20
```
///
///  ringbus call back for tdma sync
///  is set to 0
///  modes:
///    0 previously this was putting out a steady 0xdeadbeef, etc on the tone,
///      this gets set into dmode immediately as it is a kind of reset
///    4 this is set into pending
///
///  flow:
///      mode_pending -> 4
///      mode <- mode_pending  at start of ts0    line 576
///    data_value_next:
///      if mode == 4, mode = 1
///        data is sent,
///      mode goes 1,2,3 and parks at 3
///   because pending is never reset, we continue to set pending to 4
```