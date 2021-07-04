#include "DemodThread.hpp"
#include <future>

using namespace std;
#include "vector_helpers.hpp"

// #include "HiggsDriverSharedInclude.hpp"
#include "HiggsStructTypes.hpp"
#include "convert.hpp"
#include "HiggsSettings.hpp"
#include "HiggsSoapyDriver.hpp"
#include "VerifyHash.hpp"
#include "RetransmitBuffer.hpp"
#include "AirPacket.hpp"
#include "TxSchedule.hpp"
#include "AirMacBitrate.hpp"

using namespace std;



static void _handle_sliced_data_single_radio(DemodThread *that, const uint32_t *word_p, const size_t read_words) {

    for(size_t i = 0; i < read_words; i++) {

        if( i % SLICED_DATA_BEV_WRITE_SIZE_WORDS == 0 ) {
            // this branch enters 1 times for every OFDM frame

            // save the frame in index 0
            that->sliced_words[0].push_back(word_p[i]);
            for(size_t j = 0; j < (SLICED_DATA_WORD_SIZE-1); j++) {
                that->sliced_words[0].push_back(0);
            }
            // cout << "frame: " << word_p[i] << endl;
        } else {
            // this branch enters 8 times for every OFDM frame
            // size_t j = (i % SLICED_DATA_BEV_WRITE_SIZE_WORDS) - 1;

            that->sliced_words[1].push_back(word_p[i]);

            // cout << "data: " << HEX32_STRING(word_p[i]) << " <- " << j << endl;
        }
    }
    assert(that->sliced_words[0].size() == that->sliced_words[1].size());

    if( that->sliced_words[0].size() != that->sliced_words[1].size() ) {
        cout << "mismatch in _handle_sliced_data_single_radio() "
             << that->sliced_words[0].size() << ", "
             << that->sliced_words[1].size() << "\n";
    }

    // cout << r.sliced_words[0].size() << endl;
    // cout << r.sliced_words[1].size() << endl;
}



static void _handle_sliced_data_callback(struct bufferevent *bev, void *_dsp) {
     // cout << "_handle_fb_pc_callback" << endl;
    auto t1 = std::chrono::steady_clock::now();
    DemodThread *that = (DemodThread*) _dsp;

    struct evbuffer *buf  = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(buf);

    if( len == 0) {
        return;
    }

    size_t this_read = len;
    this_read = (this_read / (SLICED_DATA_BEV_WRITE_SIZE_WORDS*4)) * (SLICED_DATA_BEV_WRITE_SIZE_WORDS*4); // make sure a multiple of
    auto read_words = this_read / 4;

    uint32_t *word_p = reinterpret_cast<uint32_t*>(evbuffer_pullup(buf, this_read));

    // cout << "this_read " << this_read << " read_words " << read_words << endl;

    // auto who_gets_samples = whoGetsWhatData("sliced_data", frame);

    // for now, fixed to r[0]
    _handle_sliced_data_single_radio(that, word_p, read_words);
    // auto t2 = std::chrono::steady_clock::now();
    // std::chrono::duration<double, std::micro> t = t2 - t1;
    // std::cout << "_handle_sliced_data_single_radio " << t.count() << " us" << "\n";

    // auto t3 = std::chrono::steady_clock::now();
    evbuffer_drain(buf, this_read);
    // auto t4 = std::chrono::steady_clock::now();
    // t = t4 - t1;
    // std::cout << "handle sliced data callback " << t.count() << " us" << "\n";
    // evbuffer_drain(buf, -1);
    struct timeval val = { .tv_sec = 0, .tv_usec = 0 };
    evtimer_add(that->demod_data_timer, &val);
    auto t2 = std::chrono::steady_clock::now();
    std::chrono::duration<double> t = t2 - t1;
    // std::cout << "_handle_sliced_data_callback: " << t.count() << "\n";
}

static void _handle_sliced_data_event(bufferevent* d, short kind, void* v) {
     cout << "_handle_sliced_data_event" << endl;
}



