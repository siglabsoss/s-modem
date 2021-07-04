#include "EventDsp.hpp"
#include <future>

using namespace std;
#include "vector_helpers.hpp"
#include "CustomEventTypes.hpp"
#include "common/convert.hpp"
#include "NullBufferSplit.hpp"
#include "DispatchMockRpc.hpp"

#include "driver/AttachedEstimate.hpp"
#include "driver/RetransmitBuffer.hpp"
#include "driver/RadioEstimate.hpp"
#include "driver/TxSchedule.hpp"
#include "driver/RadioDemodTDMA.hpp"
#include "HiggsDriverSharedInclude.hpp"
#include "driver/AirPacket.hpp"
#include "EVMThread.hpp"
#include "driver/VerifyHash.hpp"
#include <math.h>
#include <cmath>


#include <boost/functional/hash.hpp>

#include "duplex_schedule.h"

#include "FileUtils.hpp"

#include "HiggsSNR.hpp"


using namespace siglabs::file;



static void _handle_udp_callback(struct bufferevent *bev, void *_dsp)
{
    #ifdef DEBUG_PRINT_LOOPS
    static int calls = 0;
    calls++;
    if( calls % 100000 == 0) {
    cout << "_handle_udp_callback()" << endl;
    }
    #endif

    EventDsp *dsp = (EventDsp*) _dsp;

    struct evbuffer *input = bufferevent_get_input(bev);
    dsp->parseFeedbackBuffer(input);
}

static void event_callback(bufferevent* d, short kind, void* v) {
    cout << "event_ca()" << endl;
}

static void _handle_all_sc_event(bufferevent* d, short kind, void* v) {
    cout << "_handle_all_sc_event" << endl;
}


static void _handle_all_sc_callback(struct bufferevent *bev, void *_dsp) {
    EventDsp *dsp = (EventDsp*) _dsp;


    auto futureCallback = std::async(
        std::launch::async, 
        [](EventDsp* _this, struct bufferevent *_bev) {
            _this->handle_all_sc_callback(_bev);
        }, dsp, bev
    );
}

// static void _handle_coarse_callback(struct bufferevent *bev, void *_dsp) {
//     EventDsp *dsp = (EventDsp*) _dsp;
//     dsp->handleCoarseCallback(bev);
// }

static void _handle_coarse_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_coarse_event" << endl;
}


static void _handle_gnuradio_callback_udp_mode(struct bufferevent *bev, void *_dsp) {
    EventDsp *dsp = (EventDsp*) _dsp;
// 
    struct evbuffer *buf  = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(buf);

    if( len < GNURADIO_UDP_SIZE ) {
        cout << "_handle_gnuradio_callback_udp_mode too small" << endl;
        return;
    }

    unsigned char* temp_read = evbuffer_pullup(buf, GNURADIO_UDP_SIZE);

    double evm = dsp->soapy->evmThread->evm;
        // cout << "evm " << evm << "\n";


    static unsigned drop_sample_cnt = 0;
    static bool overwrite_iir = false;
    static int counter = 0;
    if( counter > 50*5 ) {
        counter = 0;
        overwrite_iir ^= 1;
    }
    counter++;

    if(!dsp->grc_shows_eq_filter) {
        overwrite_iir = false;
    }

    for(auto i = 0; i < GNURADIO_UDP_PACKET_COUNT; i++) {

        unsigned char* this_packet = temp_read + ((GNURADIO_UDP_SC_WIDTH*4)*i);
    
        uint32_t* asuint = (uint32_t*)this_packet;
        int16_t aff = (evm * (double)0x7fff)  / (double)40;
        // cout << aff;
        asuint[61] = (aff & 0xffff) << 16;

        // this will show what r0's eq is estimating for the channels
        if( overwrite_iir ) {
            for(int j = 0; j < 61; j++) {
                asuint[j] = dsp->radios[0]->eq_iir[j+944];
            }
        }

        unsigned char* actual_data = this_packet;
        unsigned actual_length = (GNURADIO_UDP_SC_WIDTH*4);
        bool erase = false;

        auto &buf2 = dsp->grc_single_sc_buffer;

        if( dsp->gnuradio_stream_single_sc ) {
            actual_data += (dsp->gnuradio_stream_single_offset*4);
            actual_length = 4;
            if( buf2.size() < 1472 ) {
                buf2.push_back(actual_data[0]);
                buf2.push_back(actual_data[1]);
                buf2.push_back(actual_data[2]);
                buf2.push_back(actual_data[3]);
                actual_data = 0;
                actual_length = 0;
            } else {
                erase = true;
                actual_data = buf2.data();
                actual_length = buf2.size();
            }
        }

        // gnuradio_stream_single_offset

        // const uint32_t* read_from = (uint32_t*)temp_read;
        if(drop_sample_cnt >= dsp->downsample_gnuradio && actual_data) {
            sendto(
                dsp->soapy->grc_udp_sock_fd,
                actual_data,
                actual_length,
                0, 
                (struct sockaddr *)&(dsp->soapy->grc_udp_address),
                sizeof(dsp->soapy->grc_udp_address)
                );
            drop_sample_cnt = 0;

            if( erase ) {
                buf2.resize(0);
                buf2.reserve(1472);
            }

        }
        drop_sample_cnt++;
    }

    evbuffer_drain(buf, GNURADIO_UDP_SIZE);
}

static void _handle_gnuradio_callback_udp_mode2(struct bufferevent *bev, void *_dsp) {
    cout << "_handle_gnuradio_callback_udp_mode()" << endl;
    return;
    EventDsp *dsp = (EventDsp*) _dsp;
// 
    struct evbuffer *buf  = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(buf);

    if( len < GNURADIO_UDP_SIZE ) {
        cout << "_handle_gnuradio_callback_udp_mode too small" << endl;
        return;
    }

    static unsigned drop_sample_cnt = 0;

    auto futureCallback = std::async(
        std::launch::async, 
        [dsp](EventDsp* _this, struct bufferevent *_bev, struct evbuffer *_buf) {

            unsigned char* temp_read = evbuffer_pullup(_buf, GNURADIO_UDP_SIZE);

            // const uint32_t* read_from = (uint32_t*)temp_read;
            if(drop_sample_cnt >= dsp->downsample_gnuradio) {
                sendto(
                    _this->soapy->grc_udp_sock_fd, 
                    temp_read, 
                    GNURADIO_UDP_SIZE, 
                    0, 
                    (struct sockaddr *)&(_this->soapy->grc_udp_address),
                    sizeof(_this->soapy->grc_udp_address)
                    );
                drop_sample_cnt = 0;
            }
            drop_sample_cnt++;

            evbuffer_drain(_buf, GNURADIO_UDP_SIZE);


        }, dsp, bev, buf
    );
}



static void _handle_gnuradio_callback_udp_mode_2(struct bufferevent *bev, void *_dsp) {
    EventDsp *dsp = (EventDsp*) _dsp;
// 
    struct evbuffer *buf  = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(buf);

    if( len < GNURADIO_UDP_SIZE ) {
        cout << "_handle_gnuradio_callback_udp_mode too small" << endl;
        return;
    }

    // cout << "_handle_gnuradio_callback_udp_mode_2" << endl;

    unsigned char* temp_read = evbuffer_pullup(buf, GNURADIO_UDP_SIZE);

    // for(auto i = 0; i < GNURADIO_UDP_PACKET_COUNT; i++) {
    //     cout << temp_read[i] << endl;
    // }

    static unsigned drop_sample_cnt = 0;


    if(drop_sample_cnt >= dsp->downsample_gnuradio) {

        for(auto i = 0; i < GNURADIO_UDP_PACKET_COUNT; i++) {

            unsigned char* this_packet = temp_read + ((GNURADIO_UDP_SC_WIDTH*4)*i);

            // const uint32_t* read_from = (uint32_t*)temp_read;
            sendto(
                dsp->soapy->grc_udp_sock_fd_2,
                this_packet,
                (GNURADIO_UDP_SC_WIDTH*4),
                0, 
                (struct sockaddr *)&(dsp->soapy->grc_udp_address_2),
                sizeof(dsp->soapy->grc_udp_address_2)
                );
        }

        drop_sample_cnt = 0;
    }
    drop_sample_cnt++;

    evbuffer_drain(buf, GNURADIO_UDP_SIZE);
}




static void _handle_gnuradio_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_gnuradio_event" << endl;
}

static void _handle_gnuradio_event_2(bufferevent* d, short kind, void* v) {
     cout << "_handle_gnuradio_event_2" << endl;
}




static void _handle_custom_event_callback(struct bufferevent *bev, void *_dsp) {
    EventDsp *dsp = (EventDsp*) _dsp;
    dsp->handleCustomEventCallback(bev);
}

static void _handle_custom_event_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_custom_event_event" << endl;
}






static void _handle_demod_callback(struct bufferevent *bev, void *_dsp) {
    EventDsp *dsp = (EventDsp*) _dsp;
    dsp->handleDemodCallback(bev);
}

static void _handle_demod_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_demod_event" << endl;
}




