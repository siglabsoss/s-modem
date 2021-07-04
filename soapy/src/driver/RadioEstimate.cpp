#include "driver/RadioEstimate.hpp"

#include "EventDspFsmStates.hpp"

#include <math.h>
#include <cmath>

#define PI (M_PI)

#include <fstream>

#include "driver/EventDsp.hpp"
#include "driver/TxSchedule.hpp"
#include "HiggsDriverSharedInclude.hpp"
#include "driver/FsmMacros.hpp"
#include "driver/EventDspFsmStates.hpp"
#include "CustomEventTypes.hpp"
#include "vector_helpers.hpp"
#include "common/convert.hpp"
#include "schedule.h"
#include "driver/AirPacket.hpp"
#include "driver/VerifyHash.hpp"
#include "driver/DemodThread.hpp"
#include "common/DataVectors.hpp"
#include "driver/RadioDemodTDMA.hpp"
#include "common/GenericOperator.hpp"
#include "random.h"
#include <chrono>
#include <ctime>

#include <future>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h> 
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <boost/functional/hash.hpp>
#include <numeric>
#include "fixed_iir.h"

#include <unistd.h>


using namespace std;
using namespace siglabs::smodem;


static void handle_localfsm_tick(int fd, short kind, void *_radio)
{ 

    // cout<<"!!!!!!!!!!!!!!!!!!!!!!"<<endl;
    RadioEstimate *radio = (RadioEstimate*) _radio;
    radio->idleCheckRemoteRing();
    radio->tick_localfsm();

}


// static void handle_slow_poll_cpu_load(int fd, short kind, void *_radio)
// {
//     RadioEstimate *radio = (RadioEstimate*) _radio;
//     if(radio->GET_RADIO_ROLE() == "rx" && radio->GET_POLL_FOR_CPU_LOAD()) {
//         // dump
//         raw_ringbus_t rb0 = {RING_ADDR_CS11, PERF_CMD | (1<<16) };
//         radio->dsp->zmqRingbusPeerLater(radio->peer_id, &rb0, 0);

//         // reset
//         raw_ringbus_t rb1 = {RING_ADDR_CS11, PERF_CMD | (2<<16) };
//         radio->dsp->zmqRingbusPeerLater(radio->peer_id, &rb1, 10*1000);

//         struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
//         evtimer_add(radio->slow_poll_cpu_timer, &timeout);
//     }
// }

RadioEstimate::RadioEstimate(
    HiggsDriver *s, 
    EventDsp *_dsp, 
    size_t p, 
    struct event_base* e,
    size_t ary_index,
    bool setup_fsm
                    ) : soapy(s)
                        ,dsp(_dsp)
                        ,settings((soapy->settings))
                        ,demod(new RadioDemodTDMA(ary_index, p, DATA_TONE_NUM, SUBCARRIER_CHUNK))
                        ,dispatchOp(new siglabs::rb::DispatchGeneric())
                        ,peer_id(p)                     // peer id, (was) written on the wall in the hallway
                        ,array_index(ary_index)
                        ,evbase_localfsm(e)
                        ,should_setup_fsm(setup_fsm)
                        ,verifyHash(new VerifyHash())
                         {


    demod->setIndex(getIndexForDemodTDMA());

    // setAdvanceCallback expects a std::function.  This function must be of the form
    //        // void name(size_t,uint32_t);
    // and must not be a member function.
    // However we CAN pass a member function if we create a std::function using a lambda.
    // This lambda allows us to bind to the EventDsp pointer and pass it.

    // We put the nameless lamba directly as the argument, and it looks a bit like javascript

    // demod->setAdvanceCallback([_dsp](const size_t a, const uint32_t b) {
    //     _dsp->advancePartnerSchedule(a,b);
    // });
    demod->setSetPartnerTDMACallback([_dsp](const size_t a, const uint32_t b, const uint32_t c) {
        _dsp->setPartnerTDMA(a,b,c);
    });
    demod->setRingbusCallback([_dsp](const size_t a, const raw_ringbus_t* const b) {
        _dsp->zmqRingbusPeerLater(a, b, 0);
    });

    dispatchOp->setCallback([](const uint32_t _sel, const uint32_t _val) {
        cout << "Variable index #" << _sel << " has value " << HEX32_STRING(_val) << "\n";
    }, GENERIC_READBACK_PCCMD);



    cfo_update_counter = 100000;
    cfo_estimated_sent = 0.0;
    sfo_estimated_sent = 0.0;
    coarse_ok = false;
    times_cfo_sent = 0;
    times_sfo_sent = 0;
    times_sto_sent = 0;
    times_residue_phase_sent = 0;
    times_eq_sent = 0;
    times_coarse_estimated = 0;
    coarse_state = 0;
    trigger_coarse = false;
    print_first_residue_phase_sent = false; // for printing
    delay_cfo = 0;
    delay_residue_phase = 0;
    prev_coarse_ok = false;
    applied_sfo = false;
    should_run_background = false;
    should_mask_data_tone_tx_eq = true;
    should_mask_all_data_tone = false;
    pause_eq = false;

    // read this value once now into a bool which is used in loops below for performance
    should_print_estimates = GET_PRINT_SFO_CFO_ESTIMATES();

    all_eq_mask.resize(1024);

    enable_residue_to_cfo = false;
    residue_to_cfo_delay = 1000*1000;
    residue_to_cfo_factor = 0.9;
    pause_residue = false;

    times_sfo_estimated = 0;
    times_sto_estimated = 0;
    times_cfo_estimated = 0;
    times_sto_estimated_p = 0;
    times_sfo_estimated_p = 0;
    times_cfo_estimated_p = 0;
    times_dsp_run_channel_est = 0;


    old_demod_print_counter_limit = 128;
    old_demod_print_counter = 0;

    est_remote_perf = 0;
    // cpu_load.resize(1); // FIXME



    // allow_epoc_adjust = false;

    map_mov_packets_sent = 0;
    map_mov_acks_received = 0;


    sto_estimated = 0.0;
    residue_phase_estimated = 0.0;
    residue_phase_estimated_pre = 0.0;

    // sfo_done_flag = false;
    // sto_done_flag = false;
    // cfo_done_flag = false;
    cheq_done_flag = false;
    cheq_onoff_flag = false;
    reset_flag = true;
    residue_phase_flag = false;

    equalizer_coeff_sent.resize(1024);
    channel_angle.resize(1024);

    pilot_angle.resize(1024);

    channel_angle_sent.resize(1024);
    channel_angle_sent_temp.resize(1024);

    cheq_background_counter = 0;

    // only set this once, so we can use init.json to change
    // ("dsp.residue.rate_divisor")
    // but its easy to change via rc console
    drop_residue_target = GET_RESIDUE_RATE_DIVISOR();

    // sfo_estimated_sent_array.resize(SFO_ARRAY_SIZE);


    margin = GET_STO_MARGIN();

    initChannelFilter();


    applyeqonce = true;


    /////////////////////////////////////////////////////
    // for(int index = 0; index<SFO_ARRAY_SIZE; index++)
    // {
    //     sfo_estimated_sent_array[index] = 0.0;
    // }
    // sfo_estimated_sent_array_index = 0;


    /////////////////////////////////////////////////////

    _dashboard_eq_update_ratio = GET_ONLY_UPDATE_RATE_DIVISOR();
    _dashboard_eq_update_counter = 0;
    _dashboard_only_update_channel_angle_r0 = GET_ONLY_UPDATE_CHANNEL_ANGLE_R0();
    demod->_print_fine_schedule = GET_PRINT_RE_FINE_SCHEDULE();




    // const uint32_t SYNC_WORD = 0xcafebabe;

    feedback_alive_count = 0;

    _cfo_symbol_num = GET_CFO_SYMBOL_NUM();
    _sfo_symbol_num = GET_SFO_SYMBOL_NUM();

    _eq_use_filter = GET_EQ_USE_FILTER();

    _print_on_new_message = GET_PRINT_RE_NEW_MESSAGE();

    most_recent_event.d0 = NOOP_EV;
    fsm_event_pending = NOOP_EV;

    last_print_small_mag = init_timepoint = last_residue_timepoint = std::chrono::steady_clock::now();



    setupEqOne();

    epoc_valid = false;
    setupEqToSto();

    // initTun();


    if(should_setup_fsm) {
        tieAll();
    }

    // sub constructor
    dspSetupDemod();

    if(should_setup_fsm) {
        setup_localfsm();
        init_localfsm();
    }
}

uint32_t RadioEstimate::getPeerId() const {
    return peer_id;
}
uint32_t RadioEstimate::getArrayIndex() const {
    return array_index;
}

/// returns the index into demod_buf (for all radios)
/// remember that there are originally 64 full subcarriers we forward to the pc
/// then we strip out all the pilots.
/// this must be parallel to getAllTransmitTDMA()
/// however this value is modified by the reverse mover because
/// this value is an index into the OUTPUT of the reverse mover divided by 2
/// these fixed values are assuming a fixed reverse mover
std::vector<unsigned> RadioEstimate::getAllDemodTDMA() {
    std::vector<unsigned> values = {31,30};
    return values;
}

/// All subcarrier index from the transmitters indexing scheme
/// is used for TDMA subcarrier
/// This must be parallel to getAllDemodTDMA()
/// This is also the index scheme used for transmit eq
std::vector<unsigned> RadioEstimate::getAllTransmitTDMA() {
    // 1024 - 1007 = 17
    std::vector<unsigned> values = {17,19};
    return values;
}

/// Index into demod_buf this radio should use
unsigned RadioEstimate::getIndexForDemodTDMA() const {
    const std::vector<unsigned> values = getAllDemodTDMA();

    if(array_index >= values.size()) {
        cout << "ILLEGAL VALUE IN getIndexForDemodTDMA() " << array_index << "\n";
        return 0;
    }
    return values[array_index];
}


/// tx Subcarrier to use for TDMA
unsigned RadioEstimate::getScForTransmitTDMA() const {
    const std::vector<unsigned> values = getAllTransmitTDMA();

    if(array_index >= values.size()) {
        cout << "ILLEGAL VALUE IN getScForTransmitTDMA() " << array_index << "\n";
        return 0;
    }
    return values[array_index];
}

#define VEC_ID(ai, idx) ((TIE_VECTOR_RANGE) + (TIE_VECTOR_PER_INDEX*(ai)) + (idx))

void RadioEstimate::tieAll() {
    // RE.update('toxJkynrJzB4h4Beo', {'$set' : {sfo:{sfo_estimated:99}}})
    // soapy->tieDashboard
    uint32_t ai = this->array_index;


    // sto
    dsp->tieDashboard<size_t>(&times_sto_estimated, ai, "sto", "times_sto_estimated" );
    dsp->tieDashboard<double>(&sto_estimated, ai, "sto", "sto_estimated");
    dsp->tieDashboard<double>(&sto_delta, ai, "sto", "sto_delta");

    // sfo
    dsp->tieDashboard<double>(&sfo_estimated, ai, "sfo", "sfo_estimated" );
    dsp->tieDashboard<double>(&sfo_estimated_sent, ai, "sfo", "sfo_estimated_sent" );
    dsp->tieDashboard<size_t>(&times_sfo_estimated, ai, "sfo", "times_sfo_estimated" );
    dsp->tieDashboard<size_t>(&times_sfo_sent, ai, "sfo", "times_sfo_sent");
    dsp->tieDashboard<bool>(&applied_sfo, ai, "sfo", "applied_sfo");
    
    // cfo
    dsp->tieDashboard<double>(&cfo_estimated, ai, "cfo", "cfo_estimated");
    dsp->tieDashboard<double>(&cfo_estimated_sent, ai, "cfo", "cfo_estimated_sent");
    dsp->tieDashboard<size_t>(&times_cfo_sent, ai, "cfo", "times_cfo_sent" );
    dsp->tieDashboard<size_t>(&times_cfo_estimated, ai, "cfo", "times_cfo_estimated" );

    // residue
    dsp->tieDashboard<double>(&residue_phase_trend, ai, "residue", "residue_phase_trend" );

    // eq (not vector parts of eq)
    dsp->tieDashboard<size_t>(&times_eq_sent, ai, "eq", "times_eq_sent" );
    // this is the data tone that the schecule sync is running on



    dsp->tieDashboard<double>(&cpu_load[0], ai, "system", "cs20", "cpu_load" );
    dsp->tieDashboard<size_t>(&peer_id, ai, "system", "peer_id");

// size_t times_cfo_sent;
//     size_t times_cfo_estimated;

    dsp->tieDashboard(&radio_state, ai, "fsm", "state");
    dsp->tieDashboard(&should_run_background, ai, "fsm", "control", "should_run_background");
    dsp->tieDashboard(&should_mask_data_tone_tx_eq, ai, "fsm", "control", "should_mask_data_tone_tx_eq");
    dsp->tieDashboard(&should_mask_all_data_tone, ai, "fsm", "control", "should_mask_all_data_tone");

    // dsp->tieDashboard<int32_t>(&demod_est_common_phase, ai, "demod", "demod_est_common_phase");
    // dsp->tieDashboard<int32_t>(&radio_state, ai, "demod", "demod_special_phase");

    //     should_run_background = false;
    // should_mask_data_tone_tx_eq = true;

    // dsp->tieDashboard<vector<double>>(&channel_angle, VEC_ID(ai,0), "vector");

    // dsp->tieDashboard<vector<double>>(&radio_state, ai, "schedule", "history");

    dsp->tieDashboard(&demod->tdma_phase, ai, "demod", "tdma_phase");
    dsp->tieDashboard(&demod->times_matched_tdma_6, ai, "demod", "times_matched_tdma_6");
    dsp->tieDashboard(&demod->data_subcarrier_index, ai, "demod", "data_subcarrier_index");
    dsp->tieDashboard(&demod->track_demod_against_rx_counter, ai, "demod", "track_demod_against_rx_counter");
    dsp->tieDashboard(&demod->track_record_rx, ai, "demod", "track_record_rx");
    dsp->tieDashboard(&demod->last_mode_sent, ai, "demod", "last_mode_sent");
    dsp->tieDashboard(&demod->last_mode_data_sent, ai, "demod", "last_mode_data_sent");
    

    dsp->tieDashboard(&demod->td->found_dead, ai, "demod", "td", "found_dead");
    dsp->tieDashboard(&demod->td->sent_tdma, ai, "demod", "td", "sent_tdma");

    dsp->tieDashboard(&demod->td->lifetime_tx, ai, "demod", "td", "lifetime_tx");
    dsp->tieDashboard(&demod->td->lifetime_rx, ai, "demod", "td", "lifetime_rx");
    dsp->tieDashboard(&demod->td->fudge_rx, ai, "demod", "td", "fudge_rx");
    dsp->tieDashboard(&demod->td->needs_fudge, ai, "demod", "td", "needs_fudge");




    

    dsp->tieDashboard<bool>(&cheq_done_flag, ai, 
        "fsm", "flags", "cheq_done_flag");
    dsp->tieDashboard<bool>(&cheq_onoff_flag, ai,
        "fsm", "flags", "cheq_onoff_flag");


    // vector types
    // for vectors
    // I will do it a bit different, because they are always large
    // alwaus use "vector", we will use the UUID to differentiate

    dsp->tieDashboard<vector<double>>(&channel_angle, VEC_ID(ai,0), "vector");

    dsp->tieDashboard<vector<double>>(&pilot_angle, VEC_ID(ai,0), "vector");

    dsp->tieDashboard<vector<double>>(&channel_angle_sent, VEC_ID(ai,1), "vector");

    //
    // dsp->tieDashboard<vector<double>>(&channel_angle_sent, VEC_ID(ai,1), "vector");

    // FIXME get a better way for cpu loads

    // dsp->tieDashboard<vector<double>>(&channel_angle_sent, VEC_ID(ai,1), "vector");





    // dsp->tieDashboard<size_t>(&times_cfo_estimated, ai, "sfo", "times_sfo_estimated");

    // dsp->tieDashboard<uint32_t>(&b, array_index, (tie_path_t){"sfo", "b"});
    // dsp->tieDashboard<double>(&f, array_index, (tie_path_t){"sfo", "f"});





}

void RadioEstimate::handleCustomEvent(const custom_event_t* const e) {
    if( _print_on_new_message ) {
        cout << "RadioEstimate::handleCustomEvent() d0 " << e->d0 << " d1 " << e->d1 << endl;
    }
    switch(e->d0) {
        case REQUEST_FINE_SYNC_EV:
            if( e->d1 == array_index) {
                most_recent_event = *e; // this is how we tell the fsm we got an event
                
                if( _print_on_new_message ) {
                    cout << "Radio " << this->array_index << " matched event" << endl;
                    cout << "After event_active" << endl;
                }
                
            }
            break;
        case CHECK_TDMA_EV:
        case REQUEST_TDMA_EV:
            if( e->d1 == array_index) {
                most_recent_event = *e;
            }

            break;
    }
}

void RadioEstimate::sendEvent(const custom_event_t e) {
    dsp->sendEvent(&e);
}


void RadioEstimate::resetCoarseState() {
    if( dsp->_rx_should_insert_sfo_cfo ) {
        cout << "Warning resetCoarseState() while dsp's _rx_should_insert_sfo_cfo flag is set\n";
    }

    // sfo_buf
    sfo_estimated = 0;

    size_t burned;

    burned = sfo_buf.dump();
    cout << "sfo_buf dumped " << burned << " elements\n";
    burned = cfo_buf.dump();
    cout << "cfo_buf dumped " << burned << " elements\n";


    stopBackground();

    residue_phase_estimated = 0;
    residue_phase_estimated_pre = 0;
    residue_phase_flag = false;
    residue_phase_trend = 0;
    residue_phase_history.resize(0);
    times_residue_phase_sent = 0;

    // resetting this to zero is illegal, we should reset either to the data in init.json
    // or to the hot start data..ugh
    // cfo_estimated = 0;
    // cfo_estimated_sent = 0;
    // dsp->setPartnerCfo(peer_id, cfo_estimated*-1);

    // times_sto_estimated = 0;
    // times_sfo_estimated = 0;
    // times_cfo_estimated = 0;

    // dspRunResiduePhase
    // dspRunCfo_v1
    // updatePartnerCfo

}

