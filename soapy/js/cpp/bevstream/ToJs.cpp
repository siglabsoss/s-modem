#include "ToJs.hpp"
#include <iostream>
#include <cassert>


namespace BevStream {

using namespace std;

// We have a nice cascasing stream setup within libevent, but now we need to get the data back to js
// The idea is that we will fire a pending callback (From the async rain example) and then block
// on a condition variable as described in https://en.cppreference.com/w/cpp/thread/condition_variable

ToJs::ToJs(bool defer_callback, bool print, std::mutex *m, std::condition_variable *cv, std::vector<tojs_t> *pointers, bool* shutdownBool) :
   BevStream(defer_callback,print)
   ,mutfruit(m)
   ,streamOutputReadyCondition(cv)
   ,outputPointers(pointers)
   ,requestShutdown(shutdownBool) {
  this->name = "ToJs";
}


void ToJs::stopThreadDerivedClass() {
  shutdownJsHelper();
}

// runs from same place (thread) that constructed us
void ToJs::setBufferOptions(bufferevent* in, bufferevent* out) {
  bufferevent_setwatermark(out, EV_READ, 1, 0);
  // bufferevent_set_max_single_read(out, (1024)*4 );
}

void ToJs::shutdownJsHelper() {
    // scope for a lock
  {
    std::lock_guard<std::mutex> lk(*this->mutfruit);
    *requestShutdown = true;
    // *(this->outputReady) = true;
    // this->outputPointers->push_back((tojs_t){ptr,this_read});

    // this->outputPointers->erase(this->outputPointers->begin(), this->outputPointers->end());
  }
  this->streamOutputReadyCondition->notify_one();

}

// runs on _thread
void ToJs::gotData(struct bufferevent *bev, struct evbuffer *buf, size_t lengthIn) {
  if(_print) {
    cout << name << " got data with " << lengthIn << endl;
  }


  const size_t this_read = lengthIn;//(lengthIn / 4) * 4;

  void* ptr = malloc(this_read);

  if( ptr == 0 ) {
    cout << "fatal malloc returned 0" << endl;
    return;
  }

  // unsigned char* temp_read = evbuffer_pullup(buf, this_read);
  // uint32_t* temp_read_uint = (uint32_t*)temp_read;

  // http://www.wangafu.net/~nickm/libevent-book/Ref7_evbuffer.html:

  // The evbuffer_remove() function copies and removes the first datlen bytes from the front 
  // of buf into the memory at data. If there are fewer than datlen bytes available, the function
  // copies all the bytes there are. The return value is -1 on failure, and is otherwise the number
  // of bytes copied.

  int res = evbuffer_remove(buf, ptr, this_read);

  if( (size_t)res != this_read ) {
    cout << "fatal evbuffer_remove() didn't write everything" << endl;
    return;
  }

  if( res < 0 ) {
    cout << "fatal in evbuffer_remove()" << endl;
    return;
  }

  // cout << name << " this_read " << this_read << endl;

  // scope for a lock
  {
    std::lock_guard<std::mutex> lk(*this->mutfruit);
    // *(this->outputReady) = true;
    this->outputPointers->push_back((tojs_t){ptr,this_read});
    if(_print) {
      std::cout << "ToJs signals data ready for processing" << endl;;
    }
  }
  this->streamOutputReadyCondition->notify_one();

  // cout << temp_read[0] << "," << temp_read[1] << "," << temp_read[2] << endl;
  // if(_print) {
  //   cout << temp_read_uint[0] << "," << temp_read_uint[1] << "," << temp_read_uint[2] << endl;
  // }
  // char* badPractice = (char*)malloc(1024*4);
  // bufferevent_write(next->pair->in, badPractice, 1024*4);


  // evbuffer_drain(buf, n_to_add);

  // evbuffer_invoke_callbacks_(next->input);

    // if(name == "gain1") {
    // static int times = 0;
    // if( times < 2 ) {
    // times++;
    // bufferevent_write(next->pair->in, badPractice, 4*1024);
    // }
    // }

}




} // namespace