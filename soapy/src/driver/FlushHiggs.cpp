#include "FlushHiggs.hpp"
#include <future>

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>

using namespace std;
#include "vector_helpers.hpp"

#include "driver/EventDsp.hpp"
#include "driver/RadioEstimate.hpp"
#include "driver/AttachedEstimate.hpp"
#include "driver/AirPacket.hpp"
#include "driver/TxSchedule.hpp"

#define SENDING_IDLE 0
#define SENDING_NORMAL 1
#define SENDING_LOW 2


///
/// There is new data ready to be sent out to higgs UDP
///
static void _handle_for_higgs_callback(struct bufferevent *bev, void *_dsp) {
     // cout << "_handle_for_higgs_callback" << endl;
    
    // FlushHiggs *dsp = (FlushHiggs*) _dsp;
    // dsp->handleHiggsCallback(bev);
}

static void _handle_for_higgs_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_rb_event" << endl;
}



///
/// There is new data ready to be sent out to higgs UDP
/// but the data came over zmq
///
static void _handle_for_higgs_via_zmq_callback(struct bufferevent *bev, void *_dsp) {
     // cout << "_handle_for_higgs_via_zmq_callback" << endl;
    
    // FlushHiggs *dsp = (FlushHiggs*) _dsp;

    // if(dsp->allow_zmq_higgs) {
    //     struct evbuffer *buf  = bufferevent_get_input(bev);
    //     dsp->petSendToHiggs(buf);
    // } else {
    //     cout << "_handle_for_higgs_via_zmq_callback deffering due to dsp->allow_zmq_higgs" << endl;
    // }

}

static void _handle_for_higgs_via_zmq_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_rb_event" << endl;
}




/////////////////////////////
///
/// for send to higgs
/// blah

static void _handle_sendto_higgs_timer(int fd, short kind, void *_dsp) {
    FlushHiggs *dsp = (FlushHiggs*) _dsp;

    // grab this pointer off the class.
    // because this is the TIMER event, not the NEW DATA callback
    // we are forced to have this timer event because of they we need to drain the output
    // buffer

    struct evbuffer *buf = bufferevent_get_input(dsp->for_higgs_bev->out);

    size_t len = evbuffer_get_length(buf);

    // FIXME this will only work if we only have one instance of FlushHiggs (probably ok assumption)
    static int counter = 0;

    if( counter % (4*40) == 0 && len != 0) {
        cout << "len: " << len << endl;
    }
    counter++;

    dsp->updateDrainRate();

    dsp->petSendToHiggs(buf);

    // if(dsp->allow_zmq_higgs) {
    //     struct evbuffer *buf_low_pri = bufferevent_get_input(dsp->for_higgs_via_zmq_bev->out);
    //     dsp->petSendToHiggs(buf_low_pri);
    // }

    struct timeval val = { .tv_sec = 0, .tv_usec = 512 };
    evtimer_add(dsp->sendto_higgs_timer, &val);
}

static std::vector<size_t> osReadUdpSocketBuff(std::vector<std::string> match) {

    ifstream myfile;
    myfile.open ("/proc/net/udp", ios::binary);

    if(!myfile) {
        cout << "couldn't read /proc" << endl;
    }


    std::vector<size_t> out;
    out.resize(match.size()*2);


    // port 40001
    // 0102020A:9C41



    // port 10001
    // 00000000:2711


    std::vector<size_t> found;
    found.resize(match.size(), 0);

    // port 


    // std::string myText("some-text-to-tokenize");
    // std::istringstream iss(myText);
    std::string token;
    std::string j1;
    std::string j2;
    std::string j3;
    std::string t2;
    std::string tx_queue;
    std::string rx_queue;
    std::string inodestr;

    // get to first line
    // std::getline(myfile, token, ':');

    int lines = 0;

    while(1) {
        if(!std::getline(myfile, t2, ':')) {
            break;
        }
        
        std::getline(myfile, t2, ':');
        std::getline(myfile, t2, ':');

        std::getline(myfile, j3, ' ');
        std::getline(myfile, j3, ' ');
        std::getline(myfile, tx_queue, ':');
        // cout << "tx_queue " << tx_queue << endl;

        std::getline(myfile, rx_queue, ' ');
        // cout << "rx_queue " << rx_queue << endl;



        // std::getline(myfile, t2, ':');
        std::getline(myfile, t2, ':');
        std::getline(myfile, t2, ' ');
        std::getline(myfile, t2, ' ');


        do {
            std::getline(myfile, j2, ' ');
        }while(j2.size() == 0);
        // cout << "first " << j2 << endl;
        do {
            std::getline(myfile, j2, ' ');
        }while(j2.size() == 0);
        // cout << "second " << j2 << endl;

        do {
            std::getline(myfile, inodestr, ' ');
        }while(inodestr.size() == 0);

        // cout << "third " << inodestr << endl;

        int i = 0;
        for( const auto &x : match ) {
            if( inodestr == x ) {
                // cout << "match: " << x << endl;
                int idx_a = i*2;
                int idx_b = i*2+1;

                out[idx_a] = std::stoi(tx_queue, 0, 16);
                out[idx_b] = std::stoi(rx_queue, 0, 16);

                found[i]++;

            }
            i++;
        }


        // while(j1.size)

        // std::cout << t2 << " <- " << t2.size() << std::endl;

        if(std::getline(myfile, j2, '\n'))
        {
            // break;
        }
        lines++;

        // if(!res) {
        //     break;
        // }
        // if( lines > 3 ) {
        //     break;
        // }
    }

    // cout << "matched lines: " << lines << endl;

    int i = 0;
    for( auto x : found ) {
        if( x != 1 ) {
            cout << "Index " << i << " was only found " << x << " times!" << endl;
        }
        i++;
    }

    // while (std::getline(myfile, token, ' '))
    // {
    //     std::cout << token << std::endl;
    // }

    return out;
}


