#pragma once


#include "HiggsStructTypes.hpp"

#include "HiggsEvent.hpp"

#include "HiggsSoapyDriver.hpp"

// #include "driver/AttachedEstimate.hpp"
#include "driver/SubcarrierEstimate.hpp"

#include "driver/HiggsSettings.hpp"
#include "driver/StatusCode.hpp"
// #include "driver/AirPacket.hpp"
#include "driver/VerifyHash.hpp"
// #include "driver/RetransmitBuffer.hpp"


// #include "HiggsDriverSharedInclude.hpp"

// also gets feedback_bus.h from riscv
// #include "feedback_bus_tb.hpp"

// #include "vector_helpers.hpp"


#include <event2/bufferevent.h>
#include <event2/buffer.h>
// #include <type_traits>
#include <typeinfo>
#include <future>
#include <functional>
#include <atomic>
#include <chrono>

class RadioEstimate;
class AttachedEstimate;
class StatusCode;
class RetransmitBuffer;
class NullBufferSplit;
class DispatchMockRpc;


#include "DashTie.hpp"
#include "3rd/json.hpp"

typedef std::function<void(const bool, const double)> zmq_fetch_double_t;
typedef std::function<void(const bool, const uint32_t)> zmq_fetch_uint32_t_t;
typedef std::function<void(const bool, const std::pair<uint32_t,uint32_t>)> zmq_fetch_pair_uint32_t_uint32_t_t;

typedef std::function<size_t(void)> packets_ready_t;
typedef std::function<std::vector<uint32_t>(void)> next_packet_t;
typedef std::function<size_t(void)> first_packet_len_t;
typedef std::function<uint32_t(const std::vector<uint32_t>&)> hash_for_words_t;
typedef std::function<void(const std::vector<uint32_t>&, const std::vector<std::vector<uint32_t>>&, const uint32_t)> retransmit_t;



class EventDsp : public HiggsEvent
{
public:
    EventDsp(HiggsDriver* s);
    void setupBuffers();
    void setupRadioVector();
private:
    void setupAttachedRadio();
    HiggsSettings* settings;
public:
    StatusCode* status;
    VerifyHash* verifyHashArb;
    RetransmitBuffer* retransmitArb;
    
private:
    const static unsigned int num_subcarrier = 32;
    const unsigned int test_value = 0xdeadbeef;
    double evm_buffer[num_subcarrier];
    // copied from json one time
public:
    std::string role;

    // struct event* event_ev;

    // cross thread between rx_thread and this one
    BevPair* udp_payload_bev = 0;
    // created on this thread
    BevPair* all_sc_buf = 0;
    BevPair* coarse_bev = 0;

    // created on this thread, drains into other thread
    BevPair* gnuradio_bev = 0;
    BevPair* gnuradio_bev_2 = 0;
    BevPair* fb_pc_bev = 0;
    BevPair* rb_in_bev = 0;
    // BevPair* sliced_data_bev = 0;

    // master source of enabled subcarriers
    mutable std::mutex _demod_buf_enabled_mutex;
    std::vector<unsigned> demod_buf_enabled_sc;


    std::vector<RadioEstimate*> radios;
    ///
    /// The attached member only works on tx side for now
    /// we could add one for the rx side
    /// any new behaviour should just subclass RadioEstimate*
    /// eventually we can be really smart about subclassing
    AttachedEstimate* attached = 0;

    uint32_t nodeIdForRadioIndex(const size_t) const;
    size_t radioIndexForNodeId(const uint32_t) const;
    uint32_t subcarrierForBufferIndex(const size_t i) const;
    // void enableDemodScIndex(const std::vector<unsigned> &in);