static void _handle_fb_pc_callback(struct bufferevent *bev, void *_dsp) {
     // cout << "_handle_fb_pc_callback" << endl;
    EventDsp *dsp = (EventDsp*) _dsp;
    dsp->handleInboundFeedbackBusCallback(bev);
}

static void _handle_fb_pc_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_fb_pc_event" << endl;
}

static void _handle_rb_callback(struct bufferevent *bev, void *_dsp) {
     // cout << "_handle_fb_pc_callback" << endl;
    EventDsp *dsp = (EventDsp*) _dsp;

    struct evbuffer *buf  = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(buf);

    // const size_t largest_read = RINGBUS_DISPATCH_BATCH_LIMIT*4;

    // size_t this_read = min(len, largest_read);
    size_t this_read = len;
    this_read = (this_read / 4) * 4; // make sure a multiple of 4
    auto read_words = this_read / 4;

    unsigned char* temp_read = evbuffer_pullup(buf, this_read);

    // size_t runs = 0;
    // bool more_work_left = len > largest_read;

    uint32_t *word_p = reinterpret_cast<uint32_t*>(temp_read);

    for(unsigned int i = 0; i < read_words; i++) {
        dsp->dspDispatchAttachedRb(word_p[i]);
    }

    // do a tiny sleep to give other stuff a chance to run
    // if(more_work_left) {
    //     auto t = (struct timeval){ .tv_sec = 0, .tv_usec = 1 };
    //     evtimer_add(dsp->poll_rb_timer, &t);
    // } else {
    //     // schedule us again in the future
    //     auto t = (struct timeval){ .tv_sec = 0, .tv_usec = 1000 };
    //     evtimer_add(dsp->poll_rb_timer, &t);
    // }
    evbuffer_drain(buf, this_read);
}

static void _handle_rb_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_rb_event" << endl;
}





static void _handle_sliced_data_single_radio(RadioEstimate &r, const uint32_t *word_p, const size_t read_words) {

    // std::unique_lock<std::mutex> lock(r.sliced_words_mutex);

    constexpr bool force_inject = false;
    // FIXME static variables will not work for 2 radios
    static bool need_inject = true;
    static uint32_t inject_counter = 0x0000f000;

    if( force_inject ) {
        if( need_inject ) {

            auto v = file_read_hex("/home/x/Desktop/data/qam16/tail11.txt");

            size_t trim = 40*11150;
            v.erase(v.begin(),v.begin()+trim);
            size_t total_len = 40*600;
            v.resize(total_len);

            // for( auto w : v ) {
            //     cout << HEX32_STRING(w) << '\n';
            // }

            for(size_t i = 0; i < v.size(); i++) {
                if( i % SLICED_DATA_WORD_SIZE == 0 ) {
                    // this branch enters 1 times for every OFDM frame

                    // save the frame in index 0
                    r.sliced_words[0].push_back(inject_counter);
                    inject_counter++;
                    for(size_t j = 0; j < (SLICED_DATA_WORD_SIZE-1); j++) {
                        r.sliced_words[0].push_back(0);
                    }
                    // cout << "frame: " << word_p[i] << endl;
                } 

                r.sliced_words[1].push_back(v[i]);
            }

            // exit(0);
            need_inject = false;
            cout << "injected " << v.size() << endl;
        }
        return;
    }





    for(size_t i = 0; i < read_words; i++) {

        if( i % SLICED_DATA_BEV_WRITE_SIZE_WORDS == 0 ) {
            // this branch enters 1 times for every OFDM frame

            // save the frame in index 0
            r.sliced_words[0].push_back(word_p[i]);
            for(size_t j = 0; j < (SLICED_DATA_WORD_SIZE-1); j++) {
                r.sliced_words[0].push_back(0);
            }
            // cout << "frame: " << word_p[i] << endl;
        } else {
            // this branch enters 8 times for every OFDM frame
            // size_t j = (i % SLICED_DATA_BEV_WRITE_SIZE_WORDS) - 1;

            r.sliced_words[1].push_back(word_p[i]);

            // cout << "data: " << HEX32_STRING(word_p[i]) << " <- " << j << endl;
        }
    }
    assert(r.sliced_words[0].size() == r.sliced_words[1].size());

    // cout << r.sliced_words[0].size() << endl;
    // cout << r.sliced_words[1].size() << endl;

}



// static void _handle_sliced_data_callback(struct bufferevent *bev, void *_dsp) {
//      // cout << "_handle_fb_pc_callback" << endl;
//     EventDsp *dsp = (EventDsp*) _dsp;

//     struct evbuffer *buf  = bufferevent_get_input(bev);
//     size_t len = evbuffer_get_length(buf);

//     size_t this_read = len;
//     this_read = (this_read / (SLICED_DATA_BEV_WRITE_SIZE_WORDS*4)) * (SLICED_DATA_BEV_WRITE_SIZE_WORDS*4); // make sure a multiple of
//     auto read_words = this_read / 4;

//     uint32_t *word_p = reinterpret_cast<uint32_t*>(evbuffer_pullup(buf, this_read));

//     // cout << "this_read " << this_read << " read_words " << read_words << endl;

//     // auto who_gets_samples = whoGetsWhatData("sliced_data", frame);

//     // for now, fixed to r[0]
//     _handle_sliced_data_single_radio(*(dsp->radios[0]), word_p, read_words);


//     evbuffer_drain(buf, this_read);

// }

// static void _handle_sliced_data_event(bufferevent* d, short kind, void* v) {
//      cout << "_handle_sliced_data_event" << endl;
// }




// example callback I found online, I haven't seen any errors here
static void event_dsp_libevent_log_callback(int severity, const char *msg)
{
    cout << "event_dsp_libevent_log_callback severity " << severity << endl;
    cout << msg << endl;
}




///////////////////////////////////////////////////////////////
//
//  Above this line are static functions which comprise events for this class
//

EventDsp::EventDsp(HiggsDriver* s):
    
    //
    // class wide initilizers
    //
     HiggsEvent(s)
    ,status(s->status)
    ,verifyHashArb(new VerifyHash())
    ,retransmitArb(new RetransmitBuffer())
    ,_rx_should_insert_sfo_cfo(false)
    ,should_inject_r0(false)
    ,should_inject_r1(false)
    ,should_inject_attached(false)
    ,fb_jamming(-1)
    ,dump_fine_sync(false) // cfo and sfo
    // ,estimate_filter_mode(0)
    ,valid_count_default_stream(0)
    ,valid_count_all_sc_stream(0)
    ,valid_count_fine_sync(0)
    ,valid_count_default_stream_p(0)
    ,valid_count_all_sc_stream_p(0)
    ,valid_count_fine_sync_p(0)

    //
    // function starts:
    //
{
    settings = (soapy->settings);

    updateSettingsFlags(); // settings must be set for this to work

    role = GET_RADIO_ROLE();

    // setup libevent logging
    event_set_log_callback(event_dsp_libevent_log_callback);

    specialSettingsByRole(role); // MUST call before setupBuffers


    while(soapy->higgsRunBoostIO && !_thread_should_terminate) {
        if( soapy->boostThreadReady() ) {
            break;
        }
        usleep(100);
    }


    dashTieSetup();

    setupBuffers();

    if( role != "arb" ) {
        setupRadioVector();
    }

    setupAttachedRadio();

    if( role == "rx") {
        should_inject_r0 = true;
        should_inject_r1 = true;
    }

    if( role == "tx") {
        should_inject_attached = true;
    }

    if( init_fsm ) {
        setupFsm();
    }

    // setup fb bus parsing stuff
    if( init_feedback_bus ) {
        setupFeedbackBus();
    }

    cout << "After SetupBuffers" << endl;

    // lazy way to wrap, maybe we can skip this
    // std::function<void(std::string)> cb = [this](std::string parsed) {
    //     // std::cout << "cb len " <<  parsed.size() << ": " << parsed <<  std::endl;
    //     this->gotCompleteMessageFromMock(parsed);
    // };

    dispatchRpc = new DispatchMockRpc(settings, soapy);

    std::function<void(std::string&)> cb2 =
        std::bind(&EventDsp::gotCompleteMessageFromMock, this, std::placeholders::_1);

    // changed this to not be a reference because we are passing a lambda that only lives
    // on the stack
    splitRpc = new NullBufferSplit(cb2);


    most_recent_event.d0 = NOOP_EV;

    future_cfo_0_running = false;
    future_residue_0_running = false;
    future_residue_1_running = false;

    // technically these GET_ sfo/cfo are used if the HiggsFineSync
    // thread is awake, but we can assume that checking for arb mode is close enough
    if( role != "arb" ) {
        _cfo_symbol_num = GET_CFO_SYMBOL_NUM();
        _sfo_symbol_num = GET_SFO_SYMBOL_NUM();
        _enable_file_dump_sliced_data = GET_FILE_DUMP_SLICED_DATA_ENABLED();
        _debug_loopback_fine_sync_counter = GET_DEBUG_LOOPBACK_FINE_SYNC_COUNTER();
    }

    setupRepeat();

    booted = _track_counter_timepoint = handle_sfo_cfo_timepoint = std::chrono::steady_clock::now();

    soapy->_dsp_thread_ready = true;

}