double previous_time = 0;
static void _handle_demod_timer(int fd, short kind, void *_dsp) {
    std::chrono::milliseconds ms = 
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    if (previous_time > 0) {
        // std::cout << "Time Difference Demod Timer: " << ms.count() - previous_time << " ms\n";
    }
    previous_time = ms.count();
    auto start = std::chrono::steady_clock::now();
    // cout << "_handle_demod_timer\n";
    DemodThread *that = (DemodThread*) _dsp;

    if( that->pending_dump > 0 ) {
        that->handlePendingDump();
    } else {
        // auto t1 = std::chrono::steady_clock::now();
        that->check_subcarrier();
        // auto t2 = std::chrono::steady_clock::now();
        // std::chrono::duration<double, std::micro> t = t2 - t1;
        // std::cout << "check_subcarrier: " << t.count() << " us" << "\n";
    }
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> t = end - start;
    // std::cout << "_handle_demod_timer: " << t.count() << "\n";

}



///////////////////////////////////////////////////////////////
//
//  Above this line are static functions which comprise events for this class
//

void DemodThread::setRetransmitCallback(const retransmit_cb_t cb) {
    retransmit = cb;
}


///
/// Constructor used by tests
DemodThread::DemodThread(HiggsSettings* set): 
    HiggsEvent(0)
    ,airRx(new AirPacketInbound())
    ,verifyHash(new VerifyHash())
    ,dropSlicedData(true)
    {
        cout << "Unit test Constructor()\n";

        settings = set;
        sliced_words.resize(2);
        peer_id = 1;
        retransmit_to_peer_id = 0;
        setupBuffers();
        setAirParams();
        init_timepoint = std::chrono::steady_clock::now();
    }


DemodThread::~DemodThread() {
    _thread_should_terminate = true;
    event_base_loopbreak(evbase);

    if (_thread.joinable()) {
        _thread.join();
    }

    cout << "~DemodThread()\n";
}


DemodThread::DemodThread(HiggsDriver* s, air_set_inbound_t cb):
    
    //
    // class wide initilizers
    //
    HiggsEvent(s)
    ,airRx(new AirPacketInbound())
    ,verifyHash(new VerifyHash())
    ,macBitrate(new AirMacBitrateRx())
    ,dropSlicedData(true)
    ,setup_air_cb(cb)

    //
    // function starts:
    //
{
    settings = (soapy->settings);


    macBitrate->setSeed(0x44);


    // role = GET_RADIO_ROLE();

    cout << "DemodThread::DemodThread()" << endl;

    sliced_words.resize(2);

    peer_id = soapy->this_node.id;

    // Retransmits during beamform are sent to the arbiter
    retransmit_to_peer_id = GET_PEER_ARBITER();


    setupBuffers();
    setAirParams();


    soapy->_demod_thread_ready = true;
    cout << "_demod_thread_ready true\n";

    init_timepoint = std::chrono::steady_clock::now();
}

void DemodThread::setAirSettingsCallback(const air_set_inbound_t cb) {
    setup_air_cb = cb;
}

void DemodThread::setAirParams() {
    std::cout << "----------------------------------------\n";
    if( setup_air_cb ) {
        setup_air_cb(airRx); // points at soapy->sharedAirPacketSettings(airRx);
    }
    std::cout << "----------------------------------------\n";
}

void DemodThread::stopThreadDerivedClass() {

}



void DemodThread::setupBuffers() {

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
    sliced_data_bev = new BevPair();
    sliced_data_bev->set(rsps_bev);//.enableLocking();

    if(init_drain_for_sliced_data) {
        bufferevent_setwatermark(sliced_data_bev->out, EV_READ, SLICED_DATA_BEV_WRITE_SIZE_WORDS*SLICED_DATA_CHUNK*4, 0);
        // only get callbacks on read side
        bufferevent_setcb(sliced_data_bev->out, _handle_sliced_data_callback, NULL, _handle_sliced_data_event, this);
        bufferevent_enable(sliced_data_bev->out, EV_READ | EV_WRITE | EV_PERSIST);
    }



    ////////////////
    //
    // Fire up timer for demod
    //
    demod_data_timer = evtimer_new(evbase, _handle_demod_timer, this);

    // setup once
    struct timeval first_stht_timeout = { .tv_sec = 0, .tv_usec = 1000*800 };
    evtimer_add(demod_data_timer, &first_stht_timeout);

}