    void statusTimerVerifyRxDataRates();
    // lump in methods from SoapyFeedbackBus
    void handleFineSync(const feedback_frame_vector_t *vec);
    void parseFeedbackBuffer(struct evbuffer *input);
    void handleAllSc(const uint32_t seq, const uint32_t* data, const uint32_t len);
    void handleParsedFrame(const uint32_t* full_frame, const size_t length);  // came over the wire from our attached higgs
    void handleParsedFrameForPc(uint32_t* full_frame, size_t length);  // came over the wire from zmq marked for PC
    void handlePcPcVectorType(const uint32_t* const full_frame, const size_t length_words);
    void handleSlicedData(const feedback_frame_vector_t *vec);
    void printFeedbackBusStatus();
    void setupFeedbackBus();
    std::atomic<bool> _rx_should_insert_sfo_cfo;
    std::atomic<bool> should_inject_r0;
    std::atomic<bool> should_inject_r1;
    std::atomic<bool> should_inject_attached;
private:
    int fb_jamming;
    int fb_error_count;
    bool got_jam;
    std::chrono::steady_clock::time_point feedback_bus_print_time;
    bool dump_fine_sync; // cfo and sfo
    std::string dump_fine_filename; // sfo
    std::ofstream *dump_fine_ostream; // sfo
    std::string dump_fine_cfo_filename;
    std::ofstream *dump_fine_cfo_ostream;
    std::vector<uint32_t> fine_sync_buf;
    std::vector<uint32_t> parse_fine_history;
    std::vector<uint32_t> parse_stream_history;
public:
    // SafeQueue<uint32_t> all_sc_buf;
    // SafeQueueTracked<uint32_t, uint32_t> coarse_buf;
    SafeQueueTracked<uint32_t, uint32_t> demod_buf[DATA_TONE_NUM];
    
    // Callback wrappers etc
    void handle_sfo_cfo_callback(struct bufferevent *bev);
    void handle_all_sc_callback(struct bufferevent *bev);
    // void handleCoarseCallback(struct bufferevent *bev);
    void handleGnuradioCallback(struct bufferevent *bev);
    void handleCustomEventCallback(struct bufferevent *bev);
    void handleDemodCallback(struct bufferevent *bev);
    void handleInboundFeedbackBusCallback(struct bufferevent *bev);
    void handleCfoTimer();
    void handleCfoTimer2();
    void outsideFillDemodBuffers(SafeQueueTracked<uint32_t, uint32_t>(&drain_from) [DATA_TONE_NUM]);
    void tickAllRadios();

    // Helpers
    std::vector<uint32_t> whoGetsWhatData(const std::string estimate, const uint32_t rx_frame);

    // Controls
    // int estimate_filter_mode;
    bool init_drain_udp_payload = true;
    bool init_drain_all_sc = true;
    bool init_drain_coarse = true;
    bool init_drain_for_gnuradio = true;
    bool init_drain_for_gnuradio_2 = true;
    bool init_drain_for_inbound_fb_bus = true;
    bool init_drain_custom_events = true;
    bool init_drain_ringbus_in = true;
    // bool init_drain_for_sliced_data;
    
    bool drain_gnurado_stream; // copied from settings object
    bool drain_gnurado_stream_2; // copied from settings object

    bool gnuradio_stream_single_sc = false;
    unsigned gnuradio_stream_single_offset = 62;
    std::vector<unsigned char> grc_single_sc_buffer;

    unsigned downsample_gnuradio;

    bool init_fsm = true;
    bool init_feedback_bus = true;

private:
    // FSM
    void setupFsm();
public:
    void tickFsmRx();
    void tickFsmTx();
private:
    void fsmSetup_fixme();
    void executeChangeState();
public:
    int changeState(const uint32_t desired);
    void resetCoarseState();
    struct event* fsm_init = 0;
    struct event* fsm_next = 0;
    struct event* fsm_next_timer = 0;
    struct event* tx_fsm_next = 0;
    struct event* tx_fsm_next_timer = 0;
    struct event* radios_tick_timer = 0;
    mutable std::mutex _handle_sfo_cfo_callback_running_mutex;
private:

    mutable std::mutex _change_pre_mutex;
    uint32_t _pending_change_state;
    bool _change_pending = false;

    uint32_t times_blocked_fsm_start;
public:
    struct event* poll_rb_timer; // fixme replace with direct socket monitor
    struct event* sendto_higgs_timer; // feed data into data port of higgs

private:
    std::future<void> future_cfo_0;
    bool future_cfo_0_running;

    std::future<void> future_residue_0;
    bool future_residue_0_running;