void RadioEstimate::resetTdmaState() {
    // track_demod_against_rx_counter
    // times_wait_tdma_state_0
}


void RadioEstimate::setup_localfsm()
{
    localfsm_next       = event_new(evbase_localfsm, -1, EV_PERSIST, handle_localfsm_tick, this);
    localfsm_next_timer = evtimer_new(evbase_localfsm, handle_localfsm_tick, this);
    // slow_poll_cpu_timer = evtimer_new(evbase_localfsm, handle_slow_poll_cpu_load, this);


    // struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
    // evtimer_add(slow_poll_cpu_timer, &timeout);
}

// call this to start the fsm running
// init and reset the fsm state
void RadioEstimate::init_localfsm()
{
    
    radio_state_pending = radio_state = DID_BOOT;

    // calls the event ON THE SAME STACK as us
    event_active(localfsm_next, EV_WRITE, 0);
    // cout<<"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@"<<endl;
}


/**
Calculates an sfo estimate by consuming `_sfo_symbol_num` samples at a time,
which are removed from `sfo_buf`.

This actually **runs on the HiggsFineSync thread (an async from there)**


Input Member Variables:
-------------------
- sfo_buf

Output Member Variables:
-----------------------
- sfo_buf (removes values)
- sto_estimated
- times_sto_estimated
- times_sfo_estimated

*/

void RadioEstimate::dspRunSfo_v1(void) {

    if(sfo_buf.size() < _sfo_symbol_num) {
        cout << "r" << array_index << " dspRunSfo_v1() called with too few samples " << sfo_buf.size() << "\n";
        return;
    }

    cout << "dspRunSfo_v1.size() " << sfo_buf.size() << endl;

   
    // if(peer_id == 0)
    // {
    //         time_t now = time(0);
   
    //        // convert now to string form
    //        char* dt = ctime(&now);

    //        cout << "SFO estimate timer:    "<<dt<<endl;
    // }



    ////////////////////////////////////////////////////////////

    std::vector<uint32_t> sfo_buf_chunk = sfo_buf.get(_sfo_symbol_num);

    //// for debug purpose

    // if((array_index == 0))
    // {
    //     for(int index; index<1000;index++)
    //     {
    //         cout<<sfo_buf_chunk[index]<<","<<endl;
    //     }
    // }
    
    ///////////////////////////////////////////////////////////////

    if( sfo_sto_use_duplex == false ) {

        double constant_for_pilot_gap = 1.0;
        (void)constant_for_pilot_gap;
        ///// calculate sfo
        double sfo_x_mean = (_sfo_symbol_num-1)/2.0;
        double sfo_y_mean = 0.0;
        

        double sfo_xx_sum = 0.0;

        {
            uint32_t x = 0;
            for(auto n : sfo_buf_chunk)
            {
                int32_t temp;

                if(n>0x7fff)
                //if(n>0x9fff)
                {
                    temp = n - 0x10000;
                }
                else
                {
                    temp = n;
                }

                sfo_y_mean = sfo_y_mean + temp*(1.0);

                sfo_xx_sum = sfo_xx_sum + (x*1.0-sfo_x_mean)*(x*1.0-sfo_x_mean);

                x++;
               
            }
        }

        sfo_y_mean = sfo_y_mean/_sfo_symbol_num;

        


        double sfo_yx_sum = 0.0;
        
        {
            uint32_t x = 0;
            for(auto n : sfo_buf_chunk)
            {
                int32_t temp;

                if(n>0x7fff)
                //if(n>0x9fff)
                {
                    temp = n - 0x10000;
                }
                else
                {
                    temp = n;
                }

                sfo_yx_sum = sfo_yx_sum + (temp*(1.0)-sfo_y_mean)*(x*1.0-sfo_x_mean);

                x++;
            }
        }

        sfo_estimated = SFO_ADJ*1.0*sfo_yx_sum/sfo_xx_sum;
        sto_estimated = sfo_y_mean;

        if( should_print_estimates ) {
            cout << "r" << array_index << " sfo_estimated                " << sfo_estimated <<"   "<< HEX32_STRING(uint32_t(sto_estimated))<<"   "<<((uint32_t)(abs(sto_estimated))>>STO_ADJ_SHIFT)<< endl;
        }

        /////// to see if it can improve the accuracy of estimation
        double sfo_slope = sfo_yx_sum/sfo_xx_sum;
        double sfo_intercept = sfo_y_mean - sfo_slope*sfo_x_mean;

        double sfo_data_remove_outlier[_sfo_symbol_num];
        double data_temp = 0.0;

        uint32_t index = 0;

        int sfo_fitting_threshold = GET_SFO_FITTING_THRESHOLD();

        for(auto n : sfo_buf_chunk)
        {
                  
      
            if(n>0x7fff)
            //if(n>0x9fff)    
            {
                data_temp = (65536-n)*(-1.0);
            }
            else
            {
                data_temp = n*1.0;
            }
            if(abs(data_temp-(index*sfo_slope+sfo_intercept))<= sfo_fitting_threshold)  //// this threashold is very important, for antenna it may be 800; while for cable, it may be 100
            {
                sfo_data_remove_outlier[index] = data_temp;
            }
            else
            {
                sfo_data_remove_outlier[index] = index*sfo_slope+sfo_intercept;
            }
            index=index+1;
        }


        sfo_x_mean = (_sfo_symbol_num-1)/2.0;
        sfo_y_mean = 0.0;

        sfo_xx_sum = 0.0;

        for(uint32_t x = 0; x<_sfo_symbol_num; x++)
        {
            
            sfo_y_mean = sfo_y_mean + sfo_data_remove_outlier[x];

            sfo_xx_sum = sfo_xx_sum + (x*1.0-sfo_x_mean)*(x*1.0-sfo_x_mean);
           
        }

        sfo_y_mean = sfo_y_mean/_sfo_symbol_num;

        sfo_yx_sum = 0.0;
        
        for(uint32_t x = 0; x<_sfo_symbol_num; x++)  
        {
            sfo_yx_sum = sfo_yx_sum + (sfo_data_remove_outlier[x]*1.0-sfo_y_mean)*(x*1.0-sfo_x_mean);

        }  

        sfo_estimated = SFO_ADJ*1.0*sfo_yx_sum/sfo_xx_sum;
        sto_estimated = sfo_y_mean;
    }

    ///////////////////////////////////////////////////////////////////

    // sfo_done_flag = true;
    // sto_done_flag = true;

    times_sto_estimated++;
    times_sfo_estimated++;

    dsp->tickle(&times_sfo_estimated);
    dsp->tickle(&times_sto_estimated);
    dsp->tickle(&sto_estimated);
    dsp->tickle(&sfo_estimated);


    //////////////////////simple try for sto_estimated////////////////////////////

    if( sfo_sto_use_duplex ) {
        std::vector<uint32_t> sfo_array_no_zero;

        for(unsigned index=0; index<_sfo_symbol_num; index++)
        {
            if(sfo_buf_chunk[index] != 0)
            {
                sfo_array_no_zero.push_back(sfo_buf_chunk[index]);
            }
        }

        auto sfo_array_no_zero_len = sfo_array_no_zero.size();


        double sfo_y_mean = 0.0;

       for(auto n : sfo_array_no_zero)
        {
            int32_t temp;

            if(n>0x7fff)
            //if(n>0x9fff)
            {
                temp = n - 0x10000;
            }
            else
            {
                temp = n;
            }

            sfo_y_mean = sfo_y_mean + temp*(1.0);
        }

        sto_estimated = sfo_y_mean/(sfo_array_no_zero_len*1.0);
    }


/////////////////////////////////////////////////////////////////////////////////////////

	bool use_cfo_for_sfo = true;
	
	if( use_cfo_for_sfo ) {
	    sfo_estimated = cfo_estimated/30.0;
	}

    if( should_print_estimates ) { 
        cout << "r" << array_index << " sfo_estimated (reestimated)  " << sfo_estimated <<"   "<< HEX32_STRING(uint32_t(sto_estimated))<<"   "<<((uint32_t)(abs(sto_estimated))>>STO_ADJ_SHIFT)<<endl;
    }


    // cout << "r" << array_index << " sfo_estimated (from cfo_estimated)  " << sfo_estimated <<endl;

    // //for debug purpose
    // if(abs(sfo_estimated)>GET_SFO_RESTIMATE_TOL())
    // {
    //     // for(int index=0; index<1000; index++)
    //     // {
    //     //     cout<<sfo_buf_chunk[index*100]<<","<<endl;
    //     // }

    //     dspRunSfo_v11(sfo_buf_chunk);

    // }

    // if( adjust_sto_using_sfo ) {
    //     updateStoUsingSfo();
    // }

    // sfo_tracking_0 = sfo_tracking_1;
    // sfo_tracking_1 = sfo_estimated;



    // //for debug purpose
    // if((sfo_estimated)<-20)
    // {
    //     // for(int index=0; index<1000; index++)
    //     // {
    //     //     cout<<sfo_buf_chunk[index*100]<<","<<endl;
    //     // }

    //     dspRunSfo_v11(sfo_buf_chunk);


    // }

    // if((abs(sfo_estimated)<=1)&&(((uint32_t)(abs(sto_estimated))>>STO_ADJ_SHIFT)>0))
    // {
    //     for(int index = 0; index<1000; index++)
    //     {
    //         cout<<sfo_buf_chunk[index*100]<<","<<endl;
    //     }
    // }
}

void RadioEstimate::compensateFractionalSto(void) {

    double x = ((uint32_t)(abs(sto_estimated)))*1.0/256.0;

    cout << "!!!!!!!!!!!!!franctional sto is:            " << x<< "\n";

    double x_sign = 1.0;

    if(sto_estimated>0)
    {
        x_sign = -1.0;
    }
    else
    {
        x_sign=1.0;
    }

    rotateVectorBySto(channel_angle_sent, channel_angle_sent_temp, x, x_sign);

    const double mag_coeff = 32767.0 / GET_EQ_MAGNITUDE_DIVISOR();

    for(int index = 0; index<1024; index++) {
        int32_t real_part = (int32_t)(cos(channel_angle_sent_temp[index])*mag_coeff);
        int32_t imag_part = (int32_t)(sin(channel_angle_sent_temp[index])*mag_coeff);
        equalizer_coeff_sent[index] = (((uint32_t)(imag_part&0xffff))<<16) + ((uint32_t)(real_part&0xffff));
    }



    // for flip positive and negative frequency
    // eventually, we do not need this part.
    for(int index = 1; index < 512; index++)
    {
        uint32_t temp = equalizer_coeff_sent[index];

        equalizer_coeff_sent[index] = equalizer_coeff_sent[1024-index];
        equalizer_coeff_sent[1024-index] = temp;
    }







}

void RadioEstimate::updateStoUsingSfo(void) {
    if( update_sto_sfo_counter < update_sto_sfo_delay) {
        update_sto_sfo_counter++;
        return;
    }
    update_sto_sfo_counter = 0;

    const double local_sto = sto_estimated / 256.0;
    
    if( abs(local_sto) <= update_sto_sfo_tol ) {
        return;
    }



    const double direction = local_sto>0?1:-1;

    const double adjust = update_sto_sfo_bump * direction;

    sfo_estimated = adjust;

    cout << "updateStoUsingSfo() choosing " << sfo_estimated << "\n";

    updatePartnerSfo();
}


void RadioEstimate::dspRunSfo_v11(const std::vector<uint32_t>& sfo_buf_chunk) {

    cout << "dspRunSfo_v11 " << sfo_buf_chunk.size() << endl;

    ///// calculate sfo
    double sfo_x_mean = (_sfo_symbol_num-1)/2.0;
    double sfo_y_mean = 0.0;
    
    uint32_t x = 0;

    double sfo_xx_sum = 0.0;

    for(auto n : sfo_buf_chunk)
    {
        int32_t temp;

        temp = n;

        sfo_y_mean = sfo_y_mean + temp*(1.0);

        sfo_xx_sum = sfo_xx_sum + (x*1.0-sfo_x_mean)*(x*1.0-sfo_x_mean);

        x=x+1;
       
    }

    sfo_y_mean = sfo_y_mean/_sfo_symbol_num;

    


    double sfo_yx_sum = 0.0;
    
    x = 0;

    for(auto n : sfo_buf_chunk)  
    {
        int32_t temp;

        temp = n;

        sfo_yx_sum = sfo_yx_sum + (temp*(1.0)-sfo_y_mean)*(x*1.0-sfo_x_mean);

        x=x+1;
    }  

    sfo_estimated = SFO_ADJ*1.0*sfo_yx_sum/sfo_xx_sum;
    sto_estimated = sfo_y_mean;
    
    if( should_print_estimates ) {
        cout << "r" << array_index << " sfo_estimated                " << sfo_estimated <<"   "<< HEX32_STRING(uint32_t(sto_estimated))<<"   "<<((uint32_t)(abs(sto_estimated))>>STO_ADJ_SHIFT)<< endl;
    }

    /////// to see if it can improve the accuracy of estimation
    double sfo_slope = sfo_yx_sum/sfo_xx_sum;
    double sfo_intercept = sfo_y_mean - sfo_slope*sfo_x_mean;

    double sfo_data_remove_outlier[_sfo_symbol_num];
    double data_temp = 0.0;

    int sfo_fitting_threshold = GET_SFO_FITTING_THRESHOLD();

    uint32_t index = 0;
    for(auto n : sfo_buf_chunk)
    {
              
  
        
        data_temp = n*1.0;
    

        if(abs(data_temp-(index*sfo_slope+sfo_intercept))<=sfo_fitting_threshold)
        {
            sfo_data_remove_outlier[index] = data_temp;
        }
        else
        {
            sfo_data_remove_outlier[index] = index*sfo_slope+sfo_intercept;
        }
        index=index+1;
    }


    sfo_x_mean = (_sfo_symbol_num-1)/2.0;
    sfo_y_mean = 0.0;

    sfo_xx_sum = 0.0;

    for(uint32_t i = 0; i<_sfo_symbol_num; i++)
    {
        
        sfo_y_mean = sfo_y_mean + sfo_data_remove_outlier[i];

        sfo_xx_sum = sfo_xx_sum + (i*1.0-sfo_x_mean)*(i*1.0-sfo_x_mean);
       
    }

    sfo_y_mean = sfo_y_mean/_sfo_symbol_num;

    sfo_yx_sum = 0.0;
    
    for(uint32_t i = 0; i<_sfo_symbol_num; i++)  
    {
        sfo_yx_sum = sfo_yx_sum + (sfo_data_remove_outlier[i]*1.0-sfo_y_mean)*(i*1.0-sfo_x_mean);

    }  

    sfo_estimated = SFO_ADJ*1.0*sfo_yx_sum/sfo_xx_sum;
    sto_estimated = sfo_y_mean;

    ///////////////////////////////////////////////////////////////////

    // sfo_done_flag = true;
    // sto_done_flag = true;



    if( should_print_estimates ) {
        cout << "sfo_estimated (reestimated)  " << sfo_estimated <<"   "<< HEX32_STRING(uint32_t(sto_estimated))<<"   "<<((uint32_t)(abs(sto_estimated))>>STO_ADJ_SHIFT)<< endl;
    }
    //for debug purpose
    // if((sfo_estimated)<-20)
    // {
    //     for(int index=0; index<1000; index++)
    //     {
    //         cout<<sfo_buf_chunk[index*100]<<","<<endl;
    //     }

        
    // }

}

void RadioEstimate::startBackground() {
    times_sto_estimated_p = times_sto_estimated;
    times_sfo_estimated_p = times_sfo_estimated;
    times_cfo_estimated_p = times_cfo_estimated;
    should_run_background = true;
    pause_residue = false;
    last_eq_to_sto = std::chrono::steady_clock::now();
    eq_to_sto_allowed = true;
    use_sto_eq_method = false;
    times_eq_sent = 0;
    pause_eq = false;
    // fractional_eq_allowed = true;
    if(GET_CFO_ADJUST_USING_RESIDUE()) {
        cout << "GET_CFO_ADJUST_USING_RESIDUE() was true\n";
        enable_residue_to_cfo = true;
    }
}

void RadioEstimate::stopBackground() {
    times_sfo_estimated = 0;
    times_sto_estimated = 0;
    times_cfo_estimated = 0;
    times_sto_estimated_p = 0;
    times_sfo_estimated_p = 0;
    times_cfo_estimated_p = 0;
    should_run_background = false;
    enable_residue_to_cfo = false;
    eq_to_sto_allowed = false;
    // fractional_eq_allowed = false;
    use_sto_eq_method = false;
}

