#pragma once


#include "HiggsStructTypes.hpp"

#include "HiggsEvent.hpp"

#include "HiggsSoapyDriver.hpp"

// #include "driver/RadioEstimate.hpp"
// #include "driver/AttachedEstimate.hpp"
// #include "driver/SubcarrierEstimate.hpp"
// #include "driver/HiggsSettings.hpp"

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

class HiggsFineSync : public HiggsEvent
{
public:
    ///
    /// Cruft for integrating with s-modem 
    /// 
    HiggsSettings* settings;

    ///
    /// unique logic stuff

    BevPair* sfo_cfo_buf;

    struct event* process_sfo_timer;
    struct event* process_cfo_timer;

    HiggsFineSync(HiggsDriver* s);
    void setupBuffers();
    void stopThreadDerivedClass();
    void dump_sfo_cfo_buffer();
    
private:
    mutable std::mutex _fill_mutex;
};