///////////////////////////////////////////////////////////////
//
//  Above this line are static functions which comprise events for this class
//
FlushHiggs::FlushHiggs(HiggsDriver* s):
    
    //
    // class wide initilizers
    //
     HiggsEvent(s)
    ,valid_8k(false)
    ,cs20_early_flag(false)
    ,member_early_frames(3000)
    ,allow_zmq_higgs(false)

    //
    // function starts:
    //
{
    settings = (soapy->settings);

    sending_to = SENDING_IDLE;


    // pet_feed_higgs_as_tx = (soapy->dspEvents->role == "tx");
    updateFlags();
    

    // cout << "!!!!!!!!!!!!!!!! FlushHiggs::FlushHiggs() " << endl;
    // cout << soapy->dspEvents->role << endl;
    // cout << "with pet_feed_higgs_as_tx " << pet_feed_higgs_as_tx << endl;

    feed_higgs_tp = std::chrono::steady_clock::now();

    should_pause_p = false;
    requestExitSendingLow = false;

    enableBuffers();
    setupBuffers();

    socket_buffer_inodes.push_back(soapy->tx_sock_inode);
    socket_buffer_inodes.push_back(soapy->rx_sock_inode);

    // for( auto x : socket_buffer_inodes) {
    //     cout << "Inode: " << x << endl;
    // }

    soapy->_flush_higgs_ready = true;
}

void FlushHiggs::updateFlags() {
    print = GET_PRINT_TX_BUFFER_FILL();

    pad_zeros_before_low_pri = !GET_ALLOW_CS20_FB_CRASH();

    print_low_words = GET_PRINT_TX_FLUSH_LOW_PRI();

    print_evbuf_to_socket_burst = GET_PRINT_EVBUF_SOCKET_BURST();

#ifdef DEBUG_CHOOSE_RATE
    if( soapy->txScheduleReady() ) {

        AirPacketOutbound* airTx = soapy->txSchedule->airTx;
        if( airTx->modulation_schema == FEEDBACK_MAPMOV_QAM16 &&
            airTx->subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
            double fetch = settings->getDefault<double>(3.9, "runtime", "tx", "flush_higgs", "drain_rate");

            if( fetch != drain_rate ) {
                // cout << "Loading new drain_rate of " << fetch << "\n";
            }

            drain_rate = fetch;
        }
    }
#endif

}

void FlushHiggs::stopThreadDerivedClass() {

}


static std::string printState(int v) {
    switch(v) {
        case SENDING_IDLE:
            return "SENDING_IDLE";
            break;
        case SENDING_NORMAL:
            return "SENDING_NORMAL";
            break;
        case SENDING_LOW:
            return "SENDING_LOW";
            break;
        default:
            break;
    }
    return "";
}

