#pragma once


#include "HiggsStructTypes.hpp"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
// #include <typeinfo>
// #include <future>

#include <math.h>
#include <complex>
#include <vector>
#include <cmath>
#include <iostream>

// #include "common/convert.hpp"
#include "common/constants.hpp"

#include "driver/HiggsEvent.hpp"

#include <chrono>
#include <ctime>
#include <atomic>
#include <mutex>

#include "duplex_schedule.h"

#define SNR_PACK_SIZE 87

class EventDsp;
class HiggsSettings;
class HiggsDriver;

class HiggsSNR : public HiggsEvent
{
public:
    ///
    /// Cruft for integrating with s-modem 
    /// 
    HiggsSettings* settings = 0;

    BevPair* snr_buf = 0;

    HiggsSNR(HiggsDriver* s);
    
    void setupBuffers();
    void handle_snr_callback(struct bufferevent *bev);
    bool snrCalculate(uint32_t* raw_data, uint32_t length);
    void stopThreadDerivedClass();
    bool print_all = false;
    bool print_avg = false;
    std::atomic<double> avg_snr;
    std::vector<std::vector<double>> history;
    mutable std::mutex history_mutex;
    unsigned max_history = 15;
    bool expect_single_qpsk = false;

    void SetSNR(double value);
    double GetSNR() const;

    std::vector<double> GetHistory(const unsigned idx);
};
