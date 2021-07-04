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
class AirPacketInbound;


typedef std::function<void(const size_t peer, const uint8_t seq, const std::vector<uint32_t>& indices)> retransmit_cb_t;



class EVMThread : public HiggsEvent
{
public:
    ///
    /// Cruft for integrating with s-modem 
    /// 
    HiggsSettings* settings = 0;


    bool init_drain_for_sliced_data = true;
    BevPair* data_bev = 0;
    struct event* demod_data_timer = 0;

    
    bool print = true;

    size_t peer_id;


    bool print_dump = false;

    // Controls
    std::atomic<bool> dropSlicedData;

    EVMThread(HiggsDriver* s);
    // EVMThread(HiggsSettings* s);
    virtual ~EVMThread();
    void stopThreadDerivedClass();
    // void setupFsm();
    void setupBuffers();

    std::pair<bool, std::vector<uint32_t>> getDataToJs();
    std::mutex _data_to_js_mutex;

    std::atomic<double> evm;

    // void setAirParams();
private:
    // void debug_zmq();
public:

    // void discardSlicedData();
    // void demodGotPacket(std::vector<uint32_t> &p, const uint32_t got_at, const air_header_t &found_header);
    // void check_subcarrier();
    // void trimSlicedData(const unsigned erase);

    std::chrono::steady_clock::time_point init_timepoint;

    // for dumping
    // std::string dump_name = "./dump_demod.hex";
    // std::ofstream dump_file;
    // unsigned pending_dump = 0; // actual trigger

    // void handlePendingDump(void);

    // callbacks
    // void setRetransmitCallback(const retransmit_cb_t);

    // retransmit_cb_t retransmit;

};
