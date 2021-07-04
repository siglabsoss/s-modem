#include "HiggsSoapyDriver.hpp"
#include "HiggsDriverSharedInclude.hpp"

// also gets feedback_bus.h from riscv
// #include "feedback_bus_tb.hpp"

// #include "tb_ringbus.hpp"

#include "vector_helpers.hpp"

#include <fstream>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "driver/FsmMacros.hpp"
#include "driver/RadioEstimate.hpp"
#include "driver/EventDsp.hpp"
#include "driver/DemodThread.hpp"
#include "driver/EVMThread.hpp"


using namespace std;

// FIXME these are old MMSE variables that should be moved
static const unsigned int num_points = 32;
static const unsigned int array_length = num_points * 2;
static double input_buffer[array_length];
static double output_buffer[array_length];


// static double evm_buffer[num_points];
static double eq_buffer[array_length];
static bool init_eq = true;
// FIXME This method should be placed in a constructor
static void init_eq_buffer(double* const _eq_buffer, const unsigned int _array_length) {
    for (unsigned int i = 0; i < _array_length; i++) {
        _eq_buffer[i] = (i + 1) % 2;
    }
}

// seems to be slow? or at the parent function is stalling when we get
// 2,6
// runtimes for this are
/*
 0
us 0
us 1
us 0
us 0
us 0
us 0
us 0
us 0
us 0
us 3
us 143
us 0
us 0
us 0
*/
void EventDsp::handleSlicedData(const feedback_frame_vector_t *vec) {

    const unsigned int* data = ((const unsigned int *)vec)+FEEDBACK_HEADER_WORDS;
    uint32_t vector_words = vec->length - FEEDBACK_HEADER_WORDS;

    const uint32_t* header = (const uint32_t*)vec;
	(void)header;

	// use only for debugging INTEGRATION_TEST
    // cout << "Seq B: " << header[5] << "\n";
    // cout << "Data: " << data[0] << "\n";

    if( _enable_file_dump_sliced_data ) {

        if(!_file_dump_sliced_data_open) {
            _sliced_data_f.open(GET_FILE_DUMP_SLICED_DATA_PATH());
            _file_dump_sliced_data_open = true;
        }

        if( soapy->demodThread->times_sync_word_detected != 0 ) {
            _sliced_data_f << "got " << vector_words << " words s " << vec->seq << endl;

            for(unsigned i = 0; i < vector_words; i++) {
                _sliced_data_f << HEX32_STRING(data[i]) << endl;
            }
        }
    }


    auto now = std::chrono::steady_clock::now();

    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    now - _track_invalid_demod_timepoint
    ).count();

    if( elapsed_us > 1000000 && vector_words != (SLICED_DATA_BEV_WRITE_SIZE_WORDS-1)) {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
        cout << "! handleSlicedData() data sizes do not match up (" << vector_words << ", " <<  (SLICED_DATA_BEV_WRITE_SIZE_WORDS-1) << "), demod will not be happy" << endl;
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!" << endl;

        _track_invalid_demod_timepoint = now;
    }

    // reinterpret_cast<char *>(&vec->seq)

    bufferevent_write(
        soapy->demodThread->sliced_data_bev->in,
        reinterpret_cast<const char *>(&vec->seq),
        sizeof(vec->seq) );

    bufferevent_write(
        soapy->demodThread->sliced_data_bev->in,
        reinterpret_cast<const char *>(data),
        sizeof(uint32_t)*vector_words );


}

#include <fstream>

// on event thread
void EventDsp::handleFineSync(const feedback_frame_vector_t *vec) {


    auto thena = std::chrono::steady_clock::now();

#ifdef PRINT_LIBEV_EVENT_TIMES
    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    thena - handle_fine_timepoint
    ).count();
