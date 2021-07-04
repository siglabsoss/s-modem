#include "TxSchedule.hpp"
#include <future>

using namespace std;
#include "vector_helpers.hpp"

#include "HiggsDriverSharedInclude.hpp"
#include "HiggsStructTypes.hpp"
#include "convert.hpp"
#include "FsmMacros.hpp"
#include "EventDsp.hpp"
#include "FlushHiggs.hpp"
#include "EventDspFsmStates.hpp"
#include "HiggsSettings.hpp"
#include "HiggsSoapyDriver.hpp"
#include "AttachedEstimate.hpp"
#include "VerifyHash.hpp"
#include "RetransmitBuffer.hpp"
#include "AirPacket.hpp"
#include "ScheduleData.hpp"
#include "AirMacBitrate.hpp"
#include "random.h"

using time_point = std::chrono::steady_clock::time_point;
using steady_clock = std::chrono::steady_clock;

static void handle_fsm_boot_tx_schedule(int fd, short kind, void *_dsp)
{
    TxSchedule *dsp = (TxSchedule*) _dsp;
    // cout << "handle_fsm_boot_tx_schedule" << endl;

    dsp->dsp_state_pending = dsp->dsp_state = PRE_BOOT;

    // cout << "resetting estimate_filter_mode" << endl;
    // dsp->estimate_filter_mode = 0;

    // calls the event ON THE SAME STACK as us
    event_active(dsp->fsm_next, EV_WRITE, 0);
}

static void handle_fsm_tick_tx_schedule(int fd, short kind, void *_dsp)
{
    // cout << "handle_fsm_tick_tx_schedule" << endl;
    TxSchedule *dsp = (TxSchedule*) _dsp;
    dsp->tickFsmSchedule();
}


static std::string getDeltaTime(const std::chrono::steady_clock::time_point& start, const std::chrono::steady_clock::time_point& end) {
    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
            end - start
            ).count();
    auto as_seconds = ((double)elapsed_us)/1E6;
    // cout << as_seconds;
    return std::to_string(as_seconds);
}



///////////////////////////////////////////////////////////////
//
//  Above this line are static functions which comprise events for this class
//
TxSchedule::TxSchedule(HiggsDriver* s):
    
    //
    // class wide initilizers
    //
    HiggsEvent(s)
    ,macBitrate(new AirMacBitrateTx())
    ,airTx(new AirPacketOutbound())
    ,scheduleData(new ScheduleData(airTx))
    ,duplex_usage_map(new bool[DUPLEX_FRAMES])

    //
    // function starts:
    //
{
    settings = (soapy->settings);

    while(!_thread_should_terminate) {
        if( soapy->dspThreadReady() ) {
            break;
        }
        usleep(100);
    }

    while(!_thread_should_terminate && !soapy->thisNodeReady() && !soapy->dspThreadReady()) {usleep(100);}

    role = GET_RADIO_ROLE();


    /// Setup for duplex
    setupDuplexAsRole(role);



    cout << "TxSchedule::TxSchedule()" << endl;

    print = true;

    warmupDone = false;
    doWarmup = false;
    exitDataMode = false;
    requestPause = false;
    debugBeamform = false;
    flushZerosEnded = 0;
    errorDetected = 0;
    debugBeamformSize = 4000;
    setLinkUp(false);

    verifyHashTx = new VerifyHash();
    verifyHashTx->do_print = GET_PRINT_VERIFY_HASH();

    retransmitBuffer = new RetransmitBuffer();

    setupPointers();

    // zrs.resize(32, 0);

    // while(!_thread_should_terminate && !soapy->flushHiggsReady()) {usleep(100);}
    
    setupFsm();

    last_low = last_tickle = init_timepoint = steady_clock::now();

    soapy->_tx_schedule_ready = true;
}

// you can call this in debug, or as many times as you want
// and the output should work with no side effects (aka it's not just for setup)
void TxSchedule::setupDuplexAsRole(const std::string _role) {
    /// Setup for duplex
    const int32_t tnum = soapy->getTxNumber();
    const uint32_t role_enum = soapy->duplexEnumForRole(_role, tnum);

    cout << "setupDuplexAsRole for " << _role << " using " << role_enum << "\n";
    init_duplex(&duplex, role_enum);

    duplex_build_tx_map(&duplex, duplex_usage_map, false);
}

void TxSchedule::stopThreadDerivedClass() {

}

void TxSchedule::setupPointers() {

    dsp = soapy->dspEvents;

    attached = dsp->attached;

    status = soapy->status;

}



void TxSchedule::setupFsm() {
    fsm_next       = event_new(evbase, -1, EV_PERSIST, handle_fsm_tick_tx_schedule, this);
    fsm_next_timer = evtimer_new(evbase, handle_fsm_tick_tx_schedule, this);

    
    fsm_init = evtimer_new(evbase, handle_fsm_boot_tx_schedule, this);

    auto t2 = TINY_SLEEP;
    evtimer_add(fsm_init, &t2);
}


void TxSchedule::debug(const unsigned length) {
    if(length == 0) {
        return;
    }
    pending_valid = 1;
    pending_from_peer = 0;
    epoc_frame_t woke_est;
    int error;
    woke_est = attached->predictScheduleFrame(error, false, false);

    pending_epoc = woke_est.epoc + 2;
    pending_ts = 12;

    static uint8_t debug_seq = 0;
    pending_seq = debug_seq;
    debug_seq++;


    pending_packets.resize(0);

    std::vector<uint32_t> packet;

    constexpr unsigned packetsz = 300;
    for(unsigned i = 0; i < length; i++) {
        if( (i % packetsz) == (packetsz-1) ) {
            pending_packets.push_back(packet);
            packet.resize(0);
        }
        packet.push_back(0xf0000000 + i);
    }

// << woke_est.epoc << ", " << (woke_est.frame / 512) << " (" << woke_est.frame << ")\n";
}

void TxSchedule::debugFillBuffers(const unsigned recursion) {
    if( !GET_DEBUG_TX_FILL_LEVEL() ) {
        return;
    }

    // there are 20 packets in here, each with the same size of 375 words
    std::vector<uint32_t> data0 = _file_read_hex("../src/data/datapacket0.hex");
    const int packets = 20;
    const int size = 375;
    const int minpull = 4;

    int this_fill = (rand() % (packets - minpull)) + minpull;
    cout << "debugFillBuffers() adding " << this_fill << " packets" << endl;

    for(int i = 0; i < this_fill; i++) {

        std::vector<uint32_t> packet;
        packet.assign(
            data0.begin() + (i * size),
            data0.begin() + ((i+1) * size)
        );

        soapy->txTun2EventDsp.enqueue(packet);
    }

    if( recursion ) {
        debugFillBuffers(recursion-1);
    }
}

void TxSchedule::debugFillBeamform(uint32_t now_epoc, uint32_t now_frame, int wake_ts) {
    if( !debugBeamform ) {
        return;
    }

    cout << "FillBeamform now_epoc " << now_epoc << ", " << now_frame << ", wake_ts: " << wake_ts << "\n";


    if( wake_ts == 12 && (now_epoc % 2 == 0) ) {
        size_t total = 0;
        constexpr size_t pack = 350;

        // uint32_t sr = 0xdead0000;

        std::vector<uint32_t> packet;
        unsigned int state = 0xdead0000;
        for(size_t i = 0; i < pack; i++ ) {
            state = xorshift32(state, &state, 1);
            packet.push_back(state);
        }

        // for(unsigned i = 0; i < pack; i++ ) {
        //     uint32_t out = (sr + (sr << 13)) ^ (sr >> 17) ^ (sr << 5); // probably not the same as random.c
        //     packet.push_back(out);
        //     sr += out;
        // }

        while(total < debugBeamformSize) {
            soapy->txTun2EventDsp.enqueue(packet);
            total += packet.size();
        }

        cout << "debugFillBeamform() queued " << total << " words\n";
    }

    // uint32_t epoc;
    // uint32_t frame;
}