// void EventDsp::enableDemodScIndex(const std::vector<unsigned> &in) {
//     if(in.size() >= demod_buf_enabled_sc.size()) {
//         cout << "enableDemodScIndex() called with illegally large input of length: " << in.size() << "\n";
//         return;
//     }
//     for(unsigned i = 0; i < in.size(); i++) {
//         demod_buf_enabled_sc[i] = in[i];
//     }
//     demod_buf_enabled_sc_count = in.size();
// }

/**
 * Called by constructor and afterwards if needed
 * Copies from a settings to our local object to save the shared lock
 */

void EventDsp::updateSettingsFlags() {
    drain_gnurado_stream = GET_GNURADIO_UDP_DO_CONNECT();
    drain_gnurado_stream_2 = GET_GNURADIO_UDP_DO_CONNECT2();
    gnuradio_stream_single_sc = GET_GNURADIO_UDP_SEND_SINGLE();


    _dump_residue_upstream = GET_RESIDUE_UPSTREAM_DUMP_ENABLED();
    _dump_reside_upstream_fname = GET_RESIDUE_UPSTREAM_DUMP_FILENAME();

    downsample_gnuradio = GET_GNURADIO_UDP_DOWNSAMPLE();
}

// void EventDsp::handleCoarseCallback(struct bufferevent *bev) {

//     struct evbuffer *buf  = bufferevent_get_input(bev);

//     size_t len = evbuffer_get_length(buf);

//     const size_t largest_read = ( (len) / (2*4) ) * (2*4);

//     // linearize this part of the buffer and get it
//     unsigned char* temp_read = evbuffer_pullup(buf, largest_read);

//     const uint32_t* read_from = (uint32_t*)temp_read;

//     uint32_t word_len = largest_read / 4;
//     uint32_t struct_len = word_len / 2;
//     (void)struct_len;

//     auto who_gets_samples = whoGetsWhatData("coarse", read_from[0]);

//     // cout << who_gets_samples[0] << " coarse " << largest_read << " " << word_len << endl;


//     // for(int i = 0; i < word_len; i += 2) {
//     //     for(auto id : who_gets_samples) {
//     //         radios[id]->coarse_buf.enqueue(read_from[i+1]);
//     //     }
//     // }


//     uint32_t recent_frame;
//     recent_frame = read_from[word_len-2];
//     fuzzy_recent_frame = recent_frame;
//     // cout << "set recent frame" << endl;

//     evbuffer_drain(buf, largest_read);
// }


// this code assumes the watermark is set to sizeof(custom_event_t)
void EventDsp::handleCustomEventCallback(struct bufferevent *bev) {

    // cout << "_handle_custom_event_callback handleCustomEventCallback()" << endl;

    struct evbuffer *buf  = bufferevent_get_input(bev);

    size_t len = evbuffer_get_length(buf);

    constexpr size_t byte_sz = sizeof(custom_event_t);


    size_t willEatFrames = len / byte_sz;
    size_t this_read = willEatFrames*byte_sz;

    // cout << "handleCustomEventCallback will dispatch " << willEatFrames << " messages\n";

    unsigned char* temp_read = evbuffer_pullup(buf, this_read);

    size_t consumed = 0; // in bytes, not "frames"

    while((consumed+byte_sz) <= len) {
        auto read_at = temp_read+consumed; // pointer arithmetic to get to the one we want

        // dispatch to consumers
        custom_event_t *cep = (custom_event_t *)read_at;
        this->handleCustomEvent(cep);
        for(auto r : radios) {
            // call new() in a loop and use std vector for pointers
            r->handleCustomEvent(cep);
        }
        attached->handleCustomEvent(cep);

        // bump for next loop
        consumed += byte_sz;
    }


    evbuffer_drain(buf, this_read);

    // for()
    // radio[0]->handleCustomEvent();

    // in the future this can be tricky to only call back functions that request
    // it

}

void EventDsp::handleDemodCallback(struct bufferevent *bev) {
    struct evbuffer *buf  = bufferevent_get_input(bev);

    size_t len = evbuffer_get_length(buf);

    const size_t sz = DEMOD_BEV_SIZE_WORDS*4;

    if( len < sz ) {
        return;
    }


    // unsigned char* temp_read = evbuffer_pullup(buf, sz);

    // if(temp_read == NULL) {
    //     cout << "evbuffer_pullup NULL"  << endl;
    //     return;
    // }

    // const uint32_t* read_from = (uint32_t*)temp_read;

    // const uint32_t word_len = sz / 4;
    // // uint32_t struct_len = word_len / 2;

    // // index 0 is sequence
    // // auto who_gets_samples = whoGetsWhatData("demod", read_from[0]);



    // size_t radio_i = 0;
    // for(auto r : radios) {
    //     // call new() in a loop and use std vector for pointers
    //     // +1 is actually +4 bytes but because pointer arith
    //     r->drainBev(read_from[0], read_from+1, word_len-1);
    //     r->dspRunDemod();
    // }





    // outsideFillDemodBuffers(demod_buf);


    // size_t radio_i = 0;
    // for(auto r : radios) {
    //     // call new() in a loop and use std vector for pointers
    //     // +1 is actually +4 bytes but because pointer arith
    //     // r->drainBev(read_from[0], read_from+1, word_len-1);
    //     // r->drainDemodBuffers(demod_buf);
    //     r->dspRunDemod();
    // }







    evbuffer_drain(buf, len);
    // evbuffer_drain(buf, sz);

}
/// Run by the handle_radios_tick()
/// Takes values out of demod SafeQueue
/// FIXME not currently setup for attached radio
/// @param[in] drain_from: A SafeQueueTracked which has demod data.  This should be replaced with something better
void EventDsp::outsideFillDemodBuffers(SafeQueueTracked<uint32_t, uint32_t>(&drain_from) [DATA_TONE_NUM]) {



    if(false) {
        static int print_loop = 0;
        if( print_loop++ == 5000 ) {
            // cout << "here\n";
            print_loop = 0;
            cout << "=======================================  outsideFillDemodBuffers\n\n";
            for(uint32_t i = 0; i < DATA_TONE_NUM; i++) {
                cout << "i: " << i << ",  " << drain_from[i].size() << "\n";
            }
        }
    }

    std::vector<unsigned> enabled_sc;
    {
        std::unique_lock<std::mutex> lock(_demod_buf_enabled_mutex);
        enabled_sc = demod_buf_enabled_sc;
    }

    // if we need to drain for either radio, drain
    // when this is set, fill all radios for the same i value
    bool drained = false;
    // for used data tones
    for(auto i : enabled_sc) {

        // cout << "outsideFillDemodBuffers() " << i << " " << drain_from[i].size() << endl;

        drained = false;
        std::vector<std::pair<uint32_t, uint32_t>> chunk;
        // for every radio
        for(auto r : radios) {

            // list of all sc for this radio
            auto ii = r->demod->keep_demod_sc;
            bool active_subcarrier = false;
            // funky loop left over from when enabled_sc vector was not used and instead every
            // subcarrier was checked 
            for(unsigned k = 0; k < ii.size(); k++ ) {
                active_subcarrier |= (ii[k] == i);
            }
            if( active_subcarrier ) {
                // loop through and pull from the queue which feeds the data into this thread
                // this way we can control what's going on
                if(drain_from[i].size() >= SUBCARRIER_CHUNK && drained == false) {

                    // grab a chunk (which takes a lock)
                    chunk = drain_from[i].get(SUBCARRIER_CHUNK);
                    // save it by setting this flag
                    drained = true;

                    // cout << chunk[0].second << endl;
            
                    // append to a version only we touch
                }

                // always fill when flag is set
                if(drained) {
                    VEC_APPEND(r->demod->demod_buf_accumulate[i], chunk);
                }


            } else {
                // radio is not interested in this data
            } // if interested
        } // for radios
    } // for tones
} // outsideFill


void EventDsp::handleGnuradioCallback(struct bufferevent *bev) {

#ifdef DEBUG_PRINT_LOOPS
    static int calls = 0;
    calls++;
    if( calls % 10000 == 0) {
        cout << "handleGnuradioCallback" << endl;
    }
#endif

}


// looking at internal state, returns list of radio indices that get this data
// note this does not return by ID, maybe we can rewrite this to do that or a clone
std::vector<uint32_t> EventDsp::whoGetsWhatData(const std::string estimate, const uint32_t rx_frame) {
        return {0, 1};
    // if( estimate_filter_mode == 0) {
    //     return {0};
    // }

    // if( estimate_filter_mode == 1) {
    //     return {1};
    // }

    // if( estimate_filter_mode == -2) {
    //     return {0, 1};
    // }
}


// this actually runs on the HiggsFineSync thread
void EventDsp::handle_sfo_cfo_callback(struct bufferevent *bev) {
    // cout << "handle_sfo_callback" << endl;

    // when this function is running, we hold this mutex
    // the main purpose is to prevent contention with the dump_sfo_cfo_buffer function
    std::unique_lock<std::mutex> lock(_handle_sfo_cfo_callback_running_mutex);
    

    auto thena = std::chrono::steady_clock::now();

#ifdef PRINT_LIBEV_EVENT_TIMES
    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    thena - handle_sfo_cfo_timepoint
    ).count();

    cout << "handle_sfo_cfo_callback() elapsed_us " << elapsed_us << endl;
