#include "IShortToIFloat64.hpp"
#include "convert.hpp"
#include <iostream>
#include <cassert>



namespace BevStream {

using namespace std;

IShortToIFloat64::IShortToIFloat64(bool print) : BevStream(true,print) {
  name = "IShortToIFloat64";
}

IShortToIFloat64::~IShortToIFloat64() {
  if(_print) {
    cout << "~IShortToIFloat64()" << endl;
  }
}


void IShortToIFloat64::stopThreadDerivedClass() {
  // perform any gain stream specific tasks
}

// runs from same place (thread) that constructed us
void IShortToIFloat64::setBufferOptions(bufferevent* in, bufferevent* out) {
  bufferevent_setwatermark(out, EV_READ, element_sz, 0);
  // bufferevent_set_max_single_read(out, (1024)*4 );
}

// runs on _thread
void IShortToIFloat64::gotData(struct bufferevent *bev, struct evbuffer *buf, size_t lengthIn) {
  // cout << name << " got data with " << lengthIn << endl;


  if(next) {
    // if(_print) {
    //   cout << "next: " << next->name << endl;
    // }
  } else {
    cout << name << " dumping data due to this->next not being set" << endl;
    evbuffer_drain(buf, lengthIn);
    return;
  }

  
  // round down
  size_t willEatFrames = lengthIn / byte_sz;

  // cout << "will eat " << willEatFrames << " frames" << endl;

  size_t this_read = willEatFrames*byte_sz;

  // cout << "this_read " << this_read << " len " << lengthIn << endl;

  unsigned char* temp_read = evbuffer_pullup(buf, this_read);


  std::vector<std::complex<double>> output;

  IShortToComplexMulti(output, temp_read, willEatFrames);

  // for( auto x : output ) {
  //   cout << x << endl;
  // }

  bufferevent_write(next->input, output.data(), willEatFrames*out_element_sz);

  evbuffer_drain(buf, this_read);
}


} // namespace