void TxSchedule::debugFillZmq() {
    if( !GET_DEBUG_TX_EPOC_AND_ZMQ() ) {
        return;
    }

    bool pull_or_naw = (rand() % 100) < 25;

    if( !pull_or_naw ) {
        return;
    }

    cout << "debugFillZmq will inject an eq adjustment" << endl;

    std::vector<uint32_t> zrs2;
    zrs2.resize(1024, 0);

    auto packet = feedback_vector_packet(
                FEEDBACK_VEC_TX_EQ,
                zrs2,
                0,
                FEEDBACK_DST_HIGGS);

    unsigned char* char_packet = reinterpret_cast<unsigned char*>(packet.data());
    unsigned char_len = packet.size()*4;

    soapy->sendToHiggsLowPriority(char_packet, char_len);
}

/// should return data in the same order as tunGetPacket()
unsigned TxSchedule::tunFirstLength() const {
    if( retransmitBuffer->packetsReady() ) {
        return retransmitBuffer->firstLength();
    }
    if( soapy->txTun2EventDsp.size() ) {
        auto peek = soapy->txTun2EventDsp.peek();
        return peek.size();
    }

    cout << "warning tunFirstLength() was called when no packets were ready" << endl;
    return 0;
}
unsigned TxSchedule::tunPacketsReady() const {
    return soapy->txTun2EventDsp.size() + retransmitBuffer->packetsReady();
}
/// should return data in the same order as tunFirstLength()
std::vector<uint32_t> TxSchedule::tunGetPacket() {
    if( retransmitBuffer->packetsReady() ) {
        return retransmitBuffer->getPacket();
    }
    if( soapy->txTun2EventDsp.size() ) {
        return soapy->txTun2EventDsp.dequeue();
    }

    cout << "warning tunGetPacket() was called when no packets were ready" << endl;
    return {};
}

size_t TxSchedule::bufferedBeamform() const {
    std::unique_lock<std::mutex> lock(_beamform_buffer_mutex);
    return beamform_buffer.size();
}

/// Zmq sends us a packed vector which contains epoc, ts, seq, and packets
/// this function will unpack that into 
/// pending_epoc
/// pending_ts
/// pending_seq
/// pending_packets
///
/// when this happens we set pending_valid
/// if this flag is already set, we don't do this
void TxSchedule::loadPendingMember() {
    if( pending_valid ) {
        // pending_* are already loaded with a different packet
        return;
    }

    // only get to here in pending_valid = 0

    // uint32_t from_peer;

    std::unique_lock<std::mutex> lock(_beamform_buffer_mutex);
    if( beamform_buffer.size() ) {
        AirPacket::unpackJointZmq(beamform_buffer[0], pending_from_peer, pending_epoc, pending_ts, pending_seq, pending_packets);
        pending_valid = 1;
        beamform_buffer.erase(beamform_buffer.begin());
    }
}


void TxSchedule::dumpPendingPacket(const std::string reason, const uint32_t now_epoc, const uint32_t now_frame) {
    cout << "dumpPendingPacket() " << " dropped LATE packet because: " << reason << "\n";
    dsp->sendJointAck(pending_from_peer, pending_epoc, pending_ts, pending_seq, now_epoc, now_frame, true);
    pending_valid = 0;
    return;
}

void TxSchedule::dumpPendingPacket(const std::string reason, const uint32_t now_lifetime32) {
    cout << "dumpPendingPacket() " << " dropped LATE packet because: " << reason << "\n";
    dsp->sendJointAck(pending_from_peer, pending_lifetime, pending_seq, now_lifetime32, true);
    pending_valid = 0;
    return;
}

/// Lazy way to beamform
/// we if the time is right, we copy the packet into txTun2EventDsp
/// this copy could be removed by re-writing pullAndPrepTunData()
void TxSchedule::loadZmqBeamform(const uint32_t now_epoc, const uint32_t now_frame) {

    if( pending_valid != 1 ) {
        return;
    }

    if( now_epoc > pending_epoc ) {
        cout << "loadZmqBeamform() " << " dropped LATE packet seq: " << (int)pending_seq << " scheduled for " << pending_epoc << " as it is now " << now_epoc << "\n";
        dsp->sendJointAck(pending_from_peer, pending_epoc, pending_ts, pending_seq, now_epoc, now_frame, true);
        pending_valid = 0;
        return;
    }

    constexpr auto drop_early_seconds = 20;

    if( (now_epoc + drop_early_seconds) < pending_epoc ) {
        cout << "loadZmqBeamform() " << " dropped EARLY packet seq: " << (int)pending_seq << " scheduled for " << pending_epoc << " as it is now " << now_epoc << "\n";
        dsp->sendJointAck(pending_from_peer, pending_epoc, pending_ts, pending_seq, now_epoc, now_frame, true);
        pending_valid = 0;
        return;
    }


    if( (now_epoc == pending_epoc) || ( (now_epoc+1)  == pending_epoc) ) {
        cout << "loadZmqBeamform() loading " << pending_packets.size() << " packets\n";
        cout << "loadZmqBeamform() now_epoc " << now_epoc << ", " << now_frame << ", pending_ts: " << pending_ts << "\n";
        cout << "loadZmqBeamform() pending_epoc " << pending_epoc << "\n";

        unsigned dumped = dsp->dumpTunBuffer();

        if( dumped > 0 ) {
            cout << "WARNING there were " << dumped << " packets in tun but are doing joint tx\n";
        }

        for( const auto& packet : pending_packets ) {
            soapy->txTun2EventDsp.enqueue(packet);
        }

        // hack to get the sequence right, re-writing pullAndPrepTunData() to remove
        airTx->tx_seq = pending_seq;
        pending_valid = 2;
    }
}

/// Loads packets from tun buffer into a vector that we can send via feedback bus
/// this was extracted from pullAndPrepTunData()
/// @param[in] output_limit_words: Keep pulling packets out of tun buffer until we reach this limit, 0 to disable
void TxSchedule::loadAllPackets(
     std::vector<uint32_t>& hashes
    ,std::vector<std::vector<uint32_t>>& for_retransmit
    ,std::vector<uint32_t>& build_packet
    ,int& input_total
    ,int& total_packets
    ,const bool verify_hash
    ,const int output_limit_words ) {

    unsigned packetsReady = tunPacketsReady();

    bool more = packetsReady > 0;

    // if( packetsReady != 0 ) {
    //     cout << "packets ready " << packetsReady << endl;
    // }

    while(more) {
        total_packets++;
        auto fromTun = tunGetPacket();

        auto packet_hash = verifyHashTx->getHashForWords(fromTun);

        if(verify_hash) {
            // cout << "  saved hash " << HEX32_STRING(packet_hash) << endl;
            hashes.emplace_back(packet_hash);
            for_retransmit.push_back(fromTun);
        }

        if( true ) {
            // push hash on
            build_packet.push_back(packet_hash);
        }

        // string together packets
        for( auto x : fromTun ) {
            build_packet.push_back(x);
        }

        // output_progress += last_output;
        input_total += fromTun.size();

        more = false;

        // cout << "pullAndPrepTunData " << soapy->txTun2EventDsp.size() << output_limited << endl;

        if( tunPacketsReady() > 0 ) {
            int pk_sz = tunFirstLength();

            // cout << "peek " << pk_sz << endl;

            if( output_limit_words != 0 ) { // only check output limit if this is a non zero value
                if( (pk_sz + 1 + input_total) < output_limit_words ) {
                    more = true;
                }
            } else {
                more = true;
            }
        }
    }
}