void DemodThread::discardSlicedData() {
    sliced_words[0].resize(0);
    sliced_words[1].resize(0);
    // cout << r.sliced_words[0].size() << endl;
}


void DemodThread::dataToJs(const std::vector<uint32_t> &p, const uint32_t got_at, const air_header_t &found_header) {
    // this->dsp->sendToMock("foo bar bax hello");
    // std::tuple<std::vector<uint32_t>, uint32_t, air_header_t> a;

    std::unique_lock<std::mutex> lock(_data_to_js_mutex);

    // got_data_t a(p, got_at, found_header);

    data_for_js.emplace_back(p, got_at, found_header);

    // b.push_back(a);
}

// returns:
//   valid
//   data if valid
std::pair<bool, got_data_t> DemodThread::getDataToJs() {
    std::unique_lock<std::mutex> lock(_data_to_js_mutex);

    if( data_for_js.size() == 0 ) {
        return std::pair<bool, got_data_t>(false, {});
    } else {
        std::pair<bool, got_data_t> ret(true, data_for_js[0]);
        data_for_js.erase(data_for_js.begin());
        return ret;
    }
}


void DemodThread::debug_zmq() {
    static int runs = 0;

    if( runs == 1000 ) {
        uint8_t foo = 44;
        std::vector<uint32_t> vvec = {2};
        size_t peer = 1;
        cout << "PRE CRASH???\n";
        retransmit(peer, foo, vvec);  // dsp->sendRetransmitRequestToPartner(peer, foo, vvec);
    }
    runs++;
}



// only works for a single radio, two will cause race
// void save_demod_data(RadioEstimate *r) {

//     static bool print_demod_array_open = false;
//     static ofstream myfile;

//     if(!print_demod_array_open) {
//         myfile.open ("demod_buf_raw_ota_all.txt");
//         print_demod_array_open = true;
//     }

//     for( auto x : r->sliced_words[1] ) {
//         myfile << HEX32_STRING(x) << '\n';
//     }
    
// }

void DemodThread::handlePendingDump() {
    if( pending_dump == 0) {
        return;
    }

    auto len = sliced_words[0].size();

    if( len < pending_dump ) {
        cout << "Buffering demod for dump, at " << len << " of " << pending_dump << "\n";
        return;
    }

    cout << "Opening " << dump_name << " about to write " << pending_dump << "  words\n";

    dump_file.open(dump_name);

    constexpr size_t print_counter = 20;


    for(size_t i = 0; i < pending_dump; i++) {
        if( i % SLICED_DATA_BEV_WRITE_SIZE_WORDS == 0 ) {
            if( (i/SLICED_DATA_BEV_WRITE_SIZE_WORDS) < print_counter ) {
                cout << "Dumping counter: " << sliced_words[0][i] << "\n";
            }
        }

        dump_file << HEX32_STRING(sliced_words[1][i]) << "\n";
    }

    dump_file.close();

    pending_dump = 0;

// that->sliced_words[1].push_back(word_p[i]);
// for(size_t i = 0; i < read_words; i++) {


// // counter
// that->sliced_words[0].push_back(word_p[i]);

// // data
// that->sliced_words[1].push_back(word_p[i]);


    return;
}