#endif

    handle_sfo_cfo_timepoint = thena;


    // user must not set callbacks on this returned object
//    size_t len = evbuffer_get_length(buf);
    // Note: application may only remove (not add) data on the input buffer, 
    // and may only add (not remove) data from the output buffer.
    //
    // In addition to above, we have a buffer event pair,
    // which means that on the ->out buffer we call get_input()
    //
    // http://www.wangafu.net/~nickm/libevent-book/Ref6_bufferevent.html
    //
    struct evbuffer *buf  = bufferevent_get_input(bev);

    // in bytes
    size_t len = evbuffer_get_length(buf);

    // in bytes, so the pack size etc need to be converted to bytes
    const size_t largest_read = ( (len) / (SFO_CFO_PACK_SIZE*4) ) * (SFO_CFO_PACK_SIZE*4);

    // linearize this part of the buffer and get it
    unsigned char* temp_read = evbuffer_pullup(buf, largest_read);

    const uint32_t* const read_from = (const uint32_t* const)temp_read;

    const uint32_t word_len = largest_read / 4;
    const uint32_t struct_len = word_len / SFO_CFO_PACK_SIZE;
    (void)struct_len;

    // at this point we have a buffer of memory, and we know how many loops we want to do
    // now we need to know who gets this 

    // cout << "handle_sfo_cfo_callback() word_len " << word_len << endl;


    // how to deal with this buffer overlaping multiple types of filtering events
    // hack
    // read_from[0] is the earliest frame counter we have
    // auto who_gets_samples = whoGetsWhatData("sfo_cfo", read_from[0]);

    // auto thenb = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point thenc;

    // this context will control the locks we need
    {
        // copy these atomic to the stack because we will use them more than once
        bool _should_inject_r0 = should_inject_r0;
        bool _should_inject_r1 = should_inject_r1;
        bool _should_inject_attached = should_inject_attached;

        // cout << (int)_should_inject_r0 << " " << (int)_should_inject_r1 << " " << (int)_should_inject_attached << "\n";

        
        // Lock ordering just means that you prevent deadlocks by obtaining locks in a fixed order, and do not obtain locks again after you start unlocking.
        // https://stackoverflow.com/questions/1951275/would-you-explain-lock-ordering
        //
        if( _should_inject_r0 ) {
            std::unique_lock<std::mutex> lock_0_s(radios[0]->sfo_buf.m);
        }
        if( _should_inject_r1 ) {
            std::unique_lock<std::mutex> lock_1_s(radios[1]->sfo_buf.m);
        }
        if( _should_inject_attached ) {
            std::unique_lock<std::mutex> lock_a_s( attached->sfo_buf.m);
        }

        if( _should_inject_r0 ) {
            std::unique_lock<std::mutex> lock_0_c(radios[0]->cfo_buf.m);
        }
        if( _should_inject_r1 ) {
            std::unique_lock<std::mutex> lock_1_c(radios[1]->cfo_buf.m);
        }
        if( _should_inject_attached ) {
            std::unique_lock<std::mutex> lock_a_c( attached->cfo_buf.m);
        }

        thenc = std::chrono::steady_clock::now();



        // in this loop, i goes up by 5 per loop
        for(uint32_t i = 0; i < word_len; i+= SFO_CFO_PACK_SIZE) {
            // fixme this is based on mod 4 + 0 and mod 4 + 2

            if( _should_inject_r0 ) {
                radios[0]->sfo_buf.unsafeEnqueue(read_from[i+1]);
            }
            if( _should_inject_r1 ) {
                radios[1]->sfo_buf.unsafeEnqueue(read_from[i+3]);
            }
            if( _should_inject_attached ) {
                attached ->sfo_buf.unsafeEnqueue(read_from[i+1]);
            }
            
            // radios[0]->cfo_buf_accumulate.push_back(read_from[i+2]);
            // radios[1]->cfo_buf_accumulate.push_back(read_from[i+4]);
            if( _should_inject_r0 ) {
                radios[0]->cfo_buf.unsafeEnqueue(read_from[i+2]);
            }
            if( _should_inject_r1 ) {
                radios[1]->cfo_buf.unsafeEnqueue(read_from[i+4]);
            }
            if( _should_inject_attached ) {
                attached ->cfo_buf.unsafeEnqueue(read_from[i+2]);
            }

            ////////


            counter = read_from[i+0];

            next_counter = counter + 1;

            temp_data = read_from[i+3];

            // if(counter%128 == 42){
            //     const uint32_t temp_benchmark = temp_data;
            //     const int16_t temp_benchmark_r = (temp_benchmark&0xffff);
            //     const int16_t temp_benchmark_i = ((temp_benchmark>>16)&0xffff);

            //     temp_qpsk_scale = sqrt((temp_benchmark_r*1.0)*(temp_benchmark_r*1.0) + (temp_benchmark_i*1.0)*(temp_benchmark_i*1.0))/(sqrt(2));

            //     temp_snr = 0;
            // }else if((counter%128 > 42) && (counter%128 < 128)){

            //     const int16_t temp_data_r = (temp_data&0xffff);
            //     const int16_t temp_data_i = ((temp_data>>16)&0xffff);

            //     const double noise_r = fabs(temp_data_r*1.0) - temp_qpsk_scale;
            //     const double noise_i = fabs(temp_data_i*1.0) - temp_qpsk_scale;

            //     temp_snr = 2*temp_qpsk_scale*temp_qpsk_scale/(noise_r*noise_r+noise_i*noise_i);


            //     if((temp_snr>0.0) && (temp_snr< 10000000)){
            //         filtered_snr = filtered_snr + (temp_snr - filtered_snr) * 0.9;
            //     }
                

            // }else{
            //     temp_snr = 0;
            // }

            // /////////////////////cout<<"             "<<HEX32_STRING(read_from[i+0])<<"             "<<HEX32_STRING(read_from[i+3])<<"             "<<temp_snr<<"             "<<filtered_snr<<endl;
            

      

            // if((counter%DUPLEX_FRAMES != DUPLEX_B1) && (snr_first_time_flag)) {
            //     write_to_evbuff = false;
            // } else {
            //     snr_first_time_flag = false;
            // }

            //cout<<"counter is:                  "<<counter%DUPLEX_FRAMES<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;

            if(counter%DUPLEX_FRAMES == DUPLEX_B1){
                write_to_buffer_flag = true;
            }

            if(write_to_buffer_flag) {

                if((counter%DUPLEX_FRAMES)==DUPLEX_B1){
                    if(write_word_counter == 0){
                        bufferevent_write(soapy->higgsSNR->snr_buf->in, &temp_data, sizeof(temp_data));

                        //cout<<"start:               "<<HEX32_STRING(temp_data)<<endl;

                        write_word_counter = 1;
                    }else{

                        if(write_word_counter < SNR_PACK_SIZE){
                            while(write_word_counter < SNR_PACK_SIZE){
                                uint32_t insert_data = 0;
                                bufferevent_write(soapy->higgsSNR->snr_buf->in, &insert_data, sizeof(insert_data));
                                write_word_counter++;
                            }

                            bufferevent_write(soapy->higgsSNR->snr_buf->in, &temp_data, sizeof(temp_data));
  
                            //cout<<"start:               "<<HEX32_STRING(temp_data)<<endl;

                            write_word_counter = 1;
                        }

                    }
                    
                }else if((counter%DUPLEX_FRAMES>DUPLEX_B1)&&(counter%DUPLEX_FRAMES<DUPLEX_FRAMES-1)){
                    bufferevent_write(soapy->higgsSNR->snr_buf->in, &temp_data, sizeof(temp_data));
                    write_word_counter++;
                    if(write_word_counter == SNR_PACK_SIZE){
                        write_word_counter = 0;
                        write_to_buffer_flag = false;
                    }

                }else if((counter%DUPLEX_FRAMES == (DUPLEX_FRAMES-1))){
                    bufferevent_write(soapy->higgsSNR->snr_buf->in, &temp_data, sizeof(temp_data));
                    write_word_counter++;
                    if(write_word_counter < SNR_PACK_SIZE){
                        while(write_word_counter < SNR_PACK_SIZE){
                            bufferevent_write(soapy->higgsSNR->snr_buf->in, &temp_data, sizeof(temp_data));
                            write_word_counter++;
                        }


                    }

                    //cout<<"finish:             "<<write_word_counter<<endl;

                    write_word_counter = 0;
                    write_to_buffer_flag = false;

                }


            }
            // radios[0]->rolling_data_queue.enqueue(std::make_pair(read_from[i+5], read_from[i+6]));

            // radios[0]->sfo_buf.enqueue(read_from[i+2]);
            // radios[1]->sfo _buf.enqueue(read_from[i+4]);


            if(i + SFO_CFO_PACK_SIZE >= word_len) {
                fuzzy_recent_frame = read_from[i+0];
            }
        }
    }

    // auto thend = std::chrono::steady_clock::now();

    //cout << "handle_sfo_cfo_callback() s/c buf.size() " << radios[0]->sfo_buf.size() << ", " << radios[0]->cfo_buf.size() << endl;

    //////////////////////////////////////////////////////////
    ////////////original try
    // if(radios[0]->sfo_buf.size() >= _sfo_symbol_num) {
        
    //     radios[0]->dspRunSfo_v1();
    // }

    // if either are full, go
    if(
        radios[0]->sfo_buf.size() >= _sfo_symbol_num || 
        radios[1]->sfo_buf.size() >= _sfo_symbol_num ||
        attached ->sfo_buf.size() >= _sfo_symbol_num
        )  {
        
            struct timeval tt = { .tv_sec = 0, .tv_usec = 1000 };
            evtimer_add(soapy->higgsFineSync->process_sfo_timer, &tt);

        //radios[1]->dspRunSfo_v1();
        }


    // trigger in 1 ms, if not already pending
    auto is_pending = event_pending( soapy->higgsFineSync->process_cfo_timer, EV_TIMEOUT, NULL );
    if( !is_pending ) {
        struct timeval tt = { .tv_sec = 0, .tv_usec = 100 };
        evtimer_add(soapy->higgsFineSync->process_cfo_timer, &tt);
    }

    evbuffer_drain(buf, largest_read);

