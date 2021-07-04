#pragma once

#include "BevStream.hpp"
#include "ToJsHelper.hpp"

#include <mutex>
#include <condition_variable>
#include <vector>



namespace BevStream {

class ToJs : public BevStream
{
public:

  ToJs(bool defer_callback, bool print, std::mutex *m, std::condition_variable *cv, std::vector<tojs_t> *pointers, bool* shutdownBool);

  void stopThreadDerivedClass();
  void setBufferOptions(bufferevent* in, bufferevent* out);
  void gotData(struct bufferevent *bev, struct evbuffer *buf, size_t len);
  void shutdownJsHelper();

  std::mutex *mutfruit = 0;
  std::condition_variable *streamOutputReadyCondition = 0;
  std::vector<tojs_t> *outputPointers;
  bool *requestShutdown;

};












}