    std::future<void> future_residue_1;
    bool future_residue_1_running;


public:
    // puppet master fsm data
    size_t dsp_state;
    size_t dsp_state_pending;
    size_t tx_dsp_state;
    size_t tx_dsp_state_pending;
    size_t _do_wait;
private:
    std::pair<bool, uint32_t> optional_rb_word;
    uint32_t coarse_est;
    std::chrono::steady_clock::time_point _delay_period;// = clock::now();
    std::chrono::duration<double> _delay_elapsed;
public:
    uint32_t fuzzy_recent_frame;

    // vectors for schedule
    std::vector<uint32_t> schedule_on;
    std::vector<uint32_t> schedule_off;
    std::vector<uint32_t> schedule_0;
    std::vector<uint32_t> schedule_1;
    std::vector<uint32_t> schedule_toggle;
    std::vector<uint32_t> schedule_bringup;
    std::vector<uint32_t> schedule_bringup_inverse;
    std::vector<uint32_t> schedule_missing_5;
    std::vector<uint32_t> schedule_missing_6;
    std::vector<uint32_t> schedule_missing_10;
    std::vector<uint32_t> schedule_missing_20;
private:

    // file dump
    bool _enable_file_dump_sliced_data = false;
    bool _file_dump_sliced_data_open = false;
    std::ofstream _sliced_data_f;
    bool _dump_residue_upstream = false;
    bool _dump_residue_upstream_fileopen = false;
    std::string _dump_reside_upstream_fname;
    std::ofstream _dump_residue_upstream_ofile;
    
public:
    void generateSchedule();
    int32_t getPeerMaskForFrame(const uint32_t frame) const;
    void printSchedule(const std::string& name, const std::vector<uint32_t>& schedule) const;
    uint32_t timeslotForFrame(const uint32_t frame) const;



    // flags
    void updateSettingsFlags();

    // other junk
    // uint32_t dspMonitorRb();

    // around both of these vectors
    mutable std::mutex _rb_later_mutex;
    std::vector<ringbus_later_t> pending_ring_data;
    std::vector<struct event*> pending_ring_events;
    void ringbusPeerLater(const size_t peer, const raw_ringbus_t& rb, const __suseconds_t future_us);
    void zmqRingbusPeerLater(const size_t peer, const raw_ringbus_t* rb, const __suseconds_t future_us);
    void ringbusPeerLater(const size_t peer, const raw_ringbus_t* rb, const __suseconds_t future_us);
    void ringbusPeerLater(const size_t peer, const std::vector<uint32_t> rbv, const __suseconds_t future_us);
    void ringbusPeerLater(const size_t peer, const uint32_t ttl, const uint32_t data, const __suseconds_t future_us);