#ifdef PRINT_LIBEV_EVENT_TIMES
    auto thene = std::chrono::steady_clock::now();
    
    size_t rta = std::chrono::duration_cast<std::chrono::microseconds>( 
    thenb - thena
    ).count();

    size_t rtb = std::chrono::duration_cast<std::chrono::microseconds>( 
    thenc - thenb
    ).count();

    size_t rtc = std::chrono::duration_cast<std::chrono::microseconds>( 
    thend - thenc
    ).count();

    size_t rtd = std::chrono::duration_cast<std::chrono::microseconds>( 
    thene - thend
    ).count();


    cout << "handle_sfo_cfo_callback() runtime_us " << rta
    << ", " << rtb
    << ", " << rtc
    << ", " << rtd
     << endl;
#endif

}

void EventDsp::handleCfoTimer() {
    auto cfoFuture = std::async(
        std::launch::async, 
        [](EventDsp* _this){ _this->handleCfoTimer2(); }, this);
}

void EventDsp::handleCfoTimer2() {

    auto r0_cfo_size = radios[0]->cfo_buf.size();
    auto r1_cfo_size = radios[1]->cfo_buf.size();
    auto a_cfo_size  = attached ->cfo_buf.size();

    // FIXME: can be static or something
    const auto residue_chunk = _cfo_symbol_num/GET_CFO_SYMBOL_PULL_RATIO();
    const auto slow_chunk = 2*residue_chunk;
    bool slowing_down = false;

    if( r0_cfo_size > slow_chunk || 
        r1_cfo_size > slow_chunk ||
        a_cfo_size  > slow_chunk
        ) {
            cout << "Skipping Residue for old data " << residue_chunk << " / " << r0_cfo_size << endl;
            slowing_down = true;
    }

    // FIXME improve performance with
    // https://stackoverflow.com/questions/17010005/how-to-use-c11-move-semantics-to-append-vector-contents-to-another-vector
    if(r0_cfo_size >= residue_chunk) {

        std::vector<uint32_t> cfo_buf_chunk = radios[0]->cfo_buf.get(residue_chunk);

        if( ! slowing_down ) {
            radios[0]->dspRunResiduePhase(cfo_buf_chunk);
        }

        VEC_APPEND(radios[0]->cfo_buf_accumulate, cfo_buf_chunk);
    }

    if(r1_cfo_size >= residue_chunk) {

        std::vector<uint32_t> cfo_buf_chunk = radios[1]->cfo_buf.get(residue_chunk);

        if( ! slowing_down ) {
            radios[1]->dspRunResiduePhase(cfo_buf_chunk);
        }

        VEC_APPEND(radios[1]->cfo_buf_accumulate, cfo_buf_chunk);
    }

    if(a_cfo_size >= residue_chunk) {

        std::vector<uint32_t> cfo_buf_chunk = attached->cfo_buf.get(residue_chunk);

        if( ! slowing_down ) {
            attached->dspRunResiduePhase(cfo_buf_chunk);
        }

        VEC_APPEND(attached->cfo_buf_accumulate, cfo_buf_chunk);
    }

    ///////////////////////////////////////////////////////////////////////////

    if(radios[0]->cfo_buf_accumulate.size() >= _cfo_symbol_num) {
        auto futWork = std::async(
            std::launch::async, 
            [](RadioEstimate* _re) {
                // work
                _re->dspRunCfo_v1(_re->cfo_buf_accumulate);

                // erase
                _re->cfo_buf_accumulate.erase(
                    _re->cfo_buf_accumulate.begin(),
                    _re->cfo_buf_accumulate.end()
                );
            }, radios[0]
        );

        // cout << "cfo_buf_accumulate.size()" << radios[rid]->cfo_buf_accumulate.size() << endl;
        // radios[0]->dspRunCfo_v1(radios[0]->cfo_buf_accumulate);
        // radios[0]->cfo_buf_accumulate.erase(
        //     radios[0]->cfo_buf_accumulate.begin(),
        //     radios[0]->cfo_buf_accumulate.end());
     
        // return; // This allows the next one to process the next time.. this might leve the radios staggerded but whatever
    }

    if(radios[1]->cfo_buf_accumulate.size() >= _cfo_symbol_num) {
        auto futWork = std::async(
            std::launch::async, 
            [](RadioEstimate* _re) {
                // work
                _re->dspRunCfo_v1(_re->cfo_buf_accumulate);

                // erase
                _re->cfo_buf_accumulate.erase(
                    _re->cfo_buf_accumulate.begin(),
                    _re->cfo_buf_accumulate.end()
                );
            }, radios[1]
        );
        // cout << "cfo_buf_accumulate.size()" << radios[rid]->cfo_buf_accumulate.size() << endl;
        // radios[1]->dspRunCfo_v1(radios[1]->cfo_buf_accumulate);
        // radios[1]->cfo_buf_accumulate.erase(
        //     radios[1]->cfo_buf_accumulate.begin(),
        //     radios[1]->cfo_buf_accumulate.end());
       
    }

    if(attached->cfo_buf_accumulate.size() >= _cfo_symbol_num) {
        auto futWork = std::async(
            std::launch::async, 
            [](RadioEstimate* _re) {
                // work
                _re->dspRunCfo_v1(_re->cfo_buf_accumulate);

                // erase
                _re->cfo_buf_accumulate.erase(
                    _re->cfo_buf_accumulate.begin(),
                    _re->cfo_buf_accumulate.end()
                );
            }, attached
        );
        // cout << "cfo_buf_accumulate.size()" << radios[rid]->cfo_buf_accumulate.size() << endl;
        // radios[1]->dspRunCfo_v1(radios[1]->cfo_buf_accumulate);
        // radios[1]->cfo_buf_accumulate.erase(
        //     radios[1]->cfo_buf_accumulate.begin(),
        //     radios[1]->cfo_buf_accumulate.end());
       
    }

}



/// FIXME not setup for attached
void EventDsp::handle_all_sc_callback(struct bufferevent *bev) {
   


   struct evbuffer *buf  = bufferevent_get_input(bev);

    // uint32_t temp[ALL_SC_BUFFER_SIZE];

    std::vector<uint32_t> dataVec;

    dataVec.resize(ALL_SC_BUFFER_SIZE);

    unsigned char* writeto = (unsigned char*)&dataVec[0];

    // copy to a buffer that is not libevent controlled
    evbuffer_remove(buf, writeto, ALL_SC_BUFFER_SIZE*4);
    // cout << HEX32_STRING(temp[0]) << " - scidx " << HEX32_STRING(temp[1]) << "(" << ")" << endl;

    // cout << "size " << dataVec.size() << endl;
    // for( auto a: dataVec) {
    //     cout << a << ", " << endl;
    // }
    // cout << endl;

    std::vector<uint32_t> ccopy(dataVec.begin()+1, dataVec.end());


    // update this so fsm can have an idea when now is
    fuzzy_recent_frame = dataVec[0];

    /////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////
    // struct evbuffer *buf  = bufferevent_get_input(bev);

    // // in bytes
    // size_t len = evbuffer_get_length(buf);

    // cout<<len<<endl;

    // // in bytes, so the pack size etc need to be converted to bytes
    // const size_t largest_read = (1024)*4;

    // // linearize this part of the buffer and get it
    // unsigned char* temp_read = evbuffer_pullup(buf, largest_read);

    // const uint32_t* read_from = (uint32_t*)temp_read;

    // cout<<"eeeeeeeeeeeeeeeeeeeeeeee"<<endl; 

    // std::vector<uint32_t> ccopy;

    //  // ccopy.resize(1024);
    //  for(int index=0;index<1024;index++)
    //  {
    //     //ccopy[index]=read_from[index];
    //    cout<<index<<"             "<<read_from[index]<<endl;
    //  }

     
    /////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////

    // dataVec[0] is the only frame coutner we have
    

    auto who_gets_samples = whoGetsWhatData("all_sc_buf", dataVec[0]);
     // std::vector<uint32_t> who_gets_samples={0,1};

     //////////////////////////////////////////////

    for(auto id : who_gets_samples) {
        radios[id]->all_sc = ccopy;
        radios[id]->dspRunChannelEst();
    }

}