std::vector<uint32_t> TxSchedule::pullAndPrepTunData(const int max_mapmov_size) {

    // shave off header, which will be added later
    // with arbiter, the function argument is zero, so we set this to zero as well so loadAllPackets() will not stop
    const int output_limit_words = (max_mapmov_size==0) ? (0) : (max_mapmov_size - (airTx->header_overhead_words));

    // should we calculate and send hashes to the rx side?
    const bool verify_hash = GET_DATA_HASH_VALIDATE_ALL();
    std::vector<uint32_t> hashes;
    // boost::hash<std::vector<unsigned char>> char_hasher;


    if( retransmitBuffer->packetsReady() ) {
        cout << " There are " << retransmitBuffer->packetsReady() << " retransmit packets " << endl;
    }

    int input_total = 0;
    int total_packets = 0;
    std::vector<uint32_t> build_packet;
    std::vector<std::vector<uint32_t>> for_retransmit;

    loadAllPackets(
        hashes,              // output
        for_retransmit,      // output
        build_packet,        // output
        input_total,         // output
        total_packets,       // output
        verify_hash,         // input
        output_limit_words); // input

    // cout << "at the end, build packet was " << build_packet.size() << " " << input_total << " words large allowed (" << max_mapmov_size << ")\n";

    // std::vector<uint32_t> transformed;

    // wraps, prefixes stuff
    uint8_t tx_seq;
    std::vector<uint32_t> d = airTx->transform(build_packet, tx_seq, 0);


    // for( auto x : d ) {
    //     cout << HEX32_STRING(x) << "," << endl;
    // }

    // optionally zend to zmq
    // if(verify_hash) {

    //     // when we call transform, the sequence is bumped, this gets the value before the bump
    //     // uint8_t tx_seq = airTx->getPrevTxSeq();

    //     // prepend the hashes with the sequence number of this packet
    //     hashes.insert(hashes.begin(), (uint32_t)tx_seq);

    //     // save the packet we just sent, for later
    //     retransmitBuffer->sentOverAir(tx_seq, for_retransmit);


    //     auto send_hash_to = GET_PEER_RX();
    //     dsp->sendHashesToPartner(send_hash_to, "", hashes);
    // }

    // cout << "!!!!!!!!!!!!!!! pullAndPrepTunData total packets " << total_packets << "\n";
    return d;
}

void TxSchedule::dumpUserData() {
    unsigned packetsReady = tunPacketsReady();

    bool more = packetsReady > 0;

    unsigned burned = 0;

    while(more) {
        tunGetPacket(); // burn
        packetsReady = tunPacketsReady();
        more = packetsReady > 0;
        burned++;
    };

    cout << "dumpUserData dropped " << burned << " packets\n";
}

void TxSchedule::tickleAttachedProgress() {
    const uint32_t seconds = 2;
    const uint32_t count = seconds*24; // fundamental property of code, see cs20

    auto now = std::chrono::steady_clock::now();
    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
    now - last_tickle
    ).count();

    if( elapsed_us < 1E6 ) {
        return;
    }

    last_tickle = now;

    if( elapsed_us >= ((double)seconds * 1E6) ) {
        cout << "\n\nWARNING tickleAttachedProgress() is not being called fast enough!!!!!!!!!\n\n";
    }



    raw_ringbus_t rb0 = {RING_ADDR_TX_PARSE, TX_PROGRESS_CMD|count};
    dsp->ringbusPeerLater(attached->peer_id, &rb0, 0);
}


void TxSchedule::setLinkUp(const bool up) {
    settings->vectorSet(up, (std::vector<std::string>){"runtime","tx","link","up"});
}



static std::vector<uint32_t> _get_counter(const int start, const int stop) {
    std::vector<uint32_t> out;
    for(int i = start; i < stop; i++) {
        out.push_back(i);
    }
    return out;
}


// if pending valid is non zero, this function MUST return false
// we rely on pending_valid staying the same through a function, we maybe
// should just cache this value
bool TxSchedule::isDebugDuplex(const unsigned _pending_valid) {

    if( _pending_valid != 0 ) {
        return false;
    }

    if( debug_busy_valid ) {
        int error;
        // isDebugDuplex checks the time and this is what we will use for the purpose
        // of deciding if we are busy or not as this funciton is only called once in
        // the fsm
        const uint32_t life_for_check = attached->predictLifetime32(error);

        if( life_for_check < (debug_duplex_busy_until+debug_extra_gap) ) {
            return false;
        }

        debug_busy_valid = false;
    }

    // const uint32_t frames_from_data = sent_length / airTx->sliced_word_count;
    // debug_duplex_busy_until = pending_lifetime + frames_from_data; // should already include "early" frames
    // debug_busy_valid = true;

    // if we don't have a valid debug_duplex_busy_until, that means we can send
    // so we are ready to send if our enable counter is non-zero
    return debug_duplex > 0;
}

// apply timing etc
void TxSchedule::applyDebugDuplex1() {
    int error;
    const uint32_t lifetime32 = dsp->attached->predictLifetime32(error, false);

    // const uint32_t 

    cout << "Predicted lifetime32 as " << lifetime32 << "\n";

    const uint32_t start_frame = (dsp->role == "rx") ? DUPLEX_START_FRAME_RX : DUPLEX_START_FRAME_TX;

    // const uint32_t bump = 

    const uint32_t schedule_at = duplex_next_frame(lifetime32 + debug_duplex_ahead, start_frame);

    cout << "Scheduleing For " << schedule_at << " (" << schedule_at % DUPLEX_FRAMES << ")\n";

    pending_lifetime = schedule_at;
}

std::vector<uint32_t> TxSchedule::applyDebugDuplex2() {

    std::vector<uint32_t> padded;
    // const unsigned debug_duplex_size = 40;

    // auto input_data_base = [](const uint32_t data_length, const uint32_t base) {
    //     std::vector<uint32_t> input_data;
    //     uint32_t data;
    //     for(uint32_t i = 0; i < data_length; i++) {
    //         data = i;
    //         input_data.push_back(base+data);
    //     }
    //     return input_data;
    // };

    // data source
    std::vector<uint32_t> counter;

    switch(debug_duplex_type) {
        default:
        case 0: {
                counter = _get_counter(0xff000000, 0xff000000+debug_duplex_size);
            }
            break;
        case 1: {
                counter = macBitrate->generate(debug_duplex_size, duplex_seq);
                duplex_seq++;
            }
            break;
    }

    uint8_t tx_seq;
    padded = airTx->transform(counter, tx_seq, 0); // generate (unpadded)
    airTx->padData(padded); // actually pad

    return padded;
}

void TxSchedule::finishedDebugDuplex(const uint32_t sent_length, const bool skipped) {

    if( !skipped ) {

        // busy calc
        
        // if we are sending more than 85 frames, we need a complicated function to estimate our busy
        debug_duplex_busy_until = estimateBusy(pending_lifetime, sent_length);

        cout << "Estimate that we will finish sending at lifetime32: " << debug_duplex_busy_until << "\n";
        
        // simple version which does not account for gaps, aka times in which we are not transmitting
        // debug_duplex_busy_until = pending_lifetime + frames_from_data; // should already include "early" frames

        debug_busy_valid = true;

        if( debug_duplex > 0 ) {
            debug_duplex--;
        }
    } else {
        cout << "Debug packet was skipped, will send later\n";
    }
}


// if we are sending more than 85 frames, we need a complicated function to estimate our busy
uint32_t TxSchedule::estimateBusy(const uint32_t from, const uint32_t sent_length) {
    // busy calc
    const uint32_t frames_from_data = sent_length / airTx->sliced_word_count;

    const uint32_t extimated_future_lifetime = duplex_duty_frame_done(from, frames_from_data, duplex_usage_map);

    return extimated_future_lifetime;
}










