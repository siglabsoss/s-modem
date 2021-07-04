#pragma once


#include "HiggsStructTypes.hpp"

#include "HiggsEvent.hpp"

#include "HiggsSoapyDriver.hpp"

// #include "driver/AttachedEstimate.hpp"
#include "driver/SubcarrierEstimate.hpp"
#include "driver/HiggsSettings.hpp"

// #include "HiggsDriverSharedInclude.hpp"

// also gets feedback_bus.h from riscv
// #include "feedback_bus_tb.hpp"

// #include "vector_helpers.hpp"

#include <event2/bufferevent.h>
#include <event2/buffer.h>
// #include <type_traits>
#include <typeinfo>
#include <future>

class AttachedEstimate;
class RadioEstimate;
class HiggsEvent;
class EventDsp;


#include "DashTie.hpp"
#include "3rd/json.hpp"

class FlushHiggs : public HiggsEvent
{
public:
    ///
    /// Cruft for integrating with s-modem 
    /// 
    HiggsSettings* settings;

    ///
    /// unique logic stuff
    ///

    std::chrono::steady_clock::time_point time_prev_pet_send_higgs;
    std::chrono::steady_clock::time_point time_prev_sent_pet_higgs;
    std::chrono::steady_clock::time_point feed_higgs_tp;
    std::chrono::steady_clock::time_point time_8k_start;
    std::atomic<bool> valid_8k;
    uint32_t words_8k = 0;
    uint32_t words_8k_sent = 0;
    int32_t early_frame_8k = 0;
    bool early_frame_8k_applied = false;
    std::atomic<bool> cs20_early_flag;
    uint32_t cs20_early_rb = 0;
    uint32_t packets_after_early_8k = 0;
    uint32_t fill_when_enter_8k = 0;
    uint32_t early_target_8k = 3000;
    bool did_pause_8k = false;

    std::atomic<int> member_early_frames;

    std::chrono::steady_clock::time_point last_fill_taken_at;
    uint32_t last_fill_level;

    ///
    /// Event and bev stuff
    ///
    bool drain_higgs;
    // bool pet_feed_higgs_as_tx;
    struct event* sendto_higgs_timer; // feed data into data port of higgs
    BevPair* for_higgs_bev;
    BevPair* for_higgs_via_zmq_bev;
    bool should_pause_p;
    bool should_use_new_level;
    bool print;
    bool printSending = false;
    bool printSending2 = false;
    std::atomic<bool> allow_zmq_higgs;
    int sending_to;
    bool print_when_almost_full = true;
    bool dump_low_pri_buf = false;
    bool keep_data_history = false; // saves all memory sent to higgs, will balloon memory
    bool pad_zeros_before_low_pri = true;
    double drain_rate = 1.6;
    int max_packet_burst = 8;
    bool do_force_boost = false;
    bool print_low_words = false;
    bool print_evbuf_to_socket_burst = false;
    // bool print_length_of
    std::vector<std::string> socket_buffer_inodes;

    std::vector<unsigned char> data_history;
    std::atomic<bool> requestExitSendingLow;

    // FSM
    FlushHiggs(HiggsDriver* s);
    void stopThreadDerivedClass();
    void enableBuffers();
    void setupBuffers();
    void setupFsm();
    int petSendToHiggsSub(unsigned int allow_repeat);
    uint32_t petSendToHiggs(struct evbuffer *buf);
    uint32_t petSendToHiggs2(struct evbuffer *buf);
    void handleHiggsCallback(struct bufferevent *bev);
    int evbufToSocketBurst(struct evbuffer *buf, size_t request_len, const std::string& who="", bool force_boost = false);
    void consumeUserdataLatencyEarly(const uint32_t word);
    void consumeRingbusFill(const uint32_t word);
private:
    void sendZmqHiggsNow(std::string who = "unset");
public:
    void updateSendingFlag(int caller);
    void exitSendingLowMode();
    void checkOsUdpBuffers();
    size_t getNormalLen();
    size_t getLowLen();
    void dumpLow();
    void dumpNormal();
    std::vector<unsigned char> getDataHistory();
    void eraseDataHistory();
    void inspectLowPri(std::string who = "unset2");
    void updateFlags();
    void updateDrainRate();



    std::pair<uint32_t, unsigned long> fillAge();

private:
    mutable std::mutex _fill_mutex;
    mutable std::mutex _cs20_early_mutex;
    mutable std::mutex _keep_data_mutex;
};