///
/// pulls from HiggsZmq.cpp where bev is written
/// FIXME: there is NO checking if we ever get out of sync.
/// if that happens there is NO recovery mechanism.
///
/// When I first wrote this, I assumed that if we left samples in the buffer that are
/// higher than the watermark, we would be called again, that is not true
/// if there is data over the watermark, we only are guarenteed 1 callback, with a posiblity of getting up to 4
/// callbacks.  in the case where we get 1, the length will be there
/// it would f
void EventDsp::handleInboundFeedbackBusCallback(struct bufferevent *bev) {


    struct evbuffer *buf  = bufferevent_get_input(bev);


    const size_t length_read_bytes = 4;


    // in bytes
    size_t len = evbuffer_get_length(buf);

    // cout << "handleInboundFeedbackBusCallback()" << " bev len " << len << endl;

    if(len < 8) {
        // cout << "handleInboundFeedbackBusCallback() exiting early" << endl;
        return;
    }

    // first pullup is small
    unsigned char* temp_read_0 = evbuffer_pullup(buf, length_read_bytes);

    // cast pointer
    uint32_t *word_p = (uint32_t*)temp_read_0;

    // read one 32 bit word, this is how many bytes to read next
    uint32_t byte_count = *word_p;

    // cout << "handleInboundFeedbackBusCallback() bev byte_count " << byte_count << endl;

    if( len < byte_count ) {
        // cout << "handleInboundFeedbackBusCallback() exiting early 2" << endl;
        return;
    }

    uint32_t bytes_consumed = 4 + byte_count;

    // cout << "handleInboundFeedbackBusCallback() bytes_consumed " << bytes_consumed << endl;

    // NOT aligned
    unsigned char* temp_read_1 = evbuffer_pullup(buf, bytes_consumed);


    ///
    /// now for the words
    ///

    // aligned with vec
    uint32_t* word_ptr = (uint32_t*)temp_read_1;
    word_ptr++;  // bump to align

    size_t word_length = bytes_consumed / 4;
    word_length--;

    ///
    ///  put to a fn
    ///

    // print_feedback_generic(   (feedback_frame_t *) word_ptr  );

    handleParsedFrameForPc(word_ptr, word_length);


    // drain the +4 value because we need to align up with the next 4 byte length value
    evbuffer_drain(buf, bytes_consumed);

    size_t calculate_remaining = len - (size_t)bytes_consumed;
    if( calculate_remaining >= 8 ){
        // cout << "Recursing with calculated " << calculate_remaining << " bytes left" << endl;
        // FIXME, rewrite as a loop
        // RECURSION!! oh the fun we will have
        handleInboundFeedbackBusCallback(bev);
    }
}

/// A peer (tx or rx) has sent a vector type with the PC flag set
/// this will be called when the message arrives
void EventDsp::handlePcPcVectorType(const uint32_t* const full_frame, const size_t length_words) {
    const feedback_frame_vector_t* const vec = (const feedback_frame_vector_t* const)full_frame;
    // this is not the same as the one above
    const uint32_t sequence_number = vec->seq;
    (void)sequence_number;
    const uint32_t d_word_len = (length_words-FEEDBACK_HEADER_WORDS);
    const uint32_t* d_start = (const uint32_t*)(full_frame+FEEDBACK_HEADER_WORDS);

    // it's possible data_vec will not be needed below, but we create it always for DRY
    std::vector<uint32_t>  data_vec;
    data_vec.assign(d_start, d_start + d_word_len); // this uses 2 pointers to make it as an iterator, 2nd argument is pointer arithmatic from first

    bool do_peer_print = GET_PRINT_PEER_RB();

    bool smodem_consumed = true;

    // cout << "handlePcPcVectorType(" << vec->vtype << ")\n";

    switch(vec->vtype) {
        case FEEDBACK_VEC_TRANSPORT_RB: {
                // cout << "phew got a FEEDBACK_VEC_TRANSPORT_RB length " << length_words << "  llen " << d_word_len << endl;
                // cout << "d_start " << d_start[0] << " " << d_start[1] << endl;

                auto payload_words = length_words - FEEDBACK_HEADER_WORDS;

                auto msg_peer_id = d_start[0];
                auto num_rbs =  payload_words - 1;

                // cout << "num_rbs " << num_rbs << endl;

                bool found = false;
                RadioEstimate *saved = 0;

                // these two lines create a new temporary vector for the loop below
                std::vector<RadioEstimate*> _all_radios = radios;
                _all_radios.push_back(attached);

                for(auto r : _all_radios) {
                    // cout << r->array_index << endl;
                    if(r->peer_id == msg_peer_id) {
                        found = true;
                        saved = r;
                    }
                }
                if( !found ) {
                    cout << "handlePcPcVectorType() received a forwarded rb with peer " << msg_peer_id << " which we do not have a RadioEstimate() for" << endl;
                    break;
                }

                for(uint32_t i = 0; i < num_rbs; i++) {
                    auto rb_word = d_start[i+1];
                    if(do_peer_print) {
                        cout << "Ringbus R" << saved->array_index << ": 0x" << HEX32_STRING(rb_word) << endl;
                    }
                    saved->remote_rb_buffer.push_back(rb_word);
                }

            }


            break;

        case FEEDBACK_VEC_DELIVER_FSM_EVENT: {
                cout << "FEEDBACK_VEC_DELIVER_FSM_EVENT()" << endl;
                // we just "send" the event as if it came internally, but it came from our peer
                auto ev = vector_to_custom_event(data_vec);
                sendEvent(ev);
            }
            break;

        case FEEDBACK_VEC_UPLINK: {
                soapy->demodRx2TunFifo.enqueue(data_vec);

                // boost::hash<std::vector<uint32_t>> word_hasher;
                // cout << "handlePcPcVectorType() got word hash " << HEX32_STRING(word_hasher(data_vec)) << endl;
        }
        break;

        case FEEDBACK_VEC_PACKET_HASH: {
            soapy->txSchedule->verifyHashTx->gotHashListFromPeer(data_vec);
        }
        break;

        case FEEDBACK_VEC_REQ_RETRANSMIT: {
            if( d_word_len > 1 ) {
                uint8_t seq = data_vec[0];
                data_vec.erase(data_vec.begin());
                if( role == "arb" ) {
                    // cout << "arb got retransmits\n";
                    // for(auto w : data_vec ) {
                    //     cout << HEX32_STRING(w) << ", ";
                    // }
                    // cout << "\n";
                    retransmitArb->scheduleRetransmit(seq, data_vec);
                } else {
                    soapy->txSchedule->retransmitBuffer->scheduleRetransmit(seq, data_vec);
                }

            } else {
                cout << "FEEDBACK_VEC_REQ_RETRANSMIT got message too short" << endl;
            }
        }
        break;

        case FEEDBACK_VEC_REQUEST_ID: {
            // cout << "FEEDBACK_VEC_REQUEST_ID \n";
            // for(auto w : data_vec) {
            //     cout << HEX32_STRING(w) << "\n";
            // }

            if( data_vec.size() < 1 ) {
                cout << "FEEDBACK_VEC_REQUEST_ID got illegally small length of 0\n";
                break;
            }
            auto id = data_vec[0];

            if( id >= 32 ) {
                cout << "FEEDBACK_VEC_REQUEST_ID got illegally large argument in [0] of " << id << "\n";
                break;
            }

            cout << "replying to FEEDBACK_VEC_REQUEST_ID\n";
            sendVectorTypeToPartnerPC(id, FEEDBACK_VEC_REPLY_ID, {soapy->this_node.id});
        }
        break;

        case FEEDBACK_VEC_REPLY_ID: {
            // cout << "FEEDBACK_VEC_REPLY_ID \n";
            // for(auto w : data_vec) {
            //     cout << HEX32_STRING(w) << "\n";
            // }

            if( data_vec.size() < 1 ) {
                cout << "FEEDBACK_VEC_REPLY_ID got illegally small length of 0\n";
                break;
            }
            auto id = data_vec[0];

            if( id >= 32 ) {
                cout << "FEEDBACK_VEC_REPLY_ID got illegally large argument in [0] of " << id << "\n";
                break;
            }

            peerConnectedReply(id);
            break;
        }

        case FEEDBACK_VEC_FETCH_REQUEST: {
            zmqFetch(data_vec);
        }
        break;

        case FEEDBACK_VEC_FETCH_REPLY: {
            zmqFetchDispatchReply(data_vec);
        }
        break;

        case FEEDBACK_VEC_JOINT_TRANSMIT: {
            handleJointTransmit(data_vec);
        }
        break;

        case FEEDBACK_VEC_FINE_SYNC:
        default:
            smodem_consumed = false;
            break;
    }

    // std::vector<uint32_t> header_vec = feedback_frame_vec_to_vector(vec);


    bool callback_consumed = false;
    if( zmqCallback ) {
        // bool found = false;
        // (void) found;
        for( auto x : zmqCallbackVTypes) {
            if( vec->vtype == x ) {

                // only build this vector if we match the vtype
                std::vector<uint32_t> full_vec;
                full_vec.assign(full_frame, full_frame + length_words);

                // if( x == FEEDBACK_VEC_WORDS_CORRECT ) {
                    // cout << "vcorrect: " << full_vec[19] << ", " << full_vec[20] << "\n";
                // }

                zmqCallback(full_vec);
                callback_consumed = true;
                break;
            }
        }

        // if( found ) {
        //     cout << HEX32_STRING(cmd_type) << " passed rb to callback" << endl;
        // } else {
        //     cout << HEX32_STRING(cmd_type) << " skipped callback" << endl;
        // }
    } else {
        // cout << "Skipping ringCallback due to callback not being set" << endl;
    }

    if( (!smodem_consumed) && (!callback_consumed)) {
        cout << "handlePcPcVectorType() does not recognize type " << vec->vtype << endl;
    }
}