void DemodThread::check_subcarrier() {
    // int start_subcarrier = DATA_SUBCARRIER_INDEX;
    // int subcarrier_length = 32;
    // debug_zmq();
    if (dropSlicedData) { 
        // cout << "will discard sliced data\n";
        discardSlicedData();
        // did remove
        return; 
    }

    // save_demod_data(this);

    // usleep(1000*500);

    // std::cout << "did not discard\n";


    bool found_something;
    bool found_something_again;
    unsigned do_erase;
    uint32_t found_at_frame;
    unsigned found_at_index; // found_i
    uint32_t found_length;
    air_header_t found_header;

    // auto start = std::chrono::steady_clock::now();
    airRx->demodulateSlicedHeader(
        sliced_words,
        found_something,
        found_length,
        found_at_frame,
        found_at_index,
        found_header,
        do_erase
        );
    // auto end = std::chrono::steady_clock::now();
    // std::chrono::duration<double> t = end - start;
    // std::cout << "demodulateSlicedHeader: " << t.count() << "\n";
    unsigned do_erase_sliced = 0;
    if( found_something ) {
        times_sync_word_detected++;

        std::vector<uint32_t> potential_words_again;

        /// replace above demodulate with this
        // bool hack_force = true;
        // cout << "before: " << found_at_index << '\n';
        uint32_t force_detect_index = airRx->getForceDetectIndex(found_at_index);
        // cout << "after: " << force_detect_index << '\n';
        // cout << "force_detect_index " << force_detect_index << endl;
        uint32_t found_at_frame_again;
        unsigned found_at_index_again;
        
        airRx->demodulateSliced(
            sliced_words,
            potential_words_again,
            found_something_again,
            found_at_frame_again,
            found_at_index_again,
            do_erase_sliced,
            force_detect_index,
            found_length);

        if(!found_something_again) {
            // this means that demodulateSlicedHeader said we were ok
            // but demodulateSliced said we were too short
            // cout << "XXXXXXXX !found_something_again XXXXXXXXXXXXXXX\n";
            return;
        }


        // hack use first found at time
        if( incoming_data_type == 1 ) {
            macBitrate->got(potential_words_again);
        } else if( incoming_data_type == 0 ) {
            auto t1 = std::chrono::steady_clock::now();
            demodGotPacket(potential_words_again, found_at_frame, found_header);
            auto t2 = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::micro> t = t2 - t1;
        }
        // std::cout << "demodGotPacket " << t.count() << " us" << "\n";
        // print_found_demod_final(found_at_index);

        // exit(0);
       // if(do_erase_sliced) {
       //      cout << "INSIDE ERASE!!!!!!!!1  " << sliced_words[0].size() << endl;
       //      trimSlicedData(do_erase_sliced);
       //  }
    }

    if(do_erase) {
        trimSlicedData(do_erase);
        // demod_buf_accumulate.resize(0);
        // trickle_erase(storage, erase);
        // remove_subcarrier_data2(demod_buf_accumulate, demod_buf_accumulate[0].size());
    } else if( do_erase_sliced ) {
        trimSlicedData(do_erase_sliced);
    }
    // exit(0);

    // remove_subcarrier_data(start_subcarrier, subcarrier_length);
}

void DemodThread::trimSlicedData(const unsigned erase) {
    std::vector<std::vector<uint32_t>> &erase_from = this->sliced_words;
    // cout << "TRIMMING SLICED for " << erase << endl;
    // cout << "before " << erase_from[0].size() << endl;
    erase_from[0].erase(erase_from[0].begin(), erase_from[0].begin()+erase);
    erase_from[1].erase(erase_from[1].begin(), erase_from[1].begin()+erase);
    // cout << " after " << erase_from[0].size() << endl;
}



