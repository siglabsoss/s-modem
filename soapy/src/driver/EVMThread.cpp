#include "EVMThread.hpp"
#include <future>

using namespace std;
// #include "vector_helpers.hpp"

// #include "HiggsDriverSharedInclude.hpp"
#include "HiggsStructTypes.hpp"
#include "common/convert.hpp"
#include "driver/HiggsSettings.hpp"
#include "HiggsSoapyDriver.hpp"
#include "cplx.hpp"
// #include "driver/VerifyHash.hpp"
// #include "driver/RetransmitBuffer.hpp"
// #include "driver/AirPacket.hpp"
// #include "driver/TxSchedule.hpp"
// #include "ScheduleData.hpp"

using namespace std;

#define EVM_WATERMARK (EVM_FRAMES*EVM_TONE_NUM*4)

// #define EVM_CHUNK_BYTES (EVM_POINTS*EVM_TONE_NUM*4)

#define EVM_KEEP (1024)

#define EVM_DROPPING_CHUNK (EVM_KEEP*EVM_TONE_NUM)

#define EVM_INTERNAL_CHUNK (EVM_FRAMES*EVM_TONE_NUM)

#define EVM_TONE_BYTES ((EVM_TONE_NUM)*4)
// how many bytes to pull from the 
#define EVM_FRAMES (1024*4)

// how many to run at once
#define EVM_POINTS (1024)


// FIXME these are old MMSE variables that should be moved
static const unsigned int num_points = EVM_POINTS;
static constexpr unsigned int array_length = EVM_POINTS * 2;
static double input_buffer[array_length];
static double output_buffer[array_length];
static double evm_buffer[num_points];
static double eq_buffer[array_length];

static void init_eq_buffer(double* const _eq_buffer, const unsigned int _array_length) {
    for (unsigned int i = 0; i < _array_length; i++) {
        _eq_buffer[i] = (i + 1) % 2;
    }
}

std::vector<uint32_t> ppack (const uint32_t* const src, const unsigned int num_points, const unsigned int step) {

    unsigned int wrote = 0;
    std::vector<uint32_t> ret;
    unsigned int i = 0;
    for (; i < num_points; i += step) {
        const uint32_t x = src[i];
        // const double x_re = f64_ci32_real(x, q);
        // const double x_im = f64_ci32_imag(x, q);
        // dst[2 * i] = x_re;
        // dst[2 * i + 1] = x_im;
        ret.push_back(x);
        wrote += 2;
    }
    // std::cout << "num points " << num_points << " wrote " << wrote << "\n";
    return ret;
}


// static variable, local to this thread
// used to bridge these naked functions and the class
// naked functions write to this (under a lock)  and then 
// getDataToJs() will just grab it
static std::vector<uint32_t> selected;

// mmse code
// see https://observablehq.com/@drom/blind-mmse
std::vector<double> handle_evm(const std::vector<uint32_t>& vec0) {




    // cout << "handle_evm got " << vec0.size() << "\n";

    const unsigned num_words = 1024 * 32;

    std::vector<double> out;
    // const int chunks = vec0.size() / num_points;

    const int step = EVM_TONE_NUM;
    const uint32_t q = 12;

    // for(int i = 0; i < chunks; i++ ) {
        // for( int j = 0; j < num_points; j++) {
            // const uint32_t* int_d_start = (const uint32_t*)vec0.data();

    const uint32_t* int_d_start = ((const uint32_t*)vec0.data());

    selected = ppack(int_d_start, num_words, step);

    // cout << "packa"  << endl;
    cf64_ci32_unpack_vector(int_d_start, input_buffer, q, num_words, step);
    // cout << "packb"  << endl;
    mmse_freq_vector(input_buffer, output_buffer, eq_buffer, num_points);
    // cout << "packc"  << endl;
    evm_vector(output_buffer, evm_buffer, num_points);
    // cout << "packd"  << endl;

    for (unsigned i = 0; i < num_points; i++) {
        out.push_back(evm_buffer[i]);
        // std::cout << std::hex << "Eq Index[" << i << "]: " << eq_buffer[i] << " " << eq_buffer[i+1] << std::endl; 
        // std::cout << std::hex << "EVM Index[" << i << "]: " << evm_buffer[i] << std::endl;
    }



        // }
    // }

    // cout << "Evm ran in " << chunks << " chunks\n";

// #ifdef evmm



    // cf64_ci32_unpack_vector(int_d_start, input_buffer, q, num_points, step);
    // mmse_freq_vector(input_buffer, output_buffer, eq_buffer, num_points);
    // evm_vector(output_buffer, evm_buffer, num_points);
    // for (int i = 0; i < num_points; i++) {
    //     // std::cout << std::hex << "Eq Index[" << i << "]: " << eq_buffer[i] << " " << eq_buffer[i+1] << std::endl; 
    //     std::cout << std::hex << "EVM Index[" << i << "]: " << eq_buffer[i] << std::endl;
    // }
// #endif
    return out;
}