/// see HiggsSoapyFeedbackBus.cpp
/// this is a PC->PC feedback bus buffer that was sent to us via zmq.
/// the buffer arives at our pc via zmq on the zmq thread.  After the tag matches us etc
/// it gets put into a libevent buffer. This gets it off the zmq thread
void EventDsp::handleParsedFrameForPc(uint32_t* full_frame, size_t length_words) {
    feedback_frame_t *v;

    // cout << "handleParsedFrameForPc() " << endl;

    // generic type
    v = (feedback_frame_t*) full_frame;

    if( v->destination0 & FEEDBACK_PEER_SELF ) {
        cout << "handleParsedFrameForPc illegal destination SELF" << endl;
        return;
    }

    if( v->destination1 & FEEDBACK_DST_PC ) {
        if( v->type == FEEDBACK_TYPE_VECTOR ) {
            handlePcPcVectorType(full_frame, length_words);
        }
    }

    // FIXME: for now we are forwarding to everyone
    // we should improve this based on 
    if( v->destination1 & FEEDBACK_PEER_ALL ) {

    }
}





void EventDsp::handleJointTransmit(const std::vector<uint32_t>& data_vec) {

    // for(unsigned i = 0; i < data_vec.size() && i < 4; i++) {
    //     cout << HEX32_STRING(data_vec[i]) << "\n";
    // }
    uint32_t from_peer; // id of arbiter
    uint32_t epoc;
    uint32_t ts;
    uint8_t  seq;
    AirPacket::unpackJointZmqHeader(data_vec, from_peer, epoc, ts, seq);

    if( ts >= SCHEDULE_SLOTS ) {
        cout << "handleJointTransmit() rejecting illegal timeslot: " << ts << "\n";
        return;
    }

    {
        std::unique_lock<std::mutex> lock(soapy->txSchedule->_beamform_buffer_mutex);
        soapy->txSchedule->beamform_buffer.emplace_back(data_vec);
    }
    // report time after we grab the lock because technically we can't process the data
    // before we get the lock

    // return;



    int error;

    epoc_frame_t est;
    est = attached->predictScheduleFrame(error, false, false);

    sendJointAck(from_peer, epoc, ts, seq, est.epoc, est.frame, false);

    
    cout << "handleJointTransmit() GOT FEEDBACK_VEC_JOINT_TRANSMIT " << epoc << ", " << ts << " seq: " << (int)seq << " size: " << data_vec.size() <<  "\n";
};












///
///  Below here is setup
///


void EventDsp::specialSettingsByRole(const std::string& r) {
    cout << "EventDsp :: specialSettingsByRole()\n";

    if( r == "arb" ) {
        // all set to false except
        // init_drain_custom_events
        // init_drain_for_inbound_fb_bus

        init_drain_udp_payload = false;
        init_drain_all_sc = false;
        init_drain_coarse = false;
        init_drain_for_gnuradio = false;
        init_drain_for_gnuradio_2 = false;
        init_drain_ringbus_in = false;

        // other settings
        init_fsm = false;
        init_feedback_bus = false;
    }
}