void RadioEstimate::continualBackgroundEstimates() {

    // size_t times_sto_estimated;
    // size_t times_sfo_estimated;
    // size_t times_cfo_estimated;

    ///
    //0: disable data preparation
    //1: reset data to 0x7fff
    //2: get data from pilot frame

    

    //return;

    if(!should_run_background){
        return;
    }

    if(!applyeqonce)
    {
        return;
    }

    cheq_background_counter++;

    // hand tuned value
    const double check_per_second = 250;

    // dsp.eq.update_seconds is a double, we multiply by the radio hand
    // calulated above
    const unsigned counter_target = check_per_second * GET_EQ_UPDATE_SECONDS();

    if(cheq_background_counter >= counter_target)
    {
        cheq_background_counter = 0;

        auto timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
        cout << "r" << array_index << "//// Continuous updatePartnerEq////    " << ctime(&timenow) <<endl;
        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////updatePartnerEq(false, true);
        soapy->cs32EQData(2);
        applyeqonce = false;
        cout << "r" << array_index << " HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH" << ctime(&timenow) <<endl;

        // cs32EQData() causes the gain in cs32 to change a lot
        // so we compensate here by changing the barrel shift of eq stage
        dsp->ringbusPeerLater(peer_id, RING_ADDR_RX_FINE_SYNC, APP_BARREL_SHIFT_CMD | 0x50000 | 0xe, 0);


        if( times_eq_sent == 2 ) {
            // if more than 2 eq are sent, switch to using STO eq method
            cout << "ENTERING use_sto_eq_method\n";


            freezeChannelAngleForStoEq();
            if( GET_STO_ADJUST_USING_EQ() ) {
                use_sto_eq_method = true;
            }
        }

    }
}

void RadioEstimate::freezeChannelAngleForStoEq() {
    channel_angle_sent_frozen = channel_angle_sent;
}


// wrapper around cs00SetSfoAdvance
// we call this on the local rx when updating sfo
// look at internal state, and calculate an estimate
// and send that estimate
void RadioEstimate::updateSfoEstimate(const double e) {
    double estimate = e;
    if(!applied_sfo) {
        applied_sfo = true;
        dsp->tickle(&applied_sfo);

   
        // for(int index = 0; index<10000; index++)
        // {
        //     cout<<"0x"<<HEX32_STRING(sfo_buf_chunk[index+90000])<<","<<"   "<<HEX32_STRING(index)<<"   "<<HEX32_STRING(cfo_buf_chunk[index+90000])<<endl;

        // }
        estimate = sfo_estimated_sent+estimate;

        uint32_t amount = 25600.0 / abs(estimate);
        


        cout << array_index << "//// updateSfoEstimate(2) " << estimate << " ////"<< endl;

        bool adjust_sfo_on_rx = false;

        if(adjust_sfo_on_rx) {
            uint32_t direction  = (estimate>0)?2:1;
            soapy->cs00SetSfoAdvance(amount, direction);
        } else {
            uint32_t direction  = (estimate>0)?1:2;
            dsp->setPartnerSfoAdvance(peer_id, amount, direction);
        }
        
        sfo_estimated_sent = estimate;
        dsp->tickle(&sfo_estimated_sent);

        times_sfo_sent++;
        dsp->tickle(&times_sfo_sent);
    }
}

// returns 0 for success
size_t RadioEstimate::sfoState() {
    cout << "sfoState() returned ";
    if((abs(sfo_estimated)>=GET_SFO_STATE_THRESH()))
    {
        cout << "2\n";
        updatePartnerCfo();
        usleep(500);
        updatePartnerSfo();
        // next_state = SFO_STATE;

        return 2;
    }
    else if((abs(sfo_estimated)<GET_SFO_STATE_THRESH()))
    {
        cout << "0\n";
        // next_state = STO_STATE;
        return 0;
    }
    else
    {
        cout << "1\n";
        return 1;
    }
}

// little converter
uint32_t RadioEstimate::getStoAdjustmentForEstimated(const double est) const {
    auto sto_adjustment = ((uint32_t)(abs(est))>>STO_ADJ_SHIFT);
    return sto_adjustment;
}

// returns 0 for success
size_t RadioEstimate::stoState() {

    auto sto_adjustment = getStoAdjustmentForEstimated(sto_estimated);

    cout << "STO_STATE ESTIMATE " << sto_adjustment << "\n";
    if( sto_adjustment > 128 ) {
        cout << "STO_STATE ESTIMATE was illegaly large " << sto_adjustment << "\n";
        return 1;
    }

    bool selfAdjustSto = true;


    if(sto_estimated<0) {
        if( ((sto_adjustment)>=(unsigned)margin) ) {
            cout << "STO_STATE A\n";
            updatePartnerSto(sto_adjustment - margin);
        } else {
            unsigned value = (margin - sto_adjustment);
            cout << "STO_STATE B " << value << "\n";
            if( selfAdjustSto ) {
                // soapy->cs31CoarseSync(value, 4);
                dsp->setPartnerSfoAdvance(peer_id, value, 4);
            } else {
                dsp->setPartnerSfoAdvance(peer_id, value, 3);
            }
            cout << "r" << array_index << " STO adjustment with small moving backward     " << value <<endl;
        }
    } else if(sto_estimated>0) {

        unsigned value = (128-sto_adjustment) + (128 - margin) ;
        cout << "STO_STATE C " << value << "\n";
        if( !GET_STO_DISABLE_POSITIVE() ) {
            if( selfAdjustSto ) {
                // soapy->cs31CoarseSync(value, 3);
                dsp->setPartnerSfoAdvance(peer_id, value, 3);
            } else {
                dsp->setPartnerSfoAdvance(peer_id, value, 4);
            }
            cout << "r" << array_index << " STO adjustment with large moving forward     " << value <<endl;
        } else {
            cout << "STO_STATE C  DISABLED!!\n";
        }
    } else {
        cout << "STO_STATE D\n";
    }


    return 0;

}

// returns 0 for success
size_t RadioEstimate::cfoState() {
    const auto cfo_thresh = GET_CFO_STATE_THRESHOLD();//0.01;
    cout << "cfoState(1)" << endl;
     if((abs(cfo_estimated)>=cfo_thresh))
     {
        cout << "cfoState(2)" << endl;
        updatePartnerCfo();
        usleep(500);
        updatePartnerSfo();
        // next_state = CFO_STATE;
        return 1;
     }
     else if((abs(cfo_estimated)<cfo_thresh))
     {
        cout << "cfoState(3) 0000000000000000000000000000000000000000000" << endl;
        // next_state = ADJ_STATE;

        // if(residue_phase_flag==false)
        // {
        // cout << "cfoState(4)" << endl;
        //      residue_phase_flag = true;
        //      cout << "r" << array_index << "R" << array_index << " start residue phase compensation   !!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
        // }
        return 0;
     }
     else
     {
        cout << "cfoState(5)" << endl;
        // next_state = CFO_STATE;
        return 2;
     }
 }

// look at internal state, and calculate an estimate
// and send that estimate

void RadioEstimate::updatePartnerSfo()

{

    
    double max_step = GET_SFO_MAX_STEP();
    if( max_step < 0 ) {
        cout << "dsp.sfo.max_step has illegal value of " << max_step << "\n";
        max_step = 0.1; // hardcoded default in case of incorrect configuration
    }

    // limit maximum sfo value we apply aka "very simple sfo filtering"
    // if((abs(sfo_estimated_sent)>0.0)) {
    //     if(sfo_estimated > max_step) {
    //         cout << "r" << array_index << " capping sfo of " << sfo_estimated << " to " << (max_step) << "\n";
    //         sfo_estimated = max_step;
    //     } else if(sfo_estimated < (-max_step)) {
    //         cout << "r" << array_index << " capping sfo of " << sfo_estimated << " to " << (-max_step) << "\n";
    //         sfo_estimated = (-max_step);
    //     } else {
    //         // value is ok
    //     }
    // }
    ///////////////////////////////////////////////////////////////////

    double estimate = sfo_estimated_sent+sfo_estimated;


    bool is_zero = abs(estimate) < 1E-9;


    if( is_zero ) {
        cout << array_index << "//// updateSfoEstimate() detected ZERO " << estimate << " ////"<< endl;
        uint32_t direction  = 0;//(estimate>0)?1:2;
        dsp->setPartnerSfoAdvance(peer_id, 0, direction);
    } else {

        // if estimate is 0, this will result in double "infinity"
        // which is then cast to uint32_t which casts as 0
        uint32_t amount = 25600.0 / abs(estimate);

         dsp->tickle(&sfo_estimated_sent);

         // sfo_estimated_sent_array[sfo_estimated_sent_array_index] = sfo_estimated_sent;

         // sfo_estimated_sent_array_index++;

        cout << array_index << "//// updateSfoEstimate() " << estimate << ", " << amount << " ////"<< endl;

        bool adjust_sfo_on_rx = false;

        if(adjust_sfo_on_rx) {
            uint32_t direction  = (estimate>0)?2:1;
            soapy->cs31CoarseSync(amount, direction);
        } else {
            uint32_t direction = 0;

            if( negate_sfo_updates) {
                direction = (estimate>0)?2:1;
            } else {
                direction = (estimate>0)?1:2;
            }

            dsp->setPartnerSfoAdvance(peer_id, amount, direction);
        }
    }

    sfo_estimated_sent = estimate;
}

// void RadioEstimate::scheduleOff() {
//     dsp->setPartnerSchedule(this, dsp->schedule_off);
// }

// void RadioEstimate::scheduleOn() {
//     dsp->setPartnerSchedule(this, dsp->schedule_on);
// }


// look at internal state, and calculate an estimate
// and send that estimate
void RadioEstimate::updatePartnerCfo()
{
    //// very simple cfo filtering
    // if(abs(cfo_estimated)<1.0) {
    //     cfo_estimated = (cfo_estimated*0.95) + cfo_estimated_sent;
    // } else {
    //     cfo_estimated = cfo_estimated+ cfo_estimated_sent;
    // }
    //////////////////////////////////////////////////////////////////////

    const double cfo_estimated_temp = cfo_estimated + cfo_estimated_sent;

            // send to partner
    dsp->setPartnerCfo(peer_id, cfo_estimated_temp*-1, true);
            // update tracking variable
    cfo_estimated_sent = cfo_estimated_temp;
    dsp->tickle(&cfo_estimated_sent);

    cout << array_index << " //// Applying CFO: " << cfo_estimated_sent*-1 << " //// (1)" << endl;
}

/// thread safe and can be called from js
/// sets member all_eq_mask
/// enforces that all_eq_mask is length 1024
/// see maskAllEq()
void RadioEstimate::setAllEqMask(const std::vector<uint32_t>& vec) {
    std::unique_lock<std::mutex> lock(_all_eq_mutex);

    all_eq_mask = vec;

    if( vec.size() != 1024)  {
        cout << "Warning, setAllEqMask called with an argument that was length " << vec.size() << "instead of 1024!!\n";
        all_eq_mask.resize(1024);
    }
}

/// pass a vector of length 1024
/// member variable all_eq_mask will be element wise checked
/// each element that is nonzero will cause a mask in the same position of the input
/// the new masked vector is returned
std::vector<uint32_t> RadioEstimate::maskAllEq(const std::vector<uint32_t>& vec) const {
    std::vector<uint32_t> out;

    if( vec.size() != all_eq_mask.size() ) {
        // do nothing in this case
        cout << "maskAllEq() was not able to work " << vec.size() << ", " << all_eq_mask.size() << "\n";
        return vec;
    }

    std::unique_lock<std::mutex> lock(_all_eq_mutex);

    out.resize(vec.size(),0);
    
    for(unsigned i = 0; i < vec.size(); i++) {
        if( all_eq_mask[i] ) {
            // this location should be zero, so do nothing
        } else {
            // this location should be the original value
            out[i] = vec[i];
        }
    }
    return out;
}

std::vector<uint32_t> RadioEstimate::maskChannelVector(
    const size_t option,
    const std::vector<uint32_t>& vec
) const {

    std::vector<uint32_t> output;
    output.resize(vec.size());


    // cout<<"@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@   "<<peer_id<<"   "<<array_index<<"  "<<option<<"   "<<vec.size()<<endl;

    if( option == 0) {

        if( should_unmask_all_eq ) {
            for(unsigned i = 0; i < vec.size(); i++) {
                output[i] = vec[i];
            }
        } else {
            for(unsigned i = 0; i < vec.size(); i++) {
                if( i % 4 == 0) {
                    output[i] = 0;
                } else {
                    output[i] = vec[i];
                }
            }
        }
    }


    if( option == 1 ) {
        for(unsigned i = 0; i < vec.size(); i++) {
            if( i % 4 == 2) {
                output[i] = 0;
            } else {
                output[i] = vec[i];
            }
        }
    }


    ///
    /// another control
    ///
    if( should_mask_all_data_tone ) {
        for(unsigned i = 0; i < vec.size(); i++) {
            // if( i == 17 || i == 19 || i == 21 || i == 23 ) {
            if( 
                // i >= 17 && i < (17+64) && (i % 2 == 1)
                (i % 2 == 1)
                ) {
                output[i] = 0;
            } else {
                // output[i] = vec[i];
            }
        }
    }


    auto tx_tdma_sc = getScForTransmitTDMA();
    ///
    ///
    /// even if loop above wrote zero, we copy it again
    if( should_mask_data_tone_tx_eq ) {
        output[tx_tdma_sc] = 0;
    } else {
        output[tx_tdma_sc] = vec[tx_tdma_sc];
    }

    return output;
}

std::vector<double> RadioEstimate::getChannelAngle() const {
    std::vector<double> copy;
    {
        std::unique_lock<std::mutex> lock(_channel_angle_sent_mutex);
        copy = channel_angle;
    }
    return copy;
}

std::vector<double> RadioEstimate::getChannelAngleSent() const {
    std::vector<double> copy;
    {
        std::unique_lock<std::mutex> lock(_channel_angle_sent_mutex);
        copy = channel_angle_sent;
    }
    return copy;
}

std::vector<double> RadioEstimate::getChannelAngleSentTemp() const {
    std::vector<double> copy;
    {
        std::unique_lock<std::mutex> lock(_channel_angle_sent_mutex);
        copy = channel_angle_sent_temp;
    }
    return copy;
}

std::vector<uint32_t> RadioEstimate::getEqualizerCoeffSent() const {
    std::vector<uint32_t> copy;
    {
        std::unique_lock<std::mutex> lock(_channel_angle_sent_mutex);
        copy = equalizer_coeff_sent;
    }
    return copy;
}





void RadioEstimate::setupEqToSto() {
    std::vector<double> rotated;
    rotated.resize(1024);

    compareEq.resize(256);
    
    // initial random rotation
    const auto random_rotation_array = siglabs::data::vectors::randomRotation();

    // convert initial random rotation
    for(unsigned sc = 0; sc<1024; sc++)
    {
        double n_imag;
        double n_real;
        ishort_to_double(random_rotation_array[sc], n_imag, n_real);
        double atan_angle = imag_real_angle(n_imag, n_real);

        rotated[sc] = atan_angle;
    }

    // create 256 STO rotated versions
    for(unsigned i = 0; i < compareEq.size(); i++) {
        compareEq[i].resize(1024);
        rotateVectorBySto(rotated, compareEq[i], i, 1);
    }

    // if(array_index == 0) {
    //     cout << "eq = [";
    //     for(auto eq : compareEq) {
    //         cout << "\n[";
    //         for(auto sc : eq ) {
    //             cout << "," << sc;
    //         }
    //         cout << "]";
    //     }
    //     cout << "\n]";
    // }

    // debug view, do not run this:
    // channel_angle_sent = compareEq[1];
    // dsp->tickle(&channel_angle_sent);
}

double RadioEstimate::calculateEqStoDelta(void) const {

    std::vector<double> pick;

    const unsigned limit = GET_STO_ADJUST_USING_EQ_HALF_SC();

    unsigned quad_pilot = 2;
    if( array_index == 1) {
        quad_pilot = 0;
    }


    for(unsigned sc = 0; sc < 1024; sc++) {
        if( ((sc % 4) == quad_pilot) && sc < limit) {
            pick.push_back(channel_angle[sc]);
        }
    }

    unwrap_phase_inplace(pick);

    // https://en.cppreference.com/w/cpp/algorithm/adjacent_difference
    // these two lines are equivalent to np.diff()
    std::adjacent_difference(pick.begin(), pick.end(), pick.begin());
    pick.erase(pick.begin());


    const double average = std::accumulate(pick.begin(), pick.end(), 0.0) / pick.size();

    const double factor = -42.026;


    return average * factor;
}

int RadioEstimate::calculateEqToSto(const bool print) const {
    cout << "calculateEqToSto()\n";

    unsigned limit = GET_STO_ADJUST_USING_EQ_HALF_SC();

    const auto copy = getChannelAngleSent();

    std::vector<double> results;
    results.resize(compareEq.size(), 0.0);

    unsigned best_idx = 0;
    double best_value = 9E9;
    for(unsigned i = 0; i < compareEq.size(); i++) {
        auto &ideal = compareEq[i];
        for(unsigned sc = 0; sc < 1024; sc++) {
            if(
                (sc % 2 == 1)
                && ((sc < limit) || (sc>(1024-limit)))
                ) {
                results[i] += abs(ideal[sc] - copy[sc]);
            }
        }

        // check for best fit
        if( results[i] < best_value ) {
            best_value = results[i];
            best_idx = i;
        }

     }

     if(print) {
         cout << "RESULTS: \n\n";
         for(unsigned i = 0; i < results.size(); i++) {
            cout << "  " << i << ":  " << results[i] << "\n";
         }
         cout << "\n\n";
     }

     cout << "BEST: idx " << best_idx << ", value: " << best_value << "\n";

     return margin - ((signed)best_idx);
}

void RadioEstimate::applyEqToSto2() {
    int sto_error = calculateEqToSto();
    int tol = GET_STO_ADJUST_USING_EQ_THRESHOLD();

    if( abs(sto_error) < tol ) {
        cout << "applyEqToSto() skipping update because error is only " << sto_error << "\n";
        return;
    }

    int mode = 3;
    if( sto_error < 0 ) {
        mode = 4;
    }

    cout << "applyEqToSto() using mode " << mode << " adjust " << abs(sto_error) << "\n";

    dsp->setPartnerSfoAdvance(peer_id, abs(sto_error), mode);

    // apply random rotation
    // updates
    //   channel_angle_sent_temp 
    //   equalizer_coeff_sent
    setRandomRotationEq(false); // false for don't send to partner

    // copy channel_angle_sent_temp into channel_angle_sent
    channel_angle_sent_temp_into_channel_angle_sent();

    // apply default sto rotation
    // uses channel_angle_sent and modifies channel_angle_sent_temp with sto rotation
    dspRunStoEq(1.0);

    // send to partner
    // copyies channel_angle_sent_temp into channel_angle_sent
    // and then sends equalizer_coeff_sent?
    updatePartnerEq(false, false);
}

