#include "RadioDemodTDMA.hpp"
// #include "constants.hpp"
#include "vector_helpers.hpp"
#include "common/convert.hpp"
#include "schedule.h"
#include "cpp_utils.hpp"
#include "random.h"
#include "common/GenericOperator.hpp"


//////////////
#include "ringbus.h"
#include "ringbus2_pre.h"
#define OUR_RING_ENUM RING_ENUM_PC
#include "ringbus2_post.h"
//////////////


#include <iostream>

using namespace std;

#undef DATA_TONE_NUM

namespace siglabs {
namespace smodem {

/// @param[in] _array_index: Which array index are we (of dsp->radios)
/// @param[in] _peer_id: Which peer id are re talking with (init.json: exp.our.id)
/// @param[in] _data_tone_num: How many options are there for passing data
/// @param[in] _subcarrier_chunk: Wait till this many samples before process (should be much larger than 32)
RadioDemodTDMA::RadioDemodTDMA(const size_t _array_index, const size_t _peer_id, const unsigned _data_tone_num, const unsigned _subcarrier_chunk)
        : array_index(_array_index)
        , peer_id(_peer_id)
        , data_tone_num(_data_tone_num)
        , subcarrier_chunk(_subcarrier_chunk)
        {

    tdma.resize(data_tone_num);
    demod_buf_accumulate.resize(data_tone_num);

    // cout << "Radio " << array_index << " has sync " << HEX32_STRING(SYNC_WORD) << endl;

    // the units here are data tone indices
    // for now this means *2 + 1
    // see also subcarrierForBufferIndex()
    // keep_demod_sc = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
    // keep_demod_sc = {30,31};
    // for(unsigned i = 0; i < data_tone_num; i++) {
        
    // }
}

/// Call with the index of the iq data buffer which has data for us
/// this also sets the member variable td which is heavily used by RadioEstimate directly
/// @param[in] index: Which index (must be less than data_tone_num) are we using
void RadioDemodTDMA::setIndex(const unsigned index) {
    if( index >= data_tone_num ) {
        cout << "Illegal value " << index << " in setIndex()\n";
        return;
    }
    data_subcarrier_index = index;
    td = &(tdma[data_subcarrier_index]);

    keep_demod_sc = {index};
}

// void RadioDemodTDMA::setAdvanceCallback(const advance_tdma_cb_t cb) {
//     advancePartnerSchedule = cb;
// }

void RadioDemodTDMA::setSetPartnerTDMACallback(const set_partner_tdma_cb_t cb) {
    setPartnerTDMA = cb;
}

void RadioDemodTDMA::setRingbusCallback(const ringbus_peer_cb_t cb) {
    ringbusPeer = cb;
}

void RadioDemodTDMA::partnerOp(const std::string s, const uint32_t _sel, const uint32_t _val) {

    auto pack = siglabs::rb::op(s, _sel, _val, GENERIC_OPERATOR_CMD);

    raw_ringbus_t rb0 = {RING_ADDR_TX_PARSE, pack[0] };
    raw_ringbus_t rb1 = {RING_ADDR_TX_PARSE, pack[1] };
    ringbusPeer(peer_id, &rb0);
    ringbusPeer(peer_id, &rb1);
}




void RadioDemodTDMA::resetDemodPhase() {
    // demod_est_common_phase = -1;
    tdma_phase = -1;
}

void RadioDemodTDMA::resetTdmaAndLifetime() {
    HiggsTDMA& td = tdma[data_subcarrier_index];
    tdma_phase = -1;
    td.sent_tdma = true;
    td.lifetime_rx = 0;
    td.lifetime_tx = 0;
}

void RadioDemodTDMA::resetTdmaIndexDefault() {
    HiggsTDMA& td = tdma[data_subcarrier_index];
    td.reset(); // reset our tracking for our single td object
}


uint32_t RadioDemodTDMA::get_demod_2_bits(const int32_t data) {
    uint32_t bit_from_real;
    uint32_t bit_from_imag;
    uint32_t bit_value;

    bit_from_imag = data > 0 ? 1:0;
    bit_from_real = (data << 16) > 0 ? 1:0;
    bit_value  = (bit_from_real << 1) | bit_from_imag;

    return bit_value;
}

size_t RadioDemodTDMA::getBuffSize(unsigned i) {
    if( i >= demod_buf_accumulate.size() ) {
        return 0;
    }

    return demod_buf_accumulate[i].size();
}




/// Slices QPSK into a buffer of uint32_t words.
/// The slicing only occurs when tdma_phase is set to not -1;
/// We save the frame of the last ofdm symbol. As a marker for each word.  This could be backed out
/// and if so should be done inside this function
void RadioDemodTDMA::alternateDemodTDMA(const uint32_t buffer_index, const uint32_t expected_subcarrier, const bool do_erase) {
    auto& chunk = demod_buf_accumulate[buffer_index];
    auto& dout = demod_words[buffer_index];

    constexpr int m = 16;

    if(times_eq_sent == 0 && do_erase) {
        // rename chunk to..
        chunk.erase(chunk.begin(), chunk.end());
        return;
        // empty buffer and do nothing
    }

    uint32_t sr = 0;

    uint32_t frame;
    uint32_t i = 0;
    for(const auto ppair : chunk)
    {
        const uint32_t n = ppair.first;
        frame = ppair.second;

        // instead of masking ourselves
        int16_t real, imag;
        ishort_to_parts(n, imag, real);

        uint32_t bit_value = 0;

        if( real <= 0 && imag <= 0) {
            bit_value = 0x0<<30;
            // cout << "00" << endl;
        } else if (real <= 0 && imag > 0) {
            bit_value = 0x1<<30;
            // cout << "01" << endl;
        } else if (real > 0 && imag <= 0) {
            bit_value = 0x2<<30;
            // cout << "10" << endl;
        } else if (real > 0 && imag > 0) {
            bit_value = 0x3<<30;
            // cout << "11" << endl;
        }


        sr = (sr>>2) | bit_value;

        // mode 0 will sent words over the air that make this print
        if(
            _print_fine_schedule && 
            (sr && 0xffffff00 == 0xca5e0000)
            ) {
            cout << "r" << array_index << " TDMA idx: " << buffer_index << " found marker "
            << tdma_phase << " at frame " << frame << " sr " << HEX32_STRING(sr)
            << endl;
        }

        // 0xbeefbeef is used in mode 4 to set the phase
        // 0xdeadbeef is used in mode 6 to set the phase

        if( sr == 0xdeadbeef || sr == 0xbeefbeef ) {
            if( tdma_phase == -1 ) {
                tdma_phase = frame;
                if(_print_fine_schedule) {
                    cout << "r" << array_index << " TDMA in mode " << last_mode_sent << " idx: " << buffer_index << " set special phase to " << tdma_phase << " using keyword " << sr << endl;
                }
            }
        }

        if( print_sr_count > 0 ) {
            cout << "r" << array_index << " TDMA idx: " << buffer_index << " at frame " << frame << " sr " << HEX32_STRING(sr) << "\n";
            print_sr_count--;
        }
        


        if(tdma_phase != -1) {

            /// crazy phase based equation is based on tdma_phase
            /// decides which QPSK point we split into demod_words
            uint32_t this_phase = (frame-tdma_phase+(m*1024)) % m;


            if(this_phase == 0) {
                dout.push_back(std::pair<uint32_t,uint32_t>(sr,frame)); // same a demod_words
                sr = 0;
            }
        }
        i++;
    }

    if( do_erase ) {
        uint32_t tail = chunk.size() % m;
        chunk.erase(chunk.begin(), chunk.end()-tail);
    }

}




// void RadioDemodTDMA::alignSchedule(const uint32_t tx, const uint32_t rx) {
//     // how many frames after the beginning of the schedule
//     // was the timer sampled
//     // (firmware sends some zeros and then a magic word before timer)
//     constexpr uint32_t tx_delay = 7*16;

//     // how many frames late was the rx timer sampled
//     // rx logic grabs timer value of the LAST ofdm frame
//     constexpr uint32_t rx_delay = 16 + 1; // 1 is fudge

//     uint32_t tx2 = tx-tx_delay;
//     uint32_t rx2 = rx-rx_delay;

//     // left in for now
//     cout << "r" << array_index <<  " //// alignSchedule(0) /////////////////////////////////////////////////////////////////////////" << endl;
    
//     if(_print_fine_schedule) {
//         cout << "tx2: " << tx2 << " rx2: " << rx2 << endl;
//     }

//     tx2 = tx2 % SCHEDULE_FRAMES;
//     rx2 = rx2 % SCHEDULE_FRAMES;

//     if(_print_fine_schedule) {
//         cout << "tx2: " << tx2 << " rx2: " << rx2 << endl;
//     }

//     int32_t delta =  (int32_t)rx2 - (int32_t)tx2;

//     if(_print_fine_schedule) {
//         cout << "delta: " << delta << endl;
//     }

//     uint32_t delta_final = (delta + SCHEDULE_FRAMES + SCHEDULE_FRAMES) % SCHEDULE_FRAMES;

//     cout << "final delta: " << delta_final << endl;

//     if(0) {
//         delta_final = SCHEDULE_FRAMES - delta_final;
//     }

//     // dsp->advancePartnerSchedule
//     advancePartnerSchedule(peer_id, delta_final);
// }

// void RadioDemodTDMA::alignSchedule3() {
//     HiggsTDMA& td = tdma[data_subcarrier_index];
//     alignSchedule2(td);
// }

// void RadioDemodTDMA::alignSchedule2(HiggsTDMA& td) {
//     if(!td.needs_fudge) {
//         cout << "r" << array_index << " do not call RadioDemodTDMA::alignSchedule2() without needs fudge false" << endl;
//         return;
//     }
//     td.needs_fudge = false;

//     cout << "r" << array_index <<  " //// alignSchedule(2) /////////////////////////////////////////////////////////////////////////" << endl;

//     // uint32_t delta_final = ( SCHEDULE_FRAMES + SCHEDULE_FRAMES - td.fudge_rx) % SCHEDULE_FRAMES;
    
//     // this number is determined by structure of how things are pulled out of buffers
//     // as well as structure of parseDemodWords()
//     const int32_t delay_fudge = -32;

//     // this value MUST be positive
//     uint32_t delta_final = td.fudge_rx + delay_fudge;
    
//     cout << "final delta (2): " << delta_final << endl;

//     // dsp->advancePartnerSchedule
//     advancePartnerSchedule(peer_id, delta_final);
// }

// returns 0 for success
// returns other for errors
int RadioDemodTDMA::alignSchedule4() {
    HiggsTDMA& td = tdma[data_subcarrier_index];


    uint32_t found_tx_counter = td.lifetime_tx;
    uint32_t found_frame = td.lifetime_rx;
    uint32_t found_epoc = td.epoc_tx;

    if(td.needsUpdate()) {
        cout << "r" << array_index << " do not call RadioDemodTDMA::alignSchedule2() without setting lifetime_tx/lifetime_rx" << endl;
        return 1; // error
    }

    cout << "r" << array_index <<  " //// alignSchedule4() /////////////////////////////////////////////////////////////////////////" << endl;

    cout << "r" << array_index << " TDMA found_tx_counter " << found_tx_counter << "\n";
    cout << "r" << array_index << " TDMA found_frame " << found_frame << "\n";
    cout << "r" << array_index << " TDMA found_epoc " << found_epoc << "\n";

    std::string op = "";
    uint32_t adj = 0;
    
    if( found_tx_counter < found_frame ) {
        op = "add";
        adj = found_frame - found_tx_counter;

    } else if(found_tx_counter > found_frame) {
        op = "sub";
        adj = found_tx_counter - found_frame;
    } else {
        cout << "r" << array_index << " TDMA counters are already equal, skipping update\n";
    }
    


    // 0   &lifetime_32
    // 1   &frame_counter
    // 2   &schedule->offset
    // 3   &schedule->epoc_time
    // 4   &tdma_subcarrier
    // 5   &tdma_mode
    // 15  &debug_generic_op

    if( op != "" ) {
        cout << "r" << array_index << " TDMA adj: '" << op << "' " << adj << "\n";
        partnerOp(op, 0, adj);
    }

    return 0; // success
}

// returns 0 for success
// returns other for errors
int RadioDemodTDMA::alignSchedule5() {
    HiggsTDMA& td = tdma[data_subcarrier_index];


    // uint32_t found_tx_counter = td.lifetime_tx;
    uint32_t found_frame = td.lifetime_rx;
    uint32_t found_epoc = td.epoc_tx;

    cout << "r" << array_index <<  " //// alignSchedule5() /////////////////////////////////////////////////////////////////////////" << endl;

    // cout << "r" << array_index << " TDMA found_tx_counter " << found_tx_counter << "\n";
    cout << "r" << array_index << " TDMA found_frame " << found_frame << "\n";
    cout << "r" << array_index << " TDMA found_epoc " << found_epoc << "\n";

    uint32_t calc_epoc = found_frame / SCHEDULE_FRAMES;
    
    cout << "r" << array_index << " TDMA calc_epoc " << calc_epoc << "\n";

    std::string op = "";
    uint32_t adj = 0;
    
    if( calc_epoc < found_epoc ) {
        op = "add";
        adj = found_epoc - calc_epoc;

    } else if(calc_epoc > found_epoc) {
        op = "sub";
        adj = calc_epoc - found_epoc;
    } else {
        cout << "r" << array_index << " TDMA epocs are already equal, skipping update\n";
    }
    


    // 0   &lifetime_32
    // 1   &frame_counter
    // 2   &schedule->offset
    // 3   &schedule->epoc_time
    // 4   &tdma_subcarrier
    // 5   &tdma_mode
    // 15  &debug_generic_op

    if( op != "" ) {
        cout << "r" << array_index << " TDMA epoc adj: '" << op << "' " << adj << "\n";
        partnerOp(op, 3, adj);
    }

    return 0; // success
}


// returns 0 for success
// returns other for errors
// int RadioDemodTDMA::alignSchedule6() {
//     // HiggsTDMA& td = tdma[data_subcarrier_index];


//     // uint32_t found_tx_counter = td.lifetime_tx;
//     // uint32_t found_frame = td.lifetime_rx;
//     // uint32_t found_epoc = td.epoc_tx;

//     cout << "r" << array_index <<  " //// alignSchedule6() /////////////////////////////////////////////////////////////////////////" << endl;


//     raw_ringbus_t rb0 = {RING_ADDR_TX_PARSE, LIFETIME_TO_EPOC_CMD};
//     ringbusPeer(peer_id, &rb0);


//     // cout << "r" << array_index << " TDMA found_tx_counter " << found_tx_counter << "\n";
//     // cout << "r" << array_index << " TDMA found_frame " << found_frame << "\n";
//     // cout << "r" << array_index << " TDMA found_epoc " << found_epoc << "\n";

//     // uint32_t calc_epoc = found_frame / SCHEDULE_FRAMES;
    
//     // cout << "r" << array_index << " TDMA calc_epoc " << calc_epoc << "\n";

//     // std::string op = "";
//     // uint32_t adj = 0;
    
//     // if( calc_epoc < found_epoc ) {
//     //     op = "add";
//     //     adj = found_epoc - calc_epoc;

//     // } else if(calc_epoc > found_epoc) {
//     //     op = "sub";
//     //     adj = calc_epoc - found_epoc;
//     // } else {
//     //     cout << "r" << array_index << " TDMA epocs are already equal, skipping update\n";
//     // }
    


//     // 0   &lifetime_32
//     // 1   &frame_counter
//     // 2   &schedule->offset
//     // 3   &schedule->epoc_time
//     // 4   &tdma_subcarrier
//     // 5   &tdma_mode
//     // 15  &debug_generic_op

//     // if( op != "" ) {
//     //     cout << "r" << array_index << " TDMA epoc adj: '" << op << "' " << adj << "\n";
//     //     partnerOp(op, 3, adj);
//     // }

//     return 0; // success
// }

/// Top level entry for work.  RadioEstimate should call this during it's tick()
/// Pass fresh arguments each time we run.  This way we avoid having multiple and inconsistent
/// outside accessors
void RadioDemodTDMA::run(const size_t _eq) {
    times_eq_sent = _eq;
    // cout << "r" << array_index << " tdma run with " << times_eq_sent << "\n";
    alternateDspRunDemod();
}



void RadioDemodTDMA::setDemodEnabled(bool _v) {
    demod_enabled = _v;
}

bool RadioDemodTDMA::getDemodEnabled() const {
    return demod_enabled;
}


void RadioDemodTDMA::alternateDspRunDemod() {
    if (demod_enabled) {
        if(demod_buf_accumulate[data_subcarrier_index].size() >= subcarrier_chunk) {
            alternateDemodTDMA(data_subcarrier_index, 0, true);
            parseDemodWords(data_subcarrier_index);
        }
    } else {
        if(demod_buf_accumulate[data_subcarrier_index].size() >= subcarrier_chunk) {
            dumpBuffers();
        }
    }
}




void RadioDemodTDMA::parseDemodWords(const uint32_t buffer_index) {

    auto& dout = demod_words[buffer_index];

    HiggsTDMA& td = tdma[buffer_index];

    // uint32_t local_delta_mod_estimate;

    

    ///
    /// This loop considers data that has been demodulated by the phase above
    /// 
    /// If we dont adjust, and we are in mode 4, we can reliably get 
    ///   "Schedule Sync found delta"  to print down below with fixed offsets
    /// this means the counters are locked
    ///
    /// there is a 5 word memory which is what all the p,pp,ppp is about
    /// each p stands for "previous" and this is a manual way to 
    /// keep a short history or state.  they are behaving like a shift register
    ///
    if( dout.size() > 1000 ) {
        uint32_t i = 0;
        uint32_t p,pp,ppp,pppp,ppppp;
        ppppp = pppp = ppp = pp = p = -1;
        uint32_t frame_p,frame_pp,frame_ppp,frame_pppp,frame_ppppp;
        frame_p = frame_pp = frame_ppp = frame_pppp = frame_ppppp = -1;

        for(auto ppair : dout)
        {
            uint32_t word = ppair.first;
            uint32_t frame = ppair.second;

            if( 
                first_dprint > 0 
                // && word == 0xdeadbeef
                 ) {
                cout << "f:" << frame << " " << (frame % SCHEDULE_FRAMES)
                << " " << ((double)(frame % SCHEDULE_FRAMES) / (double)SCHEDULE_LENGTH)
                << " " << ((frame % SCHEDULE_FRAMES) % SCHEDULE_LENGTH)
                << " " << HEX32_STRING(word) << endl;
                first_dprint--;
            }

            if(word == 0xdeadbeef) {
                if(!td.found_dead) {
                    // soapy->setPartnerTDMA(peer_id, 4, 0);
                    // td.sent_tdma = true;
                    if( _print_fine_schedule ) {
                        cout << "r" << array_index << " TDMA in mode " << last_mode_sent << " " << "found dead/tdma command at " << frame << endl;
                    }
                    if(track_demod_against_rx_counter) {
                        if( _print_fine_schedule ) {
                            cout << endl << endl;
                            cout << "-----------------------------------" << endl;
                            cout << "should save" << endl;
                        }
                        track_record_rx = frame;

                        if( print_count_after_beef != 0  ) {
                            // doesn't seem to overflow us
                            first_dprint = print_count_after_beef;
                        }
                    }
                }
                    
                td.found_dead = true;
            }

            if( print_all_coarse_sync ) {
                cout << "r" << array_index << " frame " << frame << " word " << HEX32_STRING(word) << endl;
            }

            // deadbeef      pppp
            // lifetime      ppp
            // epoc_time     pp
            // hash          p
            // ff000000      word

            if( word == 0xff000000 && pppp == 0xdeadbeef ) {

                // we sample the tx counter agasint the rx frame counter here:
                // we abstract all the p and ppp's away at the beginning of the function
                // so we can change it later if needed
                uint32_t found_tx_counter = ppp;
                uint32_t found_frame = frame_ppp;

                uint32_t found_epoc = pp;
                uint32_t found_hash = p;

                uint32_t hash_bundle[3] = {pppp,ppp,pp};

                cout << "\n";
                cout << HEX32_STRING(pppp) << "\n";
                cout << HEX32_STRING(ppp) << "\n";
                cout << HEX32_STRING(pp) << "\n";
                cout << HEX32_STRING(p) << "\n";
                cout << HEX32_STRING(word) << "\n";

                // beyond this point don't use word, p's, frame, frame_p's


                // constexpr uint32_t tx_counter_fudge = 0;


                cout << "r" << array_index << " TDMA in mode " << last_mode_sent << " frame " << found_frame << " matched TDMA 6 " << endl;
                times_matched_tdma_6++;

                uint32_t calculated_hash = xorshift32(1, (unsigned int*)hash_bundle, 3);
                
                cout << "r" << array_index << " TDMA hash found " << HEX32_STRING(found_hash) << " , calculated " << HEX32_STRING(calculated_hash) << "\n";



                if( td.needsUpdate() ) {
                    // we take the rx frame counter from the word
                    // that went the lifetime_32.
                    // this means we shouldn't need any further offsets and we just want to make them the same
                    td.lifetime_tx = found_tx_counter;
                    td.lifetime_rx = found_frame;
                    td.epoc_tx = found_epoc;

                    if( _print_fine_schedule ) {
                        cout << "r" << array_index << " TDMA needsUpdate() in mode " << last_mode_sent << " " <<  "Fine Schedule Sync found delta: " << found_tx_counter << " at " << found_frame << " (" << found_frame % SCHEDULE_FRAMES << ")\n";
                    }
                }
                auto delta_estimate = td.lifetime_tx - td.lifetime_rx;

                // if( td.needs_fudge == false ) {
                //     cout << "TDMA marked " << " in mode " << last_mode_sent << endl;
                //     td.fudge_rx = delta_estimate;
                //     td.needs_fudge = true;
                // }

                if(track_demod_against_rx_counter) {
                    // cout << "adding to history" << endl;

                    history_tx_rx_deltas.push_back(
                        std::pair<uint32_t,uint32_t>(
                            found_frame, delta_estimate
                            )
                        );
                }
            }


            ///
            /// shift register behavior
            ///
            ppppp = pppp;
            pppp = ppp;
            ppp = pp;
            pp = p;
            p = word;
            frame_ppppp = frame_pppp;
            frame_pppp = frame_ppp;
            frame_ppp = frame_pp;
            frame_pp = frame_p;
            frame_p = frame;
            i++;

        }

        // dout.erase(dout.begin(), dout.end()-10);
        dout.erase(dout.begin(), dout.end());
        (void)p;
        (void)pp;
        (void)ppp;
        (void)pppp;
        (void)ppppp;
        (void)frame_p;
        (void)frame_pp;
        (void)frame_ppp;
        (void)frame_pppp;
        (void)frame_ppppp;
    }

}





void RadioDemodTDMA::dumpBuffers() {
    for (unsigned i = 0; i < demod_buf_accumulate.size(); i++) {
        demod_buf_accumulate[i].resize(0);
    }
}


void RadioDemodTDMA::found_sync(const int found_i) {
#ifdef ENABLE_VERBOSE_DEMOD_BUF_PRINT
    if( print_found_sync <= 0 ) {
        return;
    }

    if( array_index != 0 ) {
        return;
    }

    static bool print_demod_array_open = false;
    static ofstream myfile;

    if(!print_demod_array_open) {
        myfile.open ("demod_buf_accumulate.txt");
        print_demod_array_open = true;
    }

    for(int k = 0; k < data_tone_num; k++) {
        myfile << "demod_buf_accumulate["<< k << "].size() " << demod_buf_accumulate[k].size() << endl;
    }

    myfile << "found_i " << found_i << " was at frame " << demod_buf_accumulate[0][found_i].second << endl;
    myfile << "print start" << endl << "----------" << endl << endl;




    for(unsigned j = 0; j < data_tone_num; j++) {
        for(unsigned i = found_i; i < demod_buf_accumulate[j].size(); i++) {
            auto bit_value = get_demod_2_bits(demod_buf_accumulate[j][i].first);
            myfile << "a[" << j << "][" << i << "] = " << bit_value << ";" << endl;
        }
        myfile << endl;
    }

    print_found_sync--;
#endif
}


void RadioDemodTDMA::print_found_demod_final(int found_i) {
    int found_i_positive;
    // subtract 32, but force to be positibe
    found_i_positive = std::max(found_i-32, 0);

    // does nothing, acts as if always enabled
    if( print_found_sync == 0) {
        print_found_sync = 100;
    }

    found_sync(found_i_positive);
}




void RadioDemodTDMA::erasePrevDeltas() {
    history_tx_rx_deltas.erase(history_tx_rx_deltas.begin(),
        history_tx_rx_deltas.end());
}


} // namespace
} // namespace