    // Moving partner RB here
    void resetPartnerBs(const size_t peer, const std::string& which="");
    void resetPartnerEq(const size_t peer);
    void setPartnerTDMA(const RadioEstimate* peer, const uint32_t dmode, const uint32_t value);
    void setPartnerTDMA(const size_t peer, const uint32_t dmode, const uint32_t value);
    void setPartnerCfo(const size_t peer, const double cfo_estimated, bool alsoRxChain = false);
    void setPartnerPhase(const size_t peer, const double rads);
    void setPartnerSfoAdvance(const size_t peer, const uint32_t amount, const uint32_t direction, const bool alsoRxChain = true);
    void setPartnerSfoAdvance(const RadioEstimate* peer, const uint32_t amount, const uint32_t direction, const bool alsoRxChain = true);
    void setPartnerSfoAdvance(const RadioEstimate& peer, const uint32_t amount, const uint32_t direction, const bool alsoRxChain = true);
    void zeroPartnerEq(const size_t peer);
    void sendPartnerNoop(const size_t peer);
    void setPartnerEq(const size_t peer, const std::vector<uint32_t>& s);
    void setPartnerEq(const RadioEstimate& peer, const std::vector<uint32_t>& s);
    void setPartnerEqOne(const size_t peer, const std::vector<uint32_t>& s);
    void setPartnerEqOne(const RadioEstimate& peer, const std::vector<uint32_t>& s);
    void setPartnerEqCore(const size_t peer, const std::vector<uint32_t>& s, const uint32_t type);
    void setPartnerEqCore(const RadioEstimate& peer, const std::vector<uint32_t>& s, const uint32_t type);
    void transportRingToPartner(const size_t peer, const std::vector<uint32_t>& s);
    // void resetPartnerScheduleAdvance(const size_t peer);
    // void resetPartnerScheduleAdvance(const RadioEstimate& peer);
    // void advancePartnerSchedule(const size_t peer, const uint32_t amount);
    // void setPartnerScheduleMode(const size_t peer, const uint32_t value, const uint32_t sub_value = 0);
    // void setPartnerScheduleModeIndex(const size_t array_index, const uint32_t value, const uint32_t sub_value = 0);
    void pingPartnerFbBus(const size_t peer);
    void sendCustomEventToPartner(const size_t peer, const uint32_t d0, const uint32_t d1);
    void sendCustomEventToPartner(const size_t peer, const custom_event_t e);
    void sendHashesToPartner(const size_t peer, const std::string& us, const std::vector<uint32_t>& hashes);
    void sendHashesToPartnerSeq(const size_t peer, const std::string& us, const std::vector<uint32_t>& hashes, const uint32_t tx_seq);
    void sendRetransmitRequestToPartner(const size_t peer, const uint8_t seq, const std::vector<uint32_t>& indices);
    void setPartnerTDMASubcarrier(const size_t peer, const uint32_t sc);
    void sendVectorTypeToPartnerPC(const size_t peer, const size_t vtype, const std::vector<uint32_t>& args);
    void sendVectorTypeToMultiPC(const std::vector<size_t> peers, const size_t vtype, const std::vector<uint32_t>& args);
    void debugJoint(const std::vector<size_t>& peers, const uint8_t seq, const size_t size, const uint32_t epoc, const uint32_t ts);
    std::pair<size_t,size_t> scheduleJointFromTunData(
        const std::vector<size_t>& peers,
        const uint8_t seq,
        const size_t size,
        const uint32_t epoc,
        const uint32_t ts
        );
    
    std::pair<size_t,size_t> scheduleJointFromTunDataFunctional(
        const std::vector<size_t>& peers,
        const uint8_t seq,
        const size_t max_words,
        const uint32_t epoc,
        const uint32_t ts,
        const bool doRetransmit,
        // std::vector<uint32_t>& hashes,
        packets_ready_t&    getPacketsReady,
        next_packet_t&      getNextPacket,
        first_packet_len_t& getNextLength,
        hash_for_words_t&   getHash,
        retransmit_t&       updateRetransmit
        );
    
    unsigned dumpTunBuffer(void);
    size_t wordsInTun(void);
    


private:
    void createStatusTx();
    void createStatusRx();
    void createStatus();
public:


    // event system
    BevPair* events_bev;
    // void registerEventCallback(custom_event_callback_t); // delete
    void sendEvent(const custom_event_t* e);
    void sendEvent(const custom_event_t e);

    void stopThreadDerivedClass();
    void shutdownThread();
    void shutdownThreadRx();
    void shutdownThreadTx();

    void specialSettingsByRole(const std::string& r);
    


    // consume event system
    custom_event_t most_recent_event;
    void clearRecentEvent();
    void handleCustomEvent(const custom_event_t* const e);
    void handleCustomEventRx(const custom_event_t* const e);
    void handleCustomEventTx(const custom_event_t* const e);


    // consume ringbus etc
    std::pair<bool, uint32_t> checkGetCoarseRb();
    void dspDispatchAttachedRb(const uint32_t word);
    bool dspDispatchAttachedRb_TX(const uint32_t word);
    bool dspDispatchAttachedRb_RX(const uint32_t word);
    std::vector<std::uint32_t> rb_buf_coarse_sync;


    unsigned _cfo_symbol_num = 0;
    unsigned _sfo_symbol_num = 0;

    int debug_coarse_sync_counter;
    int debug_coarse_sync_times_matched_6;
    int debug_coarse_sync_did_send;

    bool outstanding_fsm_ping;
    std::chrono::steady_clock::time_point fsm_ping_sent;
    std::chrono::steady_clock::time_point handle_sfo_cfo_timepoint;

    std::chrono::steady_clock::time_point handle_fine_timepoint;

    int reset_zeros_counter;

    uint32_t times_tried_attached_feedback_bus;