#endif

    handle_fine_timepoint = thena;




    const feedback_frame_t* const generic = (const feedback_frame_t* const) vec;
    (void)generic;
    // print_feedback_generic(generic);

    const unsigned int* data = ((const unsigned int *)vec)+FEEDBACK_HEADER_WORDS;

    // static uint32_t debug_counter = 0;
    // safe_cfo.enqueue(debug_counter);
    // debug_counter++;

    //fine_sync_buf
    // header is vec
    // length of vec is  

    uint32_t vector_words = vec->length - FEEDBACK_HEADER_WORDS;
    (void)vector_words;

    uint32_t frame_sequence_number = vec->seq;
 
    uint32_t temp_buf[SFO_CFO_PACK_SIZE];

    if( _debug_loopback_fine_sync_counter ) {
        static unsigned slowloop = 0;
        if( slowloop++ % 100 == 0 ) {
            ticklePath(frame_sequence_number, 100, "fb", "frame_counter");
        }
    }

    
    if(_rx_should_insert_sfo_cfo) {
        // cout << "sfo/cfo frame " << frame_sequence_number << endl;
        // soapy->sfo_buf.enqueue(data[0],frame_sequence_number);

        temp_buf[0] = frame_sequence_number;
        temp_buf[1] = data[0]; // sfo_buf_0
        temp_buf[2] = data[1]; // cfo_buf_0
        temp_buf[3] = data[2]; // sfo_buf_1
        temp_buf[4] = data[3]; // cfo_buf_1
        temp_buf[5] = data[4]; // rolling data
        temp_buf[6] = data[5]; // rolling data
        

#ifdef OVERWRITE_FINE_CFO_WITH_STREAM
        temp_buf[2] = parse_stream_history[0]; // cfo_buf_0
#endif


        // bufferevent_write(sfo_cfo_buf->in, temp_buf, SFO_CFO_PACK_SIZE*4);
        bufferevent_write(soapy->higgsFineSync->sfo_cfo_buf->in, temp_buf, SFO_CFO_PACK_SIZE*4);


        if( _dump_residue_upstream ) {
            if(!_dump_residue_upstream_fileopen) {
                _dump_residue_upstream_fileopen = true;
                _dump_residue_upstream_ofile.open(_dump_reside_upstream_fname);
                cout << "OPENED " << _dump_reside_upstream_fname << "\n";
            }

            _dump_residue_upstream_ofile << HEX32_STRING(data[1]) << "\n";
                // myfile << "found_i " << found_i << " was at frame " << demod_buf_accumulate[0][found_i].second << endl;
            // cout << "d";
        }




        // soapy->cfo_buf.enqueue(data[1],frame_sequence_number);
        // soapy->sfo_buf_2.enqueue(data[2],frame_sequence_number);
        // soapy->cfo_buf_2.enqueue(data[3],frame_sequence_number);
#ifdef OVERWRITE_GNU_WITH_CFO
        parse_fine_history[0] = data[0];
        parse_fine_history[1] = data[1];
        parse_fine_history[2] = data[2];
        parse_fine_history[3] = data[3];
#endif

    }

#ifdef PRINT_LIBEV_EVENT_TIMES
    auto thenb = std::chrono::steady_clock::now();

    size_t rta = std::chrono::duration_cast<std::chrono::microseconds>( 
    thenb - thena
    ).count();


    if( elapsed_us > 5000 || rta > 200 ) {
        cout << "handleFineSync() elapsed_us " << elapsed_us << endl;
        cout << "handleFineSync() runtime_us " << rta
         << endl;
     }
#endif




    // if (dump_fine_sync && dump_fine_ostream->is_open()){
    //     *dump_fine_ostream << "0x" << HEX32_STRING(data[0]) << std::endl;
    //     *dump_fine_cfo_ostream << "0x" << HEX32_STRING(data[1]) << std::endl;

    //     // cout << HEX32_STRING(data[0]) << endl;

    //     // for(auto it = din.begin(); it != din.end(); it++) {
    //     // }
    //     // dump_fine_ostream->close();
    // }

}