void FlushHiggs::updateSendingFlag(int caller) {
    struct evbuffer *buf_normal = bufferevent_get_input(for_higgs_bev->out);
    struct evbuffer *buf_low_pri = bufferevent_get_input(for_higgs_via_zmq_bev->out);

    const size_t len_normal = evbuffer_get_length(buf_normal);
    const size_t len_low = evbuffer_get_length(buf_low_pri);

    if( printSending ) {
        cout << "len_normal " << len_normal << endl;
        cout << "len_low " << len_low << endl;
        cout << printState(sending_to) << endl;
    }

    if( requestExitSendingLow ) {
        if( printSending2 ) {
            cout << "requested exit sending low: ";
        }
        requestExitSendingLow = false;
        if( sending_to == SENDING_LOW ) {
            if( printSending2 ) {
                cout << "went from " << printState(sending_to) << " to ";
            }
            sending_to = SENDING_IDLE;
            if( printSending2 ) {
                cout << printState(sending_to) << endl;
            }
        } else {
            if( printSending2 ) {
                cout << "was already in " << printState(sending_to) << endl;
            }
        }
    }

    int sending_to_p = sending_to;

    switch(sending_to) {
        case SENDING_IDLE:
            if(len_normal != 0) {
                sending_to = SENDING_NORMAL;
            } else if( len_low != 0 && allow_zmq_higgs ) {
                sending_to = SENDING_LOW;
            }
            break;
        case SENDING_NORMAL:
            if(len_normal == 0) {
                if(len_low != 0 && allow_zmq_higgs ) {
                    sending_to = SENDING_LOW;
                } else {
                    sending_to = SENDING_IDLE;
                }
            }
            break;
        case SENDING_LOW:
            if(len_low == 0) {
                if(len_normal != 0) {
                    sending_to = SENDING_NORMAL;
                } else {
                    sending_to = SENDING_IDLE;
                }
            }
            break;
        default:
            cout << "Illegal state " << sending_to << "in updateSendingFlag()" << endl;
            sending_to = SENDING_IDLE;
    }

    if( printSending2 ) {
        if( sending_to_p != sending_to ) {
            cout << "with " << allow_zmq_higgs
                << " zmq, switched from "
                << printState(sending_to_p) << " to " << printState(sending_to) << " (" << caller << ")" << endl;
        }
    }
}

void FlushHiggs::exitSendingLowMode() {
    requestExitSendingLow = true;
}

size_t FlushHiggs::getNormalLen() {
    struct evbuffer *buf_normal = bufferevent_get_input(for_higgs_bev->out);
    const size_t len_normal = evbuffer_get_length(buf_normal);
    return len_normal;
}

size_t FlushHiggs::getLowLen() {
    struct evbuffer *buf_low = bufferevent_get_input(for_higgs_via_zmq_bev->out);
    const size_t len = evbuffer_get_length(buf_low);
    return len;
}

void FlushHiggs::sendZmqHiggsNow(std::string who) {

    // struct evbuffer *buf = bufferevent_get_input(for_higgs_via_zmq_bev->out);


    struct evbuffer *buf_normal = bufferevent_get_input(for_higgs_bev->out);
    struct evbuffer *buf_low_pri = bufferevent_get_input(for_higgs_via_zmq_bev->out);

    const size_t len_normal = evbuffer_get_length(buf_normal);
    size_t len_low = evbuffer_get_length(buf_low_pri);

    if( dump_low_pri_buf ) {
        cout << "DUMPING LOW PRI " << len_low << "\n";
        evbuffer_drain(buf_low_pri, len_low);
        len_low = 0;
    }

    updateSendingFlag(1);

    if( len_low == 0 ) {
        // cout << "0 in sendZmqHiggsNow()" << endl;
        return;
    }


    bool allowed_to_send_low_priority = (sending_to == SENDING_LOW);

    if( allowed_to_send_low_priority ) {
        cout << "ALLOWED to send in sendZmqHiggsNow() " << len_low << endl;
    } else {
        cout << "NOT ALLOWED to send in sendZmqHiggsNow() " << len_low << ", " << len_normal << endl;
        return;
    }

    inspectLowPri(who);

#ifdef asdfasdfasd
    // this number is the upper bounds we are allowed to send this moment
    evbufToSocketBurst(buf_low_pri, (1024+16)*4);

    size_t new_len = evbuffer_get_length(buf_low_pri);

    if( new_len != 0 ) {
        cout << "sendZmqHiggsNow() had " << new_len << " left" << endl;
    }
#endif
}

void FlushHiggs::inspectLowPri(std::string who) {
    struct evbuffer *buf_low_pri = bufferevent_get_input(for_higgs_via_zmq_bev->out);
    size_t in_buffer = evbuffer_get_length(buf_low_pri);


    size_t in_buffer_floored = in_buffer;
    in_buffer_floored = (in_buffer_floored / 4) * 4; // make sure a multiple of 4

    // size_t actual_read = in_buffer_floored;

    unsigned char* temp_read = evbuffer_pullup(buf_low_pri, in_buffer_floored);
    // unsigned char* data = temp_read;

    // uint32_t* temp_read_int = (uint32_t*)temp_read;
    if( print_low_words ) {
        uint32_t* temp_read_int = (uint32_t*)temp_read;
        for(unsigned i = 0; i < (in_buffer_floored/4); i++) {
            cout << HEX32_STRING(temp_read_int[i]) << " ";
            if( i % 16 == 15 ) {
                cout << "\n";
            }
        }
    }

    /////////////////////////
    // for(unsigned i = 0; i < (in_buffer_floored/4); i++) {
    //     cout << HEX32_STRING(temp_read_int[i]) << " ";
    //     if( i % 16 == 15 ) {
    //         cout << "\n";
    //     }
    // }

    uint32_t advance = 1;
    bool adv_error = false;

    advance = feedback_word_length((feedback_frame_t*)temp_read, &adv_error);

    if(!adv_error) {
        cout << "no advance error low pri " << advance << endl;
        print_feedback_generic((feedback_frame_t*)temp_read);

        // if( pad_zeros_before_low_pri ) {
        //     int sendto_ret;
        //     (void) sendto_ret;
        //     std::vector<uint32_t> zrs;
        //     zrs.resize(64,0);
        //     unsigned char* start = (unsigned char*)zrs.data();
        //     sendto_ret = sendto(soapy->tx_sock_fd, start, zrs.size()*4, 0, (struct sockaddr *)&(soapy->tx_address), soapy->tx_address_len);
        //     cout << "pre sending zeros " << zrs.size() << endl;
        // }

        if( (advance) >= (1024*8) ) {
            cout << " WE WILL HAVE A SPLIT CHECK PROBLEM IN LOW PRI evbufToSocketBurst() !!!\n";
        }

        evbufToSocketBurst(buf_low_pri, in_buffer, who);

        // evbuffer_drain(buf_low_pri, advance*4);

    } else {
        cout << "advance error low pri      !!!!!!!!!!!!!!!!!!" << endl;
        cout << "Draining entire zmq buffer !!!!!!!!!!!!!!!!!!" << endl;

        evbuffer_drain(buf_low_pri, in_buffer);
    }
}