/// A ReedSolomon block failed and it took out our hash/len combo
/// This function tries to recover the next hash/len combo to prevent losing the
/// entire "siglabs packet"
static bool handleRecovery(
    int& erase,
    std::vector<uint32_t> &p,
    unsigned& consumed,
    std::function<uint32_t(const std::vector<uint32_t>&)> wrapHash,
    const size_t full_len
    ) {

    // taken from VerifyHash.cpp (not exactly? (convert.cpp (it was packetToBytes())))
    // an enformenet of MTU in worst case packet drop
    constexpr unsigned maxlen = 375; //(0xfff);

    // consumed is the number of words we've eaten
    // if we just found a 0 hash, it means tha tthis entire IP packet is corrupted
    // however we will be able to recover the future
    // in this case we need 2 packets ahead because this one is dropped
    // but we would need at least a full one in the future (how about partials?)
    // (this can be reduced or just check end of input)
    constexpr uint32_t minlook = 6; // don't look for anthing this soon, this would be a short ass packet
    constexpr uint32_t window = 40*maxlen; // at some point this gets so large we hit a LARGE cpu

    // can also be seen as a flag meaning "recovery failed"
    bool bbreak = true;

    if( (minlook + maxlen) < p.size() ) {
        cout << "There are " << p.size() << " unconsidered words remaining" << "\n";

        bool recovered = false;
        for(unsigned look_start = minlook; (look_start < window) && ((look_start+window) < p.size()) ; look_start++ ) {
            
            // window is how many we will try in the loop
            // this is just many words we need for a single try
            unsigned look_end =  look_start + (2*maxlen);

            // look_start = minlook;

            // this is a seriously expensive copy
            // look at replacing with std range
            std::vector<uint32_t> look;
            look.assign(p.begin()+look_start,p.begin()+look_end);

            int look_erase = 0;

            auto chars2 = packetToBytesHashMode(look,look_erase,wrapHash);

            bool hashok = chars2.size() != 0;

            if( hashok ) {
                cout << "HASH MATCH!\n";
                cout << "i " << look_start << "\n";
                cout << "    chars2 " << chars2.size() << "\n";
                cout << "    look_erase " << look_erase << "\n";
                cout << "\n";

                const int fudge_erase = look_start;
                p.erase(p.begin(), p.begin()+fudge_erase);
                consumed += (unsigned)fudge_erase;

                erase = fudge_erase; // this will change the flag from -1
                // to a new value
                // while this value is not used, it is checked against -1
                // so setting it to anything positive would also work

                recovered = true; // we recovered
                bbreak = false; // tell parent loop to NOT break (demodGotPacket())
                return bbreak;
            }
        }

        // this is what happenes when we are still at -1 by this point
        // aka we searched but didn't find anything
        if( !recovered ) {
            cout << "demodGotPacket() tried to recover but failed" << " " << full_len << " "<< consumed << " " <<  p.size() << "\n";
            bbreak = true;
            return bbreak;
        }

    } else {
        bbreak = true;
        return bbreak;
    }

    return bbreak;
}






void DemodThread::demodGotPacket(std::vector<uint32_t> &p, const uint32_t got_at, const air_header_t &header) {
    ///
    /// Got the packet
    /// you need put it in the tun at this point, it will be destroyed after this function exits
    ///

    auto now = std::chrono::steady_clock::now();
    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    now - init_timepoint
    ).count();

    auto full_len = p.size();

    const double elapsed_sec =  (double)elapsed_us / 1E6;


    uint32_t header_length;
    uint8_t  header_seq;
    uint8_t  header_flags;

    std::tie(header_length, header_seq, header_flags) = header;

    // if( header_length != p.size() ) {
    //     cout << "\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\ndemodGotPacket() has mismatch " << header_length << ", " << p.size() << "\n";
    // }

    std::vector<uint32_t> missing_seq;
    if( header_seq != expected_seq ) {
        cout << "\nDROPPED packet!" << (int)header_seq << " " << (int) expected_seq << "\n\n";

        for(unsigned i = 0; i < 255; i++) {
            const uint8_t ppush = expected_seq + i;
            missing_seq.push_back(ppush);

            cout << "notifying missing sequence number " << (int)ppush << "\n";

            const uint8_t test = ppush + 1;

            if( test == header_seq ) {
                break;
            }
        }

        // reset
        expected_seq = header_seq + 1;
        // seqgap = true;
    } else {
        expected_seq++;
    }

    if( missing_seq.size() ) {
        const std::vector<uint32_t> all_missing = {0xffffffff};
        for( auto w : missing_seq ) {
            retransmit(this->retransmit_to_peer_id, w, all_missing);
        }
    }



    cout << "Got Packet at frame " << got_at << " aka " << (got_at % SCHEDULE_FRAMES) << " len " << p.size() << endl;
    cout << "!!!!!!!!!!!!!   Seq: " << (int) header_seq << "  " << elapsed_sec << endl;

    if( p.size() == 0 ) {
        cout << "Got Packet got EMPTY PACKET, decode probably failed!\n";
        return;
    }

// #define PRINT_ALL_RE
#ifdef PRINT_ALL_RE
    auto futureCallback = std::async(
        std::launch::async, 
        [](std::vector<uint32_t> copy) {
            int i = 0;
            for(auto x : copy) {
                cout << HEX32_STRING(x) << " ";
                if( i % 8 == 7 ) {
                    cout << "\n";
                }
                i++;
            }
        }, p
    );
