#include "BevPair2.hpp"

#include <event2/buffer.h>

namespace BevStream {

BevPair2::BevPair2() : in(0), out(0) {
    
}


BevPair2& BevPair2::set(struct bufferevent * rsps_bev[2]) {
    in = rsps_bev[0];
    out = rsps_bev[1];
    return *this;
}

BevPair2& BevPair2::enableLocking() {
    evbuffer_enable_locking(bufferevent_get_input(in), NULL);
    evbuffer_enable_locking(bufferevent_get_output(in), NULL);
    evbuffer_enable_locking(bufferevent_get_input(out), NULL);
    evbuffer_enable_locking(bufferevent_get_output(out), NULL);

    return *this;
}

}