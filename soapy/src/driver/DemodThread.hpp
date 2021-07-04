#pragma once

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include "driver/HiggsEvent.hpp"
#include "schedule.h"


#include <atomic>
#include <vector>
#include <mutex>
#include <future>
#include <fstream>

class HiggsEvent;
class HiggsSettings;
class VerifyHash;
class RetransmitBuffer;
class AirPacketInbound;
class AirMacBitrateRx;


// copied from AirPacket.hpp, seems ok to do that?
typedef std::function<void(AirPacketInbound* const)> air_set_inbound_t; // copied from AirPacket.hpp





#ifndef _BUFFER_TYPES
#define _BUFFER_TYPES
typedef std::vector<std::vector<uint32_t>> sliced_buffer_t;
typedef std::vector<std::vector<std::pair<uint32_t, uint32_t>>> iq_buffer_t;
#endif

typedef std::tuple<uint32_t, uint8_t, uint8_t> air_header_t;
typedef std::tuple<std::vector<uint32_t>, uint32_t, air_header_t> got_data_t;

typedef std::function<void(const size_t peer, const uint8_t seq, const std::vector<uint32_t>& indices)> retransmit_cb_t;


class DemodThread : public HiggsEvent
{
public:
    ///
    /// Cruft for integrating with s-modem 
    /// 
    HiggsSettings* settings = 0;

    AirPacketInbound* airRx = 0;

    VerifyHash *verifyHash = 0;

    AirMacBitrateRx *macBitrate = 0;

    // std::string role;

    sliced_buffer_t sliced_words;

    bool init_drain_for_sliced_data = true;
    BevPair* sliced_data_bev = 0;
    struct event* demod_data_timer = 0;

    
    // bool print = true;

    size_t times_sync_word_detected = 0;

    // EventDsp *dsp = 0;

    std::vector<got_data_t> data_for_js;
    void dataToJs(const std::vector<uint32_t> &p, const uint32_t got_at, const air_header_t &found_header);
    std::pair<bool, got_data_t> getDataToJs();
    std::mutex _data_to_js_mutex;

    size_t peer_id;
    size_t retransmit_to_peer_id; // who do we retransmit to?

    // Controls
    std::atomic<bool> dropSlicedData;

    // future
    std::future<void> setPointerFuture;

    // std::chrono::steady_clock::time_point schedule_timepoint;

    // fsm and basics
    DemodThread(HiggsDriver* s, air_set_inbound_t cb);
    DemodThread(HiggsSettings* s); // called by testbench
    virtual ~DemodThread();
    void stopThreadDerivedClass();
    // void setupFsm();
    void setupBuffers();
    void setAirParams();

    void setAirSettingsCallback(const air_set_inbound_t cb);
    air_set_inbound_t setup_air_cb = 0;

private:
    void debug_zmq();
public:

    void discardSlicedData();
    void demodGotPacket(std::vector<uint32_t> &p, const uint32_t got_at, const air_header_t &found_header);
    void check_subcarrier();
    void trimSlicedData(const unsigned erase);

    std::chrono::steady_clock::time_point init_timepoint;

    // for dumping
    std::string dump_name = "./dump_demod.hex";
    std::ofstream dump_file;
    unsigned pending_dump = 0; // actual trigger
    unsigned incoming_data_type = 1; // FIXME: remove once we get new duplex header working

    void handlePendingDump(void);

    // callbacks
    void setRetransmitCallback(const retransmit_cb_t);

    retransmit_cb_t retransmit;

    uint8_t expected_seq = 0;

};
