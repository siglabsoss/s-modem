#pragma once

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "driver/HiggsEvent.hpp"
// #include "HiggsStructTypes.hpp"
#include "schedule.h"
#include "duplex_schedule.h"


#include <atomic>
#include <vector>
#include <mutex>

class HiggsEvent;
class EventDsp;
class HiggsSettings;
class AttachedEstimate;
class StatusCode;
class VerifyHash;
class RetransmitBuffer;
class AirPacketOutbound;
class ScheduleData;
class AirMacBitrateTx;



class TxSchedule : public HiggsEvent
{
public:
    ///
    /// Cruft for integrating with s-modem 
    /// 
    HiggsSettings* settings;

    AirMacBitrateTx* macBitrate = 0;

    
    ///
    /// Event and bev stuff
    ///
    bool print;

    // FSM
    size_t dsp_state;
    size_t dsp_state_pending;
    size_t _do_wait;
    struct event* fsm_init;
    struct event* fsm_next;
    struct event* fsm_next_timer;
    std::string role;
    std::atomic<bool> doWarmup;
    std::atomic<bool> warmupDone;
    std::atomic<bool> requestPause;
    std::atomic<bool> exitDataMode;
    std::atomic<bool> debugBeamform;
    std::atomic<size_t> errorDetected;
    std::atomic<size_t> flushZerosEnded;
    std::atomic<size_t> flushZerosStarted;
    std::atomic<size_t> debugBeamformSize;
    // bool needs_pre_zero = false;

private:
    // std::vector<uint32_t> zrs;
    size_t errorDetectedLocal = 0;
    size_t sendLowLoops = 0;
public:

    size_t errorRecoveryAttempts = 0;
    size_t errorFlushZerosWaiting = 0;

    // stuff copied from TxFsm
    AttachedEstimate* attached = 0;
    EventDsp *dsp = 0;
    RetransmitBuffer *retransmitBuffer = 0;
    AirPacketOutbound *airTx = 0;
    StatusCode* status = 0;
    VerifyHash* verifyHashTx = 0;
    ScheduleData* scheduleData = 0;


    int debug_est_run = 0;
    int debug_est_run_target = 0;
    int warmup_epoc_est = 0;
    int warmup_epoc_est_target = 0;
    uint32_t debug_grow_size = 0;
    std::chrono::steady_clock::time_point integration_test_start_timepoint;
    std::chrono::steady_clock::time_point schedule_timepoint;
    std::chrono::steady_clock::time_point last_low;
    uint32_t periodic_schedule_last = 0;
    long int fsm_us_track = 0; // used for debug for now


    void debug(const unsigned length=0);
    void debugFillBuffers(const unsigned recursion=0);
    void debugFillBeamform(const uint32_t now_epoc, const uint32_t now_frame, const int wake_ts);
    void debugFillZmq();
    unsigned tunFirstLength() const;
    void dumpUserData();
    std::vector<uint32_t> pullAndPrepTunData(const int max_mapmov_size);
    void loadAllPackets(std::vector<uint32_t>& hashes, std::vector<std::vector<uint32_t>>& for_retransmit, std::vector<uint32_t>& build_packet, int& input_total, int& total_packets, const bool, const int output_limit_words );
    std::vector<uint32_t> tunGetPacket();
    unsigned tunPacketsReady() const;

    // fsm and basics
    TxSchedule(HiggsDriver* s);
    void stopThreadDerivedClass();
    void setupFsm();
    void tickFsmSchedule();
    void setupPointers();


    // new stuff
    void tickleAttachedProgress();
    // static void printDeltaTime(std::chrono::steady_clock::time_point&, std::chrono::steady_clock::time_point&);
    std::chrono::steady_clock::time_point tx_loop_start; // when we first enetered the tx loop (may be reset to 0)
    std::chrono::steady_clock::time_point state_went_to_sleep; // time that we end the packet event
    // std::chrono::steady_clock::time_point state_went_to_sleep; // time that we end the packet event

    void setLinkUp(const bool);

    uint8_t debug_joint_seq = 0;

    mutable std::mutex _beamform_buffer_mutex;
    std::vector<std::vector<uint32_t>> beamform_buffer;

    size_t bufferedBeamform() const;

    /// are the pending_ values loaded with good data?
    /// Here are the values (modes):
    ///
    ///   In mode 0, if beamform_buffer() has anything, it will be unpacked
    ///   into pending_valid, pending_epoc, pending_ts, pending_seq, pending_packets
    ///   and then it will be set to 1
    ///
    ///   In mode 1, if the time is right, pending_packets will be emptied into
    ///   tun buffer, however pending_valid, pending_epoc, pending_ts, pending_seq remain
    ///   untouched and are still valid.  mode is set to 2
    ///
    ///   In mode 2, we need to empty tun buffer.  once this happens mode is set to 0
    ///   and pending_valid, pending_epoc, pending_ts, pending_seq are invalid (but remain set)
    ///
    unsigned pending_valid = 0;
    uint32_t pending_from_peer;
    uint32_t pending_epoc;
    uint32_t pending_ts;
    uint8_t pending_seq;
    std::vector<std::vector<uint32_t>> pending_packets;

    // new with duplex, should we work with those above?
    uint32_t pending_lifetime;


    void loadPendingMember(void);
    void loadZmqBeamform(const uint32_t now_epoc, const uint32_t now_frame);
    void __attribute__((deprecated))  dumpPendingPacket(const std::string reason, const uint32_t now_epoc, const uint32_t now_frame);
    void dumpPendingPacket(const std::string reason, const uint32_t lifetime32);

    std::chrono::steady_clock::time_point init_timepoint;
    std::chrono::steady_clock::time_point last_tickle;

    int idle_print = 0;

    int long_sleep_print = 0;

    uint32_t tx_mapmov_rb_for_eth = 0;

    bool print1 = false;

    unsigned debug_duplex = 0;

    int32_t debug_counter_delta = 0; // do not change fixme remove
    uint32_t debug_duplex_type = 1;
    uint32_t debug_duplex_size = 40;
    uint32_t debug_extra_gap = 0;
    uint32_t debug_duplex_busy_until = 0;
    bool debug_busy_valid = false;
    unsigned debug_duplex_ahead = 128*40;
    uint32_t duplex_seq = 0;
    bool duplex_debug_split = false;
    bool duplex_skip_body = true;

    bool isDebugDuplex(const unsigned _pending_valid);
    void applyDebugDuplex1();
    std::vector<uint32_t> applyDebugDuplex2();
    void finishedDebugDuplex(const uint32_t sent_length, const bool skipped = false);

    duplex_timeslot_t duplex;

    // specific to our role
    bool* duplex_usage_map = 0;

    void setupDuplexAsRole(const std::string);
    uint32_t estimateBusy(const uint32_t from, const uint32_t sent_length);


};