    size_t valid_count_default_stream;
    size_t valid_count_all_sc_stream;
    size_t valid_count_fine_sync;
    size_t valid_count_default_stream_p;
    size_t valid_count_all_sc_stream_p;
    size_t valid_count_fine_sync_p;

    struct event* watch_status_data_timer;
    std::chrono::steady_clock::time_point watch_status_timepoint;

    struct event* print_unhealth_status_codes_timer;

    void slowUpdateDashboard();
    std::chrono::steady_clock::time_point reset_hash_timer;


    

    void sendToMock(const std::string &s);
    void sendToMock(const nlohmann::json &j);
    void sendToMock(const char* s);
    void gotFromMock(const size_t word);
    NullBufferSplit *splitRpc = 0;
    DispatchMockRpc *dispatchRpc = 0;
    void gotCompleteMessageFromMock(const std::string& s);
    mutable std::mutex _send_to_mock_mutex;

    bool _debug_loopback_fine_sync_counter = false;

    uint32_t _track_received_counter_counter = 0;
    uint32_t _previous_received_counter = 0;
    std::chrono::steady_clock::time_point _track_counter_timepoint;
    std::vector<std::pair<size_t, uint32_t>> _track_counter_history;

    void updateReceivedCounter(const uint32_t c);

    std::chrono::steady_clock::time_point _track_invalid_demod_timepoint;

    bool allPeersConnected();
    void peerConnectedReply(uint32_t id);
    std::vector<size_t> peer_state;
    std::vector<size_t> peer_state_sent;
    std::vector<size_t> peer_state_got;
    // nlohmann::json peer_state;

    // zmq to js
    void registerZmqCallback(zmq_cb_t);
    void zmqCallbackCatchType(uint32_t);
    zmq_cb_t zmqCallback=0;
    std::vector<uint32_t> zmqCallbackVTypes;


    ////////////////////////
    /// Fetch
    void zmqFetch(const std::vector<uint32_t>& data_vec);

    std::chrono::steady_clock::time_point booted;
    uint32_t fetch_seq = 0; // shared for all types

    std::vector<std::tuple<uint32_t,uint64_t,zmq_fetch_double_t>> pending_fetch_double;
    std::vector<std::tuple<uint32_t,uint64_t,zmq_fetch_uint32_t_t>> pending_fetch_uint32_t;
    std::vector<std::tuple<uint32_t,uint64_t,zmq_fetch_pair_uint32_t_uint32_t_t>> pending_fetch_pair_uint32_t_uint32_t;
    std::tuple<uint32_t,uint64_t,std::vector<uint32_t>> zmqFetchFromPartner_shared(const size_t peer, const uint32_t index, const uint32_t type);
    void zmqFetchFromPartner_double(const size_t peer, const uint32_t index, const zmq_fetch_double_t);
    void zmqFetchFromPartner_uint32_t(const size_t peer, const uint32_t index, const zmq_fetch_uint32_t_t);
    void zmqFetchFromPartner_pair_uint32_t_uint32_t(const size_t peer, const uint32_t index, const zmq_fetch_pair_uint32_t_uint32_t_t);


    void zmqFetchReply_double(const double* p, const uint32_t from, const uint32_t type, const uint32_t index, const uint32_t seq);
    void zmqFetchReply_uint32_t(const uint32_t* p, const uint32_t from, const uint32_t type, const uint32_t index, const uint32_t seq);
    void zmqFetchReply_pair_uint32_t_uint32_t(const std::pair<uint32_t,uint32_t>* p, const uint32_t from, const uint32_t type, const uint32_t index, const uint32_t seq);

    void zmqFetchDispatchReply(const std::vector<uint32_t>& data_vec);
    /// Fetch
    ////////////////////////


    void handleJointTransmit(const std::vector<uint32_t>& data_vec);
    void __attribute__((deprecated))  sendJointAck(const size_t peer, const uint32_t epoc, const uint32_t tx, const uint8_t seq, const uint32_t, const uint32_t, const bool);

    void sendJointAck(
                const size_t peer
                ,const uint32_t packet_lifetime
                ,const uint8_t seq
                ,const uint32_t now_lifetime
                ,const bool dropped
                );

    std::atomic<bool> enableRepeat;
    std::vector<std::vector<std::vector<uint32_t>>> repeatBuffer;
    unsigned repeatHistory = 0;
    void setupRepeat();