void EventDsp::handleAllSc(const uint32_t seq, const uint32_t* data, const uint32_t len) {

    if(len!=1024) {
        cout << "handleAllSc got wrong length" << endl;
        return;
    }

    bufferevent_write(all_sc_buf->in, &seq, 1*4);
    bufferevent_write(all_sc_buf->in, data, len*4);

    // for(uint32_t i = 0; i < 1024; i++) {
    //     all_sc_buf.enqueue(data[i]);
    // }
}

void EventDsp::updateReceivedCounter(const uint32_t counter) {
    // constexpr unsigned period = 128;
    constexpr auto avg = 3u; // ends up being 4 somehow

    auto now = std::chrono::steady_clock::now();

    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    now - _track_counter_timepoint
    ).count();

    auto delta = counter - _previous_received_counter;

    // cout << "Delta time: " << elapsed_us << ", delta counter: " << delta << endl;

    _track_counter_history.push_back(std::pair<size_t, uint32_t>(elapsed_us, delta));


    if( _track_counter_history.size() >= avg ) {
        size_t xus;
        uint32_t ycounter;
        size_t time_sum = 0;
        uint32_t counter_sum = 0;
        double sum = 0.0;
        for( auto packed : _track_counter_history ) {
            std::tie(xus,ycounter) = packed;
            // cout << "xus " << xus << " ycounter " << ycounter << endl;
            time_sum += xus;
            counter_sum += ycounter;

            sum += (double) ycounter / (xus/1E6);// << endl;
            // cout << "    " << (double) ycounter / (xus/1E6) << endl;
        }
        double result;
        // result = ((double)counter_sum) / time_sum / (_track_counter_history.size()) / 4E6;
        result = sum / _track_counter_history.size();

        double slowdown = SCHEDULE_FRAMES_PER_SECOND / result;
        // cout << "result: " << result << endl;
        // cout << "slowdown: " << slowdown << endl;

        soapy->settings->vectorSet(slowdown, (std::vector<std::string>){"runtime","slowdown"});

    }

    if( _track_counter_history.size() > avg ) {
        _track_counter_history.erase(_track_counter_history.begin());
    }

    _track_counter_timepoint = now;
    _previous_received_counter = counter;

    // _track_counter_history

}

static uint32_t rearrange[32];

// void EventDsp::handleDemodSubcarrier(uint32_t sample) {
//     cout << sample << endl;
// }

