#pragma once

#include <stdint.h>
#include <thread>
#include <atomic>

#include <event2/event.h>
#include <event2/thread.h>
#include <event2/bufferevent.h>

class HiggsDriver;

// Write to in
// Read from out
class BevPair {
    public:
        bufferevent* in;
        bufferevent* out;
        BevPair();
        BevPair& set(struct bufferevent * rsps_bev[2]);
        BevPair& enableLocking();
};

class HiggsEvent {
public:
    HiggsEvent(HiggsDriver*, bool _run_timer = false);

    struct event_base *evbase;
    void stopThread();
    virtual void stopThreadDerivedClass() = 0;

    void threadMain();
    void setThreadPriority(int policy, int priority);

    struct event *_example_event;

    std::thread _thread;
    sched_param thread_priority;
    int thread_schedule_policy = 0;


    // std::atomic<bool> _thread_can_terminate = false; // set by inside when ready??

    HiggsDriver* soapy;

    std::atomic<bool> _thread_should_terminate; // set by outside
    
    bool run_timer;
};