// std::pair<uint32_t,uint32_t> = word size, lifetime bump
// pass pre
static std::vector<uint32_t> splitFeedbackVectorMapmovScheduledSized(
    const std::vector<uint32_t> _d,
    std::pair<uint32_t,uint32_t> j0,
    std::pair<uint32_t,uint32_t> j1,
    const uint32_t slicewc,
    const uint32_t enabled_subcarriers,
    const uint32_t lstart,
    const uint32_t constellation,
    const bool print = true) {

    const uint32_t vtype = FEEDBACK_VEC_TX_USER_DATA;
    // std::vector<std::vector<uint32_t>> ret; // vector of vertor version
    std::vector<uint32_t> ret; // one

    if( _d.size() % slicewc != 0 ) {
        cout << "Fb Cutter input was not length mod " << _d.size() << " % " << slicewc << " was not zero\n";
        return ret;
    }

    // if( (lstart % 128) != )



    int built = 0;

    uint32_t jump_count = 0;
    std::pair<uint32_t,uint32_t>* jump = &j0;

    const std::vector<uint32_t>& d = _d; // const copy


    for(uint32_t lifetime_now = lstart; built < (int)d.size(); /**/) {

        uint32_t word_size, bump;
        std::tie(word_size,bump) = *jump;

        uint32_t using_size = std::min(((int)d.size() - built),   (int)word_size);

        if( print ) {
            cout << lifetime_now <<"," << (lifetime_now%128) << ":    size " << word_size << " bump " << bump << " using size " << using_size << "  (ovr 20: " << ((double)using_size / 20.0) << ")\n";
        }

        //////////////////////////// work
        {
            // resize for a chunk for this run
            std::vector<uint32_t> chunk;
            chunk.assign(d.begin()+built, d.begin()+built+using_size );
            // std::vector<uint32_t> row;
            auto chunkLoaded = 
                feedback_vector_mapmov_scheduled_sized(
                    vtype, 
                    chunk, 
                    enabled_subcarriers, 
                    0,
                    FEEDBACK_DST_HIGGS,
                    0, // timeslot fillin, overwritten later
                    0,  // epoc fillin, overwritten later
                    constellation
                    );
            set_mapmov_lifetime_32(chunkLoaded, lifetime_now);



            // ret.emplace_back(chunkLoaded);  // vector of vector version
            ret.insert(ret.end(), chunkLoaded.begin(), chunkLoaded.end());


            // if(print) {
            //     cout << "loading " << chunkLoaded.size() << "\n";
            // }
        }

        //////////////////////////// work


        // update
        built += using_size;
        lifetime_now += bump;
        jump_count++;
        if(jump_count % 2 == 0) {
            jump = &j0;
        } else {
            jump = &j1;
        }
    }

    return ret;
}







