
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "TxSchedule.hpp"
#include "ScheduleData.hpp"
#include "FlushHiggs.hpp"

using namespace std;

#include "BindCustomEvent.hpp"
namespace siglabs {
namespace bind {
namespace event {


// CToJavascript stuff
std::mutex mutfruit;
std::condition_variable readyCondition;
std::vector<uint32_t> toJsWords;
Napi::Env env_saved = NULL;

napi_ref* callbackRef;



// called by soapy when an event happens
static void soapyHasEvent(size_t d1, size_t d2) {
    // cout << "  rbCallback ds: " << d1 << "," << d2 << endl;
    std::lock_guard<std::mutex> lk(mutfruit);
    toJsWords.push_back((uint32_t)d1);
    toJsWords.push_back((uint32_t)d2);
    readyCondition.notify_one();
}

// not published to js
static void setCallback(void) {
    soapy->registerEventCallback(&soapyHasEvent);
}



// used by workFactory() when you want to debug or want a short cycle
// void nullFactory() {
//     cout << "nullFactory()" << endl;
// }


// creates a CToJavascript() which is inherited from Napi::AsyncWorker
// When this class is notified, it pulls all values from a std vector
// and pushes them into javascript.  Once this is done javascript runtime will
// call the destructor automatically.  To allow continuous function
// we pass a std::function<> which gets called after the Async job is done
// this std::function<> just calls back to the factory which makes a new object
// and the cycle repeats.  Currently no way to stop (FIXME see CToJavascript::requestShutdown)
void workFactory() {
    // cout << "workFactory()" << endl;
    // std::function<void()> factory = nullFactory;

    napi_value fromRef;

    napi_get_reference_value(env_saved, *callbackRef, &fromRef);

    Function fromRefFunction = Function(env_saved, fromRef);

    auto wk = new CToJavascript<uint32_t>(fromRefFunction, mutfruit, toJsWords, readyCondition, workFactory);
    wk->Queue();

}




size_t timesRegisterCallbackCalled = 0;

void registerCallback(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    cout << "registerCallback()" << endl;

    // this function can only be called once because if the way we chain callbacks of the CToJavascript()
    if( timesRegisterCallbackCalled > 0 ) {
        Napi::TypeError::New(env, "Illegal to call registerCallback() more than once.").ThrowAsJavaScriptException();
        return;
    }

    setCallback();

    env_saved = env; // copy to global for workFactory() usage

    //
    // Account for known potential issues that MUST be handled by
    // synchronously throwing an `Error`
    //
    if (info.Length() != 1) {
        Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
        return;
    }

    if (!info[0].IsFunction()) {
        Napi::TypeError::New(env, "Invalid argument types").ThrowAsJavaScriptException();
        return;
    }

    
    // local only copy
    Function ringbusCallback = info[0].As<Napi::Function>();

    // reference the factory
    std::function<void()> factory = workFactory;


    callbackRef = new napi_ref();

    napi_create_reference(env, ringbusCallback, 1, callbackRef);

    workFactory();

    timesRegisterCallbackCalled++;

}









void init(Napi::Env env, Napi::Object& exports) {

  napi_status status;
  (void)status;

  Object subtree = Object::New(env);

  status = napi_set_named_property(env, subtree, "registerCallback", Napi::Function::New(env, registerCallback));


  exports.Set("customEvent", subtree);
}


}
}
}