    bool grc_shows_eq_filter = false;

    void handleFineSyncMessage(const uint32_t);

    void jsEval(const std::string& in);

    void partnerOp(const size_t peer, const uint32_t fpga, const std::string s, const uint32_t _sel, const uint32_t _val);
    void op(const uint32_t fpga, const std::string s, const uint32_t _sel, const uint32_t _val);

    int blockFsm = -1;

    void forceZeros(const size_t peer, const bool force_zeros);

    void test_fb(void);

    bool fsm_print_peers = true;

    void suppressFeedbackEqUntil(const uint32_t lifetime, const bool also_upper = false);















    ///
    /// Dashboard stuff
    /// void tieDashboardInt(int32_t *p, uint32_t uuid, std::vector<std::string> & path);

    template <typename T>
    void tieDashboard(T *p, const uint32_t uuid, std::vector<std::string> & path) {
        DashTie *tie = new DashTie();

        nlohmann::json j_test = *p;

        bool found = true;

        // tie->type = j_test.type();

        // edit this table to add a new type
        // also edit EventDspDash.cpp::buildString()
        // see https://stackoverflow.com/questions/36577718/same-typeid-name-but-not-stdis-same
        static_assert(
               std::is_same<T,size_t>::value
            || std::is_same<T,int32_t>::value
            || std::is_same<T,double>::value
            || std::is_same<T,bool>::value
            || std::is_same<T,std::vector<double>>::value
            || std::is_same<T,uint32_t>::value
            , "Illegal type in tieDashboard()");

        if( typeid(T) == typeid(size_t) ) {
            tie->type = dash_type_t::dash_type_size_t;
#ifdef DEBUG_PRINT_TIE_TICKLE
            std::cout << "found type_t" << std::endl;
#endif
        } else if ( typeid(T) == typeid(int32_t) ) {
            tie->type = dash_type_t::dash_type_int32_t;
#ifdef DEBUG_PRINT_TIE_TICKLE            
            std::cout << "found int32_t" << std::endl;
#endif
        } else if ( typeid(T) == typeid(double) ) {
            tie->type = dash_type_t::dash_type_double;
#ifdef DEBUG_PRINT_TIE_TICKLE            
            std::cout << "found double" << std::endl;
#endif
        } else if ( typeid(T) == typeid(bool) ) {
            tie->type = dash_type_t::dash_type_bool;
#ifdef DEBUG_PRINT_TIE_TICKLE            
            std::cout << "found bool" << std::endl;
#endif
        } else if ( typeid(T) == typeid(std::vector<double>) ) {
            tie->type = dash_type_t::dash_type_vec_double;
#ifdef DEBUG_PRINT_TIE_TICKLE            
            std::cout << "found vector<double>" << std::endl;
#endif   
        } else if ( typeid(T) == typeid(uint32_t) ) {
            tie->type = dash_type_t::dash_type_uint32_t;
#ifdef DEBUG_PRINT_TIE_TICKLE            
            std::cout << "found uint32_t" << std::endl;
#endif
        } else {
            std::cout << " tieDashboard doesn't know about that type" << std::endl;
            
            found = false;
        }



        tie->p = (void*)p;
        tie->uuid = uuid;
        tie->path = path;

#ifdef DEBUG_PRINT_TIE_TICKLE
        std::cout << "tied type enum: " << (int)tie->type << std::endl;
        std::cout << "  tied ptr " << (void*)p << std::endl;
        std::cout << "  tied path[0] " << tie->path[0] << std::endl;
        if( tie->path.size() > 1 ) {
            std::cout << "  tied path[1] " << tie->path[1] << std::endl;
        }
        if( tie->path.size() > 2 ) {
            std::cout << "  tied path[2] " << tie->path[2] << std::endl;
        }
        std::cout << "  tied uuid " << tie->uuid << std::endl;
#endif

        // assert later here because we want to know debug
        assert(found);
        (void)found;

        std::unique_lock<std::mutex> lock(_ties_mutex);
        _ties.push_back(tie);
    }