void RadioEstimate::applyEqToSto() {

    if(!eq_to_sto_allowed) {
        return;
    }

    if(!GET_STO_ADJUST_USING_EQ()) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    now - last_eq_to_sto
    ).count();

    if( elapsed_us < 10E6 ) {
        return;
    }




    int tol = GET_STO_ADJUST_USING_EQ_THRESHOLD();

    if( abs(sto_delta) < tol ) {
        return;
    }

    last_eq_to_sto = now;

    int mode = 3;
    if( sto_delta < 0 ) {
        mode = 4;
    }

    cout << "applyEqToSto() using mode " << mode << " adjust " << abs(sto_delta) << "\n";

    dsp->setPartnerSfoAdvance(peer_id, abs(sto_delta), mode);
    applyEqToSfo(sto_delta);
}

// only call when an eq -> sto estimate was performed
void RadioEstimate::applyEqToSfo(const double delta) {

    if(!GET_SFO_ADJUST_USING_EQ()) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    uint64_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    now - init_timepoint
    ).count();

    eq_sfo_history.emplace_back(elapsed_us, delta);

    cout << "R" << array_index << " applyEqToSfo()\n";
    for(const auto& x : eq_sfo_history) {
        uint64_t a;
        double b;
        std::tie(a,b) = x;
        cout << "    us: " << a << ", " << b << "\n";
    }

    calculateEqToSfo();
}


/// We use the eq to update the STO in applyEqToSto()
/// This function measures those updates.  This function assumes that those updates are only 1 sample
/// which will not always be true. However if this is always enabled, we should never reach >1 sample adjustments by applyEqToSto()
/// dsp.sfo.adjust_using_eq.seconds is measuring distance between STO updates.  This serves as a filter.  Sto 
/// updates that take longer than this tolerance will not be considered. Larger values will make
/// this function more sensative, and apply updates for smaller values of delta sfo.
/// dsp.sfo.adjust_using_eq.trend is the number of subsequent STO updates that must match all conditions
/// before we apply an adjustmnet.  This number is also the number of estimates that are averaged when applying
/// dsp.sfo.adjust_using_eq.factor is multiplied against the final estimate before updating
void RadioEstimate::calculateEqToSfo() {

    cout << "R" << array_index << " calculateEqToSfo()\n";

    // must be this many or more in a row (only this many are considered for the update)
    const unsigned trend = GET_SFO_ADJUST_USING_EQ_TREND();

    if( eq_sfo_history.size() < (trend+1) ) {
        cout << "too small\n";
        return;
    }

    const double tol_seconds = GET_SFO_ADJUST_USING_EQ_SECONDS();    // consider updates spaced less than this


    uint64_t d;
    uint64_t p;
    std::tie(p,std::ignore) = eq_sfo_history[0];
    const uint64_t start = p;

    std::vector<double> match;
    
    uint64_t a;
    double b;
    for(unsigned i = 1; i < eq_sfo_history.size(); i++) {
        std::tie(a,b) = eq_sfo_history[i];
        d = a - p;
        
        const double delta_seconds = d / 1E6;
        const double age_seconds = (a-start) / 1E6;
        
        if( delta_seconds <= tol_seconds ) {
            match.emplace_back(delta_seconds);
            if( match.size() > trend ) {
                // erase if larger so we only have at max trend in the match vector
                match.erase(match.begin());
            }
        } else {
            match.resize(0);
        }
        
        cout << "[" << i << "]    us: " << delta_seconds << ", " << b << "     age: " << age_seconds << "\n";
        p = a;
    }
    
    
    for(auto w: match) {
        cout << w << "\n";
    }

    if( match.size() != trend ) {
        cout << "no update needed\n";
        return; // data is not ready for update
    }
    // the last N samples were under the tolerance
    
    bool set = false;
    bool neg = false;
    bool negthis = false;
    for(unsigned i = eq_sfo_history.size()-trend; i < eq_sfo_history.size(); i++) {
        // cout << "sign check " << i << "\n";
        std::tie(a,b) = eq_sfo_history[i];
        if(!set) {
            neg = b < 0;
            set = true;
        } else {
            negthis = b < 0;
            if( negthis != neg ) {
                cout << "last " << trend << " values were not all the same sign\n";
                return;
            }
        }
        
    }
    
    const double factor = GET_SFO_ADJUST_USING_EQ_FACTOR();
    
    
    double avg = 0;
    for(auto w: match) {
        avg += w;
    }
    avg /= (double)match.size();
    
    cout << "average: " << avg << "\n";
    
    double hz = (1/avg) * factor;
    
    if( neg ) {
        hz = -hz;
    }
    
    cout << "R" << array_index << " calculateEqToSfo() applying hz: " << hz << "\n";

    // erase once we apply
    eq_sfo_history.resize(0);

    sfo_estimated = hz;
    // updatePartnerSfo();
}

// fixme (builds) a mask and sends to our peer id
void RadioEstimate::initialMask() {
    std::vector<uint32_t> def;
    def.resize(1024);
    for(int index = 0; index<1024; index++)
    {
        def[index] = 0x00007fff;
    }


    int option;
    if( array_index == 0 ) {
        option = 0;
    } else {
        option = 1;
    }

    auto eq_coeffs_masked = maskChannelVector(option, def);

    dsp->setPartnerEq(peer_id, eq_coeffs_masked);
    saveIdealEqHash(eq_coeffs_masked);
}

// takes the lock
void RadioEstimate::channel_angle_sent_temp_into_channel_angle_sent(void) {
    std::unique_lock<std::mutex> lock(_channel_angle_sent_mutex);

    // channel_angle_sent_temp is written to a lot in different places by
    // this class.  When we send over zmq, we copy it into channel_angle_sent
    channel_angle_sent = channel_angle_sent_temp;
}

// look at internal state, and calculate an estimate
// and send that estimate
// defaults are false, true
void RadioEstimate::updatePartnerEq(const bool send_existing, const bool update_counter)
{
    cout << "r" << array_index
         << " //// Channel Estimate -> zmq ////  UPDATES: " << update_counter << " will ";

    if( !update_counter ) {
        cout << "NOT";
    }
    cout << " update counter\n";
            // cout << "dspRunChannelEst() is sending over zmq" << endl;

    if(!send_existing) {
        channel_angle_sent_temp_into_channel_angle_sent();
    } else {
        cout << "non default call to updatePartnerEq()" << endl;
    }

    dsp->tickle(&channel_angle_sent);


    int option;
    if( array_index == 0 ) {
        option = 0;
    } else {
        option = 1;
    }

    if(update_counter) {
        times_eq_sent++;
        dsp->tickle(&times_eq_sent);
    }


    // if(radio_state != STO_EQ_0)
    // {
    //     for(int index = 0; index < 1024; index++)
    //          {
    //     equalizer_coeff_sent[index] = 0x7fff;
    //         }
    // }
    

    // we send from a different array
    const auto eq_coeffs_masked = maskChannelVector(option, equalizer_coeff_sent);

    const auto eq_coeffs_masked_again = maskAllEq(eq_coeffs_masked);
        // convert into feedback bus vector
    dsp->setPartnerEq(peer_id, eq_coeffs_masked_again);

    cout<<"Check EQ data::::::::::::::::::::::::::::::::"<<endl;
    cout<<HEX32_STRING(eq_coeffs_masked_again[1])<<"   "<<HEX32_STRING(eq_coeffs_masked_again[2])<<"   "<<HEX32_STRING(eq_coeffs_masked_again[3])<<"   "<<HEX32_STRING(eq_coeffs_masked_again[4])<<"   "<<endl;
    cout<<HEX32_STRING(eq_coeffs_masked_again[1023])<<"   "<<HEX32_STRING(eq_coeffs_masked_again[1022])<<"   "<<HEX32_STRING(eq_coeffs_masked_again[1021])<<"   "<<HEX32_STRING(eq_coeffs_masked_again[1020])<<"   "<<endl;
    cout<<"Check EQ data end::::::::::::::::::::::::::::"<<endl;
    saveIdealEqHash(eq_coeffs_masked_again);


    cheq_done_flag = false;
    cheq_onoff_flag = false;
}

// call with a value and this will guard the estimate min/max and then apply
void RadioEstimate::updatePartnerSto(const uint32_t sto_adjustment)
{
    cout << array_index << "//// Updating STO " << sto_adjustment << " ////"<<endl;
            
    bool adjust_sto_on_rx = true;
            // run with a non zero value
    if((sto_estimated<0.0) && (sto_adjustment>0))
    {
        if(adjust_sto_on_rx) {
                    // soapy->cs31CoarseSync(sto_adjustment, 3);
                    dsp->setPartnerSfoAdvance(peer_id, sto_adjustment, 3, true);
                    //soapy->cs00SetSfoAdvance(sto_adjustment, 3);
        } else {
                    // transmit side
                    dsp->setPartnerSfoAdvance(peer_id, sto_adjustment, 4);
        }
    }
    else if((sto_estimated>0.0) && (sto_adjustment>0))
    {
        if(adjust_sto_on_rx) {
                    // soapy->cs31CoarseSync(sto_adjustment, 4);
                    dsp->setPartnerSfoAdvance(peer_id, sto_adjustment, 4, true);
                    //soapy->cs00SetSfoAdvance(sto_adjustment, 4);
        } else {
                    dsp->setPartnerSfoAdvance(peer_id, sto_adjustment, 3);
        }
    }
}

void RadioEstimate::dspRunCfo_v1(const std::vector<uint32_t>& cfo_buf_chunk) {

    cout << "dspRunCfo_v1() " << cfo_buf_chunk.size() << endl;
 
    uint32_t gap = 128;

    ///////// calculate cfo
    double cfo_complex_real[_cfo_symbol_num];
    double cfo_complex_imag[_cfo_symbol_num];

    ///// get cfo_complex
    uint32_t index2 = 0;
    for(const auto n : cfo_buf_chunk)
    {
        double n_imag;
        double n_real;
        ishort_to_double(n, n_imag, n_real);
        cfo_complex_real[index2] = n_real;
        cfo_complex_imag[index2] = n_imag;
        index2 = index2+1;
    }


    

    double cfo_complex_cmul_real[_cfo_symbol_num-gap];
    double cfo_complex_cmul_imag[_cfo_symbol_num-gap];

    for(uint32_t index=gap; index<(_cfo_symbol_num); index++)
    {
        cfo_complex_cmul_real[index-gap] = cfo_complex_real[index-gap]*cfo_complex_real[index]+cfo_complex_imag[index-gap]*cfo_complex_imag[index];
        cfo_complex_cmul_imag[index-gap] = cfo_complex_real[index-gap]*cfo_complex_imag[index]-cfo_complex_imag[index-gap]*cfo_complex_real[index];
    }

    double cfo_sum_real = 0.0;
    double cfo_sum_imag = 0.0;

    for(uint32_t index =0; index<(_cfo_symbol_num)-gap; index++)
    {
        cfo_sum_real = cfo_sum_real + cfo_complex_cmul_real[index];
        cfo_sum_imag = cfo_sum_imag + cfo_complex_cmul_imag[index];
    }
    

    double cfo_angle = imag_real_angle(cfo_sum_imag, cfo_sum_real);
    
   

    this->cfo_estimated = cfo_angle*31.25*1024*1024/1280/(2*PI)/(gap*1.0);
    // cfo_done_flag = true;

    if( should_print_estimates ) {
        cout << array_index << " cfo_estimated (new)                                                 " << cfo_estimated << endl;
    }
    if(abs(this->cfo_estimated)<10.0)
    {
        gap = 1024;

        for(uint32_t index=gap; index<_cfo_symbol_num; index++)
        {
            cfo_complex_cmul_real[index-gap] = cfo_complex_real[index-gap]*cfo_complex_real[index]+cfo_complex_imag[index-gap]*cfo_complex_imag[index];
            cfo_complex_cmul_imag[index-gap] = cfo_complex_real[index-gap]*cfo_complex_imag[index]-cfo_complex_imag[index-gap]*cfo_complex_real[index];
        }
        
        double cfo_sum_real_re = 0.0;
        double cfo_sum_imag_re = 0.0;

        for(uint32_t index =0; index<(_cfo_symbol_num-gap); index++)
        {
            cfo_sum_real_re = cfo_sum_real_re + cfo_complex_cmul_real[index];
            cfo_sum_imag_re = cfo_sum_imag_re + cfo_complex_cmul_imag[index];
        }
        

        this->cfo_estimated = imag_real_angle(cfo_sum_imag_re, cfo_sum_real_re)*31.25*1024*1024/1280/(2*PI)/(gap*1.0);
        // cfo_done_flag = true;
        if( should_print_estimates ) {
            cout << array_index << " cfo_estimated (reestimated new)                                                   " << cfo_estimated << endl;
        }
    }

     if(abs(this->cfo_estimated)<1)
    {
        gap = 10240;   ////128*80      

        for(uint32_t index=gap; index<(_cfo_symbol_num); index++)
        {
            cfo_complex_cmul_real[index-gap] = cfo_complex_real[index-gap]*cfo_complex_real[index]+cfo_complex_imag[index-gap]*cfo_complex_imag[index];
            cfo_complex_cmul_imag[index-gap] = cfo_complex_real[index-gap]*cfo_complex_imag[index]-cfo_complex_imag[index-gap]*cfo_complex_real[index];
        }
        
        double cfo_sum_real_re = 0.0;
        double cfo_sum_imag_re = 0.0;

        for(uint32_t index =0; index<(_cfo_symbol_num-gap); index++)
        {
            cfo_sum_real_re = cfo_sum_real_re + cfo_complex_cmul_real[index];
            cfo_sum_imag_re = cfo_sum_imag_re + cfo_complex_cmul_imag[index];
        }
        

        this->cfo_estimated = imag_real_angle(cfo_sum_imag_re, cfo_sum_real_re)*31.25*1024*1024/1280/(2*PI)/(gap*1.0);
        // cfo_done_flag = true;
        if( should_print_estimates ) {
            cout << array_index << " cfo_estimated (reestimated reestimated new)                                                   " << cfo_estimated << endl;
        }
    }

    times_cfo_estimated++;
    dsp->tickle(&times_cfo_estimated);
    dsp->tickle(&cfo_estimated);


}

// void RadioEstimate::handleDataToEq() {
//     const auto load_size = rolling_data_queue.size();

//     if( load_size < 10 ) {
//         return;
//     }

//     // cout << rolling_data_queue.size() << "\n";

//     for(unsigned i = 0; i < 10; i++) {
//         uint32_t a,b;
//         std::tie(a,b) = rolling_data_queue.dequeue();
//         // cout << "   " << HEX32_STRING(b) << "\n";
//     }

//     // uint32_t a,b;
//     // std::tie(a,b) = rolling_data_queue.dequeue();
//     // cout << "a: " << a << "\n";
//     // cout << "b: " << b << "\n";

//     while(rolling_data_queue.size()) {
//         rolling_data_queue.dequeue();
//     }
// }


void RadioEstimate::monitorSendResidue() {

    auto now = std::chrono::steady_clock::now();


    // cout << "delta" << endl;

    // soapy->status.set(path)
    // watch_status_timepoint = std::chrono::steady_clock::now();

    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    now - last_residue_timepoint
    ).count();

    if( !GET_SKIP_CHECK_FB_DATARATE() ) {

        if( elapsed_us > 100000 ) { // 55000
            const std::string path = std::string("rx.re[") + 
                                     std::string( std::to_string(this->array_index) ) + 
                                     std::string("].slow_residue");
            cout << path << " elapsed_us " << elapsed_us << endl;
        }
    }

    last_residue_timepoint = now;
}


void RadioEstimate::applyResidueToCfo() {
    if(enable_residue_to_cfo && (!pause_residue)) {

        auto now = std::chrono::steady_clock::now();
        size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
            now - last_cfo_residue
            ).count();

        if( elapsed_us > residue_to_cfo_delay ) {
            last_cfo_residue = now;

            double delta = residue_phase_trend * residue_to_cfo_factor;


            cout << "RESIDUE_PHASE_TREND: " << residue_phase_trend 
                 << ", pre adjust cfo: " << cfo_estimated_sent
                 << ", delta cfo: " << delta  << "\n";

            cfo_estimated = delta;

            updatePartnerCfo();

        }
    }
}

///
/// Called by dspRunResiduePhase() This prints an error when
/// that function calculates a very small residue value
/// this function is rate limited
void RadioEstimate::printSmallMagResidue(double cfo_mag2, double n_imag, double n_real, uint32_t n) {
    
    constexpr double rate_limit = 3E6;

    auto now = std::chrono::steady_clock::now();
    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    now - last_print_small_mag
    ).count();

    // supressed print
    if( elapsed_us > rate_limit ) {
        if( !sfo_sto_use_duplex ) {
            cout << "r" << array_index
                 << " first sample given to dspRunResiduePhase() had a small mag^2 of "
                 << cfo_mag2 << " " << n_imag << ", " << n_real << " "
                 << HEX32_STRING(n) << "\n";
         }

        // update timer
        last_print_small_mag = now;
    }

}