// on event thread
// see EventDsp.cpp handleParsedFrameForPc()
void EventDsp::handleParsedFrame(const uint32_t* full_frame, const size_t length_words) {
    // controls

    // soapy->_debug_crash = 1;
    // drain_feedback_bus_objects = true;
	
	if(0) { // Dump all feedback bus for PC
    // for (uint32_t i = 0; i < length_words; i++) {
    //     std::cout << HEX32_STRING(full_frame[i]) << "\n";
    // }
	}

    static int track_counter = 0;
    static uint64_t track_us = 0;

    std::chrono::steady_clock::time_point track_a = std::chrono::steady_clock::now();

    constexpr int track_loop = 1024;

    if( track_counter >= track_loop ) {
        // cout << "track " << track_us << " (" << (double)track_us /(double)track_loop << "\n";
        track_counter = 0;
        track_us = 0;
    }
    track_counter++;

    uint32_t track_type = 0;
    uint32_t track_stype = 0;
    (void)track_type;
    (void)track_stype;



    const feedback_frame_t *v;

    // generic type
    v = (const feedback_frame_t*) full_frame;

    // cout << "Parsed: " << v->type << " " << v->length << endl;

    if( v->destination0 & FEEDBACK_PEER_SELF ) {
        // cout << "Message for Self Peer" << endl;

        // soapy->_debug_crash = 2;
        track_type = v->type;

        if( v->type == FEEDBACK_TYPE_STREAM ) {
            // soapy->_debug_crash = 3;
            // cast to a stream type
            const feedback_frame_stream_t *s = (const feedback_frame_stream_t *)v;
            (void)s;
            // there is another sequence_number defined below
            uint32_t sequence_number = s->seq;
            track_stype = s->stype;
            if( s->stype == FEEDBACK_STREAM_DEFAULT ) {

                #ifdef USE_DYNAMIC_TIMERS
                if( (_track_received_counter_counter & (128-1)) == 0 ) {
                    // cout << sequence_number << endl;
                    updateReceivedCounter(sequence_number);
                }
                _track_received_counter_counter++;
                #endif


                valid_count_default_stream++;

                // cout << "s: " << s->seq << endl;
                
                // convert start of data into char pointer
                const unsigned char* d_start = (const unsigned char*)(full_frame+FEEDBACK_HEADER_WORDS);
                // conert length of data into char length
                uint32_t d_char_len = (length_words-FEEDBACK_HEADER_WORDS)*4;

                // FIXME can this cause deadlock? how to spawn an event for this?
                if(drain_gnurado_stream) {

                    #ifdef DEBUG_PRINT_LOOPS
                        static int calls = 0;
                        calls++;
                        // if( calls % 100000 == 0) {
                        if( calls % 200000 == 0) {
                            cout << "handleParsedFrame() bufferevent_write(gnuradio_bev->in" << endl;
                        }
                    #endif

                    // write data in bytes, most other ev buffers are in words
                    bufferevent_write(gnuradio_bev->in, d_start, d_char_len);
                    // cout << "write to gnuradio_bev->in" << endl;

                    // soapy->_debug_crash = 5;
                }


                // also get as a word

                // Initializing equalization vector
                // FIXME: Place in constructor
                if (init_eq) {
                    init_eq_buffer(eq_buffer, array_length);
                    init_eq =false;
                    std::cout << "Equalization vector initialized" << std::endl;
                }

                // cast default stream from chars to ints
                const uint32_t* const int_d_start = (const uint32_t* const)d_start;

                // mmse code
                // see https://observablehq.com/@drom/blind-mmse
                //uint32_t q = 12;
                //unsigned int step = 2;
                //cf64_ci32_unpack_vector(int_d_start, input_buffer, q, num_points, step);
                //mmse_freq_vector(input_buffer, output_buffer, eq_buffer, num_points);
                //evm_vector(output_buffer, evm_buffer, num_points);
                // for (int i = 0; i < num_points; i++) {
                //     // std::cout << std::hex << "Eq Index[" << i << "]: " << eq_buffer[i] << " " << eq_buffer[i+1] << std::endl; 
                //     std::cout << std::hex << "EVM Index[" << i << "]: " << eq_buffer[i] << std::endl;
                // }



                // Copy this pilot tone for a quick coarse sync
                if(false) {
                    // s-modem coarse is disabled
                    uint32_t temp_coarse[2];
                    temp_coarse[0] = sequence_number;
                    temp_coarse[1] = int_d_start[COARSE_TONE_INDEX];
                    bufferevent_write(coarse_bev->in, temp_coarse, 2*4);
                }


                ///////////
                //
                //  copy some subcarrier's to another buf
                //
                // uint32_t d_word_len = (length_words-FEEDBACK_HEADER_WORDS);
                // (void)d_word_len;

                // TDMA_STATE_0
                // if(radios[0]->radio_state == 69){
                //     for(auto i = 0; i < d_word_len/2; i++ ) {

                //         if( 1+i*2  == 31) {
                //             cout << HEX32_STRING(int_d_start[i]) << endl;
                //         }

                //     }
                // }

                // these two will result in 65 words being written
                // bufferevent_write(demod_bev->in, &sequence_number, 4);
                // bufferevent_write(demod_bev->in, int_d_start, d_char_len);

                // soapy->_debug_crash = 100;
                // size_t demod_put[] = {30,31};
                // for(int index_enqueue=0; index_enqueue<DATA_TONE_NUM; index_enqueue++)
                // {
                
                /// take lock, copy to stack
                /// this could be copied less often
                std::vector<unsigned> enabled_sc;
                {
                    std::unique_lock<std::mutex> lock(_demod_buf_enabled_mutex);
                    enabled_sc = demod_buf_enabled_sc;
                }

                // demod_buf_enabled_sc comes from getIndexForDemodTDMA()
                // and it looks something like {30,31}

                for(unsigned i = 0; i < enabled_sc.size(); i++) {
                    unsigned data_index = enabled_sc[i];
                    unsigned reverse_mover_index = 1+data_index*2;
                    demod_buf[data_index].enqueue(
                        int_d_start[reverse_mover_index],
                        sequence_number
                    );
                }



                if(true) {

                    for(unsigned i = 0; i < EVM_TONE_NUM; i++) {
                        const unsigned data_index = i;
                        unsigned reverse_mover_index = 1+data_index*2;
                        rearrange[i] = int_d_start[reverse_mover_index];
                        // evbuffer_add(tmp, &(int_d_start[reverse_mover_index]), sizeof(uint32_t));
                    }
                    bufferevent_write(soapy->evmThread->data_bev->in, rearrange, sizeof(uint32_t)*EVM_TONE_NUM);

                    // bufferevent_write_buffer(soapy->evmThread->data_bev->in, tmp);
                }


                if(false) {

                    for(unsigned i = 0; i < EVM_TONE_NUM; i++) {
                        // const unsigned data_index = i;
                        // unsigned reverse_mover_index = 1+data_index*2;
                        // evbuffer_add(tmp, &(int_d_start[reverse_mover_index]), sizeof(uint32_t));
                    }
                    // bufferevent_write(soapy->evmThread->data_bev->in, &int_d_start[reverse_mover_index], sizeof(uint32_t));

                    // bufferevent_write_buffer(soapy->evmThread->data_bev->in, tmp);
                }

                if(false) {
                    struct evbuffer *tmp = evbuffer_new();

                    for(unsigned i = 0; i < EVM_TONE_NUM; i++) {
                        const unsigned data_index = i;
                        unsigned reverse_mover_index = 1+data_index*2;
                        evbuffer_add(tmp, &(int_d_start[reverse_mover_index]), sizeof(uint32_t));
                    }

                    bufferevent_write_buffer(soapy->evmThread->data_bev->in, tmp);
                    evbuffer_free(tmp);
                }

                // for(int data_index = 0; data_index < DATA_TONE_NUM; data_index++) {
                //     int reverse_mover_index = 1+data_index*2;
                //     demod_buf[data_index].enqueue(
                //         int_d_start[reverse_mover_index],
                //         sequence_number
                //     );
                // }
                    // soapy->_debug_crash = 100 + index_enqueue;
                // }

                // soapy->_debug_crash = 200;


                // for (int index_enqueue = 0; index_enqueue<16; index_enqueue++)
                // {
                //     demod_buf.enqueue(int_d_start[1+index_enqueue*2]); // subcarrier 1 is 945
                // }

            }
            if( s->stype == FEEDBACK_STREAM_ALL_SC ) {
                // soapy->_debug_crash = 300;
                const uint32_t* const d_start = (const uint32_t* const)(full_frame+FEEDBACK_HEADER_WORDS);
                // conert length of data into char length
                uint32_t d_word_len = (length_words-FEEDBACK_HEADER_WORDS);

                static bool do_once = false;
                if(do_once) {
                    for(uint32_t kk = 0; kk < 1024; kk++) {
                        cout << d_start[kk] << endl;
                    }
                    do_once = false;
                }
                // cout << "s << d_start[0] << endl;

                handleAllSc(sequence_number, d_start, d_word_len);


                // shoot to gnuradio, this bool is copied from settings object
                if( drain_gnurado_stream_2 ) {
                    const unsigned char* const d_char_start = (const unsigned char* const)d_start;
                    uint32_t d_char_len = (length_words-FEEDBACK_HEADER_WORDS)*4;
                    bufferevent_write(gnuradio_bev_2->in, d_char_start, d_char_len);
                }

                valid_count_all_sc_stream++;
            }
        } // stream type

        if( v->type == FEEDBACK_TYPE_VECTOR ) {
            // soapy->_debug_crash = 400;
            // cast to a vector type
            const feedback_frame_vector_t *vec = (const feedback_frame_vector_t *)v;
            // this is not the same as the one above
            uint32_t sequence_number = vec->seq;
            (void)sequence_number;

            track_stype = vec->vtype;

            if( vec->vtype == FEEDBACK_VEC_FINE_SYNC ) {
                // soapy->_debug_crash = 500;
                // pass enture vec,
                // can get seq from inside
                handleFineSync(vec);
                // soapy->_debug_crash = 600;

                valid_count_fine_sync++;
            }

            if( vec->vtype == FEEDBACK_VEC_DEMOD_DATA ) {
                // uint32_t d_word_len = (length_words-FEEDBACK_HEADER_WORDS);
                // cout << "Alt length FEEDBACK_VEC_DEMOD_DATA " << d_word_len << endl;

                // for(unsigned i = 0; i < length_words; i++) {
                //     cout << HEX32_STRING(full_frame[i]) << endl;
                // }
                // cout << endl;

                // std::chrono::steady_clock::time_point track_c = std::chrono::steady_clock::now();

                handleSlicedData(vec);

                // std::chrono::steady_clock::time_point track_d = std::chrono::steady_clock::now();
                // size_t elapsed_us2 = std::chrono::duration_cast<std::chrono::microseconds>( 
                // track_d - track_c
                // ).count();
                // cout << "us " << elapsed_us2 << "\n";
            }



        }

        // if( v->destination1 & FEEDBACK_DST_HIGGS ) {
        //     cout << "ERROR: Message for Higgs, but CAME from higgs" << endl;
        // }
    
    }

    if( v->destination1 & FEEDBACK_DST_PC ) {
        // cout << "Message for PC" << endl;
    }

    // FIXME: for now we are forwarding to everyone
    // we should improve this based on 
    if( v->destination1 & FEEDBACK_PEER_ALL ) {

        // cout << "would send to zmq, stream " << v->type << " dst: " <<
        // HEX32_STRING(v->destination1) << endl;

        // size_t word_len = std::max((size_t)16, length_words);

        // zmqPublish("@0", (char*)full_frame, word_len*4 );
    }

    // cout << "dst0: " << HEX32_STRING(v->destination0) << endl;
    // cout << "dst1: " << HEX32_STRING(v->destination1) << endl;

    std::chrono::steady_clock::time_point track_b = std::chrono::steady_clock::now();

    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    track_b - track_a
    ).count();

    if(elapsed_us > 100) {
        // cout << elapsed_us << "\n";
        // cout << elapsed_us << " " << track_type << "," << track_stype << "\n";
    } 

    track_us += elapsed_us;

}