void FlushHiggs::dumpLow(void) {
    struct evbuffer *buf_low_pri = bufferevent_get_input(for_higgs_via_zmq_bev->out);
    size_t in_buffer = evbuffer_get_length(buf_low_pri);
    evbuffer_drain(buf_low_pri, in_buffer);
}

void FlushHiggs::dumpNormal(void) {
    struct evbuffer *buf_normal = bufferevent_get_input(for_higgs_bev->out);
    size_t in_buffer = evbuffer_get_length(buf_normal);
    evbuffer_drain(buf_normal, in_buffer);
}

// 
void FlushHiggs::handleHiggsCallback(struct bufferevent *bev) {


    struct evbuffer *buf  = bufferevent_get_input(bev);
    // size_t len = evbuffer_get_length(buf);

    // call it this way, with an event, this guarentess there is new data
    petSendToHiggs(buf);


}

static void slow_update_flags(FlushHiggs* that) {
    static int delay_update_flags = 0;

    // 32 was about every 100 us
    // so this should be about every 128ms
    constexpr int mask = (4096-1);

    if( (delay_update_flags & mask) == 0 ) {
        that->updateFlags();
    }
    delay_update_flags++;
}


uint32_t FlushHiggs::petSendToHiggs(struct evbuffer *buf) {

    struct evbuffer *buf_low_pri = bufferevent_get_input(for_higgs_via_zmq_bev->out);
    size_t len_low = evbuffer_get_length(buf_low_pri);

    updateSendingFlag(2);

    if(sending_to != SENDING_NORMAL) {
        if( sending_to == SENDING_LOW ) {
            if( allow_zmq_higgs ) {
                cout << "petSendToHiggs automatically sent low pri: " << len_low << endl;
                sendZmqHiggsNow();
            } else {
#ifdef PRINT_SEND_TO_HIGGS_SOCKET_CONFLICT
                // old behavior
                cout << "petSendToHiggs did not send becuase low pri: " << len_low << endl;
#endif
            }
        }
        slow_update_flags(this);
        return 0;
    }

    auto val = petSendToHiggs2(buf);

    int max_runs = 80000;
    for(int i = 0; i < max_runs; i++) {

        if( i >= max_runs ) {
            break;
        }    

        val = petSendToHiggs2(buf);
    }

    slow_update_flags(this);


    return val;
}

std::pair<uint32_t, unsigned long> FlushHiggs::fillAge() {

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::chrono::steady_clock::time_point a;
    uint32_t b;
    {
        std::unique_lock<std::mutex> lock(_fill_mutex);
        a = last_fill_taken_at;
        b = last_fill_level;
    }

    auto age = std::chrono::duration_cast<std::chrono::microseconds>( 
                now - a
                ).count();
    return std::pair<unsigned long, uint32_t>(b, age);
}

void FlushHiggs::consumeUserdataLatencyEarly(const uint32_t word) {
    cout << "consumeUserdataLatencyEarly() " << word << "\n";

    std::unique_lock<std::mutex> lock(_cs20_early_mutex);
    cs20_early_flag = true;
    cs20_early_rb = word;
}