static void write_global_selected(const std::vector<uint32_t>& vec0) {
    const unsigned num_words = 1024 * 32;

    const int step = EVM_TONE_NUM;
    // const uint32_t q = 12;

    const uint32_t* int_d_start = ((const uint32_t*)vec0.data());

    selected = ppack(int_d_start, num_words, step);
}





static void _handle_sliced_data_single_radio(EVMThread *that, const uint32_t *word_p, const size_t read_words) {


    // cout << "read_words: " << read_words << "\n";


    // const uint32_t *int_d_start = word_p;
    // (void)int_d_start;


    // uint32_t q = 12;
    // unsigned int step = 32;

    // unsigned int num_points = 64;


    std::vector<uint32_t> waste;
    waste.resize(read_words);
    for(unsigned i = 0; i < read_words; i++) {
        waste[i] = word_p[i];
    }


    // scope for the lock
    {
        std::unique_lock<std::mutex> lock(that->_data_to_js_mutex);

        // let js handle evm
        if(true) {
            write_global_selected(waste);
        } else {
            // let c handle evm (broken)
            // std::vector<double> result;
            // result = handle_evm(waste);
        }
    }

    if( that->print_dump ) {
        for(auto w : waste) {
            cout << HEX32_STRING(w) << "\n";
        }
        // exit(0);
    }



    // double sum = 0;
    // for(const auto d : result ) {
    //     sum += d;
    // }
    // sum /= (double)result.size();

    // // that->evm = sum;
    // that->evm = result[0];

    return;

    // for(int i = 0; i < 4; i++) {
    //     cout << "Evm: " << result[4] << "\n";
    // }




                // cf64_ci32_unpack_vector(int_d_start, input_buffer, q, num_points, step);
                //mmse_freq_vector(input_buffer, output_buffer, eq_buffer, num_points);
                //evm_vector(output_buffer, evm_buffer, num_points);
                // for (int i = 0; i < num_points; i++) {
                //     // std::cout << std::hex << "Eq Index[" << i << "]: " << eq_buffer[i] << " " << eq_buffer[i+1] << std::endl; 
                //     std::cout << std::hex << "EVM Index[" << i << "]: " << eq_buffer[i] << std::endl;
                // }


    // for(size_t i = 0; i < read_words; i++) {

    //     if( i % data_bev_WRITE_SIZE_WORDS == 0 ) {
    //         // this branch enters 1 times for every OFDM frame

    //         // save the frame in index 0
    //         that->sliced_words[0].push_back(word_p[i]);
    //         for(size_t j = 0; j < (SLICED_DATA_WORD_SIZE-1); j++) {
    //             that->sliced_words[0].push_back(0);
    //         }
    //         // cout << "frame: " << word_p[i] << endl;
    //     } else {
    //         // this branch enters 8 times for every OFDM frame
    //         // size_t j = (i % data_bev_WRITE_SIZE_WORDS) - 1;

    //         that->sliced_words[1].push_back(word_p[i]);

    //         // cout << "data: " << HEX32_STRING(word_p[i]) << " <- " << j << endl;
    //     }
    // }
    // assert(that->sliced_words[0].size() == that->sliced_words[1].size());

    // if( that->sliced_words[0].size() != that->sliced_words[1].size() ) {
    //     cout << "mismatch in _handle_sliced_data_single_radio() "
    //          << that->sliced_words[0].size() << ", "
    //          << that->sliced_words[1].size() << "\n";
    // }

    // // cout << r.sliced_words[0].size() << endl;
    // // cout << r.sliced_words[1].size() << endl;

}