    template <typename T>
    void ticklePath(T p, const uint32_t uuid, std::string a) {
        // FIXME static here is weird??
        static DashTie tie = DashTie();
        static T saved2 = p;
        saved2 = p; // copy to here
        std::vector<std::string> path = {a};
        
        tie.p = (void*)&saved2;
        tie.uuid = uuid;
        tie.path = path;

        ticklePathInternal(&tie);
    }

    template <typename T>
    void ticklePath(T p, const uint32_t uuid,
        std::string a,
        std::string b
        ) {
        static DashTie tie = DashTie();
        static T saved2 = p;
        saved2 = p; // copy to here
        std::vector<std::string> path = {a,b};
        // std::cout << "fn: " << saved2 << std::endl;
        tie.p = (void*)&saved2;
        tie.uuid = uuid;
        tie.path = path;

        ticklePathInternal(&tie);
    }

    template <typename T>
    void ticklePath(T p, const uint32_t uuid,
        std::string a,
        std::string b,
        std::string c
        ) {
        static DashTie tie = DashTie();
        static T saved2 = p;
        saved2 = p; // copy to here
        std::vector<std::string> path = {a,b,c};
        
        tie.p = (void*)&saved2;
        tie.uuid = uuid;
        tie.path = path;

        ticklePathInternal(&tie);
    }


    template <typename T>
    void tieDashboard(T *p, const uint32_t uuid, std::string a) {
        std::vector<std::string> path = {a};
        tieDashboard<T>(p, uuid, path);
    }

    template <typename T>
    void tieDashboard(T *p, const uint32_t uuid, 
        std::string a,
        std::string b
        ) {
        std::vector<std::string> path = {a,b};
        tieDashboard<T>(p, uuid, path);
    }

    template <typename T>
    void tieDashboard(T *p, const uint32_t uuid, 
        std::string a,
        std::string b,
        std::string c
        ) {
        std::vector<std::string> path = {a,b,c};
        tieDashboard<T>(p, uuid, path);
    }

    template <typename T>
    void tieDashboard(T *p, const uint32_t uuid, 
        std::string a,
        std::string b,
        std::string c,
        std::string d
        ) {
        std::vector<std::string> path = {a,b,c,d};
        tieDashboard<T>(p, uuid, path);
    }

    template <typename T>
    void tieDashboard(T *p, const uint32_t uuid, 
        std::string a,
        std::string b,
        std::string c,
        std::string d,
        std::string e
        ) {
        std::vector<std::string> path = {a,b,c,d,e};
        tieDashboard<T>(p, uuid, path);
    }

    template <typename T>
    void tieDashboard(T *p, const uint32_t uuid, 
        std::string a,
        std::string b,
        std::string c,
        std::string d,
        std::string e,
        std::string f
        ) {
        std::vector<std::string> path = {a,b,c,d,e,f};
        tieDashboard<T>(p, uuid, path);
    }

    // void tieDashboard(size_t *p, uint32_t uuid, 
    //     std::string a,
    //     std::string b
    //     ) {
    //     std::vector<std::string> path = {a,b};
    //     tieDashboard(p, uuid, path);
    // }

    //
    // these are found in EventDspDash.cpp
    //
    void tickle(const void *p);
    void tickle(const int32_t *p);
    void tickle(const uint32_t *p);
    void tickle(const double *p);
    void tickle(const size_t *p);
    void tickleAll(const size_t start = 0, const size_t stop = 0);
    void onetimeTieMasterFSM();
    std::vector<DashTie*> _ties;
    void tickleLog(const std::string tag, const std::string message);
    void dashTieSetup();
    void ticklePathInternal(DashTie* saved);
    uint64_t runtime_session_id; // changes each time we restart the app, set in dashTieSetup based on clock and pid
    mutable std::mutex _ties_mutex; // protects _ties
    

    // std::string buildString(DashTie *d);
    // std::string buildString2(DashTie *d);
// private:
//     void _tickleExecute(DashTie* saved);
public:


     uint32_t counter = 0;
     uint32_t next_counter = 0;
     uint32_t write_word_counter = 0;

     uint32_t temp_data = 0;
     double temp_qpsk_scale = 0.0;
     double temp_bpsk_scale = 0.0;
     double temp_snr = 0.0;
     double filtered_snr = 0.0;

     bool snr_first_time_flag = false;
     bool write_to_buffer_flag = false;

};