void FlushHiggs::consumeRingbusFill(const uint32_t word) {

    // _fill_mutex
    // take the time first
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    uint32_t data = word & 0x00ffffff;
    uint32_t cmd_type = word & 0xff000000;
    (void)cmd_type;

    // grab the lock
    {
        std::unique_lock<std::mutex> lock(_fill_mutex);
        last_fill_taken_at = now;
        last_fill_level = data;
    }


    // auto fill_age = std::chrono::duration_cast<std::chrono::microseconds>( 
    // now-last_fill_taken_at
    // ).count();

    static int do_print = 0;

    bool force_print_now = this->print_when_almost_full && (data>=7000);

    // print f: over and over
    if( force_print_now ||  (this->print && (do_print % 3 == 0))  )  {

        auto futWork = std::async( std::launch::async, [](
            uint32_t _data,
            std::chrono::steady_clock::time_point _now,
            std::chrono::steady_clock::time_point _feed_higgs_tp
            ) {

                auto fill_taken_at = std::chrono::duration_cast<std::chrono::microseconds>( 
                _now - _feed_higgs_tp
                ).count();

                std::cout << "     f: " << _data << " " << fill_taken_at << "\n";
        }, data, now, this->feed_higgs_tp );
    }

    do_print++;


    // cout << "fill: " << data << endl;
}

void FlushHiggs::checkOsUdpBuffers() {
    return;
    static int foo = 0;
    foo++;
    if( foo % 50 == 0) {
        // run
    } else {
        return;
    }

    auto res = osReadUdpSocketBuff(socket_buffer_inodes);

    int i = 0;
    for( auto x : res ) {
        if( x != 0 ) {
            cout << i << ": " << x << endl;
        }
        i++;
    }
}

