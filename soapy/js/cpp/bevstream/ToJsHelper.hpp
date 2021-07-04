#pragma once

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wcast-function-type"
#include <v8.h>
#pragma GCC diagnostic pop

#include <uv.h>
#include <vector>

using namespace v8;

namespace BevStream {

  typedef struct tojs_t {
    void* mem; // pointer to memory that was allocated with malloc and will be free'd by nodejs
    size_t length; // in bytes
  } tojs_t;

} // namespace

struct Work2 {
  uv_work_t request;
  Persistent<Function> callback;
  Persistent<Function> callbackAfterShutdown;

  Persistent<Function> callbackAux0;
  Persistent<Function> callbackAux1;
  Persistent<Function> callbackAux2;
  Persistent<Function> callbackAux3;

  std::vector<BevStream::tojs_t> workCopy;
};