// only call if the buffer is big enough
void RadioEstimate::dspRunResiduePhase(const std::vector<uint32_t>& cfo_buf_chunk) {

    //// for test purpose
    // std::vector<uint32_t> cfo_buf_chunk;

     ////////////////////////////////////////////////////////////

    const auto wanted = (_cfo_symbol_num)/GET_CFO_SYMBOL_PULL_RATIO();//500; //_cfo_symbol_num/GET_CFO_SYMBOL_PULL_RATIO();

    const auto in_size = cfo_buf_chunk.size();

    const auto slice_point = in_size - wanted;
    
    const std::vector<uint32_t> tail(cfo_buf_chunk.begin() + slice_point, cfo_buf_chunk.end());

    ///////// calculate cfo
    std::vector<double> cfo_angle;
    cfo_angle.resize(wanted, 0.0);
    double cfo_mag2;

    constexpr double mag_tol = 2000;

    double save_imag, save_real;
    bool save_set = false;

    bool mag_error = false;
    (void)mag_error;
    ///// get cfo_angle
    uint32_t index2 = 0;
    for(const auto n : tail)
    {
        double n_imag;
        double n_real;

        ishort_to_double(n, n_imag, n_real);
        cfo_mag2 = (n_imag*n_imag) + (n_real*n_real);

        // cout << "mag: " << cfo_mag2 << " " << n_imag << ", " << n_real << "\n";

        if( cfo_mag2 <= mag_tol ) {
            // data was too small

            if( !save_set ) {
                if( array_index == 0 ) {
                    printSmallMagResidue(cfo_mag2, n_imag, n_real, n);
                    mag_error = true;
                }
                // return;
            } else {
                // "hold data" by writing the previous good sample over the current bad samples
                n_imag = save_imag;
                n_real = save_real;
            }
        } else {

            // always save the data
            save_imag = n_imag;
            save_real = n_real;
            save_set = true;

        }

        // proceed as normal
        double atan_angle = imag_real_angle(n_imag, n_real);
        cfo_angle[index2] = atan_angle;

        index2++;
    }

    // if( mag_error ) {
    //     for(const auto n : tail) {
    //         cout << HEX32_STRING(n) << "\n";
    //     }
    // }
    
    //// phase unwrap
    unwrap_phase_inplace(cfo_angle);
    

    double cfo_x_mean = (wanted-1)/2.0;
    double cfo_y_mean = 0.0;
    
    uint32_t x = 0;

    double cfo_xx_sum = 0.0;

    for(unsigned index = 0; index<wanted; index++)
    {
        cfo_y_mean = cfo_y_mean + cfo_angle[index];

        cfo_xx_sum = cfo_xx_sum + (x*1.0-cfo_x_mean)*(x*1.0-cfo_x_mean);

        x=x+1;
    }

    cfo_y_mean = cfo_y_mean/wanted;

    double cfo_yx_sum = 0.0;
    
    x = 0;

    for(unsigned index = 0; index<wanted; index++)  
    {
        cfo_yx_sum = cfo_yx_sum + (cfo_angle[index]-cfo_y_mean)*(x*1.0-cfo_x_mean);

        x=x+1;
    }     

    double slope_temp = cfo_yx_sum/cfo_xx_sum;
    double intercept_temp = cfo_y_mean - slope_temp*cfo_x_mean;



    this->residue_phase_estimated = slope_temp*(wanted-100)+intercept_temp;

    
     


    double did_send = 0;

    if(residue_phase_flag)
    {

        /// here is where you would put comparisons about previous vs current estimate
        /// this could help detect oscilations
             
        this->residue_phase_estimated_pre = residue_phase_estimated; // currently unused

        // if(abs(residue_phase_estimated) > 1.5708 ) {
        //        residue_phase_estimated /= 3.0;
        // }

        drop_residue_counter++;

        monitorSendResidue();
        
        if( drop_residue_counter >= drop_residue_target ) {
            drop_residue_counter = 0;
            // keep
        } else {
            // drop
            return;
        }



        if( !pause_residue ) {
            if(cfo_estimated_sent<0) {
                dsp->setPartnerPhase(peer_id, residue_phase_estimated*(-1.0));
                did_send = residue_phase_estimated*(-1.0);
            } else {
                dsp->setPartnerPhase(peer_id, residue_phase_estimated);
                did_send = residue_phase_estimated;
            }
        }

        this->residue_phase_history.push_back(residue_phase_estimated);

        if( residue_phase_history.size() == 10 ) {
            residue_phase_trend = 0;
            for(auto t : residue_phase_history) {
                residue_phase_trend += t;
            }

            residue_phase_trend /= residue_phase_history.size();

            applyResidueToCfo();
            dsp->tickle(&residue_phase_trend);

            residue_phase_history.erase(residue_phase_history.begin(), residue_phase_history.end());
        }

            
        this->times_residue_phase_sent++;
    }


    // FIXME this is now incorrect because of addition of drop_residue_counter
    if( array_index == 0 && GET_RESIDUE_DUMP_ENABLED() ) {
        static bool fileopen = false;
        static ofstream ofile;
        if(!fileopen) {
            fileopen = true;
            std::string fname = GET_RESIDUE_DUMP_FILENAME();
            ofile.open(fname);
            cout << "OPENED " << fname << "\n";
        }

        for(auto n : tail)
        {
            ofile << HEX32_STRING(n) << "\n";
        }
            // myfile << "found_i " << found_i << " was at frame " << demod_buf_accumulate[0][found_i].second << endl;

        if( residue_phase_flag ) {
            ofile << "GG " << did_send << "\n";
        }

        // cout << "d";
    }
}


// for flip positive and negative frequency
static void flipEqSpectrum(std::vector<uint32_t>& v ) {
    for(int index = 1; index < 512; index++)
    {
        const uint32_t temp = v[index];

        v[index] = v[1024-index];
        v[1024-index] = temp;
    }
}



void RadioEstimate::setupEqOne(void) {
    equalizer_coeff_one.resize(1024);

    const auto random_rotation_array = siglabs::data::vectors::randomRotation();

    auto random_rotation_array_flipped = random_rotation_array;

    flipEqSpectrum(random_rotation_array_flipped);

    // build equalizer_coeff_one so that we either have random rotation
    // or zero
    // use this equation after we've flipped it
    for(unsigned i = 0; i<1024; i++) {
        if( (i >= (1024-(32*4))) && (i%4==2) ) {
            equalizer_coeff_one[i] = random_rotation_array_flipped[i];
        } else {
            equalizer_coeff_one[i] = 0;
        }
    }

}

void RadioEstimate::sendEqOne(void) {
    dsp->setPartnerEqOne(peer_id, equalizer_coeff_one);
}

void RadioEstimate::printEqOne(void) {
    cout << "printEqOne()\n\n";
    for(const auto w : equalizer_coeff_one) {
        cout << HEX32_STRING(w) << "\n";
    }
    cout << "\n\n";
}



void RadioEstimate::setRandomRotationEq(const bool send_to_partner)
{

    const auto random_rotation_array = siglabs::data::vectors::randomRotation();

    for(int index = 0; index<1024; index++)
    {
        double n_imag;
        double n_real;
        ishort_to_double(random_rotation_array[index], n_imag, n_real);
        double atan_angle = imag_real_angle(n_imag, n_real);

        channel_angle_sent_temp[index] = atan_angle;

        equalizer_coeff_sent[index] = random_rotation_array[index];
    }


    flipEqSpectrum(equalizer_coeff_sent);

    if(send_to_partner) {
        updatePartnerEq(false, false);
    }
}

// input
// output
// margin
// margin_sign
void RadioEstimate::rotateVectorBySto(
    const std::vector<double>& input,
    std::vector<double>& output,
    const double m,
    const double sign) {

    for(int index = 0; index<513; index++)
    {
        output[index] = 2*M_PI*index*sign*m/1024.0+input[index];
    }
    for(int index = 513; index<1024; index++)
    {
        output[index] = 2*M_PI*(index-1024)*sign*m/1024.0+input[index];
    }

}

/**
This rotates the eq, which will then be sent to the tx side.  This rotation is needed because 
we put the fft sampling instance not at the edge of the cp, but in the middle of the cp.  
This non ideal sampling instance causes all subcarreirs to rotate which
this function counteracts.  `margin` comes from `GET_STO_MARGIN()`.

Input Member Variables:
-------------------
- margin
- channel_angle_sent_temp

Output Member Variables:
-----------------------
- equalizer_coeff_sent

*/
void RadioEstimate::dspRunStoEq(const double margin_sign)
{
    double mag_coeff = 32767.0 / GET_EQ_MAGNITUDE_DIVISOR();

    cout << "dspRunStoEq()" << endl;

    std::unique_lock<std::mutex> lock(_channel_angle_sent_mutex);


     ////// may use the current sto_adjustment for margin???????????????????????
    // input
    // output
    // margin
    // margin_sign
    rotateVectorBySto(channel_angle_sent, channel_angle_sent_temp, margin, margin_sign);

    for(int index = 0; index<1024; index++) {
        int32_t real_part = (int32_t)(cos(channel_angle_sent_temp[index])*mag_coeff);
        int32_t imag_part = (int32_t)(sin(channel_angle_sent_temp[index])*mag_coeff);
        equalizer_coeff_sent[index] = (((uint32_t)(imag_part&0xffff))<<16) + ((uint32_t)(real_part&0xffff));
    }

    cout<<"Using Channel EQ to correct STO first!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;

    // for flip positive and negative frequency
    // eventually, we do not need this part.
    for(int index = 1; index < 512; index++)
    {
        uint32_t temp = equalizer_coeff_sent[index];

        equalizer_coeff_sent[index] = equalizer_coeff_sent[1024-index];
        equalizer_coeff_sent[1024-index] = temp;
    }
}

// double RadioEstimate::fractionalStoToEq(const int index) const {
//     if(!fractional_eq_allowed) {
//         return 0.0;
//     }

//     int index2 = index;

//     if(index < 513){
//     } else {
//         index2 = index-1024;
//     }

//     constexpr double sign = -1.0;

//     // currently frac_sto is used here as a "hand tune" from js
//     // eventually we won't need this and after this frac_sfo should be removed from
//     // class
//     double m = sto_delta*frac_sto;

//     return 2*M_PI*index2*sign*m/1024.0;
// }


void RadioEstimate::saveChannel(void) {
    if(!save_next_all_sc) {
        return;
    }

    all_sc_saved = all_sc;

    save_next_all_sc = false;
}

std::vector<uint32_t> RadioEstimate::defaultEqualizerCoeffSent(const double mag_coeff) {
    std::vector<uint32_t> ret;

    const int32_t initial_real_part = (int32_t)(cos(0.0)*mag_coeff);
    const int32_t initial_imag_part = (int32_t)(sin(0.0)*mag_coeff);
    const uint32_t initial_value = (((uint32_t)(initial_imag_part&0xffff))<<16) + ((uint32_t)(initial_real_part&0xffff));
    
    // fill with initial value
    ret.resize(1024, initial_value);

    return ret;
}


void RadioEstimate::dspRunChannelEst(void) {

    // bool keep_running = dspRunChannelEstBuffers();
    // bool keep_running = dspRunChannelEstBuffersComplexFilter();
    bool keep_running = dspRunChannelEstBuffersComplexFilterIIR();

    if(!keep_running) {
        return;
    }

    if( adjust_fractional_sto ) {
        if( fractional_sto_counter >= fractional_sto_delay ) {
            fractional_sto_counter = 0;
            compensateFractionalSto();
        } else {
            fractional_sto_counter++;
        }
    }


    if(pause_eq) {
        return;
    }


    if(use_all_pilot_eq_method) {
        dspRunChannelEstEqAllScAllPilot();
    } else {
         if(use_sto_eq_method) {
             // cout << "Eq using dspRunChannelEstWithSTO();\n";
             dspRunChannelEstWithSTO();
         } else {
             // cout << "Eq using dspRunChannelEstEqAllSc();\n";
             dspRunChannelEstEqAllSc();
         }
    }
}

void RadioEstimate::initChannelFilter(void) {

    for(int i=0; i<1024; i++) {
        for(int j=0; j<25; j++) {
            channel_angle_filtering_buffer[i][j] = 0.0;
        }
    }

    eq_filter_index = 0;

    // initilize Complex Filter
    eq_filter.resize(25);
    for( auto& vec : eq_filter ) {
        vec.resize(1024, {0,0});
    }

    eq_iir.resize(1024, 0);


    eq_iir_gain = GET_EQ_IIR_GAIN();
}

void RadioEstimate::updateChannelFilterIndex(void) {
    if(should_run_background && _eq_use_filter)
    {
        eq_filter_index++;
        if(eq_filter_index >= 25)
        {
            eq_filter_index = 0;

            // cout << "//// Continuous updatePartnerEq() ////" << endl;
            // updatePartnerEq();
        }
    }
}

///
/// Average or other simple things from eq data.
/// returns true if other code should run
bool RadioEstimate::dspRunChannelEstBuffers(void) {

    if(all_sc.size() != ALL_SC_CHUNK) {
        cout << "dspRunChannelEst(1) wrong !!! " << all_sc.size() << " != " << ALL_SC_CHUNK << endl;
        return false;
    }
    // std::vector<uint32_t> all_sc;
    // all_sc = all_sc_buf.get(ALL_SC_CHUNK);

    uint32_t index = 0;
    double channel_angle_current;
    for(const auto n : all_sc)
    {
        double n_imag;
        double n_real;
        ishort_to_double(n, n_imag, n_real);
        const double atan_angle = imag_real_angle(n_imag, n_real);

        //mag_coeff = 32767.0*sqrt(n_imag*n_imag + n_real*n_real)/(n_imag*n_imag + n_real*n_real+100);  //try this one later. may affect shift in cs20 at tx.
        const bool ok = 
            (n_real<=32767.0) &&
            (n_real>=-32768.0) &&
            (n_imag<=32767.0) &&
            (n_imag>=-32768.0);

        if(ok) {
            channel_angle_current = atan_angle*-1.0;
            if(_eq_use_filter) {
                /// mean filtering
                if(should_run_background) {
                    channel_angle_filtering_buffer[index][eq_filter_index] = channel_angle_current;

                    channel_angle_current = 0.0;
                    for(int j = 0; j < 25; j++) {
                        channel_angle_current += channel_angle_filtering_buffer[index][j];
                    }

                    double temp_max = channel_angle_filtering_buffer[index][0];
                    double temp_min = channel_angle_filtering_buffer[index][0];

                    for(int j = 1; j < 25; j++) {
                        if(temp_max < channel_angle_filtering_buffer[index][j]) {
                            temp_max = channel_angle_filtering_buffer[index][j];
                        }

                        if(temp_min > channel_angle_filtering_buffer[index][j]) {
                            temp_min = channel_angle_filtering_buffer[index][j];
                        }
                    }

                    channel_angle_current = (channel_angle_current - temp_min - temp_max) / (25.0-2.0);
                    //cout << index<<"            "<<eq_filter_index<<endl;
                } // should run background
                
                    //////////////////////////////////////////////////////////////////////////////
            } else {
                // don't run filter
            }
            // channel_angle[index] = 1*channel_angle_current + 0*channel_angle[index];
            channel_angle[index] = channel_angle_current;

        } else {
            cout<< "EQ angle data overflow ?????????????????????????????"<<endl; 
        }

        index++;
    }

    if(index!=1024) {
        cout << "dspRunChannelEst() had illegal value of index after loop: " << index <<"\n";
    }

    //tickleChannelAngle();

    return true;

    //for debug purpuse
    // if(should_run_background)
    // {

    // // if((index==2) || (index==6) || (index==10))
    // //     cout<<"channel  "<<index<<":            "<<channel_angle[index]<<endl;

    // //  if((index==4) || (index==8) || (index==12))
    // //     cout<<"channel  "<<index<<":                                 "<<channel_angle[index]<<endl;
    //     if(index==2)
    //     {
    //         cout<<channel_angle_current<<","<<endl;
    //     }
    // }
}

// bool RadioEstimate::demopilotangle(void){

//      if(all_sc.size() != ALL_SC_CHUNK) {
//         cout << "dspRunChannelEst(2) wrong !!! " << all_sc.size() << " != " << ALL_SC_CHUNK << endl;
//         return false;
//     }

//     uint32_t index = 0;

    
//     for(int index = 2; index < 128; index+=4)
//     {
//         double n_imag;
//         double n_real;
//         ishort_to_double(all_sc[index], n_imag, n_real);
//         pilot_angle[index] = imag_real_angle(n_imag, n_real);

//     }

//     for(index = 894; index < 1022; index+=4)
//     {
//         double n_imag;
//         double n_real;
//         ishort_to_double(all_sc[index], n_imag, n_real);
//         pilot_angle[index] = imag_real_angle(n_imag, n_real);
//     }


//     if(array_index == 0) {
//         dsp->tickle(&pilot_angle);
//     }

//     // if((print_pilot_angle_switch) && (!applyeqonce))
//     // {
//     //     auto timenow = chrono::system_clock::to_time_t(chrono::system_clock::now());
//     //     cout<<"SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSs"<<ctime(&timenow)<<endl; 
//     //     for(int index = 2; index < 128; index+=4)
//     //     {
//     //         cout<<"0x"<<HEX32_STRING(all_sc[index])<<",    "<<pilot_angle[index]<<","<<endl;
//     //     }

//     //     cout<<"0x"<<HEX32_STRING(all_sc[130])<<",    "<<"0x"<<HEX32_STRING(all_sc[134])<<",    "<<"0x"<<HEX32_STRING(all_sc[138])<<",    "<<endl;

//     //     if(all_sc[130]<32768)
//     //         cout << all_sc[130]*3.14/32768.0<<endl;
//     //     else
//     //         cout <<(0x10000-all_sc[130])*(-3.14)/32768.0<<endl;


//     //     cout<<"0x"<<HEX32_STRING(all_sc[142])<<",    "<<"0x"<<HEX32_STRING(all_sc[146])<<",    "<<"0x"<<HEX32_STRING(all_sc[150])<<",    "<<"0x"<<HEX32_STRING(all_sc[154])<<endl;