///
/// Setup of callbacks and buffer waterlevels
///
void EventDsp::setupBuffers() {

    // shared set of pointers that we overwrite and then copy out from
    // (copied to BevPair class)
    struct bufferevent * rsps_bev[2];


    int ret;

    ////////////////////
    //
    // raw udp buffer
    //
    // could be replaced by reading fid directly
    // fixme
    //
    // This buffer is cross threads while the others do not
    // this allows us to have an entire thread dedicated to rx

    if(init_drain_udp_payload) {
        ret = bufferevent_pair_new(evbase, 
            BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE, 
            rsps_bev);
        assert(ret>=0);
    
        // our little chained method for init and options all in one line
        // cout << "setupBuffers() ret " << ret << endl;

        udp_payload_bev = new BevPair();
        udp_payload_bev->set(rsps_bev);//.enableLocking();

        bufferevent_setwatermark(udp_payload_bev->out, EV_READ, (1024+16)*4, 0);

        bufferevent_setcb(udp_payload_bev->out, _handle_udp_callback, NULL, event_callback, this);
        bufferevent_enable(udp_payload_bev->out, EV_READ | EV_WRITE | EV_PERSIST);

        bufferevent_set_max_single_read(udp_payload_bev->out, (1024+16)*4 );

        bufferevent_disable(udp_payload_bev->in, EV_READ);
        bufferevent_enable(udp_payload_bev->out, EV_READ);
    }




    ////////////////
    //
    // every subcarrier buf
    //
    if(init_drain_all_sc)   {
        ret = bufferevent_pair_new(evbase, BEV_OPT_CLOSE_ON_FREE, rsps_bev);
        assert(ret>=0);

        all_sc_buf = new BevPair();
        all_sc_buf->set(rsps_bev);//.enableLocking();

        // set 0 for write because nobody writes to out
        bufferevent_setwatermark(all_sc_buf->out, EV_READ, ALL_SC_BUFFER_SIZE*4, 0);
        bufferevent_set_max_single_read(all_sc_buf->out, ALL_SC_BUFFER_SIZE*4);
        // only get callbacks on read side
        bufferevent_setcb(all_sc_buf->out, _handle_all_sc_callback, NULL, _handle_all_sc_event, this);
        bufferevent_enable(all_sc_buf->out, EV_READ | EV_WRITE | EV_PERSIST);
    }


    ////////////////
    //
    // coarse 
    //
    // ret = bufferevent_pair_new(evbase, BEV_OPT_CLOSE_ON_FREE, rsps_bev);
    // assert(ret>=0);
    // coarse_bev = new BevPair();
    // coarse_bev->set(rsps_bev);//.enableLocking();

    // if(init_drain_coarse) {
    //     bufferevent_setwatermark(coarse_bev->out, EV_READ, SUBCARRIER_CHUNK*4, 0);
    //     bufferevent_set_max_single_read(coarse_bev->out, SUBCARRIER_CHUNK*4);
    //     // only get callbacks on read side
    //     bufferevent_setcb(coarse_bev->out, _handle_coarse_callback, NULL, _handle_coarse_event, this);
    //     bufferevent_enable(coarse_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
    // }


    ////////////////
    //
    // data bound for gnuradio 
    //
    if(init_drain_for_gnuradio) {
        ret = bufferevent_pair_new(evbase, 
            BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE,
            rsps_bev);
        assert(ret>=0);

        gnuradio_bev = new BevPair();
        gnuradio_bev->set(rsps_bev);//.enableLocking();

        if( GET_GNURADIO_UDP_DO_CONNECT() ) {
            // new way where modem main is running stand-alone and sending udp to 
            bufferevent_setwatermark(gnuradio_bev->out, EV_READ, GNURADIO_UDP_SIZE, 0);
            bufferevent_set_max_single_read(gnuradio_bev->out, GNURADIO_UDP_SIZE);
            bufferevent_setcb(gnuradio_bev->out, _handle_gnuradio_callback_udp_mode, NULL, _handle_gnuradio_event, this);
            bufferevent_enable(gnuradio_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
        }
    }


    ////////////////
    //
    // eq data bound for gnuradio 
    //
    if(init_drain_for_gnuradio_2) {
        ret = bufferevent_pair_new(evbase, 
            BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE,
            rsps_bev);
        assert(ret>=0);

        gnuradio_bev_2 = new BevPair();
        gnuradio_bev_2->set(rsps_bev);//.enableLocking();

        if( GET_GNURADIO_UDP_DO_CONNECT2() ) {
            // new way where modem main is running stand-alone and sending udp to 
            bufferevent_setwatermark(gnuradio_bev_2->out, EV_READ, GNURADIO_UDP_SIZE, 0);
            bufferevent_set_max_single_read(gnuradio_bev_2->out, GNURADIO_UDP_SIZE);
            bufferevent_setcb(gnuradio_bev_2->out, _handle_gnuradio_callback_udp_mode_2, NULL, _handle_gnuradio_event_2, this);
            bufferevent_enable(gnuradio_bev_2->out, EV_READ | EV_WRITE | EV_PERSIST);
        }
    }




    ////////////////
    //
    // data bound for events
    //
    if(init_drain_custom_events) {
        ret = bufferevent_pair_new(evbase, 
            BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE, 
            rsps_bev);
        assert(ret>=0);

        events_bev = new BevPair();
        events_bev->set(rsps_bev);//.enableLocking();

        // set 0 for write because nobody writes to out, in bytes
        bufferevent_setwatermark(events_bev->out, EV_READ, sizeof(custom_event_t), 0);
        bufferevent_set_max_single_read(events_bev->out, sizeof(custom_event_t) );
        // only get callbacks on read side
        bufferevent_setcb(events_bev->out, _handle_custom_event_callback, NULL, _handle_custom_event_event, this);
        bufferevent_enable(events_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
    }


    ////////////////
    //
    // data that came from zmq, feedback bus packets that are destined for pc
    //
    if(init_drain_for_inbound_fb_bus) {
        ret = bufferevent_pair_new(evbase, 
            BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE, 
            rsps_bev);
        assert(ret>=0);
        fb_pc_bev = new BevPair();
        fb_pc_bev->set(rsps_bev);//.enableLocking();

        // set 0 for write because nobody writes to out, in bytes
        // set to 8 bytes because we will send length as 4 bytes
        bufferevent_setwatermark(fb_pc_bev->out, EV_READ, 8, 0);
        // bufferevent_set_max_single_read(fb_pc_bev->out, DEMOD_BEV_SIZE_WORDS*4 );
        // only get callbacks on read side
        bufferevent_setcb(fb_pc_bev->out, _handle_fb_pc_callback, NULL, _handle_fb_pc_event, this);
        bufferevent_enable(fb_pc_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
    }




    // not exactly a buffer

    // process_sfo_timer = evtimer_new(evbase, _handle_sfo_timer, this);

    // process_cfo_timer = evtimer_new(evbase, _handle_cfo_timer, this);



    ////////////////
    //
    // Fire up timer for sending to higgs
    //
    // sendto_higgs_timer = evtimer_new(evbase, _handle_sendto_higgs, this);

    // // setup once
    // struct timeval first_stht_timeout = { .tv_sec = 0, .tv_usec = 1000*200 };
    // evtimer_add(sendto_higgs_timer, &first_stht_timeout);


    ////////////////
    //
    // process ringbus
    // comes from another thread, should be threadsafe
    //
    if(init_drain_ringbus_in) {
        ret = bufferevent_pair_new(evbase, 
            BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE, 
            rsps_bev);
        assert(ret>=0);
        rb_in_bev = new BevPair();
        rb_in_bev->set(rsps_bev);//.enableLocking();

        bufferevent_setwatermark(rb_in_bev->out, EV_READ, 4, 0);
        // only get callbacks on read side
        bufferevent_setcb(rb_in_bev->out, _handle_rb_callback, NULL, _handle_rb_event, this);
        bufferevent_enable(rb_in_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
    }


    ////////////////
    //
    // process sliced data
    // comes from this same thread
    //
    // ret = bufferevent_pair_new(evbase, 
    //     BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS,
    //     rsps_bev);
    // assert(ret>=0);
    // sliced_data_bev = new BevPair();
    // sliced_data_bev->set(rsps_bev);//.enableLocking();

    // if(init_drain_for_sliced_data) {
    //     bufferevent_setwatermark(sliced_data_bev->out, EV_READ, SLICED_DATA_BEV_WRITE_SIZE_WORDS*SLICED_DATA_CHUNK*4, 0);
    //     // only get callbacks on read side
    //     bufferevent_setcb(sliced_data_bev->out, _handle_sliced_data_callback, NULL, _handle_sliced_data_event, this);
    //     bufferevent_enable(sliced_data_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
    // }



    (void)ret;

}



// build radio vector at start of experiment according to settings
void EventDsp::setupRadioVector() {

    // according to settings
    // auto tx_peer_0 = GET_PEER_0();
    auto tx_peer_1 = GET_PEER_1();
    auto ids = soapy->getTxPeers();


    // GET_PEER_1()

    if( !VECTOR_FIND(ids,tx_peer_1) ) {
        cout << "-----------------------------------------------------";
        cout << "-----------------------------------------------------";
        cout << "pushing " << tx_peer_1 << " to radio vectors to enforce 2 minimum\n";
        ids.push_back(tx_peer_1);
    }

    radios.resize(ids.size());
    
    // call radios constructor radio constructor RadioEstimate constructor
    size_t radio_i = 0;
    for(auto id : ids) {
        // call new() in a loop and use std vector for pointers
        radios[radio_i] =  new RadioEstimate(soapy, this, id, evbase, radio_i, true);
        radio_i++;
    }

    /// Loop through each created and call getIndexForDemodTDMA()
    /// Take a mutex and update EventDsp's demod_buf_enabled_sc
    ///
    {
        std::unique_lock<std::mutex> lock(_demod_buf_enabled_mutex);
        demod_buf_enabled_sc = {};

        for(auto r : radios) {
            demod_buf_enabled_sc.push_back(r->getIndexForDemodTDMA());
        }
    }
}

void EventDsp::setupAttachedRadio() {

    while(!soapy->thisNodeReady()) {
        // cout << "waiting for this_node" << endl;
        // cout << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" << endl;
        usleep(1);
    }

    // auto id = this_no
    auto our_peer = soapy->this_node.id;

    // for now tx is the only one that has this
    if( role != "arb" ) {
        cout << "as " << role << " side calling attached constructor with id " << our_peer << endl;
        attached = new AttachedEstimate(soapy, this, our_peer, evbase, 0, true); 
        attached->negate_sfo_updates = true;
    } else {
        attached = NULL;
    }
}

// void EventDsp::setScheduleMode() {
// add these convenience functions to the eventdsp instead of to individual radios
//


// returns 0, sane default for failure
uint32_t EventDsp::nodeIdForRadioIndex(const size_t a) const {
    for(auto r : radios) {
        if(r->array_index == a) {
            return r->peer_id;
        }
    }
    cout << "error, bad value " << a << " sent to nodeIdForRadioIndex()" << endl;
    return 0;
}

size_t EventDsp::radioIndexForNodeId(const uint32_t p) const {
    for(auto r : radios) {
        if(r->peer_id == p) {
            return r->array_index;
        }
    }
    cout << "error, bad value " << p << " sent to radioIndexForNodeId()" << endl;
    return 0;
}


void EventDsp::sendEvent(const custom_event_t* const e) {
    const void* const p = (const custom_event_t* const)e;

    // cout << "sendEvent writing\n";

    bufferevent_write(events_bev->in, p, sizeof(custom_event_t));
}

void EventDsp::sendEvent(const custom_event_t e) {
    sendEvent(&e);
}

void EventDsp::sendToMock(const char* s) {
    std::string str(s);
    sendToMock(str);
}

void EventDsp::sendToMock(const nlohmann::json &j) {
    std::string tmp = j.dump();
    sendToMock(tmp);
}

void EventDsp::sendToMock(const std::string &str) {
    // std::unique_lock<std::mutex> lock(_send_to_mock_mutex);
    auto peer = soapy->this_node.id;

    // std::thread::id this_id = std::this_thread::get_id();
    // cout << "sendToMock " << this_id << endl;

    // cout << "sendToMock " << str << endl;

    // cout << "a" << endl;

    size_t i = 0;
    uint32_t word = 0;
    for (auto it = str.begin(); it < str.end(); ++it) {
        size_t mod = i%3;
        switch(mod) {
            case 0:
                word = word | ((((uint32_t)(*it))&0xff)<<16);
                break;
            case 1:
                word = word | ((((uint32_t)(*it))&0xff)<<8);
                break;
            case 2:
                word = word | ((((uint32_t)(*it))&0xff)<<0);
                break;
            default:
                cout << "sendToMock broken" << endl;
                break;
        }

        if( mod == 2 || (it+1) == str.end() ) {
            // cout << HEX32_STRING(word) << endl;
            raw_ringbus_t rbloop = {RING_ADDR_ETH, NODEJS_RPC_CMD | word };
            zmqRingbusPeerLater(peer, &rbloop, 0);
            // usleep(1000);
            word = 0;
        }

        i++;
    }

    // flush (only needed when division above was even but no harm in sending anyways)
    raw_ringbus_t rb1 = {RING_ADDR_ETH, NODEJS_RPC_CMD | 0x0 };
    zmqRingbusPeerLater(peer, &rb1, 0);
}


void EventDsp::registerZmqCallback(zmq_cb_t f) {
    zmqCallback = f;
}

// pass a vtype
void EventDsp::zmqCallbackCatchType(uint32_t mask) {
    zmqCallbackVTypes.push_back(mask);
}

void EventDsp::suppressFeedbackEqUntil(const uint32_t lifetime, const bool also_upper) {

    if( also_upper ) {
        const uint32_t upper = (lifetime >> 24) & 0xff;
        soapy->rb->send(RING_ADDR_RX_TAGGER, SUPRESS_EQ_FEEDBACK_H_CMD | upper );
    }

    const uint32_t lower = lifetime & 0xffffff;

    soapy->rb->send(RING_ADDR_RX_TAGGER, SUPRESS_EQ_FEEDBACK_L_CMD | lower );
}