static void _handle_data_callback(struct bufferevent *bev, void *_pointer) {
     // cout << "_handle_fb_pc_callback" << endl;
    EVMThread *that = (EVMThread*) _pointer;

    struct evbuffer *buf  = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(buf);

    if( len == 0) {
        return;
    }


    size_t this_read = len;
    this_read = (this_read / (EVM_WATERMARK)) * (EVM_WATERMARK); // make sure a multiple of
    auto read_words = this_read / 4;

    // cout << "read_words " << read_words << "\n";

    uint32_t *word_p = reinterpret_cast<uint32_t*>(evbuffer_pullup(buf, this_read));

    // cout << "this_read " << this_read << " read_words " << read_words << endl;

    // auto who_gets_samples = whoGetsWhatData("sliced_data", frame);

    // for now, fixed to r[0]

    // unsigned drop = EVM_DROPPING_CHUNK;

    // if( read_words < drop ) {
    //     cout << "something seriously wrong in evm _handle_data_callback\n";
    // }

    _handle_sliced_data_single_radio(that, word_p, read_words);


    evbuffer_drain(buf, this_read);
    // evbuffer_drain(buf, -1);

}

static void _handle_sliced_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_sliced_event" << endl;
}




// static void _handle_demod_timer(int fd, short kind, void *_dsp) {
//     // // cout << "_handle_demod_timer\n";
//     // DemodThread *that = (DemodThread*) _dsp;

//     // if( that->pending_dump > 0 ) {
//     //     that->handlePendingDump();
//     // } else {
//     //     that->check_subcarrier();
//     // }

//     // struct timeval val = { .tv_sec = 0, .tv_usec = 2048 };
//     // evtimer_add(that->demod_data_timer, &val);
// }


// returns:
//   valid
//   data if valid
std::pair<bool, std::vector<uint32_t> > EVMThread::getDataToJs() {
    std::unique_lock<std::mutex> lock(_data_to_js_mutex);

    if( selected.size() == 0 ) {
        return std::pair<bool, std::vector<uint32_t> >(false, {});
    } else {
        std::pair<bool, std::vector<uint32_t> > ret(true, selected);
        // data_for_js.erase(data_for_js.begin());
        return ret;
    }
}


///////////////////////////////////////////////////////////////
//
//  Above this line are static functions which comprise events for this class
//



EVMThread::~EVMThread() {
    _thread_should_terminate = true;
    event_base_loopbreak(evbase);

    if (_thread.joinable()) {
        _thread.join();
    }

    cout << "~EVMThread()\n";
}


EVMThread::EVMThread(HiggsDriver* s):
    
    //
    // class wide initilizers
    //
    HiggsEvent(s)
    ,dropSlicedData(true)

    //
    // function starts:
    //
{
    settings = (soapy->settings);


    // role = GET_RADIO_ROLE();

    cout << "EVMThread::DemodThread()" << endl;

    // sliced_words.resize(2);

    peer_id = soapy->this_node.id;

    // FIXME, how to handle retransmits during beamform?
    // retransmit_to_peer_id = GET_PEER_0();

    init_eq_buffer(eq_buffer, array_length);
    setupBuffers();
    // setAirParams();


    // soapy->_demod_thread_ready = true;
    // cout << "_demod_thread_ready true\n";

    init_timepoint = std::chrono::steady_clock::now();
}


void EVMThread::stopThreadDerivedClass() {

}



void EVMThread::setupBuffers() {

    struct bufferevent * rsps_bev[2];

    int ret;
    (void)ret;


    ////////////////
    //
    // process sliced data
    // comes from this same thread
    //
    ret = bufferevent_pair_new(evbase, 
        BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE,
        rsps_bev);
    assert(ret>=0);
    data_bev = new BevPair();
    data_bev->set(rsps_bev);//.enableLocking();

    if(init_drain_for_sliced_data) {
        bufferevent_setwatermark(data_bev->out, EV_READ, EVM_WATERMARK, 0);
        // only get callbacks on read side
        bufferevent_setcb(data_bev->out, _handle_data_callback, NULL, _handle_sliced_event, this);
        bufferevent_enable(data_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
    }



    ////////////////
    //
    // Fire up timer for demod
    //
    // demod_data_timer = evtimer_new(evbase, _handle_demod_timer, this);

    // // setup once
    // struct timeval first_stht_timeout = { .tv_sec = 0, .tv_usec = 1000*800 };
    // evtimer_add(demod_data_timer, &first_stht_timeout);

}