//     //     cout<<"EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"<<endl;
//     // }



//     return true;






// }


bool RadioEstimate::dspRunChannelEstBuffersComplexFilterIIR(void){

    if(all_sc.size() != ALL_SC_CHUNK) {
        cout << "dspRunChannelEst(3) wrong !!! " << all_sc.size() << " != " << ALL_SC_CHUNK << endl;
        return false;
    }


   


    uint32_t index = 0;
    for(const auto n : all_sc)
    {
        double n_imag;
        double n_real;
        ishort_to_double(n, n_imag, n_real);

        //mag_coeff = 32767.0*sqrt(n_imag*n_imag + n_real*n_real)/(n_imag*n_imag + n_real*n_real+100);  //try this one later. may affect shift in cs20 at tx.
        const bool ok = 
            (n_real<=32767.0) &&
            (n_real>=-32768.0) &&
            (n_imag<=32767.0) &&
            (n_imag>=-32768.0);

        if(ok) {

            // channel_angle_current = atan_angle*-1.0;
            if(_eq_use_filter && should_run_background) {
                // add all real and imag to make large vector
                fixed_iir_16(&(eq_iir[index]), &n, eq_iir_gain);

                double imag_iir;
                double real_iir;
                ishort_to_double(eq_iir[index], imag_iir, real_iir);
                
                // note this is NEGATIVE result
                channel_angle[index] = -imag_real_angle(imag_iir, real_iir);
                //cout << index<<"            "<<eq_filter_index<<endl;
            } else {
                // note this is NEGATIVE result
                channel_angle[index] = -imag_real_angle(n_imag, n_real);
            }
        } else {
            cout<< "EQ angle data overflow ?????????????????????????????"<<endl; 
        }

        index++;
    }

    if(index!=1024) {
        cout << "dspRunChannelEst() had illegal value of index after loop: " << index <<"\n";
    }

    //tickleChannelAngle();

    return true;





}

bool RadioEstimate::dspRunChannelEstBuffersComplexFilter(void) {

    if(all_sc.size() != ALL_SC_CHUNK) {
        cout << "dspRunChannelEst(4) wrong !!! " << all_sc.size() << " != " << ALL_SC_CHUNK << endl;
        return false;
    }

    uint32_t index = 0;
    for(const auto n : all_sc)
    {
        double n_imag;
        double n_real;
        ishort_to_double(n, n_imag, n_real);

        //mag_coeff = 32767.0*sqrt(n_imag*n_imag + n_real*n_real)/(n_imag*n_imag + n_real*n_real+100);  //try this one later. may affect shift in cs20 at tx.
        const bool ok = 
            (n_real<=32767.0) &&
            (n_real>=-32768.0) &&
            (n_imag<=32767.0) &&
            (n_imag>=-32768.0);

        if(ok) {

            eq_filter[eq_filter_index][index] = std::make_pair(n_imag, n_real);

            double imag_sum = 0;
            double real_sum = 0;

            // channel_angle_current = atan_angle*-1.0;
            if(_eq_use_filter && should_run_background) {
                // add all real and imag to make large vector
                double a_imag;
                double a_real;
                for(unsigned j = 0; j < 25; j++) {
                    // eq_filter[eq_filter_index][index]
                    std::tie(a_imag, a_real) = eq_filter[j][index];
                    imag_sum += a_imag;
                    real_sum += a_real;
                    // cout << "imag: " << a_imag << "\n";
                }
                
                // note this is NEGATIVE result
                channel_angle[index] = -imag_real_angle(imag_sum, real_sum);
                //cout << index<<"            "<<eq_filter_index<<endl;
            } else {
                // note this is NEGATIVE result
                channel_angle[index] = -imag_real_angle(n_imag, n_real);
            }
        } else {
            cout<< "EQ angle data overflow ?????????????????????????????"<<endl; 
        }

        index++;
    }

    if(index!=1024) {
        cout << "dspRunChannelEst() had illegal value of index after loop: " << index <<"\n";
    }

    //tickleChannelAngle();

    return true;
}

void RadioEstimate::tickleChannelAngle(void) {
    if( _dashboard_eq_update_counter >= _dashboard_eq_update_ratio) {
        if(_dashboard_only_update_channel_angle_r0) {
            // only run for index 0, data is identital at the moment
            if(array_index == 0) {
                dsp->tickle(&channel_angle);
            }
        } else {
            // run for any index
            dsp->tickle(&channel_angle);
        }
        // use same counter for this as well
        dsp->tickle(&sto_delta);
        _dashboard_eq_update_counter = 0;
    } else {
        _dashboard_eq_update_counter++;
    }
        // cout << _dashboard_eq_update_counter << endl;
}


void RadioEstimate::dspRunChannelEstWithSTO(void) {

    const double mag_coeff = 32767.0 / GET_EQ_MAGNITUDE_DIVISOR();

    std::unique_lock<std::mutex> lock(_channel_angle_sent_mutex);

    // default value of 0 degrees
    equalizer_coeff_sent = defaultEqualizerCoeffSent(mag_coeff);

    if(array_index == 0)
    {
        // sc % 4 == 2
        for(int index = 2; index<1024; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent[index];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }

        // (sc % 4 == 1) gets it's value from +1
        for(int index = 1; index<1023; index+=4)
        {
            // if( index == 5 ) {
            //     cout << "[1," << channel_angle_sent_temp[index] << "," << channel_angle_sent[index] << "," << channel_angle_sent_temp[index+1] << "," << channel_angle_sent[index+1] << "," << channel_angle[index+1] <<  "],\n";
            // }
            channel_angle_sent_temp[index] = channel_angle_sent_frozen[index] + channel_angle[index+1];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }

        // (sc % 4 == 3) gets it's value from -1
        for(int index = 3; index<1024; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent_frozen[index] + channel_angle[index-1];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }
    }
    else if(array_index == 1)
    {
        // sc % 4 == 0
        for(int index = 4; index<1024; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent[index];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }

        for(int index = 3; index<1023; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent_frozen[index] + channel_angle[index+1];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }

        for(int index = 5; index<1024; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent_frozen[index] + channel_angle[index-1];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }
    }

    // for flip positive and negative frequency
    // eventually, we do not need this part.
    for(int index = 1; index < 512; index++)
    {
        uint32_t temp = equalizer_coeff_sent[index];

        equalizer_coeff_sent[index] = equalizer_coeff_sent[1024-index];
        equalizer_coeff_sent[1024-index] = temp;
    }

    
    updateChannelFilterIndex();


    cheq_done_flag = true;
    

    times_dsp_run_channel_est++;

    saveChannel();

    sto_delta = calculateEqStoDelta();
    applyEqToSto();
}



///
/// EventDsp.cpp handle_all_sc_callback() loads samples 
/// into our all_sc buf.  after that we get an update
///
/// flow
///   split all inputs into real,imag
///   convert this into the angle theta (atan_angle)
///   all gets written into channel_angle which is our observation of the env
///  next
///   equalizer_coeff_sent is reset to all the same value
///  in the large if(array), different pilot subcarreirs are considered
///   based on index
///   channel_angle_sent_temp is updated by computations based 
///      on channel_angle and channel_angle_sent
///   equalizer_coeff_sent is calculated at the same time
///   
///   at the bottom the eq_coeff_sent is flipped up/down
///   seems like the sent value is not actually sent, instead
///   it is ready to be sent
///   
///   channel_angle_sent is never written in this function
///   it is written in updatePartnerEq() iff we update

void RadioEstimate::dspRunChannelEstEqAllSc(void) {
    
    const double mag_coeff = 32767.0 / GET_EQ_MAGNITUDE_DIVISOR();

    std::unique_lock<std::mutex> lock(_channel_angle_sent_mutex);

    // default value of 0 degrees
    equalizer_coeff_sent = defaultEqualizerCoeffSent(mag_coeff);

    if(array_index == 0)
    {
        // sc % 4 == 2
        for(int index = 2; index<1024; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent[index] + channel_angle[index];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }

        // (sc % 4 == 1) gets it's value from +1
        for(int index = 1; index<1023; index+=4)
        {
            // if( index == 5 ) {
            //     cout << "[0," << channel_angle_sent_temp[index] << "," << channel_angle_sent[index] << "," << channel_angle_sent_temp[index+1] << "," << channel_angle_sent[index+1] << "," << channel_angle[index+1] <<  "],\n";
            // }
            channel_angle_sent_temp[index] = channel_angle_sent[index] + channel_angle[index+1];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }

        // (sc % 4 == 3) gets it's value from -1
        for(int index = 3; index<1024; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent[index] + channel_angle[index-1];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }
    }
    else if(array_index == 1)
    {
        // sc % 4 == 0
        for(int index = 4; index<1024; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent[index] + channel_angle[index];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }

        for(int index = 3; index<1023; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent[index] + channel_angle[index+1];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }

        for(int index = 5; index<1024; index+=4)
        {
            channel_angle_sent_temp[index] = channel_angle_sent[index] + channel_angle[index-1];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }
    }


    // for flip positive and negative frequency
    // eventually, we do not need this part.
    for(int index = 1; index < 512; index++)
    {
        uint32_t temp = equalizer_coeff_sent[index];

        equalizer_coeff_sent[index] = equalizer_coeff_sent[1024-index];
        equalizer_coeff_sent[1024-index] = temp;
    }

    updateChannelFilterIndex();
    


    cheq_done_flag = true;
    

    times_dsp_run_channel_est++;

    saveChannel();

    sto_delta = calculateEqStoDelta();
    applyEqToSto();
}




void RadioEstimate::dspRunChannelEstEqAllScAllPilot(void) {
    
    const double mag_coeff = 32767.0 / GET_EQ_MAGNITUDE_DIVISOR();

    std::unique_lock<std::mutex> lock(_channel_angle_sent_mutex);

    // default value of 0 degrees
    equalizer_coeff_sent = defaultEqualizerCoeffSent(mag_coeff);

    if(array_index == 0)
    {
        // sc % 4 == 2
        for(int index = 1; index<1024; index++)
        {
            channel_angle_sent_temp[index] = channel_angle_sent[index] + channel_angle[index];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }
    }
    else if(array_index == 1)
    {
        for(int index = 1; index<1024; index++)
        {
            channel_angle_sent_temp[index] = channel_angle_sent[index] + channel_angle[index];
            equalizer_coeff_sent[index] = angle_to_ishort(channel_angle_sent_temp[index],mag_coeff);
        }
    }


    // for flip positive and negative frequency
    // eventually, we do not need this part.
    for(int index = 1; index < 512; index++)
    {
        uint32_t temp = equalizer_coeff_sent[index];

        equalizer_coeff_sent[index] = equalizer_coeff_sent[1024-index];
        equalizer_coeff_sent[1024-index] = temp;
    }

    updateChannelFilterIndex();
    


    cheq_done_flag = true;
    

    times_dsp_run_channel_est++;

    saveChannel();

    sto_delta = calculateEqStoDelta();
    // applyEqToSto();
}


void RadioEstimate::resetBER() {
    cout << "Resetting BER calculations and Phase" << endl;
    for(auto& n : demod_est) {
        n.bits_correct = 0;
        n.bits_wrong = 0;
    }

    // demod_est_common_phase = -1;
}


// returns false if success
// true if error
// this considers tx_coarse_est and also
// sends an estimate to our partner
bool RadioEstimate::chooseCoarseSync() {
    auto sz = tx_coarse_est.size();
    auto ones = 0;
    // int min_index = -1;
    for(size_t i = 0; i < sz; i++) {
        if(tx_coarse_est[i]) {
            ones++;
        }
        // tx_coarse_est_mag
    }

    if( ones > 4) {
        cout << "too many ones" << endl;
        return true;
    }

    if( ones < 2 ) {
        cout << "too few ones" << endl;
        return true;
    }

    // bool found = false;
    // auto streak = 0;
    // size_t idx;
    // for(size_t i = 0; i < sz*2; i++) {
    //      idx = i % sz;
    //     if(tx_coarse_est[idx]) {
    //         streak++;
    //     } else {
    //         streak = 0;
    //     }

    //     if( streak == ones-1) {
    //         found = true;
    //         cout << "picking index " << idx << " i " << i << endl;
    //         break;
    //     }
    // }

    // if(!found) {
    //     return true;
    // }

    auto minptr = std::min_element( tx_coarse_est_mag.begin(), tx_coarse_est_mag.end() );
    size_t idx = minptr - tx_coarse_est_mag.begin();
    cout << "picking index " << idx << endl;

    // where did we leave things at the end of guess and check?
    auto current_advance = (coarse_estimates-1) * coarse_bump;
    auto desired = (idx) * coarse_bump;
    auto delta_advance = current_advance - desired;

    if( current_advance != desired ) {
        // always backwards
        cout << "Calculated advance of " << delta_advance << " (" << delta_advance / coarse_bump << ")" << endl;
        dsp->setPartnerSfoAdvance(peer_id, delta_advance, 3);
    }

    // setPartnerSfoAdvance
    return false;
}

// void RadioEstimate::triggerRxMeasureCoarse() {

// }

// void RadioEstimate::monitorRxMeasureCoarse() {
// }


void RadioEstimate::triggerCoarse() {
    trigger_coarse = true;
    coarse_finished = false;
}

void RadioEstimate::runSoapyCoarseSyncFSM() {
    
    int next = coarse_state;
    switch(coarse_state) {
        case 0:

            if( trigger_coarse ) {
                trigger_coarse = false;
                next = 1;
                tx_coarse_est.resize(0);
                tx_coarse_est_mag.resize(0);
                saved_times_coarse_estimated = times_coarse_estimated+coarse_wait;
            }
            break;
        case 1:
            if(saved_times_coarse_estimated == times_coarse_estimated) {
                tx_coarse_est.push_back(coarse_ok);
                if(coarse_ok) {
                    tx_coarse_est_mag.push_back(coarse_delta);
                } else {
                    tx_coarse_est_mag.push_back(9E15); // large number
                }
                saved_times_coarse_estimated += coarse_wait;
                dsp->setPartnerSfoAdvance(peer_id, coarse_bump, 4);
                // cout << "trying next" << endl;
            }
            if( tx_coarse_est.size() == coarse_estimates ) {
                next = 2;
            }
            break;
        case 2:
            for(size_t i = 0; i < tx_coarse_est.size(); i++)
            {
                auto n = tx_coarse_est[i];
                auto mag = tx_coarse_est_mag[i];
                cout << n << " - " << mag << endl;
            }
            cout << endl << endl;
            chooseCoarseSync();
            coarse_finished = true;
            next = 0;
            break;
        case 3:
            break;
        default:
            cout << "invalid state in runSoapyCoarseSyncFSM()" << endl;
            break;

    }
    coarse_state = next;
    
}

void RadioEstimate::runSoapyCoarseSync() {

    // also works with 2000, and 4000
    constexpr size_t chunk_size = 3000;

    if(coarse_buf.size() < chunk_size) {
        return;
    }

    cout << "runSoapyCoarseSync() " << coarse_buf.size() << endl;

    auto coarse_chunk = coarse_buf.get(chunk_size);
    std::vector<double> mags;
    mags.resize(chunk_size);


    double mmin = 9E20;
    double mmax = 0;
    double mean = 0;
    unsigned int j = 0;
    for(auto n : coarse_chunk)
    {
        double n_imag;
        double n_real;
        ishort_to_double(n, n_imag, n_real);
        double mag = n_real*n_real + n_imag*n_imag;
        mags[j] = mag;
        mean += mag;

        mmin = min(mmin, mag);
        mmax = max(mmax, mag);

        // cout << mag << endl;
        j++;
    }
    mean /= chunk_size;

    double variance = 0;
    for(unsigned int i = 0; i < chunk_size; i++) {
        variance += (mags[i] - mean) * 2;
        // variance += mags[i]*mags[i];
    }

    // variance /= chunk_size;

    // variance = std::sqrt(variance);
    

    constexpr double coarse_thresh = 3.32082e+06;
    constexpr double coarse_thresh_mean = 1.2e+07;

    coarse_delta = mmax - mmin;

    // variance -1.47793e-09 mean 1162.54 min 0 max 9925 (9925)


    if( coarse_delta <= coarse_thresh && mean > coarse_thresh_mean ) {
        coarse_ok = true;
    } else {
        coarse_ok = false;
    }

    if( !prev_coarse_ok && coarse_ok ) {
        cout << "Coarse Sync just became OK " << coarse_ok << endl;


        cout << "variance " << variance << " mean " << 
        mean << " min " << mmin << " max " << 
        mmax << " (" << coarse_delta << ")" << endl;


    } else if( prev_coarse_ok && !coarse_ok ) {
        cout << "Coarse Sync just became BAD" << endl;
    }

    prev_coarse_ok = coarse_ok;

    times_coarse_estimated++;
}




// run once
void RadioEstimate::dspSetupDemod() {


    cout << "In dspSetupDemod() with " << DATA_TONE_NUM << " enabled" << endl;

    demod_est.resize(DATA_TONE_NUM);
    // demod_est_common_phase = -1; // all subcarriers use same phase

    // Note: we are using a c++ "range based loop" here
    // if we use & for the type, we will be able to modify them
    // this is how we init;
    

    // print them out
    // for(auto n : demod_sc_phase) {
    //     cout << n << endl;
    // }

}




// returns 
// bool RadioEstimate::mostRecentTxRxDeltaFrame() {

// }



// bool RadioEstimate::tdmaCondition() {
//     HiggsTDMA& td = tdma[DATA_SUBCARRIER_INDEX];
//     return (td.lifetime_rx != 0 || td.lifetime_tx != 0);
// }