void FlushHiggs::updateDrainRate() {
    AirPacketOutbound* airTx = soapy->txSchedule->airTx;

    // drain_rate is in bytes per us

    if( airTx->modulation_schema == FEEDBACK_MAPMOV_QPSK ) {
        drain_rate = 1.6;
        max_packet_burst = 8;
        do_force_boost = false;
    }

    if( airTx->modulation_schema == FEEDBACK_MAPMOV_QAM16 &&
        airTx->subcarrier_allocation == MAPMOV_SUBCARRIER_128 ) {
        drain_rate = 1.6*2;
        max_packet_burst = 8;
        do_force_boost = false;
    }

    if( airTx->modulation_schema == FEEDBACK_MAPMOV_QAM16 &&
        airTx->subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {
        drain_rate = 3.90625;
        max_packet_burst = 8;
        do_force_boost = true;
    }

    if( airTx->modulation_schema == FEEDBACK_MAPMOV_QPSK &&
        airTx->subcarrier_allocation == MAPMOV_SUBCARRIER_320 ) {

        // drain_rate = (3.90625 / 2) * (85 / 128.0);  // DUPLEX
        // drain_rate = (3.90625 / 2); // original 


        drain_rate = (3.90625 / 2) * 0.9 ; // original  plus duplex try 2


        max_packet_burst = 8;
        do_force_boost = true;
    }

    if( airTx->modulation_schema == FEEDBACK_MAPMOV_QPSK &&
        airTx->subcarrier_allocation == MAPMOV_SUBCARRIER_640_LIN ) {
        drain_rate = 3.90625;
        max_packet_burst = 8;
        do_force_boost = true;
    }

    if( airTx->modulation_schema == FEEDBACK_MAPMOV_QAM16 &&
        airTx->subcarrier_allocation == MAPMOV_SUBCARRIER_640_LIN ) {
        drain_rate = 3.90625*2;
        max_packet_burst = 8;
        do_force_boost = true;
    }

    // cout << "choose drain rate " << drain_rate << "\n";

}

/*
before duplex
static int32_t calculateFrameDelta(const uint32_t word) {
    uint32_t dmode =      (word & 0x00ff0000) >> 16;
    int32_t frame_delta =  word & 0x0000ffff;

    // do this to sign extend
    frame_delta <<= 16;
    frame_delta >>= 16;

    int epoc_delta = (int8_t)dmode;

    // unwinds some values, I think this is just to save compute
    // in cs20
    while(0 < epoc_delta) {
        frame_delta += SCHEDULE_FRAMES;
        epoc_delta--;
    }

    // copy to class
    return frame_delta;
}
*/


/**
 * 8k mode
 * If we are asked to send more than 8k words, we timestamp this 
 * and set a flag valid_8k
 * From this we calculate an initial value of word which we want to put in the buffer
 * Then we wait until our estimated 3000 early frames
 * From this point we calculate the number of us based on the system clock
 * that we are allowed to send.  We keep track of sent words in words_8k_sent
 * This method was working very well but we were still having edge conditions
 * where we would either get the buffer too full or too low.  I noticed that
 * "epoc_delta represented as frames" was corresponding to these edge conditions
 * So we use the value reported in this ringbus to adjust our measurement of
 * How early the FPGA got the words.  This method is counter based and thus lag free
 * and works VERY well.
 *
 * 
 */
uint32_t FlushHiggs::petSendToHiggs2(struct evbuffer *buf) {

    // only on tx (for now)
    // if(!pet_feed_higgs_as_tx) {
    //     return 0;
    // }

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    // how long since we last run?
    auto us_run = 
        std::chrono::duration_cast<std::chrono::microseconds>( 
        now-time_prev_pet_send_higgs
        ).count();
    (void)us_run;

    // how long since we last sent anything?
    auto us_sent = 
        std::chrono::duration_cast<std::chrono::microseconds>( 
        now-time_prev_sent_pet_higgs
        ).count();


    auto lifetime = std::chrono::duration_cast<std::chrono::microseconds>( 
        now-feed_higgs_tp
        ).count();

    auto fill_age = std::chrono::duration_cast<std::chrono::microseconds>( 
        now-last_fill_taken_at
        ).count();

    size_t total_buffer_words = evbuffer_get_length(buf)/4;
    bool just_entered_8k = false;
    if( !valid_8k ) {
        // if we were asked to send 8k words aka more than cs20 buffer
        if( total_buffer_words >= 1024*8 ) {
            early_target_8k = member_early_frames; // copy this and use for the duration of this 8k
            valid_8k = true;
            just_entered_8k = true;
            time_8k_start = now;
            words_8k = total_buffer_words;
            words_8k_sent = 0;
            did_pause_8k = false;
            {
                std::unique_lock<std::mutex> lock(_cs20_early_mutex);
                cs20_early_flag = false;
            }
            early_frame_8k = 0;
            early_frame_8k_applied = false;
            packets_after_early_8k = 0;
            fill_when_enter_8k = last_fill_level;
            cout << "enter 8k " << fill_when_enter_8k << "\n";
        }
    } else {
        if( total_buffer_words == 0 ) {
            valid_8k = false;
            fill_when_enter_8k = 0;
            cout << "exit 8k\n";
        }
    }

    // where 3000 is number of frames early
    // 60 is an early edge we give ourselves
    // 60*40 is about 2000 which is about how many words
    // cs20 can drain from the buffer early due to vmem size
    // const double early_target_8k = 3000;
    double early_fudge = 0; // set to 0, buffer seems to ride at 2k
    int32_t after_early_holdover = 1024*2*4; // there is a period when the timer is negative, send this

    // seems like when s-modem is exceptionaly late to send, we dont need
    // this extra burst
    if( early_frame_8k_applied && early_frame_8k > 100 ) {
        after_early_holdover = 0;
    }

    if( packets_after_early_8k >= 3) {
        early_fudge = 30+14; // adds 2k buffers back
    }

    auto age_8k = std::chrono::duration_cast<std::chrono::microseconds>( 
        now-time_8k_start
        ).count();




    // cs20 will reply how early we actually sent the data
    // we can adjust our timers based on this
    if( !early_frame_8k_applied && cs20_early_flag ) {
        early_frame_8k_applied = true;
        uint32_t copy_rb;
        {
            std::unique_lock<std::mutex> lock(_cs20_early_mutex);
            copy_rb = cs20_early_rb;
        }


        const int32_t rb_delta = schedule_parse_delta_ringbus(copy_rb);
    cout << "early_frame_8k_applied with delta " << rb_delta << "\n";

        if( rb_delta > 0 ) {
            early_frame_8k = early_target_8k - rb_delta;
        }


        cout << "GOT EARLY OF " << rb_delta << "\n";

    }


    // double rate = 0.4*4; //0.2*4;

    double allowed2 = drain_rate*us_sent;

    int allowed_to_send = (int)allowed2;
    if(allowed_to_send < 0) {
        allowed_to_send = 1024*4;
        cout << "negative allowed" << endl;
    }

    const double early_us = (1.0/24414.0625) * (early_target_8k-early_frame_8k-early_fudge) * 1E6;

    const uint32_t initial_bump = max( (int32_t)((int32_t)(4*1024) - ((int32_t)fill_when_enter_8k*4)), 0 );     // words
    // constexpr uint32_t subsequent_bump = 2*1024;  // words
    if( just_entered_8k ) {
        // initial bump
        // words_8k_sent += initial_bump;
        allowed_to_send = initial_bump*4;
    } else if( valid_8k ) {
        if( age_8k < early_us ) {
            allowed_to_send = 0;
        } else {
            double delta = age_8k - early_us; // us since cs20 started draining
            allowed_to_send = drain_rate * delta - (((int32_t)words_8k_sent-(int32_t)initial_bump)*4);
            // cout << "pkt after: " << packets_after_early_8k << " amnt " << (int32_t)words_8k_sent-(int32_t)initial_bump << "\n";
            if( packets_after_early_8k == 0) {
                allowed_to_send = after_early_holdover;
                cout << "after_early_holdover " << after_early_holdover << "\n";
            }

            if(allowed_to_send < 0) {
                allowed_to_send = 0;
            }
            packets_after_early_8k++;
        }
    }





    // bool unpause_boost = false;


 
    bool attached_should_pause;

    // int flags = 0;

    if (should_pause_p && should_use_new_level) {
        // equation for when we ARE currently paused
        attached_should_pause = last_fill_level > (1024 * 5);
        // flags |= 0x1;
    } else {
        // equation for when we are not currently paused
        attached_should_pause = last_fill_level > (1024 * 3);
        // flags |= 0x01;
    }

    if( last_fill_level > (1024 * 5) ) {
        // cout << "use new level" << endl;
        should_use_new_level = true;
        // flags |= 0x001;
    }

    if( last_fill_level < (1024 * 3) ) {
        should_use_new_level = false;
        // flags |= 0x0001;
    }

    // if( last_fill_level == 0 ) {
    //     in_boost_mode = false;
    // }


    if( attached_should_pause != should_pause_p ) {

        // edge from don't pause, to pause
        if( attached_should_pause ) {
            // cout << "enter-pause (l: " << lifetime << ")" << endl;
        } else {
            // edge from pause, to don't pause
            // cout << "release-pause (l: " << lifetime << ") c: " << allowed_to_send/4 << endl;
        }

        should_pause_p = attached_should_pause;
    }

    if( !valid_8k && attached_should_pause ) {
        // if( valid_8k && !did_pause_8k ) {
        //     cout << "Pausing during 8k\n";
        did_pause_8k = true;
        // }
        return 0;
    } else { 
        did_pause_8k = false;
    }


    
    const std::string who = "petSendToHiggs2";

    int did_send = 0;
    int will_send;

    // cout << "allowed_to_send " << allowed_to_send << endl;
    will_send = (allowed_to_send-1) / SEND_TO_HIGGS_PACKET_SIZE;
    // cout << "will_send " << will_send << endl;
    will_send = min(max_packet_burst,will_send);
    // cout << "will_send " << will_send << endl;

    if( will_send > 0 ) {
        did_send = evbufToSocketBurst(buf, will_send*SEND_TO_HIGGS_PACKET_SIZE, who, do_force_boost);
        if( valid_8k ) {
            words_8k_sent += did_send / 4;
        }
    }
    
    if( false && print && did_send != 0 ) {
        cout << "a: " << allowed2 << "\n";
        cout << "l: " << lifetime << "\n";
        cout << "c: " << allowed_to_send/4 << "\n";
        cout << "d: " << did_send/4 << "\n";
        cout << "f: " << last_fill_level << " (" << fill_age << " )" << "\n";
        // cout << "a: " << HEX_STRING(flags) << endl;
        cout << "\n";
    }
    
    // checkOsUdpBuffers();


    // click over previous
    time_prev_pet_send_higgs = now;



    if( did_send > 0 ) {

        time_prev_sent_pet_higgs = now;

        // wake up in 10 us, because we sent data
        // return (struct timeval){ .tv_sec = 0, .tv_usec = 10 };
        return 1;
    } else {
        // out of data, send in a tiny bit
        // return (struct timeval){ .tv_sec = 0, .tv_usec = 8000 };
        return 0;
    }

}



int FlushHiggs::evbufToSocketBurst(struct evbuffer *buf, size_t request_len, const std::string& who, bool force_boost) {


    // struct evbuffer *buf  = bufferevent_get_input(bev);
    size_t in_buffer = evbuffer_get_length(buf);
    
    if( (in_buffer > 1024*8*4) && !valid_8k ) {
        if( print_evbuf_to_socket_burst ) {
            cout << "evbufToSocketBurst split check exit early\n";
        }
        return 0;
    }


    size_t in_buffer_floored = in_buffer;
    in_buffer_floored = (in_buffer_floored / 4) * 4; // make sure a multiple of 4
    // auto read_words = in_buffer_floored / 4;

    // size_t runs = 0;
    // bool more_work_left = len > largest_read;

    ///
    ///  Below this point, the size we are sending is finalized
    ///
    size_t actual_read = std::min(request_len, in_buffer_floored);
    size_t len = actual_read; // same thing


    unsigned char* temp_read = evbuffer_pullup(buf, actual_read);
    unsigned char* data = temp_read;


    bool didPrint = false;
    (void)didPrint;
    if(print_evbuf_to_socket_burst && len > 0) {
        // struct evbuffer *buf_normal = bufferevent_get_input(for_higgs_bev->out);
        // struct evbuffer *buf_low_pri = bufferevent_get_input(for_higgs_via_zmq_bev->out);
        // std::string who;
        // if( buf == buf_normal ) {
        //     who = "normal";
        // } if( buf == buf_low_pri ) {
        //     who = "low";
        // } else {
        //     who = "???";
        // }

        cout << "evbufToSocketBurst( " << who << ", " << request_len << " ) - " << in_buffer_floored << "\n";
        didPrint = true;
    }


        // dumb code below uses this


    int sendto_ret = -1;
        
    //     // max packet size
    int const chunk = 364*4;

    size_t sent = 0;
 // floor of int divide
    size_t floor_chunks = len/chunk;

    bool boost_mode = false;

    const int detect_boost = 13;


    if (floor_chunks > detect_boost) {
        boost_mode = true;
    }

    if( force_boost ) {
        boost_mode = true;
    }

//     // loop that +1
    for(size_t i = 0; i < floor_chunks+1; i++ ) {
        unsigned char* start = data + sent;

        // calculate remaining bytes
        int remaining = (int)len - (int)sent;
        // this chunk is the lesser of:
        int one_packet_len = std::min(chunk, remaining);

        // if exact even division, bail
        if(one_packet_len <= 0) {
            break;
        }

        // for( size_t j = 0; j < one_packet_len; j++) {
        //     std::cout << HEX_STRING((int)start[j]) << ",";
        // }
        // std::cout << std::endl;

        // run with values
        sendto_ret = sendto(soapy->tx_sock_fd, start, one_packet_len, 0, (struct sockaddr *)&(soapy->tx_address), soapy->tx_address_len);
        // usleep(5);

        if( keep_data_history ) {
            std::unique_lock<std::mutex> lock(_keep_data_mutex);
            data_history.reserve(data_history.size() + one_packet_len);
            for(size_t j = 0; j < (unsigned)one_packet_len; j++) {
                data_history.push_back(start[j]);
            }
        }

        if(sendto_ret <= 0 ) {
            cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! snd: " << sendto_ret << endl;
            cout << floor_chunks << " " << i << " " << " " << actual_read << " " << remaining << " " << one_packet_len << " " << endl;
        }

        
        if(boost_mode) {
            
        } else {
            if(i % 3 == 2) {
                usleep(1);
            }
        }


        sent += one_packet_len;
    }

    assert(actual_read == sent);




    // }
    // return sendto_ret;
    evbuffer_drain(buf, actual_read);

    // if( didPrint ) {
    //     cout << "evbufToSocketBurst( " << who << " ) exit" << endl;
    // }

    return actual_read;
}


std::vector<unsigned char> FlushHiggs::getDataHistory() {
    std::vector<unsigned char> copy;
    // copy out under mutex
    {
        std::unique_lock<std::mutex> lock(_keep_data_mutex);
        copy = data_history;
    }
    return copy;
}
void FlushHiggs::eraseDataHistory() {
    {
        std::unique_lock<std::mutex> lock(_keep_data_mutex);
        data_history.resize(0);
    }
}



///
///  Below here is setup
///


void FlushHiggs::enableBuffers() {
    drain_higgs = true; // run 1ce
}


///
/// Setup of callbacks and buffer waterlevels
///
void FlushHiggs::setupBuffers() {
    int ret;
    (void)ret;

    // shared set of pointers that we overwrite and then copy out from
    // (copied to BevPair class)
    struct bufferevent * rsps_bev[2];

    ////////////////
    //
    // Fire up timer for sending to higgs
    //
    sendto_higgs_timer = evtimer_new(evbase, _handle_sendto_higgs_timer, this);

    // setup once
    struct timeval first_stht_timeout = { .tv_sec = 0, .tv_usec = 1000*800 };
    evtimer_add(sendto_higgs_timer, &first_stht_timeout);


    ////////////////
    //
    // for higgs
    // comes from another thread, should be threadsafe
    //
    ret = bufferevent_pair_new(evbase, 
        BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE, 
        rsps_bev);
    assert(ret>=0);
    for_higgs_bev = new BevPair();
    for_higgs_bev->set(rsps_bev);//.enableLocking();

    // see enableBuffers()
    if(drain_higgs) {
        bufferevent_setwatermark(for_higgs_bev->out, EV_READ, 16*4, 0);
        // only get callbacks on read side
        bufferevent_setcb(for_higgs_bev->out, _handle_for_higgs_callback, NULL, _handle_for_higgs_event, this);
        bufferevent_enable(for_higgs_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
    }


    ////////////////
    //
    // zmq for higgs
    //
    ret = bufferevent_pair_new(evbase, 
        BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE, 
        rsps_bev);
    assert(ret>=0);
    for_higgs_via_zmq_bev = new BevPair();
    for_higgs_via_zmq_bev->set(rsps_bev).enableLocking();

    if(true) {
        // set 0 for write because nobody writes to out, in bytes
        bufferevent_setwatermark(for_higgs_via_zmq_bev->out, EV_READ, 16*4, 0);
        // only get callbacks on read side
        bufferevent_setcb(for_higgs_via_zmq_bev->out, _handle_for_higgs_via_zmq_callback, NULL, _handle_for_higgs_via_zmq_event, this);
        bufferevent_enable(for_higgs_via_zmq_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
    }



    // other variables
    {
        std::unique_lock<std::mutex> lock(_fill_mutex);
        last_fill_level = 0;
        last_fill_taken_at = std::chrono::steady_clock::now();
    }

}