// in words
#define LINEAR_SIZE (2048)

// writes status messages
///
/// Timer that fires and estimates how many messages RX should have gotten from
/// rx chain / feedback bus
///
void EventDsp::statusTimerVerifyRxDataRates() {
    const std::string path = "rx.attached.fb.datarate";
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    // double elapsed_seconds = (double)std::chrono::duration_cast<std::chrono::microseconds>( 
    // now - watch_status_timepoint
    // ).count() / 1E6;

    // cout << "statusTimerVerifyDataRates() " << endl;
    // cout << "Elapsed seconds " << elapsed_seconds << endl;
    // cout << "valid_count_default_stream  " << valid_count_default_stream << endl;
    // cout << "valid_count_all_sc_stream  " << valid_count_all_sc_stream << endl;
    // cout << "valid_count_fine_sync  " << valid_count_fine_sync << endl;
    // cout << endl;

    auto d0 = (int)valid_count_default_stream - (int) valid_count_default_stream_p;
    auto d1 = (int)valid_count_all_sc_stream - (int) valid_count_all_sc_stream_p;
    auto d2 = (int)valid_count_fine_sync - (int) valid_count_fine_sync_p;

    valid_count_default_stream_p = valid_count_default_stream;
    valid_count_all_sc_stream_p = valid_count_all_sc_stream;
    valid_count_fine_sync_p = valid_count_fine_sync;

    int d0_expected = 24512;
    int d1_expected = 24;
    int d2_expected = 24512;

    int d0_delta = d0_expected - d0;
    int d1_delta = d1_expected - d1;
    int d2_delta = d2_expected - d2;
    (void)d1_delta;
    (void)d2_delta;

    // cout << "d0_delta  " << d0_delta << endl;
    // cout << "d1_delta  " << d1_delta << endl;
    // cout << "d2_delta  " << d2_delta << endl;

    bool do_print = false;

    if( !GET_SKIP_CHECK_FB_DATARATE() ) {

        std::string slow_msg = std::string(" ") + std::to_string(d0) + std::string(" < ") + std::to_string(d0_expected);

        if( d0_delta < 18000  ) {
            status->set(path, STATUS_OK, "data streaming in ok");
        } else if( d0_delta > 18000 && d0_delta < d0_expected ) {
            status->set(path, STATUS_WARN, std::string("data streaming too slow")+slow_msg);
        } else {
            do_print = true;
            status->set(path, STATUS_ERROR, std::string("data streaming stopped")+slow_msg);
        }

    } else {
        status->set(path, STATUS_OK, "data streaming forced ok");
    }



    watch_status_timepoint = now;
    
    if( do_print && GET_RADIO_ROLE() == "rx" ) {
        // if we get this in duplicate, we can remove it
        cout << "Duplicate:: " << status->getNiceText(path) << endl;
    }

}