void RadioEstimate::tickleAllTdma() {
    dsp->tickle(&demod->tdma_phase);
    dsp->tickle(&demod->times_matched_tdma_6);
    dsp->tickle(&demod->data_subcarrier_index);
    dsp->tickle(&demod->track_demod_against_rx_counter);
    dsp->tickle(&demod->track_record_rx);
    dsp->tickle(&demod->last_mode_sent);
    dsp->tickle(&demod->last_mode_data_sent);
    dsp->tickle(&demod->td->found_dead);
    dsp->tickle(&demod->td->sent_tdma);
    dsp->tickle(&demod->td->lifetime_tx);
    dsp->tickle(&demod->td->lifetime_rx);
    dsp->tickle(&demod->td->fudge_rx);
    dsp->tickle(&demod->td->needs_fudge);
}


void RadioEstimate::tickDataAndBackground() {

    demod->run(times_eq_sent);
    // debugPrintDemod();
    continualBackgroundEstimates();

    tickleAllTdma();

    // handleDataToEq();

    // demopilotangle();



    // runSoapyCoarseSync();
    // runSoapyCoarseSyncFSM(); // run after
}


void RadioEstimate::consumePerformance(const uint32_t word) {
    
    uint32_t budget_period = 8; // FIXME we can change this later

    uint32_t data = word & 0x00ffffff;

    switch(est_remote_perf) {
        case 0:
            if(word == PERF_02_PCCMD) {
                est_remote_perf = 1;
            }
            break;
        case 1:
            if( word == (PERF_02_PCCMD | budget_period) ) {
                est_remote_perf = 2;
            } else {
                est_remote_perf = 0;
            }
            break;
        case 2:
            hold_perf.push_back(data);
            break;
        default:
            break;
        // case 64:
    }

    double ave_idle = 0;
    double ave_cycles = 0;
    uint32_t ave_i = 0;
    uint32_t ave_c = 0;
    double est_result;

    const uint32_t CLOCK_BUDGET_PER_1 = 5120;

    const double total_budget = 1.0 * CLOCK_BUDGET_PER_1 * budget_period;

    if(hold_perf.size() == 64) {
        // d has cmd stripped already
        uint32_t i = 0;
        for(auto d: hold_perf) {
            if( i % 2 == 0 ) {
                // cout << " == 0 " << d << endl;
                // idle word
                ave_idle += d;
                ave_i++;
            } else {
                // cout << " == 1 " << d << endl;
                // time word
                ave_cycles += d;
                ave_c++;
            }

            i++;
        }

        ave_idle /= ave_i;
        ave_cycles /= ave_c;



        // raw value
        double use_1 = ave_cycles / total_budget;

        // may be adjusted
        est_result = ave_cycles / total_budget;

        cout << "r" << array_index << " cs20 performance: " << "id: " << ave_idle << " cyc:" << ave_cycles << " use_1: " << use_1 << " usage: " << est_result << endl;

        cpu_load[0] = est_result;
        dsp->tickle(&cpu_load[0]);

        hold_perf.erase(hold_perf.begin(), hold_perf.end());
        est_remote_perf = 0; // reset state
    }
}


// calculates the current estimated clock on higgs.
// this is done using now() and the most recent estimate we've gotten
epoc_frame_t RadioEstimate::predictScheduleFrame(int &error, const bool useCalibrated, const bool print) const {
    error = (!epoc_valid) ? 1:0;

    // take now, and calculate duration since we last wrote to epoc_estimated
    auto now = std::chrono::steady_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::microseconds>(
              now-epoc_timestamp).count();

    constexpr double factor = 1.0;

    // figure out how many frames since we last got an estimate
    double frames_since_est = (elapsed_time / factor / 1E6) * SCHEDULE_FRAMES_PER_SECOND;
    int frame_number = int(frames_since_est) % SCHEDULE_FRAMES;

    if(print) {
        std::cout << "Elapsed time (us): " << elapsed_time << std::endl;
        std::cout << "Total frames since start: " << frames_since_est << std::endl;
        std::cout << "Current OFDM frame number: " << frame_number << std::endl;
    }

    epoc_frame_t ret;
    if( useCalibrated ) {
        ret = getCalibratedEpoc();
    } else {
        ret = epoc_estimated;
    }

    // based on how long since we got an estimate, add frames and return
    ret = add_frames_to_schedule(ret, (uint32_t)frames_since_est);

    return ret;

}


// calculates the current estimated clock on higgs.
// this is done using now() and the most recent estimate we've gotten
uint32_t RadioEstimate::predictLifetime32(int &error, const bool print) const {

    error = (!epoc_valid) ? 1:0;

    // take now, and calculate duration since we last wrote to epoc_estimated
    auto now = std::chrono::steady_clock::now();
    auto elapsed_time = chrono::duration_cast<chrono::microseconds>(
              now-epoc_timestamp).count();

    constexpr double factor = 1.0;

    // figure out how many frames since we last got an estimate
    const double frames_since_est = (elapsed_time / factor / 1E6) * SCHEDULE_FRAMES_PER_SECOND;
    // int frame_number = int(frames_since_est) % SCHEDULE_FRAMES;

    if( unlikely(frames_since_est < 0.0) ) {
        cout << "Warning: negative frames_since_est\n";
    }

    // epoc_frame_t ret = epoc_estimated;
    // (epoc_frame_t) epoc_estimated;

    const uint64_t tmp = schedule_to_pure_frames(epoc_estimated);

    uint32_t ret = ((uint32_t)tmp) + int(frames_since_est);


    if(unlikely(print)) {
        std::cout << "Elapsed time (us): " << elapsed_time << std::endl;
        std::cout << "Total frames since start: " << frames_since_est << std::endl;
        std::cout << "Tmp: " << tmp << std::endl;
        // std::cout << "Current OFDM frame number: " << frame_number << std::endl;
    }



    // based on how long since we got an estimate, add frames and return
    // ret = add_frames_to_schedule(ret, (uint32_t)frames_since_est);

    return ret;

}


// returns valid / only the latest value since a clear
std::pair<bool, int32_t> RadioEstimate::getEpocRecentReply(bool clear) const {
    bool valid = epoc_recent_reply_frames_valid;
    if(valid) {
        if( clear ) {
            valid = false;
        }
        return std::pair<bool, int32_t>(valid, epoc_recent_reply_frames);
    } else {
        return std::pair<bool, int32_t>(valid, epoc_recent_reply_frames);
    }
}

void RadioEstimate::handleFillLevelReplyDuplex(const uint32_t word) {


    const int32_t delta = schedule_parse_delta_ringbus(word);

    // was applyFillLevelToMember()
    {
        epoc_recent_reply_frames = delta;
        epoc_recent_reply_frames_valid = true;
    }



    cout << "lifetime_delta represented as frames " << delta << endl;


    if( abs(epoc_recent_reply_frames) > (SCHEDULE_FRAMES*2) ) {
        cout << "lifetime_delta is WAY out of estimate" << endl;
    }
    map_mov_acks_received++;

}

epoc_frame_t RadioEstimate::getCalibratedEpoc() const {
    epoc_frame_t cal = adjust_frames_to_schedule(epoc_estimated, epoc_calibration);
    // if( epoc_calibration != 0) {
    //     cout << "getCalibratedEpoc operating with delta of " << epoc_calibration << endl;
    // }
    return cal;
}


// same underlying data as handleEpocReply() but a more efficient way
// this is running on print ringbus thread, probably needs a lock or some shit
void RadioEstimate::consumeContinuousEpoc(const uint32_t word) {
    epoc_timestamp = std::chrono::steady_clock::now();

    uint32_t cmd_type = word & 0xff000000;
    uint32_t cmd_data = word & 0x00ffffff;
    switch(cmd_type) {
        case TX_PROGRESS_REPORT_PCCMD:
            epoc_estimated.frame = cmd_data;
            // cout << "Continuous Progress: " << epoc_estimated.frame << ", " << epoc_estimated.frame/512 << endl;
            break;
        case TX_EPOC_REPORT_PCCMD:
            epoc_estimated.frame = 0;
            epoc_estimated.epoc = cmd_data;
            epoc_valid = true; // gets set to false very infrequently
            cout << "Continuous Epoc Second: " << epoc_estimated.epoc << endl;
            break;
        default:
            cout << "illegal type passed to consumeContinuousEpoc" << endl;
            break;
    }
    
}


uint32_t RadioEstimate::getEqHash(const std::vector<uint32_t>& eq) {
    uint32_t res0 = xorshift32(1, eq.data(), eq.size());
    return res0;
}

void RadioEstimate::dispatchAttachedRb(const uint32_t word) {
    cout << "parent class dispatchAttachedRb()" << endl;
}

// look for a hash
// the rx side puts a hash in eq_hash_expected when it sends a hash to tx side
// first we loop through eq_hash_expected and erase any expired entries
// then we look for the word
void RadioEstimate::eqHashCompare(const uint32_t word) {
    auto now = std::chrono::steady_clock::now();
    uint64_t now_age = std::chrono::duration_cast<std::chrono::microseconds>( 
        now - init_timepoint
        ).count();

    constexpr uint64_t max_age = 1E6*10;

    // cout << "Started with " << eq_hash_expected.size() << " ideal eq to search\n";

    bool search_expired = true;
    while(search_expired) {
        search_expired = false;
        for(auto it = eq_hash_expected.begin(); it != eq_hash_expected.end(); ++it) {
            uint32_t ideal_word;
            uint64_t ideal_age;
            std::tie(ideal_word, ideal_age) = *it;

            if( (now_age - ideal_age) > max_age ) {
                eq_hash_expected.erase(it);
                search_expired = true;
                break;
            }
            // uint ideal_word;
        }
    }

    // cout << "After prune there were " << eq_hash_expected.size() << " ideal eq to search\n";

    bool hash_found = false;
    for(auto it = eq_hash_expected.begin(); it != eq_hash_expected.end(); ++it) {
        uint32_t ideal_word;
        uint64_t ideal_age;
        std::tie(ideal_word, ideal_age) = *it;
        if( word == ideal_word ) {
            eq_hash_expected.erase(it);
            hash_found = true;
            break;
        }

        // uint ideal_word;
    }

    if( hash_found ) {
        cout << "Correct Eq hash found " << HEX32_STRING(word) << "\n";
    } else {
        cout << "!!!!!!!!!!!!!!\n";
        cout << "!!!!!!!!!!!!!!\n";
        cout << "!!!!!!!!!!!!!!  did not find eq hash of " << HEX32_STRING(word) << "\n";
    }

}

// gets called twice by dispatchRemoteRb
// tx side forwards us two ringbus which are the result of the hash of an eq
// that we sent to cs20 on tx side
void RadioEstimate::handleEqHashReply(const uint32_t word) {
    uint32_t upper = word & 0xff0000;
    uint32_t data = word & 0xffff;

    // cout << "handleEqHashReply " << eq_hash_state << ", " << upper <<  "\n";

    if( eq_hash_state == 0 ) {
        if( upper == 0x00000 ) {
            eq_hash_word = data;
            eq_hash_state = 1;
        }
    } else {
        if( upper == 0x10000 ) {
            eq_hash_word |= data << 16;
            eqHashCompare(eq_hash_word);
            // cout << "called\n";
        }
        eq_hash_state = 0;
        eq_hash_word = 0;
    }
}

void RadioEstimate::saveIdealEqHash(const std::vector<uint32_t>& eq) {
    auto now = std::chrono::steady_clock::now();
    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
        now - init_timepoint
        ).count();

    uint32_t hash = getEqHash(eq);

    eq_hash_expected.emplace_back(hash, elapsed_us);
}


// see EventDspFsm.cpp :: dispatchAttachedRb_TX()
// see                    dspDispatchAttachedRb()
void RadioEstimate::dispatchRemoteRb(const uint32_t word) {

    // cout << "dispatchRemoteRb got " << HEX32_STRING(word) << "\n";

    uint32_t data = word & 0x00ffffff;
    uint32_t cmd_type = word & 0xff000000;
    (void)data;
    switch(cmd_type) {
        case COARSE_SYNC_PCCMD:
            break;
        case PERF_02_PCCMD:
            consumePerformance(word); // pass whole word
            break;
        case FEEDBACK_ALIVE:
            feedback_alive_count++;
            break;

        case FEEDBACK_HASH_PCCMD:
            handleEqHashReply(data);
            break;

        default:
            break;
    }

}

void RadioEstimate::resetFeedbackBackDetectRemote() {
    feedback_alive_count = 0;
}

void RadioEstimate::feedbackPingRemote() {
    dsp->pingPartnerFbBus(peer_id);
}

void RadioEstimate::idleCheckRemoteRing() {
    for(auto word : remote_rb_buffer) {
        dispatchRemoteRb(word);
    }

    auto sz = remote_rb_buffer.size();
    if( sz ) {
        // cout << "erasing size " << sz << endl;
    }

    remote_rb_buffer.erase(remote_rb_buffer.begin(), remote_rb_buffer.end());
}

// some values are directly modified on this object,
// so we can slow ticle them very time our fsm rolls over
void RadioEstimate::tickleOutsiders() {
    dsp->tickle(&should_mask_data_tone_tx_eq);
    dsp->tickle(&should_run_background);
    dsp->tickle(&should_mask_all_data_tone);
}

// pass zero length vector to unmask all subcarriers
void RadioEstimate::setMaskedSubcarriers(const std::vector<unsigned>& sc) {
    if( sc.size() == 0 ) {
        // function arguments request 0 masked subcarriers
        // in order to make this happen, we need to send subcarrier zero with index zero
        raw_ringbus_t rb0 = {RING_ADDR_TX_EQ, SET_MASK_SC_CMD | 0x00000 | 0 };
        dsp->zmqRingbusPeerLater(peer_id, &rb0, 0);
        return;
    }

    // for each subcarrier, send a ringbus with an index
    // right now cs20 only supports 4 masked ringbus, but cs20 will protect
    // against larger values
    for(unsigned i = 0; i < sc.size(); i++) {
        const uint32_t w = sc[i];

        if( w >= 1024 ) {
            cout << "Illegally large subcarrier " << w << " passed to setMaskedSubcarriers()\n";
        }

         // same as shift by 16
        const uint32_t index_bits = (0x10000)*i;

        // pass the index, and the subcarrier, masked to be less than 1024
        raw_ringbus_t rbx = {RING_ADDR_TX_EQ, SET_MASK_SC_CMD | index_bits | (w&0x3ff) };

        // we pass i as the delay. this leads to 1 us delay between each ringbus
        // when these make it to the transmit side they will be delayed longer than this 
        // to protect ringbus
        dsp->zmqRingbusPeerLater(peer_id, &rbx, i);
    }
}

void RadioEstimate::setPartnerTDMA(const uint32_t dmode, const uint32_t value) {
    dsp->setPartnerTDMA(peer_id, dmode, value);
    demod->last_mode_sent = dmode;
    demod->last_mode_data_sent = value;
}

void RadioEstimate::partnerOp(const std::string s, const uint32_t _sel, const uint32_t _val) {

    auto pack = siglabs::rb::op(s, _sel, _val, GENERIC_OPERATOR_CMD);

    raw_ringbus_t rb0 = {RING_ADDR_TX_PARSE, pack[0] };
    raw_ringbus_t rb1 = {RING_ADDR_TX_PARSE, pack[1] };
    dsp->zmqRingbusPeerLater(peer_id, &rb0, 0);
    dsp->zmqRingbusPeerLater(peer_id, &rb1, 1);
}

void RadioEstimate::setTDMASc() {
    dsp->setPartnerTDMASubcarrier(peer_id, getScForTransmitTDMA());
}