#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
void TxSchedule::tickFsmSchedule() {

    dsp_state = dsp_state_pending;
    // tickle(&dsp_state);
    auto __dsp_preserve = dsp_state;
    struct timeval tv = DEFAULT_NEXT_STATE_SLEEP;
    size_t __timer = __UNSET_NEXT__;
    size_t __imed = __UNSET_NEXT__;
    size_t next = __UNSET_NEXT__;

    // uint32_t recent_frame = fuzzy_recent_frame;

    switch(dsp_state) {
        case PRE_BOOT:

            warmup_epoc_est_target = 3;
            warmup_epoc_est = 0;
            if(true) {


                soapy->sharedAirPacketSettings(airTx);

                // after settings run, we need to pull this out:
                tx_mapmov_rb_for_eth = MAPMOV_MODE_CMD | airTx->get_subcarrier_allocation();

            }







            if( this->role == "arb" ) {
                cout << endl << endl << "TxSchedule::tickFSM detected non TX mode, shutting down" << endl << endl;
                next = GO_NOW(TX_STALL);
            } else {
                cout << "TxSchedule::tickFsmSchedule\n";
                next = GO_AFTER(DEBUG_EARLY_EVENT_0, _1_MS_SLEEP);
            }
        break;

        // FIXME rename this case to GAIN_PRE_STAGE or similar
        case DEBUG_EARLY_EVENT_0: {
                // this will optionally get ringbus to write to cs10 gain stage
                // based on what mode we chose
                /////////////////auto w = airTx->getGainRingbus(RING_ADDR_CS10, FFT_BARREL_SHIFT_CMD);

                if( GET_BARRELSHIFT_TX_SET() ) {
                    auto w = airTx->getGainRingbus(RING_ADDR_TX_FFT, FFT_BARREL_SHIFT_CMD);

                    for(unsigned i = 0; i < w.size(); i++) {
                        dsp->ringbusPeerLater(attached->peer_id, w[i], 1000+i*500);
                    }

                    cout << "Sent " << w.size() << " ringbus to cs10 gain fft stages\n";
                }

                next = GO_AFTER(WAIT_EVENTS, _1_MS_SLEEP);

                {
                // always disable mag check on start
                ///////////////////////////////raw_ringbus_t rb0 = {RING_ADDR_CS10, CS10_ENABLE_MAG_CHECK | 0};
                raw_ringbus_t rb0 = {RING_ADDR_TX_FFT, TX_ENABLE_MAG_CHECK | 0};
                dsp->ringbusPeerLater(attached->peer_id, &rb0, 0);
                }
        }
        break;

        case WAIT_EVENTS: {
            bool triggered = false;
            if( doWarmup ) {
                doWarmup = false;
                cout << "TxSchedule moving to DEBUG_TX_MAPMOV_0" << endl;
                triggered = true;
                next = GO_NOW(DEBUG_TX_MAPMOV_0);
                tx_loop_start = std::chrono::steady_clock::now();
            }

            if( !triggered ) {
                next = GO_NOW(WAIT_EVENTS);
            }
            break;
        }

        case DEBUG_TX_MAPMOV_0: {
                cout << "In event DEBUG_TX_MAPMOV_0" << endl;
                next = GO_AFTER(DEBUG_TX_MAPMOV_1, TINY_SLEEP);

                tickleAttachedProgress();

                if( tx_mapmov_rb_for_eth != 0 ) {
                    raw_ringbus_t rb0 = {RING_ADDR_ETH, tx_mapmov_rb_for_eth};
                    dsp->ringbusPeerLater(attached->peer_id, &rb0, 0);
                }




                // attached->allow_epoc_adjust = false;

                debug_est_run_target = 2;
                debug_est_run = 0;
                soapy->flushHiggs->allow_zmq_higgs = true;
            }
            break;

        case DEBUG_TX_MAPMOV_1: {
                attached->epoc_valid = false;

                tickleAttachedProgress(); // replaces REQUEST_EPOC_CMD

                // sends a ringbus to attached higgs
                // raw_ringbus_t rb0 = {RING_ADDR_CS20, REQUEST_EPOC_CMD};
                // dsp->ringbusPeerLater(attached->peer_id, &rb0, 0);
                // debug_est_run++;

                next = GO_AFTER(DEBUG_TX_MAPMOV_2, TINY_SLEEP);

            }
            break;

        case DEBUG_TX_MAPMOV_2:

            if( attached->epoc_valid ) {
                next = GO_AFTER(PERIODIC_SCHEDULE_0, TINY_SLEEP);
            } else {
                next = GO_AFTER(DEBUG_TX_MAPMOV_2, TINY_SLEEP);
            }
            break;


        case PERIODIC_SCHEDULE_0: {
                debug_grow_size = 1;
                soapy->flushHiggs->allow_zmq_higgs = false;
                // attached->allow_epoc_adjust = false;

                attached->map_mov_packets_sent = 0;
                attached->map_mov_acks_received = 0;
                integration_test_start_timepoint = std::chrono::steady_clock::now();

                // enable cs10 to check mag on our troubled subcarrier
                bool cs10_check_mag = false;
                if( cs10_check_mag ) {
                    ////////////////////raw_ringbus_t rb0 = {RING_ADDR_CS10, CS10_ENABLE_MAG_CHECK | 1};
                    raw_ringbus_t rb0 = {RING_ADDR_TX_FFT, TX_ENABLE_MAG_CHECK | 1};
                    dsp->ringbusPeerLater(attached->peer_id, &rb0, 0);
                }

                dumpUserData();

                setLinkUp(true);

                // next = GO_NOW(PERIODIC_SCHEDULE_1);
                idle_print = 0;
                next = GO_NOW(TX_SEND_BUSY_WAIT);
            }
            break;

        //FIXME rename
        // this is halt during error srate
        case TX_FSM_ERROR_CONDITION_0:
            cout << "=================================================" << endl;
            cout << "=" << endl;
            cout << "= Stopping due to missed packet, wip" << endl;
            cout << "=" << endl;
            cout << "=================================================" << endl;
            next = GO_NOW(DEBUG_STALL);
            break;


        // busy wait until next packet and then send
        // FIXME: rename
        case TX_SEND_BUSY_WAIT: {

                const auto state_woke = steady_clock::now();
                
                bool upper_print = idle_print < 3;
                idle_print++;

                if( upper_print ) {
                    cout << "-->Entering TX_SEND_BUSY_WAIT \n";
                }

                int error;

                // auto fill = soapy->flushHiggs->last_fill_level;
                // cout << "Fill at start is " << soapy->flushHiggs->last_fill_level << endl;

                // if( soapy->flushHiggs->last_fill_level < 2000 ) {
                // ok for now, we are sending 1ce a second, limit later on
                tickleAttachedProgress();
                // }

                if( errorDetected ) {
                    errorDetected = 0;
                    dsp->ringbusPeerLater(soapy->this_node.id, RING_ADDR_TX_PARSE, CHECK_LAST_USERDATA_CMD, 5);
                    next = GO_NOW(TX_FSM_RECOVER_ERROR_0_A);
                    errorRecoveryAttempts++;
                    break;
                }

                bool should_send_zmq_low = false;


                // cout << "Fill at end is " << soapy->flushHiggs->last_fill_level << endl;

                // if( soapy->flushHiggs->last_fill_level )
                // soapy->flushHiggs->dump_low_pri_buf = true; // dump only works if you call sendZmqHiggsNow
                // soapy->flushHiggs->sendZmqHiggsNow();
                // soapy->flushHiggs->dump_low_pri_buf = false; // why?

                // wait a few ticks before enabling
                if( debug_grow_size > 3 ) {
                    debugFillBuffers(3);  // this may take a long time as it reads off disk and is inefficient
                    debugFillZmq();
                }

                // if( needs_pre_zero ) {
                //     soapy->sendToHiggs(zrs);
                //     needs_pre_zero = false;
                // }

                epoc_frame_t woke_est = attached->predictScheduleFrame(error, false, false);
                if( upper_print ) {
                    cout << "                                       NOW "
                    << woke_est.epoc << ", " << (woke_est.frame / 512) << " (" << woke_est.frame << ")\n";
                }

                /// How many old packets should we try and dump
                constexpr int dump_old = 5;
                for(int i = 0; i < dump_old; i++ ) {
                    loadPendingMember(); // will set pending_valid flag
                    loadZmqBeamform(woke_est.epoc, woke_est.frame);
                }

                const bool isDebugDuplexPacket = isDebugDuplex(pending_valid);

                if( isDebugDuplexPacket ) {
                    // proceed with debug because pending_valid is not set
                    // if pending valid was set, we would not enter here

                    applyDebugDuplex1();

                } else if( pending_valid == 0 ) {

                    if(soapy->flushHiggs->getLowLen() > 0) {
                        should_send_zmq_low = true;
                    }

                    if( should_send_zmq_low ) {
                        sendLowLoops = 0;
                        next = GO_NOW(TX_FSM_SEND_LOW_PRI_0);
                    } else {
                        auto next_time = (struct timeval){ .tv_sec = 0, .tv_usec = (long int)(1000) };
                        next = GO_AFTER(TX_SEND_BUSY_WAIT, next_time);
                    }

                    break;
                    // desired_wakeup_ts = scheduleData->pickTimeslot();
                }
                // you may pass
                /// mode 1 or 2 are ok

                constexpr int tol = 250; // how early or late is acceptable?
                int desired_wakeup_frame;
                int target_wakeup;
                epoc_frame_t target_wakeup_est = {0,0};

                if( isDebugDuplexPacket ) {
                    desired_wakeup_frame = pending_lifetime;

                    int desired_early_frames = scheduleData->pickEarlyFrames();
                    soapy->flushHiggs->member_early_frames = desired_early_frames;

                    target_wakeup = desired_wakeup_frame - desired_early_frames;//(SCHEDULE_FRAMES + desired_wakeup_frame-desired_early_frames) % SCHEDULE_FRAMES;

                    // const epoc_frame_t pending = {0, 0};
                    // target_wakeup_est = adjust_frames_to_schedule(pending, 0);

                } else {
                    desired_wakeup_frame = pending_ts*512; // calculated from prev
                    

                    int desired_early_frames = scheduleData->pickEarlyFrames();
                    soapy->flushHiggs->member_early_frames = desired_early_frames;

                    target_wakeup = (SCHEDULE_FRAMES + desired_wakeup_frame-desired_early_frames) % SCHEDULE_FRAMES;

                    {
                        const epoc_frame_t pending = {pending_epoc, pending_ts*512};
                        target_wakeup_est = adjust_frames_to_schedule(pending, -desired_early_frames);
                    }

                }
                const auto before_early_pack = steady_clock::now();


                bool skip_this_packet = false;


                /// Because we are getting stuck in user data and not sending eq:
                // last_low = std::chrono::steady_clock::now();
                size_t elapsed_us_low = std::chrono::duration_cast<std::chrono::microseconds>( 
                before_early_pack - last_low
                ).count();

                const auto low_len_1 = soapy->flushHiggs->getLowLen();

                if( (elapsed_us_low > 2E6) && (low_len_1 > 0) ) {
                    cout << "Forcing LOW after " << ((double)elapsed_us_low / 1E6) << " seconds "
                    << "for " << low_len_1 << " bytes\n";
                    sendLowLoops = 0;
                    next = GO_NOW(TX_FSM_SEND_LOW_PRI_0);
                    break;
                }



                auto now_lifetime_est = attached->predictLifetime32(error, false);

                /// Because we can't abort below, we calculate another estimate of our sleep frames
                /// this is in addition to the one below.  This estimate is based of a different time
                /// but that should be close enough
                /// if our sleep time would be huge, we just break
                /// this gives us enought time to possibly enter a ZMQ low priority above if
                /// our sleep time is over 1/3 of a second
                // auto woke_est_sleep_frames = schedule_delta_frames(target_wakeup_est, woke_est);

                auto woke_est_sleep_frames = (int64_t)target_wakeup - (int64_t)now_lifetime_est;

                if( woke_est_sleep_frames > (SCHEDULE_FRAMES/3) ) {
                    if( long_sleep_print < 2 ) {
                        cout << "WOULD HAVE SLEPT FOR " << woke_est_sleep_frames << " instead BREAKING\n";
                    }
                    long_sleep_print++;

                    /// This is the one case where we  CAN do zmq low pri. because the next packet
                    /// is so far in the future

                    if(soapy->flushHiggs->getLowLen() > 0) {
                        should_send_zmq_low = true;
                    }

                    if( should_send_zmq_low ) {
                        sendLowLoops = 0;
                        next = GO_NOW(TX_FSM_SEND_LOW_PRI_0);
                    } else {
                        auto next_time = (struct timeval){ .tv_sec = 0, .tv_usec = (long int)(1000) };
                        next = GO_AFTER(TX_SEND_BUSY_WAIT, next_time);
                    }

                    break;
                }

                long_sleep_print = 0;
                idle_print = 0;

                tickleAttachedProgress();


                /// Once we get Below here (below pullAndPrepTunData)
                /// if we do not send the packet, we are DROPPING data
                /// so we must always send in this case

                 // pulling raw data from the tun
                std::vector<uint32_t> d;
                if (isDebugDuplexPacket) {

                    d = applyDebugDuplex2();

                } else if (this->tunPacketsReady() ) {
                    std::cout << "Size of FIFO is: " << soapy->txTun2EventDsp.size() << "\n";
                    // std::cout << "Begin to send ping data\n";

                    // this is "how much"
                    // uint32_t wordsPerPacket = scheduleData->pickPacketSize();


                    // only call if data is ready
                    d = this->pullAndPrepTunData(0);

                    const auto left = soapy->txTun2EventDsp.size();

                    if( left != 0 ) {
                        cout << "Warning!!! pullAndPrepTunData() left " << left << " junk in tun\n";
                    }

                    
                } else {
                    // std::cout << "Inside new_subcarrier_data_sync\n";
                    // unsigned loop_size = 3;
                    // for(unsigned int i = 0; i < loop_size; i++) {
                    //     new_subcarrier_data_sync(&d, 0x0, airTx->enabled_subcarriers);
                    //     new_subcarrier_data_sync(&d, 0x0, airTx->enabled_subcarriers);
                    // }
                    skip_this_packet = true;
                }

                const uint32_t final_d_length = d.size();

                // make sure data is a multiple of words per ofdm frame


                const uint32_t enabled_subcarriers = airTx->enabled_subcarriers;

                const uint32_t constellation = airTx->modulation_schema;


                if( duplex_skip_body ) {
                    d.resize(20*5); // 5 includes jansons header
                }

                // assumes that 0 and 1023 are the boundaries
                // auto custom_size = header + ceil((d.size()*16)/enabled_subcarriers)*1024;
                auto packet = 
                    feedback_vector_mapmov_scheduled_sized(
                        FEEDBACK_VEC_TX_USER_DATA, 
                        d, 
                        enabled_subcarriers, 
                        0,
                        FEEDBACK_DST_HIGGS,
                        0, // timeslot fillin, overwritten later
                        0,  // epoc fillin, overwritten later
                        constellation
                        );



                // epoc_frame_t est;
                int loops = 0;
                const auto before_loop = steady_clock::now();


                // est = attached->predictScheduleFrame(error, false, false);
                // auto sleep_frames = schedule_delta_frames(target_wakeup_est, est);

                const auto now_lifetime_est_bravo = attached->predictLifetime32(error, false);
                const auto sleep_frames = (int64_t)target_wakeup - (int64_t)now_lifetime_est_bravo;

                cout << "WILL SLEEP FOR " << sleep_frames << "\n";

                if( sleep_frames < 0 ) {
                    std::string reason("NEGATIVE SLEEP: ");
                    reason += std::to_string(sleep_frames);
                    dumpPendingPacket(reason, now_lifetime_est_bravo);
                    next = GO_NOW(TX_SEND_BUSY_WAIT);
                    break;
                }


                auto now_lifetime_est_charlie = attached->predictLifetime32(error, false);

                const uint32_t supress_feedback_wakeup = DUPLEX_FRAMES*2;

                const uint32_t estimate_busy_until = estimateBusy(pending_lifetime, final_d_length);

                bool sent_supress = false;
                

                // busy wait loop
                do {
                    now_lifetime_est_charlie = attached->predictLifetime32(error, false);
                    // cout << "Fetched Epoc " << endl;
                    // cout << est.epoc << endl;
                    if( ((int64_t)target_wakeup) < (int64_t)now_lifetime_est_charlie) {

                        // FIXME enable when merging the rest of ameet/arb_throughput
                        // dsp->sendJointAck(pending_from_peer, pending_epoc, pending_ts, pending_seq, now_epoc, now_frame, true);

                        pending_valid = 0;
                        skip_this_packet = true;
                        break;
                    }

                    const auto sleep_frames_charlie = (int64_t)target_wakeup - (int64_t)now_lifetime_est_charlie;
                    // uint32_t mod = ((SCHEDULE_FRAMES + est.frame - target_wakeup + tol)%SCHEDULE_FRAMES);

                    // cout << "rlie: " << sleep_frames_charlie << " " << (supress_feedback_wakeup+tol) << "\n";

                    if( !sent_supress && (sleep_frames_charlie < (supress_feedback_wakeup+tol)) )  {
                        dsp->suppressFeedbackEqUntil(estimate_busy_until, true);
                        // cout << "sent: " << sleep_frames_charlie << "\n";
                        sent_supress = true;
                    }

                    if( (sleep_frames_charlie >= -tol) && (sleep_frames_charlie <= tol) ) {
                        break;
                    }
                    // cout << est.frame << endl;
                    usleep(10);

                    loops++;

                } while (1);




                const auto after_loop = steady_clock::now();


                
                set_mapmov_lifetime_32(packet, pending_lifetime);


                if( print1 ) {
                    cout << "\n\n";
                    for(auto w : packet) {
                        cout << HEX32_STRING(w) << "\n";
                    }
                    cout << "\n\n";
                }





                const auto after_pack = steady_clock::now();

                if( skip_this_packet ) {
                    cout << "About to skip packet at " << pending_lifetime << " (" << (pending_lifetime/DUPLEX_FRAMES) << ")" <<'\n';
                } else {
                    cout << "About to send packet at " << pending_lifetime << " (" << (pending_lifetime/DUPLEX_FRAMES) << ")" <<'\n';
                }

                if( !skip_this_packet ) {

                    constexpr size_t speedup_size = 14000;

                    // in the case we have an exceptionally large packet
                    // we split it and send it to FlushHiggs early
                    // this should set the 8k timer more acuratly 
                    // and result in a smaller error from our desired 3000 early

                    if( duplex_debug_split ) {

                        const uint32_t slicewc = airTx->sliced_word_count;

                        const std::pair<uint32_t,uint32_t> j0 = {43*slicewc,43};
                        const std::pair<uint32_t,uint32_t> j1 = {42*slicewc,(43+42)};

                        auto inParts = splitFeedbackVectorMapmovScheduledSized(
                            d,
                            j0,
                            j1,
                            slicewc,
                            enabled_subcarriers,
                            pending_lifetime,
                            constellation,
                            true
                        );
                        soapy->sendToHiggs(inParts);

                    } else {


                        if(packet.size() > (speedup_size+1) ) {
                            std::vector<uint32_t> split_a(packet.begin(), packet.begin() + speedup_size);
                            soapy->sendToHiggs(split_a);

                            std::vector<uint32_t> split_b(packet.begin() + speedup_size, packet.end());
                            soapy->sendToHiggs(split_b);
                            // soapy->sendToHiggs(zrs);
                        } else {
                            soapy->sendToHiggs(packet);
                            // soapy->sendToHiggs(zrs);
                        }

                }



                    cout << "Sent packet sized: " << packet.size() << "\n";


                    if(isDebugDuplexPacket) {
                        finishedDebugDuplex(final_d_length, false);
                    }


                    if( (!isDebugDuplexPacket) && pending_valid != 2 ) {
                        cout << "Something went wrong with pending_valid value " << pending_valid << "\n";
                    }

                    pending_valid = 0; // reset so loadPendingMember() can load



                } else {
                    if( pending_valid == 2 ) {
                        cout << "skip_this_packet was set and pending_valid was " << pending_valid
                        << " We probably dropped packet: " 
                        << pending_epoc << ", " << pending_ts << ", " << (int)pending_seq << "\n";
                    }
                    if(isDebugDuplexPacket) {
                        finishedDebugDuplex(0, true); // pass true because we skipped
                    }
                }


                if( !skip_this_packet && !sent_supress ) {
                    cout << "\nWARNING we didn't supress eq feedback, something is wrong\n\n";
                }




                const auto after_send = steady_clock::now();

                if( !skip_this_packet ) {
                    attached->map_mov_packets_sent++;
                }

                // cout << "TX_SEND_BUSY_WAIT" << '\n';

                debug_grow_size++;
                // cout << airTx->getMapMovVectorInfo(d) << " (loop size) " << debug_grow_size << '\n';

                    
                // const auto epoc_reply = attached->getEpocRecentReply();

                // // DID WE GET A VALID REPLY?
                // if(!epoc_reply.first) {
                //     // status->set("tx.mapmov_reply", STATUS_ERROR, "PERIODIC_SCHEDULE_2 had issue");
                // } else {
                // }

                status->set("tx.mapmov_reply", STATUS_OK);

                constexpr bool pauseOnError1 = false;

                bool error1 = false;

                cout << "!!!!!!!!! " << attached->map_mov_packets_sent << ", " << attached->map_mov_acks_received << '\n';

                if( 
                        attached->map_mov_packets_sent != (attached->map_mov_acks_received+1)
                    && (attached->map_mov_packets_sent != attached->map_mov_acks_received)
                    ) {
                    error1 = true;
                }

                if( requestPause ) {
                    error1 = true;
                }

                bool error2 = errorDetected;
                errorDetected = 0; // reset flag

                // cout << "code above looped for " << loops << '\n';

                // assuming next packet time is 1 second away, sleep a bit shorter so loop above will run



                if( 
                    //    (debug_grow_size > 4)
                    // && ((debug_grow_size % 3) == 0)
                    (soapy->flushHiggs->getLowLen() > 0)
                     ) {
                    should_send_zmq_low = true;
                }
                // should_send_zmq_low = false;

                loadPendingMember();
                if( pending_valid ) {
                    should_send_zmq_low = false;
                }


                if( !pauseOnError1 ) {
                    error1 = false;
                }

                // for( auto w : packet ) {
                //     cout << HEX32_STRING(w) << '\n';
                // }


                if( error1 ) {
                    // we got an error condition
                    // let the rest of the printing below run
                    next = GO_NOW(TX_FSM_ERROR_CONDITION_0);

                } else if( error2 ) {
                    dsp->ringbusPeerLater(soapy->this_node.id, RING_ADDR_TX_PARSE, CHECK_LAST_USERDATA_CMD, 5);
                    next = GO_NOW(TX_FSM_RECOVER_ERROR_0_A);
                    errorRecoveryAttempts++;
                } else if( exitDataMode ) {
                    exitDataMode = false;
                    cout << "TxSchedule exiting data mode\n";
                    GO_AFTER(WAIT_EVENTS,_1_SECOND_SLEEP);
                } else {

                    // pick normal wakeup time
                    // was 0.7
                    double seconds_until_wakeup = scheduleData->pickSecondSleep();

                    if( should_send_zmq_low ) {
                        sendLowLoops = 0;
                        next = GO_NOW(TX_FSM_SEND_LOW_PRI_0);
                    } else {
                        auto next_time = (struct timeval){ .tv_sec = 0, .tv_usec = (long int)(seconds_until_wakeup*1E6) };
                        next = GO_AFTER(TX_SEND_BUSY_WAIT, next_time);
                    }
                }

                state_went_to_sleep = steady_clock::now(); // NOT A MEMBER
                // not as accuate as prints are later?? or does that matter?

                bool printTimes = true;
                if(printTimes) {
                    cout << " Wakeup at absolute: " << getDeltaTime(tx_loop_start, state_woke) << '\n';
                    cout << "   Wake to pack       : " << getDeltaTime(state_woke, before_early_pack) << '\n';
                    cout << "   Early pack ran for : " << getDeltaTime(before_early_pack, before_loop) << '\n';
                    cout << "   Loop Ran For       : " << getDeltaTime(before_loop, after_loop) << '\n';
                    cout << "   Pack ran for       : " << getDeltaTime(after_loop, after_pack) << '\n';
                    cout << "   Send ran for       : " << getDeltaTime(after_pack, after_send) << endl;
                    // cout << "   Enter Loop at delta: " << getDeltaTime(after_pack, after_send) << endl;
                }


            }
            break;

        case TX_FSM_SEND_LOW_PRI_0: {

            last_low = steady_clock::now();
            // soapy->flushHiggs->allow_zmq_higgs = true;
            bool breakLoop = false;
            const auto fill_level = soapy->flushHiggs->last_fill_level;
            const auto normal_level = soapy->flushHiggs->getNormalLen();
            const auto low_level    = soapy->flushHiggs->getLowLen();
            const bool valid_8k = soapy->flushHiggs->valid_8k;
            cout << "TX_FSM_SEND_LOW_PRI_0 with fill "
            << fill_level << ", "
            << attached->map_mov_packets_sent << ", "
            << attached->map_mov_acks_received << ", "
            << normal_level << ", "
            << low_level << ", "
            << "8k " << valid_8k
            << "\n";
            if( 
                (attached->map_mov_packets_sent == attached->map_mov_acks_received)
                && fill_level < 1
                && normal_level == 0 // normal level must be zero or else we will interject in the middle of that stream
                && !valid_8k
                ) {
                breakLoop = true;
            }

            if( sendLowLoops > 2100 ) {
                cout << "will fail!\n\n";
                dsp->ringbusPeerLater(soapy->this_node.id, RING_ADDR_TX_PARSE, CHECK_LAST_USERDATA_CMD, 5);
                // next = GO_NOW(TX_FSM_RECOVER_ERROR_0_A);
                next = GO_AFTER(TX_FSM_RECOVER_ERROR_0_A,_4_SECOND_SLEEP);
                errorRecoveryAttempts++;
                debug_grow_size = 1;
            } else {
                if( breakLoop ) {
                    next = GO_AFTER(TX_FSM_SEND_LOW_PRI_1, _10_MS_SLEEP);
                } else {
                    next = GO_AFTER(TX_FSM_SEND_LOW_PRI_0, _1_MS_SLEEP);
                }
            }
            sendLowLoops++;
        }
        break;

        case TX_FSM_SEND_LOW_PRI_1: {
            cout << "TX_FSM_SEND_LOW_PRI_1\n";
            soapy->flushHiggs->allow_zmq_higgs = true;
            // soapy->flushHiggs->sendZmqHiggsNow("TX_FSM_SEND_LOW_PRI_1 A");
            // soapy->flushHiggs->sendZmqHiggsNow("TX_FSM_SEND_LOW_PRI_1 B");
            
            // cout << "TX_FSM_SEND_LOW_PRI_1 finished" << endl;
            next = GO_AFTER(TX_FSM_SEND_LOW_PRI_2, _50_MS_SLEEP);
        }
        break;

        case TX_FSM_SEND_LOW_PRI_2: {
            soapy->flushHiggs->allow_zmq_higgs = false;
            soapy->flushHiggs->exitSendingLowMode();
            // needs_pre_zero = true;
            cout << "TX_FSM_SEND_LOW_PRI_2 finished\n";
            idle_print = 0;
            next = GO_AFTER(TX_SEND_BUSY_WAIT, _10_MS_SLEEP);
        }
        break;


        // ask js to fix feedback bus
        case TX_FSM_RECOVER_ERROR_0_A: {
            setLinkUp(false);
            soapy->flushHiggs->allow_zmq_higgs = false;
            cout << "TX_FSM_RECOVER_ERROR_0_A\n";
            // settings->getDefault<bool>(false, "runtime", "r0", "skip_tdma")
            // ringbusPeerLater(attached->peer_id, (raw_ringbus_t){RING_ADDR_CS20, SMODEM_BOOT_CMD|0x2}, 0);
            errorDetectedLocal = errorDetected;
            next = GO_AFTER(TX_FSM_RECOVER_ERROR_1_A, _15_MS_SLEEP);
        }
        break;

        case TX_FSM_RECOVER_ERROR_1_A: {
            cout << "TX_FSM_RECOVER_ERROR_1_A\n";
            if( errorDetectedLocal == errorDetected ) {
                cout << "TX_FSM_RECOVER_ERROR_1_A notifying javascript\n";
                flushZerosEnded = 0;
                flushZerosStarted = 0;
                soapy->settings->vectorSet(true, (std::vector<std::string>){"runtime","js","recover_fb","request"});
                next = GO_AFTER(TX_FSM_RECOVER_ERROR_2_A, _15_MS_SLEEP);
            } else {
                long int delta = errorDetected - errorDetectedLocal;
                cout << "TX_FSM_RECOVER_ERROR_1_A delta errors " << delta << '\n';
                next = GO_NOW(TX_FSM_RECOVER_ERROR_0_A);
            }
        }
        break;

        case TX_FSM_RECOVER_ERROR_2_A: {
            cout << "TX_FSM_RECOVER_ERROR_2_A\n";
            bool done = settings->getDefault<bool>(false, "runtime", "js", "recover_fb", "result");
            if( !done ) {
                // loop until done
                next = GO_AFTER(TX_FSM_RECOVER_ERROR_2_A, _500_MS_SLEEP);
            } else {
                cout << "TX_FSM_RECOVER_ERROR_2_A got notification from javascript\n";
                soapy->settings->vectorSet(false, (std::vector<std::string>){"runtime","js","recover_fb","result"});
                // go to previos error recovery code
                next = GO_AFTER(TX_FSM_RECOVER_ERROR_0, _15_MS_SLEEP);
            }
        }
        break;


        // we got an error, usually these come in large batches
        // copy our atomic variable and go to next step
        case TX_FSM_RECOVER_ERROR_0: {
            setLinkUp(false);
            tickleAttachedProgress();
            soapy->flushHiggs->allow_zmq_higgs = false;
            cout << "TX_FSM_RECOVER_ERROR_0\n";
            errorDetectedLocal = errorDetected;
            next = GO_AFTER(TX_FSM_RECOVER_ERROR_1, _15_MS_SLEEP);
        }
        break;


        // check if the copy of atomic variable is the same
        // if not they are still coming, so keep waiting
        // if so this means the errors have stopped
        // once the errors have stopped, set flushZerosEnded to zero
        case TX_FSM_RECOVER_ERROR_1: {
            cout << "TX_FSM_RECOVER_ERROR_1\n";
            if( errorDetectedLocal == errorDetected ) {
                flushZerosEnded = 0;
                flushZerosStarted = 0;
                next = GO_AFTER(TX_FSM_RECOVER_ERROR_2, _15_MS_SLEEP);
            } else {
                long int delta = errorDetected - errorDetectedLocal;
                cout << "TX_FSM_RECOVER_ERROR_1 delta errors " << delta << '\n';
                next = GO_NOW(TX_FSM_RECOVER_ERROR_0);
            }
        }
        break;

        // make sure there are no zeros coming as well, if so go back to beginning
        // if the zeros have stopped, we send our own zeros
        case TX_FSM_RECOVER_ERROR_2: {
            cout << "TX_FSM_RECOVER_ERROR_2\n";
            if( flushZerosStarted != 0 || flushZerosEnded != 0) {
                next = GO_NOW(TX_FSM_RECOVER_ERROR_0);
            } else if( errorDetectedLocal != errorDetected ) {
                next = GO_NOW(TX_FSM_RECOVER_ERROR_0);
            } else {
                std::vector<uint32_t> zrs2;
                zrs2.resize(64, 0);
                soapy->sendToHiggs(zrs2);
                next = GO_NOW(TX_FSM_RECOVER_ERROR_3);
                errorFlushZerosWaiting = 0;
            }
            // errorDetectedLocal = errorDetected;
            // next = GO_AFTER(TX_FSM_RECOVER_ERROR_1, _1_MS_SLEEP);
        }
        break;

        // verify that we have a single "zero flushing started" event
        case TX_FSM_RECOVER_ERROR_3: {
            cout << "TX_FSM_RECOVER_ERROR_3\n";
            if( false && flushZerosStarted == 0) {
                if( errorFlushZerosWaiting > 20 ) {
                    next = GO_AFTER(TX_FSM_RECOVER_ERROR_2, _1_MS_SLEEP);
                } else {
                    next = GO_AFTER(TX_FSM_RECOVER_ERROR_3, _1_MS_SLEEP); // wait longer
                }
            } else if( true || flushZerosStarted == 1 ) {
                // all ok
                // flushZerosStarted = 0;
                flushZerosEnded = 0;
                errorDetectedLocal = 0;
                errorFlushZerosWaiting = 0;

                std::vector<uint32_t> dumb;
                dumb.resize(16);
                auto packet = feedback_vector_packet(
                    FEEDBACK_VEC_STATUS_REPLY,
                    dumb,
                    0,
                    FEEDBACK_DST_HIGGS);
                soapy->sendToHiggs(packet);

                tickleAttachedProgress();

                next = GO_AFTER(TX_FSM_RECOVER_ERROR_4, _15_MS_SLEEP);
            } else {
                next = GO_NOW(TX_FSM_RECOVER_ERROR_0); // got more than one zero flushing, restart whole thing
            }
            errorFlushZerosWaiting++;
        }
        break;

        // verify that we have a single "zero flushing ended" event
        case TX_FSM_RECOVER_ERROR_4: {
            cout << "TX_FSM_RECOVER_ERROR_4\n";

            if( flushZerosEnded == 0) {
                if( errorFlushZerosWaiting > 20 ) {
                    next = GO_AFTER(TX_FSM_RECOVER_ERROR_2, _1_MS_SLEEP);
                } else {
                    next = GO_AFTER(TX_FSM_RECOVER_ERROR_4, _1_MS_SLEEP); // wait longer
                }
            } else if( flushZerosEnded == 1 ) {
                // all ok
                flushZerosStarted = 0;
                flushZerosEnded = 0;
                errorDetectedLocal = 0;
                attached->map_mov_acks_received = 0;
                attached->map_mov_packets_sent = 0;
                // needs_pre_zero = true;
                dumpUserData();
                setLinkUp(true);
                cout << "EXITING RECOVERY, BACK TO TX_SEND_BUSY_WAIT\n";
                idle_print = 0;
                next = GO_NOW(TX_SEND_BUSY_WAIT);
            } else {
                next = GO_NOW(TX_FSM_RECOVER_ERROR_0); // got more than one zero flushing, restart whole thing
            }
            errorFlushZerosWaiting++;
        }
        break;




        case DEBUG_STALL:
        case TX_STALL:
            setLinkUp(false);
            GO_AFTER(TX_STALL, _1_SECOND_SLEEP);
            break;

        break;
        default:
            cout << "Bad state (" << dsp_state << ") in TxSchedule::tickfsm" << endl;
            break;
    }


    if( __dsp_preserve != dsp_state ) {
        cout << endl << "ERROR dsp_state changed during operation.  Expected: " 
        << __dsp_preserve
        << " Got: " << dsp_state
        << endl;
        usleep(100);
        assert(0);
    }

    if(__timer != __UNSET_NEXT__) {
        // cout << "timer mode" << endl;
        dsp_state_pending = __timer;
// #ifdef USE_DYNAMIC_TIMERS
//         tv = time_multiply(tv, GET_RUNTIME_SLOWDOWN()); // modify in place should be ok, I don't think we re-use tv.. do we?
// #endif
        evtimer_add(fsm_next_timer, &tv);
    } else if(__imed != __UNSET_NEXT__) {
        // cout << "imed mode" << endl;
        dsp_state_pending = __imed;
        auto timm = SMALLEST_SLEEP;
        evtimer_add(fsm_next_timer, &timm);
    } else if(next != 0 && next != __UNSET_NEXT__) {
        cout << endl << "ERROR You cannot use:  ";
        cout << endl << "   next = " << endl << endl << "without GO_NOW() or GO_AFTER()      (inside TxSchedule::tickFsm)" << endl << endl << endl;
        usleep(100);
        assert(0);
    } else {
        // cout << "default mode" << endl;
        // default
        dsp_state_pending = dsp_state;
        evtimer_add(fsm_next_timer, &tv);
        // cout << "using default next in tiskFsm" << endl;
    }
}
#pragma GCC diagnostic pop