// consdered fb_buf member,
// if we have enough words in there, we can pull out feedback_bus frames
// this is written to be called as many times as possible without ill effect
// now part of EventDsp
void EventDsp::parseFeedbackBuffer(struct evbuffer *input) {

    size_t fb_buf_size_bytes = evbuffer_get_length(input); // in bytes


    #ifdef DEBUG_PRINT_LOOPS
    static int calls = 0;
    calls++;
    if( calls % 10000 == 0) {
        cout << "parseFeedbackBuffer() " << fb_buf_size_bytes << endl;
    }
    #endif


    size_t make_linear_size_words = std::min(fb_buf_size_bytes/4, (size_t)LINEAR_SIZE);

    uint32_t* fb_buf_data = (uint32_t*) evbuffer_pullup(input, make_linear_size_words*4);
    // assert(fb_buf_data != 0);


    // FIXME ??
    size_t fb_buf_size = make_linear_size_words;

    uint32_t valid_consumed = 0;

    for(uint32_t i = 0; i < fb_buf_size; /*empty*/ ) {
        uint32_t word = fb_buf_data[i];

    #ifdef DEBUG_PRINT_LOOPS
    static int calls2 = 0;
    calls2++;
    if( calls2 % 1000000 == 0) {
        cout << "for(uint32_t i = 0; i < fb_buf_size; /*empty*/ ) {" << endl;
    }
    #endif

        feedback_frame_t *v;

        // use uint32_t pointer arithmatic to and then do final cast before asignign
        v = (feedback_frame_t*) (((uint32_t*)fb_buf_data)+i);

        uint32_t advance = 1;
        bool adv_error = false;
        
        if(word != 0) {

            if((i+1)+16 > fb_buf_size) {
                // cout << "Breaking loop at word #" << i << " because header goes beyond received words" << endl;
                break;
            }

            // cout << endl;
            // print_feedback_generic(v);

            advance = feedback_word_length(v, &adv_error);

            if( (i+1 + advance) > fb_buf_size ) {
                // cout << "header + body goes beyond data we have" << endl;
                break;
            } else {
                if(!adv_error) {
                    handleParsedFrame( (((uint32_t*)fb_buf_data)+i), advance );

                    // soapy->_debug_crash = 700;
                }
            }


            if(fb_jamming != -1) {
                cout << "Was fb_Jamming was for " << fb_jamming << endl;
                fb_jamming = -1;
            }
        } else {
            if(fb_jamming == -1) {
                fb_jamming = 1;
            } else {
                fb_jamming++;

                if( fb_jamming == 367*2 ) {
                    got_jam = true;
                }
            }
        }




        if( adv_error ) {
            if( got_jam ) {
                // only report this error after we get the first jam
                cout << "Hard fail when parsing word #" << i << endl;
            }
            advance = 1;
            fb_error_count++;
        }

        if( advance != 1 ) {
            // if zero we want to print because that's wrong
            // if 1 we don't want to spam during a flushing section
            // if larger we want to print because they are few
            // cout << "Advance " << advance << endl;
        }

        i += advance;
        valid_consumed += advance;

        // soapy->_debug_crash = 10000 + i;
    }

    // uint32_t erase_start = 0;
    // uint32_t erase_length = valid_consumed;
    // fb_buf.erase(fb_buf.begin()+erase_start, fb_buf.begin()+erase_start+erase_length);

    // soapy->_debug_crash = 20000;
    evbuffer_drain(input, valid_consumed*4);
    // soapy->_debug_crash = 20001;
    // cout << "drained " << valid_consumed*4 << endl;
}

