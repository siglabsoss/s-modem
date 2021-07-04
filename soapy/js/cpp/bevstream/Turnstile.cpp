#include "Turnstile.hpp"
#include <iostream>
#include <cassert>


namespace BevStream {

using namespace std;

Turnstile::Turnstile(bool print) : BevStream(true,print), pending_advance(0) {
  name = "Turnstile";
}

Turnstile::~Turnstile() {
  if(_print) {
    cout << "~Turnstile()" << endl;
  }
}


void Turnstile::advance(size_t a) {
  pending_advance += a;
}


void Turnstile::stopThreadDerivedClass() {
  // perform any gain stream specific tasks
}

// runs from same place (thread) that constructed us
void Turnstile::setBufferOptions(bufferevent* in, bufferevent* out) {
  bufferevent_setwatermark(out, EV_READ, 1, 0);
  // bufferevent_set_max_single_read(out, (1024)*4 );
}

// runs on _thread
void Turnstile::gotData(struct bufferevent *bev, struct evbuffer *buf, size_t lengthIn) {
  if(_print) {
    cout << name << " got data with " << lengthIn << endl;
  }


  if(!next) {
    evbuffer_drain(buf, lengthIn);
    return;
  }

  if(pending_advance) {
    // cout << "pending: " << pending_advance << endl;

    // we were asked to drop bytes than we just got
    if(pending_advance > lengthIn) {
      // cout << "shave partial " << lengthIn << endl;
      evbuffer_drain(buf, lengthIn);
      pending_advance -= lengthIn; // this will have remaining value for next time

      return;
    } else {
      // cout << "shave full " << pending_advance << endl;
      // we were asked to drop less or equal bytes than we just got
      evbuffer_drain(buf, pending_advance);
      pending_advance -= pending_advance; // this will be 0

      evbuffer_add_buffer(next->input2, buf); // in the case where pending_advance == lengthIn it's still OK to call this, I checked
      return;
    }

  } else {

    // just forward

    // dst, src
    evbuffer_add_buffer(next->input2, buf);
  }
}


} // namespace