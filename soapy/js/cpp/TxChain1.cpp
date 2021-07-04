#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wcast-function-type"
#include <node.h>
#include <v8.h>
#include <nan.h>
#pragma GCC diagnostic pop





#include <uv.h>


#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono>
#include <thread>
#include <iostream>
#include <unistd.h>
#include <condition_variable>
#include <mutex>

#include "FFT.hpp"
#include "IShortToIFloat64.hpp"
#include "ToJsHelper.hpp"
#include "ToJs.hpp"


namespace ExampleChain {

using v8::Context;
using v8::Function;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Local;
using v8::NewStringType;
using v8::Null;
using v8::Object;
using v8::String;
using v8::Value;
using namespace v8;

using namespace std;



////////////////////
//
// My Stream types

#define CONVERT_INPUT_IFLOAT


BevStream::IShortToIFloat64* conv;
BevStream::FFT* fft;
BevStream::ToJs* toJs;

BevStream::BevStream* entrypointObject; // object where data enters from node

std::condition_variable streamOutputReadyCondition;
std::mutex mutfruit;
std::vector<BevStream::tojs_t> toJsPointers;
bool requestShutdown;
bool callbacksSet;
bool startCalled;



// called by libuv worker in separate thread
// BLOCK AT WILL
static void workAsync2(uv_work_t *req)
{
    Work2 *work = static_cast<Work2 *>(req->data); // grab the pointer to the object

    // cout << "before condition" << endl;
    std::unique_lock<std::mutex> lk(mutfruit);
    streamOutputReadyCondition.wait(lk, []{return (toJsPointers.size() > 0)||requestShutdown;});
    // after wait, we own the lock

    // copy into global work pointer
    work->workCopy = toJsPointers;

    // erase the shared one
    toJsPointers.erase(toJsPointers.begin(), toJsPointers.end());

    // release the lock
    lk.unlock();

    // cout << "after condition with " << work->workCopy.size() << endl;
}

// called by libuv in event loop when async function completes
// DO NOT BLOCK
static void workAsyncComplete2(uv_work_t *req, int status)
{
    Isolate * isolate = Isolate::GetCurrent();

    // Fix for Node 4.x - thanks to https://github.com/nwjs/blink/commit/ecda32d117aca108c44f38c8eb2cb2d0810dfdeb
    v8::HandleScope handleScope(isolate);

    Work2 *work = static_cast<Work2 *>(req->data);

    // cout << "work->workCopy.size() " << work->workCopy.size() << endl;

    for(unsigned i = 0; i < work->workCopy.size(); i++) {

      MaybeLocal<Object> buffer = Nan::NewBuffer((char*)work->workCopy[i].mem, work->workCopy[i].length);

      Handle<Value> argv[] = { buffer.ToLocalChecked() };

      // execute the callback
      // https://stackoverflow.com/questions/13826803/calling-javascript-function-from-a-c-callback-in-v8/28554065#28554065
      // buffer copy happens here
      Local<Function>::New(isolate, work->callback)->Call(isolate->GetCurrentContext()->Global(), 1, argv);
    }

    // empty the copy we have in the work object, it will be filled next time workAsync2 runs
    work->workCopy.erase(work->workCopy.begin(), work->workCopy.end());

    if( !requestShutdown ) {
      // always requeue unless we are shutting down
      uv_queue_work(uv_default_loop(),&work->request,workAsync2,workAsyncComplete2);
    } else { 

      Local<Function>::New(isolate, work->callbackAfterShutdown)->Call(isolate->GetCurrentContext()->Global(), 0, NULL);

      // Free up the persistent function callback
      work->callback.Reset();
      delete work;
    }

}



static void reInitVars() {
  callbacksSet = false;
  startCalled = false;

  conv = NULL;
  fft = NULL;
  toJs = NULL;
}

static void initVars() {
  requestShutdown = false;

  reInitVars();
}

static void startStream(const v8::FunctionCallbackInfo<v8::Value>&info) {

  if( !callbacksSet ) {
    Nan::ThrowError("setStreamCallbacks() must be called before startStream()");
    return; // not sure if needed?
  }

  requestShutdown = false;


  uint32_t arg0 = info[0]->Uint32Value(Nan::GetCurrentContext()).ToChecked();
  uint32_t arg1 = info[1]->Uint32Value(Nan::GetCurrentContext()).ToChecked();

  // cout << "startStream " << arg0 << ", " << arg1 << endl;

#ifdef CONVERT_INPUT_IFLOAT

  conv = new BevStream::IShortToIFloat64();

#endif


  fft = new BevStream::FFT(arg0, arg1);

  toJs = new BevStream::ToJs(true, false, &mutfruit, &streamOutputReadyCondition, &toJsPointers, &requestShutdown);

#ifdef CONVERT_INPUT_IFLOAT
  entrypointObject = conv;

  conv->pipe(fft)->pipe(toJs);
#else
  entrypointObject = fft;

  fft->pipe(toJs);
#endif

  usleep(1000); // time for threads to boot before giving control back to js

  startCalled = true;
}

// static void turnstileAdvance(const v8::FunctionCallbackInfo<v8::Value>&info) {

//   if( !startCalled ) {
//     Nan::ThrowError("turnstileAdvance() must be called after startStream()");
//     return; // not sure if needed?
//   }

//   uint32_t adv = info[0]->Uint32Value(Nan::GetCurrentContext()).ToChecked();

//   turnstile->advance((size_t)adv);
// }


// https://github.com/nodejs/nan/blob/master/doc/errors.md#api_nan_throw_error
static void stopStream(const v8::FunctionCallbackInfo<v8::Value>&args) {

  if( !startCalled ) {
    Nan::ThrowError("startStream() must be called before stopStream()");
    return; // not sure if needed?
  }

#ifdef CONVERT_INPUT_IFLOAT
  conv->stopThread();
#endif
  fft->stopThread();
  toJs->stopThread();

#ifdef CONVERT_INPUT_IFLOAT
  delete conv;
#endif

  delete fft;
  delete toJs;

  reInitVars();
}



void setStreamCallbacks(const v8::FunctionCallbackInfo<v8::Value>&args) {

    // cout << "setStreamCallbacks" << endl;
    Isolate* isolate = args.GetIsolate();


    Work2 * work = new Work2();
    work->request.data = work; // the request member will be available to us in subsequent callbacks, so we set it to ourself


    // store the callback from JS in the work package so we can
    // invoke it later
    Local<Function> callbackLocal = Local<Function>::Cast(args[0]);
    work->callback.Reset(isolate, callbackLocal);

    Local<Function> callbackLocal2 = Local<Function>::Cast(args[1]);
    work->callbackAfterShutdown.Reset(isolate, callbackLocal2);

    // kick off the worker thread
    uv_queue_work(uv_default_loop(),&work->request,workAsync2,workAsyncComplete2);


    callbacksSet = true;

    args.GetReturnValue().Set(Undefined(isolate));
}



NAN_METHOD(writeStreamData) {

  if( !startCalled ) {
    Nan::ThrowError("startStream() must be called before writeStreamData()");
    return; // not sure if needed?
  }
  // I am very unclear on the difference between this code for info[0]
  // and code beloe for info[1]
  // possibly because we want to end up with a Local<Object> in the first case
  // and a uint32_t in the second
  MaybeLocal<Object> maybeBufferObj = Nan::To<Object>(info[0]);
  Local<Object> bufferObj;
  if (!maybeBufferObj.ToLocal(&bufferObj)) {
    cout << "Argument 0 Invalid in call to TransformBuffer" << endl;
    return;
  }

  uint32_t length = info[1]->Uint32Value(Nan::GetCurrentContext()).ToChecked();

  const char* dataIn = node::Buffer::Data(bufferObj);

  // cout << "write " << length << endl;
  bufferevent_write(entrypointObject->pair->in, dataIn, length);

}


// void Init(Local<Object> exports, Local<Object> module) {
NAN_MODULE_INIT(Init) {
  NODE_SET_METHOD(target, "setStreamCallbacks", setStreamCallbacks);
  NODE_SET_METHOD(target, "stopStream", stopStream);
  NODE_SET_METHOD(target, "startStream", startStream);
  // NODE_SET_METHOD(target, "fftAdvance", turnstileAdvance);
  NAN_EXPORT(target, writeStreamData);

  initVars();
}

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wcast-function-type"
NODE_MODULE(NODE_GYP_MODULE_NAME, Init)
#pragma GCC diagnostic pop

}  // namespace demo