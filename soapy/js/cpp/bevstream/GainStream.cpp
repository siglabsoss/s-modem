#include "GainStream.hpp"
#include <iostream>
#include <cassert>


namespace BevStream {

using namespace std;

GainStream::GainStream(bool print) : BevStream(true,print) {

}

GainStream::~GainStream() {
  if(_print) {
    cout << "~GainStream()" << endl;
  }
}


void GainStream::stopThreadDerivedClass() {
  // perform any gain stream specific tasks
}

// runs from same place (thread) that constructed us
void GainStream::setBufferOptions(bufferevent* in, bufferevent* out) {
  bufferevent_setwatermark(out, EV_READ, 4, 0);
  // bufferevent_set_max_single_read(out, (1024)*4 );
}

// runs on _thread
void GainStream::gotData(struct bufferevent *bev, struct evbuffer *buf, size_t lengthIn) {
  // cout << "Gain got data with " << lengthIn << endl;

  // if(name == "gain3") {
  //   static int count = 0;
  //   cout << count << endl;
  //   count+=(lengthIn/4);
  // }

  if(next) {
    if(_print) {
      cout << "next: " << next->name << endl;
    }
  } else {
    evbuffer_drain(buf, lengthIn);
    return;
  }

  size_t this_read = (lengthIn / 4) * 4;

  unsigned char* temp_read = evbuffer_pullup(buf, this_read);
  uint32_t* temp_read_uint = (uint32_t*)temp_read;

  // cout << temp_read[0] << "," << temp_read[1] << "," << temp_read[2] << endl;
  if(_print) {
    cout << temp_read_uint[0] << "," << temp_read_uint[1] << "," << temp_read_uint[2] << endl;
  }
  // char* badPractice = (char*)malloc(1024*4);
  // bufferevent_write(next->pair->in, badPractice, 1024*4);

  // see
  // http://www.wangafu.net/~nickm/libevent-book/Ref7_evbuffer.html

  struct evbuffer_iovec v[2];
  int n, i;
  size_t n_to_add = this_read;
  size_t in_index = 0;
  size_t j;

  // If we can avoid a buffer copy, we use the evbuffer_reserve_space()
  // this returns 1 or 2 buffers (called extents) which may be shorter
  // or longer individually but will always total to more data than
  // we asked for
  // the outer loop is over these extents

  // Reserve bytes
  n = evbuffer_reserve_space(next->input2, n_to_add, v, 2);
  if (n<=0) {
    cout << "fatal in GainStream::evbuffer_reserve_space "<< n << endl;
    return; /* Unable to reserve the space for some reason. */
  }

  // cout << "iovec had " << n << " extents" << endl;

  // for each extent 
  for (i=0; i<n && n_to_add > 0; ++i) {
    size_t len = v[i].iov_len;
    if (len > n_to_add) { // Don't write more than n_to_add bytes.
       len = n_to_add;
    }
    // cout << "iovec v had len " << len << " (" << n_to_add << ")" << endl;

    char* asChar = (char*)v[i].iov_base;
    uint32_t* asUint = (uint32_t*)v[i].iov_base;
    (void)asChar;
    // asChar[0] = 'a';
    // write until we hit the end of this extent

    // for(j=0;j<len;++j) {
    //   asChar[j] = temp_read[in_index];
    //   ++in_index;
    // }

    for(j=0;j<len/4;++j) {
      asUint[j] = temp_read_uint[in_index] * this->gain;
      ++in_index;
    }


    /* Set iov_len to the number of bytes we actually wrote, so we
       don't commit too much. */
     v[i].iov_len = (len/4)*4;
     // v[i].iov_len = 1024*4;

  }

  /* We commit the space here.  Note that we give it 'i' (the number of
     vectors we actually used) rather than 'n' (the number of vectors we
     had available. */
  if (evbuffer_commit_space(next->input2, v, i) < 0) {
    cout << "fatal in GainStream::evbuffer_commit_space" << endl;
    return; /* Error committing */
  }

  evbuffer_drain(buf, n_to_add);

  // evbuffer_invoke_callbacks_(next->input);

    // if(name == "gain1") {
    // static int times = 0;
    // if( times < 2 ) {
    // times++;
    // bufferevent_write(next->pair->in, badPractice, 4*1024);
    // }
    // }

}

// // runs on _thread
// void GainStream::gotData(struct bufferevent *bev, struct evbuffer *buf, size_t lengthIn) {
//   cout << "Gain got data with " << lengthIn << endl;

//   if(next) {
//     cout << "next: " << next->name << endl;
//   }

//   unsigned char* temp_read = evbuffer_pullup(buf, 1024*4);

//   // see
//   // http://www.wangafu.net/~nickm/libevent-book/Ref7_evbuffer.html

//   struct evbuffer_iovec v[2];
//   int n, i;
//   size_t n_to_add = 1024*4;

//   // Reserve bytes
//   n = evbuffer_reserve_space(bufferevent_get_input(next->pair->in), n_to_add, v, 2);
//   if (n<=0) {
//     cout << "fatal in GainStream::evbuffer_reserve_space "<< n << endl;
//     return; /* Unable to reserve the space for some reason. */
//   }

//   for (i=0; i<n && n_to_add > 0; ++i) {
//     size_t len = v[i].iov_len;
//     cout << "iovec v had len " << len << endl;
//     if (len > n_to_add) /* Don't write more than n_to_add bytes. */
//        len = n_to_add;
//     if (
//       false
//       // generate_data(v[i].iov_base, len) < 0
//       ) {
//        /* If there was a problem during data generation, we can just stop
//           here; no data will be committed to the buffer. */
//        return;
//     }
//     /* Set iov_len to the number of bytes we actually wrote, so we
//        don't commit too much. */
//      v[i].iov_len = len;
//   }

//   /* We commit the space here.  Note that we give it 'i' (the number of
//      vectors we actually used) rather than 'n' (the number of vectors we
//      had available. */
//   if (evbuffer_commit_space(buf, v, i) < 0) {
//     cout << "fatal in GainStream::evbuffer_commit_space" << endl;
//     return; /* Error committing */
//   }

// }





} // namespace