void RadioEstimate::maskAllSc() {
    // Mask all subcarriers we know about.
    // This works, because if a subcarrier is masked and also designated for TDMA
    // TDMA takes priority
    const auto mask_sc = getAllTransmitTDMA();
    cout << "r" << array_index << " Masking " << mask_sc.size() << " subcarriers\n";
    setMaskedSubcarriers(mask_sc);
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
void RadioEstimate::tick_localfsm()
{
    // cout<<"Hello World!!!!!"<<endl; 
    
    struct timeval tv = DEFAULT_NEXT_STATE_SLEEP;
    size_t __timer = __UNSET_NEXT__;
    size_t __imed = __UNSET_NEXT__;
    size_t next = __UNSET_NEXT__;

    if( fsm_event_pending != NOOP_EV && fsm_event_pending != most_recent_event.d0) {
        // cout << "tick_localfsm() " << this->array_index << " sleeping" << endl;
        tv = _1_SECOND_SLEEP;
        evtimer_add(localfsm_next_timer, &tv);
        return;
    } else if( fsm_event_pending != NOOP_EV && fsm_event_pending == most_recent_event.d0) {
        // continue
        cout << "matching  fsm_event_pending == most_recent_event.d0 " << endl;
        cout << "radio_state " << radio_state << " " << radio_state_pending << endl;
        fsm_event_pending = NOOP_EV; // no event
    } else {
        // should be normal
    }

    radio_state = radio_state_pending;
    dsp->tickle(&radio_state);


    switch(radio_state) {

        case DID_BOOT: {
                cout << "r" << array_index << " set DID_BOOT" << endl;

                if( GET_RE_FSM_TYPE() == "js") {
                    next = GO_NOW(DEBUG_STALL); // stall this fsm, because js will do it
                } else {
                    next = GO_EVENT(PRE_SFO_STATE_0, REQUEST_FINE_SYNC_EV); // normal operations
                }

                // only use this if working with something like test_qam_5
                constexpr bool force_demod = false;
                if( force_demod ) {

                    // was setTdmaSyncFinished()
                    demod->setDemodEnabled(false);
                    soapy->demodThread->dropSlicedData = false;

                    // HiggsTDMA& td = tdma[DATA_SUBCARRIER_INDEX];
                    // td.lifetime_rx = 1;
                    // td.lifetime_tx = 1;
                    times_eq_sent = 1;
                }
            }

            break;
        
        case PRE_SFO_STATE_0:
                _prev_sfo_est = times_sfo_estimated+1;
                cout << "r" << array_index << " PRE_SFO_STATE_0 " << times_sfo_estimated << endl;
                // dsp->ringbusPeerLater(peer_id, RING_ADDR_CS11, COOKED_DATA_TYPE_CMD | 1, 1);
                next = GO_AFTER(SFO_STATE_0, _500_MS_SLEEP);
            break;

        case SFO_STATE_0:
            if(times_sfo_estimated - _prev_sfo_est == GET_SFO_ESTIMATE_WAIT() ) {
            // if(1){
                cout << "r" << array_index << " SFO_STATE_0 counter matched" << endl;
                auto failure = sfoState();
                if(!failure) {
                    next = GO_AFTER(STO_0, _1_MS_SLEEP);
                    cout << "r" << array_index << " Exiting SFO_STATE_0" << endl;
                    _prev_sto_est = times_sto_estimated;
                }
                else
                {
                    next = GO_AFTER(SFO_STATE_0, _1_MS_SLEEP);
                }
                _prev_sfo_est = times_sfo_estimated;
            }
            break;

        case STO_0:
            if( GET_STO_SKIP() ) {
                cout << "r" << array_index << " SKIPPING STO_0!\n";
                sendEvent(MAKE_EVENT(RADIO_ENTERED_BG_EV,array_index));
                next = GO_NOW(WAIT_EVENTS);
            } else if(times_sto_estimated - _prev_sto_est == 2) {
            // if(1) {
                auto failure = stoState();
                if(!failure) {

                    // note we don't check counter for this jump
                    next = GO_AFTER(STO_EQ_0, _8_SECOND_SLEEP);

                    // _prev_cfo_est = times_cfo_estimated;
                    
                    cout << "r" << array_index << " Exiting STO_0" << endl;
                } else {


                    // reset this so we will try in 2 times
                    _prev_sto_est = times_sto_estimated;
                }
            }
            break;

       case STO_EQ_0:

            ///////////////
            // 
            // Used to manually bump sfo to a wrong value on purpose
            if(false) {
                sfo_estimated += 0.02;
                updatePartnerSfo();
            }
///
       /// applies one time eq according to margin
       ///
             ////////////////////////////////////////dspRunStoEq(1.0);

             // setting 2nd to true sort of skips logic due to
             // times_eq_sent. fixme re-look at this logic
             /////////////////////////////////////////////////////////updatePartnerEq(false, false);


             _prev_cfo_est = times_cfo_estimated;
             next = GO_AFTER(CFO_0, _1_MS_SLEEP);
             // next = GO_AFTER(CFO_0, _8_SECOND_SLEEP);
             cout << "r" << array_index << " Exiting STO_EQ_0" << endl;
             break;


        case CFO_0:

            if(times_cfo_estimated - _prev_cfo_est == 2) {
            // if(1) {
                cout << "r" << array_index << " CFO_0 counter matched" << endl;
                auto failure = cfoState();
                if(!failure) {
                    cout << "Radio " << array_index << " Exiting CFO_0" << endl;
                    next = GO_AFTER(BACKGROUND_SYNC, _1_MS_SLEEP);
                }
                else
                {
                     next = GO_AFTER(CFO_0, _1_MS_SLEEP);
                }
                _prev_cfo_est = times_cfo_estimated;

            }
            break;
        case BACKGROUND_SYNC:
  
            print_pilot_angle_switch = true;

             // event_active(background_sync_next, EV_WRITE, 0);
            cout << "r" << array_index << " above startBackground()" << endl;
            startBackground(); // kick off
            sendEvent(MAKE_EVENT(RADIO_ENTERED_BG_EV,array_index));
            /////////////////////next = GO_NOW(WAIT_EVENTS);
            GO_NOW(RE_FINESYNC);
            break;

        case RE_FINESYNC:

             sfoState();

             if(((uint32_t)(abs(sto_estimated))>>STO_ADJ_SHIFT) >0)
             {
                updatePartnerSto(((uint32_t)(abs(sto_estimated))>>STO_ADJ_SHIFT));
             }

             // if(aaa == 2)
             // {
             //   next = GO_AFTER(RE_FINESYNC, _12_SECOND_SLEEP);
             // }
             // else
             // {
             //    next = GO_AFTER(RE_FINESYNC, _4_SECOND_SLEEP);
             // }

             next = GO_AFTER(RE_FINESYNC, _2_SECOND_SLEEP);
             

             

             break;

        // warning: putting very large sleep times
        // will interrupt demodulated data due to code in handle_radios_tick()
        case WAIT_EVENTS:
            // This event fires when we want to TDMA sync the first time
            // This will adjust TDMA
            if(most_recent_event.d0 == REQUEST_TDMA_EV &&
                most_recent_event.d1 == array_index) {
                clearRecentEvent();
                next = GO_NOW(GOT_TDMA_REQUEST);
            }


            // This will only check TDMA (I guess it does reset/reaquire the phase)
            // however it will not adjust
            if(most_recent_event.d0 == CHECK_TDMA_EV &&
                most_recent_event.d1 == array_index) {
                clearRecentEvent();
                next = GO_NOW(RECHECK_TDMA_0);
            }


            break;

        case GOT_TDMA_REQUEST:
            {
                 stringstream ss;
                 ss << "r" << array_index << " entered GOT_TDMA_REQUEST";
                 cout << ss.str() << "\n";
                 // dsp->tickleLog("rx", ss.str());

                 next = GO_AFTER(TDMA_STATE_0_0, _5_MS_SLEEP);
            }
            break;

        case TDMA_STATE_0_0:
            {
                stringstream ss;
                ss << "r" << array_index << " entered TDMA_STATE_0_0";
                cout << ss.str() << "\n";

                partnerOp("set", 2, 0); // reset schedule offset

                next = GO_AFTER(TDMA_STATE_0_1, _5_MS_SLEEP);
            }
            break;

        case TDMA_STATE_0_1:
            {
                stringstream ss;
                ss << "r" << array_index << " entered TDMA_STATE_0_1";
                cout << ss.str() << "\n";
                // HiggsTDMA& td = tdma[DATA_SUBCARRIER_INDEX];
                demod->td->reset(); // reset our tracking for our single td object
                demod->resetDemodPhase(); // reset common tracking
                demod->resetTdmaAndLifetime();
                cout << "r" << array_index << " after reset resetDemodPhase() and td.reset()" << endl;

                // new tdma requires we go to zero here
                // so we can hit found dead in next state


                // sends deadbeef and some other pattern
                // setPartnerTDMA(0, 0);

                // send our own thing
                // setPartnerTDMA(6, 0);
                demod->track_demod_against_rx_counter = true;

                times_wait_tdma_state_0 = 0;
                next = GO_AFTER(TDMA_STATE_2_DOT_5, _5_MS_SLEEP);
            }
            break;
        // case TDMA_STATE_0:
        //     {
        //     cout << "TDMA_STATE_0 " << times_wait_tdma_state_0 << "\n";
        //     HiggsTDMA& td = *demod->td;
        //     if(td.found_dead) {
                    
        //             stringstream ss;
        //             ss << "r" << array_index << " found dead";
        //             // dsp->tickleLog("rx", ss.str());

        //             cout << "r" << array_index << " found dead" << endl;
        //             // was this
        //             setPartnerTDMA(4, 0);

        //             // trying this
        //             // setPartnerTDMA(6, 0);

        //             // td.sent_tdma = true;
        //             td.lifetime_rx = 0;
        //             td.lifetime_tx = 0;
        //             next = GO_AFTER(TDMA_STATE_1, _1_MS_SLEEP);
                    
        //             // new
        //             td.reset();
        //             demod->resetDemodPhase();
        //         } else if (GET_RUNTIME_SKIP_TDMA_R0()) {
        //             cout << "r" << array_index << " Got GET_RUNTIME_SKIP_TDMA_R0()" << endl;
        //             demod->tdma_phase = 4;
        //             td.sent_tdma = true;
        //             td.lifetime_rx = 6;
        //             td.lifetime_tx = 6;
        //             times_eq_sent++;
        //             GO_NOW(WAIT_EVENTS);
        //         } else if (times_wait_tdma_state_0 > ((10/5)*30)) {
        //             cout << "TDMA_STATE_0 timed out, going back to GOT_TDMA_REQUEST\n";
        //             next = GO_AFTER(GOT_TDMA_REQUEST, _1_MS_SLEEP);
        //         } else {
        //             next = GO_AFTER(TDMA_STATE_0, _500_MS_SLEEP);
        //         }

        //         times_wait_tdma_state_0++;

        //     }
        //     break;
        // case TDMA_STATE_1:
        //      {
        //         // uint32_t prev_len = 
        //         // FIXME: when we enter this step, check the last 2 values in 

        //         // this td.xx flags are set by the parseDemodWords()
        //         // in my logs if i print all words, I see that the delta is stable for
        //         // at least 2 seconds
        //         HiggsTDMA& td = *demod->td;
        //         if(td.lifetime_rx != 0 || td.lifetime_tx != 0) {
        //             cout << "r" << array_index << " tx: " << td.lifetime_tx << " rx: " << td.lifetime_rx << " TDMA_STATE_1" << endl;
        //             // alignSchedule(td.lifetime_tx, td.lifetime_rx);
        //             demod->tdma_phase = -1;

        //             next = GO_AFTER(TDMA_STATE_2, _1_MS_SLEEP);
        //             // next = GO_AFTER(TDMA_STATE_2, _8_SECOND_SLEEP);
        //         }
        //     }
        //     break;

        // case TDMA_STATE_2:
        //     {   
        //         cout << "r" << array_index << " sending tdma mode 4 again to double check alignment" << endl;
        //         setPartnerTDMA(4, 0);
        //         demod->resetTdmaAndLifetime();
        //         next = GO_AFTER(TDMA_STATE_2_DOT_5, _2_SECOND_SLEEP);
        //     }
        //     break;

        case TDMA_STATE_2_DOT_5:
            cout << "r" << array_index << " TDMA_STATE_2_DOT_5 " << endl;

            setPartnerTDMA(6, 0);
            next = GO_AFTER(TDMA_STATE_2_DOT_6, _3_SECOND_SLEEP);
            break;

        // case TDMA_STATE_2_DOT_55:
        //     {
        //         HiggsTDMA& td = tdma[DATA_SUBCARRIER_INDEX];
        //         cout << "r" << array_index << " TDMA_STATE_2_DOT_55" << endl;
        //         tdma_phase = -1;
        //         td.needs_fudge = false;
        //         next = GO_AFTER(TDMA_STATE_2_DOT_6, _3_SECOND_SLEEP);
        //         break;
        //     }

        case TDMA_STATE_2_DOT_6:
            {
                cout << "r" << array_index << " TDMA_STATE_2_DOT_6" << endl;
                HiggsTDMA& td = *demod->td;
                if( td.needsUpdate() ) {
                    cout << "r" << array_index << " did not find TDMA mode 6" << endl;
                    demod->td->reset(); // reset our tracking for our single td object
                    demod->resetDemodPhase(); // reset common tracking
                    demod->resetTdmaAndLifetime();
                    next = GO_AFTER(TDMA_STATE_0_0, _1_MS_SLEEP);
                } else {
                    demod->alignSchedule4();
                    // when we call alignSchedule, we are changing the phase of the transmit side
                    // this means we need to reset the phase, or else the words put into the buffer
                    // will always be wrong
                    // we need to wait for the command to land before we reset this
                    // I saw this edge case on my 2nd run
                    // demod_special_phase = -1;
                    next = GO_AFTER(TDMA_STATE_2_DOT_7, _1_SECOND_SLEEP);
                }

                // if(1) {
                //     cout << "TDMA_STATE_2_DOT_6 force next" << endl;
                //     // next = GO_NOW(TDMA_STATE_2_DOT_5);
                //     next = GO_AFTER(TDMA_STATE_2_DOT_55, _1_SECOND_SLEEP);
                // }


                break;
            }

        case TDMA_STATE_2_DOT_7:
            {
                cout << "r" << array_index << " TDMA_STATE_2_DOT_7 reset" << endl;
                if( false ) {
                    demod->resetDemodPhase(); // same as demod->tdma_phase = -1;
                } else {
                    demod->td->reset(); // reset our tracking for our single td object
                    demod->resetDemodPhase(); // reset common tracking
                    demod->resetTdmaAndLifetime();
                }
                next = GO_AFTER(TDMA_STATE_3, _3_SECOND_SLEEP);
                break;
            }

        case TDMA_STATE_3:
            {   
                HiggsTDMA& td = *demod->td;
                if(td.lifetime_rx != 0 || td.lifetime_tx != 0) {
                    cout << "r" << array_index << " Final tx: " << td.lifetime_tx << " rx: " << td.lifetime_rx << "\n";
                    cout << "r" << array_index << " Final tx mod: " << td.lifetime_tx % SCHEDULE_FRAMES << " rx mod: " << td.lifetime_rx % SCHEDULE_FRAMES << "\n";
                    cout << "r" << array_index << " Final epoc: " << td.epoc_tx << "\n";
                    
                    

                   next = GO_NOW(TDMA_STATE_4);
                }
            }
            break;

        case TDMA_STATE_4:
            {
                cout << "TDMA_STATE_4 DOES NOT EXIST\n";
                next = GO_NOW(DEBUG_STALL);
                // demod->alignSchedule6();

                // next = GO_AFTER(TDMA_STATE_5, _2_SECOND_SLEEP);
            }
            break;


        case TDMA_STATE_5:
            {
                
                // disable my custom tone from doing work on cs20 (different than just masking with eq
                // which is what we did before)
                // !!!FIXME go to state keep here
                // mode 20 is like "parking" mode
                // note mode 21 will reset schedule->offset which we do not want here
                setPartnerTDMA(20, 0);

                sendEvent(MAKE_EVENT(FINISHED_TDMA_EV,array_index));

                demod->setDemodEnabled(false);

                cout << "r" << array_index << " going to WAIT_EVENTS\n";
                next = GO_NOW(WAIT_EVENTS);
            }
            break;


        case RECHECK_TDMA_0:
            {
                cout << "r" << array_index << " RECHECK_TDMA_0\n";
                save_tdma_mode_during_recheck = demod->last_mode_sent;
                save_tdma_should_mask_data_tone_tx_eq = should_mask_data_tone_tx_eq;

                demod->td->reset(); // reset our tracking for our single td object
                demod->resetDemodPhase(); // reset common tracking
                demod->resetTdmaAndLifetime();
                demod->setDemodEnabled(true);

                should_mask_data_tone_tx_eq = false;

                setPartnerTDMA(6, 0);

                // if TDMA was masked (True) then we need to wait for next eq to update
                if( save_tdma_should_mask_data_tone_tx_eq ) {
                    next = GO_AFTER(RECHECK_TDMA_1, _6_SECOND_SLEEP);
                } else {
                    // tdma EQ was ok before we started, jump right in
                    next = GO_AFTER(RECHECK_TDMA_1, _5_MS_SLEEP);
                }

            }
            break;

        case RECHECK_TDMA_1:
            {

                HiggsTDMA& td = *demod->td;
                if(td.lifetime_rx != 0 || td.lifetime_tx != 0) {
                    cout << "r" << array_index << " RECHECK Final tx: " << td.lifetime_tx << " rx: " << td.lifetime_rx << endl;
                    cout << "r" << array_index << " RECHECK Final tx mod: " << td.lifetime_tx % SCHEDULE_FRAMES << " rx mod: " << td.lifetime_rx % SCHEDULE_FRAMES << endl;
                    cout << "r" << array_index << " RECHECK Final epoc: " << td.epoc_tx << "\n";

                    cout << "r" << array_index << " RECHECK setting tdma mode back to " << save_tdma_mode_during_recheck << " and ZERO argument" << endl;
                    // go back to previos mode FIXME always resets argument to 0;
                    setPartnerTDMA(save_tdma_mode_during_recheck, 0);

                    demod->setDemodEnabled(false);

                    should_mask_data_tone_tx_eq = save_tdma_should_mask_data_tone_tx_eq;


                    cout << "r" << array_index << " RECHECK going to WAIT_EVENTS" << endl;
                    next = GO_NOW(WAIT_EVENTS);
                }
            }
            break;

        case DEBUG_STALL:
             // A small value here ensures ringbus are consumed faster
             next = GO_AFTER(DEBUG_STALL, _100_MS_SLEEP);
             break;

        default:
            cout << "r" << array_index << " Bad state in RadioEstimate::tickfsm" << endl;
            break;
    }



    if(__timer != __UNSET_NEXT__) {
        // cout << "timer mode" << endl;
        radio_state_pending = __timer;
        evtimer_add(localfsm_next_timer, &tv);
    } else if(__imed != __UNSET_NEXT__) {
        // cout << "imed mode" << endl;
        radio_state_pending = __imed;
        auto timm = SMALLEST_SLEEP;
        evtimer_add(localfsm_next_timer, &timm);
    } else if(next != 0 && next != __UNSET_NEXT__) {
        cout << endl << "ERROR You cannot use:  ";
        cout << endl << "   next = " << endl << endl << "without GO_NOW() or GO_AFTER()      (inside EventDsp::tickFsm)" << endl << endl << endl;
        usleep(100);
        assert(0);
    } else {
        // default
        radio_state_pending = radio_state;
        evtimer_add(localfsm_next_timer, &tv);
        // cout << "using default next in tiskFsm" << endl;
    }

    
    tickleOutsiders();

}
#pragma GCC diagnostic pop


void RadioEstimate::clearRecentEvent() {
    most_recent_event.d0 = NOOP_EV;
}