// pass raw buffers from rx socket (without seq number)
// void EventDsp::parseFeedbackBusEntryPoint(unsigned char* data, size_t length) {
//     size_t l2 = length / 4;
//     uint32_t* u_data = (uint32_t*) data;
//     for(size_t i = 0; i < l2; i++) {
//         fb_buf.push_back(u_data[i]);
//     }
//     parseFeedbackBuffer();
// }
static void handle_watch_data_tick(int fd, short kind, void *_dsp)
{
//     // cout << "handle_fsm_tick" << endl;
    EventDsp *dsp = (EventDsp*) _dsp;
    dsp->statusTimerVerifyRxDataRates();


    auto t1 = _1_SECOND_SLEEP;
    evtimer_add(dsp->watch_status_data_timer, &t1); // fire once now
}

void EventDsp::setupFeedbackBus() {
    got_jam = false;
    // feedback_type_errors = 0;
    fb_error_count = 0;
    fb_jamming = -1;
    feedback_bus_print_time = std::chrono::steady_clock::now();

    auto time_now = std::time(0);

    dump_fine_filename = "fine_sfo_sync_" + to_string(time_now) + ".hex";
    dump_fine_cfo_filename = "fine_cfo_sync_" + to_string(time_now) + ".hex";
    if(dump_fine_sync) {
        std::cout << "opening " << dump_fine_filename << " for writing" << std::endl;
        std::cout << "opening " << dump_fine_cfo_filename << " for writing" << std::endl;
        // std::ofstream outFile(filename);
        dump_fine_ostream = new ofstream(dump_fine_filename);
        dump_fine_cfo_ostream = new ofstream(dump_fine_cfo_filename);
    }

    parse_fine_history.resize(4);
    parse_stream_history.resize(1);

// watch_status_data_timer


    // only fire up this timer if we are on the RX or we are running a data stream from
    // cs20
    // if we don't fire up, the only things we don't set are
    // valid_count_fine_sync
    // valid_count_all_sc_stream
    // valid_count_default_stream
    if( role != "arb") {
        watch_status_data_timer = evtimer_new(evbase, handle_watch_data_tick, this);
        auto t1 = _1_SECOND_SLEEP;
        evtimer_add(watch_status_data_timer, &t1); // fire once now
    }


    (void)input_buffer;
    (void)output_buffer;



    watch_status_timepoint = std::chrono::steady_clock::now();
}


// fixme, change to event timer
//     printFeedbackBusStatus();
void EventDsp::printFeedbackBusStatus() {

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    constexpr uint32_t ms_print_period = 3*1000;

    std::chrono::duration<double> print_elapsed_time = now-feedback_bus_print_time;
    if( print_elapsed_time > std::chrono::milliseconds(ms_print_period) )
    {
        // cout << "Feedback Bus Errors: " << fb_error_count << endl;
        feedback_bus_print_time = now;
    }
}