#endif

    if( GET_RUNTIME_DATA_TO_JS() ) {
        std::vector<uint32_t> copy = p;
        copy.erase(copy.begin());
        dataToJs(copy, got_at, header);
    }

    // std::function<void()> f;
    std::function<uint32_t(const std::vector<uint32_t>&)> wrapHash;

    // f = []() -> uint32_t { }

    wrapHash = [this](const std::vector<uint32_t>& x) {
     // return (return verifyHash->getHashForWords(x);
        // return (uint32_t)0;
        return this->verifyHash->getHashForWords(x);
    };
    // auto foo = [i](){ cout << i << endl; };

    // boost::hash<std::vector<unsigned char>> char_hasher;

    const bool verify_hash = GET_DATA_HASH_VALIDATE_ALL();
    std::vector<uint32_t> hashes;

    unsigned consumed = 0;
    int erase = 0;
    while(erase != -1) {

        // auto v1 = packetToBytes(p, erase);
        auto v1 = packetToBytesHashMode(p,erase,wrapHash);

        if( erase != -1 ) {

            if(soapy && soapy->tun_fd) {
                if( v1.size() ) {
                    auto retval = write(soapy->tun_fd, v1.data(), v1.size() );
                    if( retval < 0 ) {
                        cout << "demodGotPacket() write !!!!!!!!!!!!!!!!!!!!!!!!!!! returned " << retval << endl;
                    }
                } else {
                    cout << "hash mismatch\n";
                }

                uint32_t hash = verifyHash->getHashForChars(v1);
                // cout << "xxxxxxxxxxx Writing " << v1.size() << " bytes to radio estimate tun char hash " << HEX32_STRING(h) << endl;

                if( verify_hash ) {
                    hashes.emplace_back(hash);
                }

                
            }

            if( erase > 0 ) {
                p.erase(p.begin(), p.begin()+erase);
                consumed += (unsigned)erase;
            }

        } else {
            cout << "Erase was -1" << endl;
            auto t1 = std::chrono::steady_clock::now();
            const bool breakDesired = handleRecovery(erase, p, consumed, wrapHash, full_len);
            auto t2 = std::chrono::steady_clock::now();
            std::chrono::duration<double, std::micro> t = t2 - t1;
            // std::cout << "handleRecovery " << t.count() << " us" << "\n";
            if( breakDesired ) {
                break;
            }
        }
    }

    if( verify_hash ) {
        // put sequence number as first "hash"
        // hashes.insert(hashes.begin(), (uint32_t)std::get<1>(header));

        soapy->txSchedule->verifyHashTx->do_print_partial = false; // quiet print

        // fixme move verifyHashTx to own object
        const auto retransmit_packed = soapy->txSchedule->verifyHashTx->gotHashListOverAir(hashes, header);

        const auto need_retransmit = std::get<0>(retransmit_packed);
        const auto retransmit_seq  = std::get<1>(retransmit_packed);
        const auto retransmit_idxs = std::get<2>(retransmit_packed);


        if( need_retransmit ) {
            cout << "In verify hash 4 (" << (int) need_retransmit << ") seq " << (int) retransmit_seq << endl;
            // dsp->sendRetransmitRequestToPartner(
            retransmit(
                this->retransmit_to_peer_id, retransmit_seq, retransmit_idxs);
        } else {
            cout << "In verify hash 5" << endl;

            constexpr bool send_fake_retransmit_req = false;
            if( send_fake_retransmit_req ) {
                if( hashes.size() > 6 ) {
                    int fake_seq = (int) std::get<1>(header);
                    int pull = rand() % 1000;
                    if( pull < 200 ) {
                        cout << "In verify hash FAKE seq " << (int) fake_seq << " Sending: ";

                        std::vector<uint32_t> a = {3,4};
                        for( auto idx : a ) {
                            cout << " idx: " << idx << " " << HEX32_STRING(hashes[idx]) << " ";
                        }
                        cout << endl;

                        // dsp->sendRetransmitRequestToPartner(
                        retransmit(
                            this->retransmit_to_peer_id, fake_seq, a);

                    } // pull
                }
            } // if( send_fake_retransmit_req )
        } // else
    } // verify_hash
}