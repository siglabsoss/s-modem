#include "FFT.hpp"
#include <iostream>
#include <cassert>

namespace BevStream {

using namespace std;

FFT::FFT(size_t removeInputCp, size_t addOutputCp, bool print) : BevStream(true,print), removeCp(removeInputCp) , addCp(addOutputCp) {
  name = "FFT";

  vec.resize(sz);
  results.resize(sz);
}

FFT::~FFT() {
  if(_print) {
    cout << "~FFT()" << endl;
  }
}


void FFT::stopThreadDerivedClass() {
  // perform any gain stream specific tasks
}

// runs from same place (thread) that constructed us
void FFT::setBufferOptions(bufferevent* in, bufferevent* out) {
  bufferevent_setwatermark(out, EV_READ, byte_sz, 0);
  // bufferevent_set_max_single_read(out, (1024)*4 );
}

// repeatedly called, but only if there are 1024 uint32_t values
void FFT::gotDataCore(struct bufferevent *bev, struct evbuffer *buf, unsigned char* chunk1_raw, size_t lengthIn) {

  // by this point vec is already 1024 long
  double* asDouble = (double*)vec.data();

  // FIXME we can make performance faster by reworking evbuffer_pullup() to instead write directly to this adDouble pointer
  // this would eliminate the memcopy
  memcpy(asDouble, chunk1_raw, byte_sz);

  // for(auto i = 0; i < 8; i++) {
  //   cout << vec[i] << endl;
  // }
  // cout << endl;

  // for( auto x: vec ) {
  //   cout << x << " ";
  // }
  // cout << endl;

  fft.inv(results, vec);
  
  // for( auto x: results ) {
  //   cout << x << " ";
  // }
  // cout << endl;


  // should we add cp to our output?
  if( addCp ) {
    size_t n_byte = sz * element_sz;
    size_t cp_bytes = addCp * element_sz;

    // cout << "adding cp of " << cp_bytes << endl;
    bufferevent_write(next->input, ((unsigned char*)results.data())+n_byte-cp_bytes, cp_bytes);
  }

  bufferevent_write(next->input, results.data(), byte_sz);


}


// runs on _thread
void FFT::gotData(struct bufferevent *bev, struct evbuffer *buf, size_t lengthIn) {
  if(_print) {
    cout << name << " got data with " << lengthIn << endl;
  }

  if(!next) {
    evbuffer_drain(buf, lengthIn);
    return;
  }

  // how many bytes for the cp?
  size_t cp_bytes = removeCp * element_sz;
  size_t byte_sz_mod = byte_sz + cp_bytes;

  // round down
  size_t willEatFrames = lengthIn / byte_sz_mod;

  // cout << name << " will eat " << willEatFrames << " frames" << endl;

  size_t this_read = willEatFrames*byte_sz_mod;

  // cout << "this_read " << this_read << " len " << lengthIn << endl;

  unsigned char* temp_read = evbuffer_pullup(buf, this_read);

  size_t consumed = 0;
  while((consumed+byte_sz_mod) <= lengthIn) {
    auto read_at = temp_read+consumed+cp_bytes; // remove the cp here with pointer arithmetic
    gotDataCore(bev, buf, read_at, lengthIn);
    consumed += byte_sz_mod;
  }

  evbuffer_drain(buf, this_read);
}

} // namespace