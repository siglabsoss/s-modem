#pragma once

#include <stdint.h>
#include <thread>
#include <atomic>

#include <event2/event.h>
#include <event2/thread.h>
#include <event2/bufferevent.h>

namespace BevStream {

// Write to in
// Read from out
class BevPair2 {
    public:
        bufferevent* in;
        bufferevent* out;
        BevPair2();
        BevPair2& set(struct bufferevent * rsps_bev[2]);
        BevPair2& enableLocking();
};

}