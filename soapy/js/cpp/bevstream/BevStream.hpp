#pragma once

#include <stdint.h>
#include <thread>
#include <atomic>

#include <event2/event.h>
#include <event2/thread.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include "BevPair2.hpp"


namespace BevStream {

class BevStream {
public:
    BevStream(bool defer_callbacks, bool print);
    virtual ~BevStream();

    BevStream* next = 0;
    BevStream* prev = 0;

    bool _print = false;
    bool _defer_callbacks = true;

    struct event_base *evbase = 0;
    void stopThread();
    virtual void stopThreadDerivedClass() = 0;

    void threadMain();

    void init();

    std::string name;


    // struct event *_example_event;

    std::thread _thread;


    // std::atomic<bool> _thread_can_terminate = false; // set by inside when ready??

    // HiggsDriver* soapy;

    // std::atomic<bool> _thread_should_terminate; // set by outside
    
    std::atomic<bool> _init_run; // not sure if atomic needed but who cares

    // thread and ev base stuff above
    // buffer and pipe stuff below
    BevPair2* pair = 0;
    void setupBuffers();

    // this points inside pair, but makes writing code easier
    struct bufferevent *input = 0;
    struct evbuffer *input2 = 0;

    virtual void setBufferOptions(bufferevent* in, bufferevent* out) = 0;
    virtual void gotData(struct bufferevent *bev, struct evbuffer *input, size_t len) = 0;

    // void init(); // finishes buffer alloc, gets derived buffer settins, fires thread

    BevStream* pipe(BevStream* arg);
};

}