#include <napi.h>
#define VERILATE_TESTBENCH
#include "feedback_bus.h"

#include "BindCommonInclude.hpp"

#include "EventDsp.hpp"
// #include "RadioEstimate.hpp"
#include "DispatchMockRpc.hpp"

#include "BindData.hpp"
#include "BindTxSchedule.hpp"
#include "BindSchedule.hpp"
#include "BindFlushHiggs.hpp"
#include "BindEventDsp.hpp"
#include "BindRadioEstimate.hpp"
#include "BindSettings.hpp"
#include "BindCustomEvent.hpp"
#include "BindHiggsDriver.hpp"
#include "BindAirPacket.hpp"
#include "BindDemodThread.hpp"
#include "BindEVMThread.hpp"
#include "BindHiggsSNR.hpp"


using namespace std;
using namespace SoapySDR;

std::string g_init;
std::string g_patch;
std::string g_role;



// namespace signals {



SoapySDR::Device* device;
SoapySDR::Stream* rxStream;
HiggsDriver* soapy;

// ringbus stuff
std::mutex mutfruit;
std::condition_variable ringbusReadyCondition;
std::vector<uint32_t> toJsRingbusWords;
// napi_escapable_handle_scope ringbus_scope;
Napi::Env rbenv = NULL;

napi_ref* ringbusCallbackRef;




//////////// modem main /////////////////////
int main_stream_ints(void)
{

  cout << "Modem Boot" << endl;

  SoapySDR::Kwargs kwfind;
  kwfind["init_path"] = g_init;
  kwfind["patch_path"] = g_patch;

  SoapySDR::KwargsList results = findHiggs(kwfind);

  cout << "Enumeration found " << results.size() << " devices" << endl;

  for (size_t i = 0; i < results.size(); i++)
  {
    cout << "Found device #" << i << endl;

    for (auto const& x : results[i])
    {
        std::cout << x.first  // string (key)
                  << ':' 
                  << x.second // string's value 
                  << std::endl ;
    }
  }

  // std::cout << "Make device " << argStr << std::endl;
  try
  {
    device = SoapySDR::Device::make(kwfind);
    std::cout << "  driver=" << device->getDriverKey() << std::endl;
    std::cout << "  hardware=" << device->getHardwareKey() << std::endl;
    for (const auto &it : device->getHardwareInfo())
    {
        std::cout << "  " << it.first << "=" << it.second << std::endl;
    }

    soapy = (HiggsDriver*)device;
    
  }
  catch (const std::exception &ex)
  {
        std::cerr << "Error making device: " << ex.what() << std::endl;
        return EXIT_FAILURE;
  }
  std::cout << std::endl;
  return 0;
}

int main_stream_ints_part_2(void) {

    std::vector<size_t> channels = {0,1};

    // Note that these settings are all string type
    SoapySDR::Kwargs kwsettings;

    // kwsettings["init_path"] = g_init;
    // kwsettings["patch_path"] = g_patch;

    rxStream = soapy->setupStream(
      SOAPY_SDR_RX,
      SOAPY_SDR_CS16,
      channels,
      kwsettings
      );

    // cout << device << "\n";
    // cout << soapy << "\n";
    // exit(0);

    soapy->activateStream(
      rxStream,
      0,
      0,
      0
      );
    return 0;
}


///////////////////////// end ///////////////////////////



// due to quirk, cannot say std::string
_NAPI_BOUND(soapy->dspEvents->dispatchRpc->,dispatch,void,string);


void preSmodemSetGlobals(void) {
    if( g_role == "arb" ) {
        g_init = DEFAULT_JSON_PATH_ARBITER;
        g_patch = "";
    } else {
        g_init = DEFAULT_JSON_PATH;
        g_patch = "";
    }
}

void blockUntilStarted(void) {
    
    bool done = false;
    int loops = 0;
    while(!done) {
        done = 
           soapy->flushHiggsReady()
        && soapy->dspThreadReady()
        && soapy->thisNodeReady()
        && soapy->zmqThreadReady();
        loops++;
        // cout << "done: " << done << endl;
    }

    // cout << "blocked for " << loops << " loops" << endl;

    // auto r1 = soapy->flushHiggsReady();
    // auto r2 = soapy->dspThreadReady();
    // cout << "r1 " << r1 << ", r2 " << r2 << endl;
}

_NAPI_BOUND(soapy->,ringbusCallbackCatchType,void,uint32_t);

// this is called directly by Soapy when a ringbus matches ours
void rbCallback(uint32_t word) {
    // cout << "  rbCallback word " << word << endl;
    std::lock_guard<std::mutex> lk(mutfruit);
    toJsRingbusWords.push_back(word);
    ringbusReadyCondition.notify_one();
    // cout << "  After notify" << endl;
    // if(_print) {
    //   std::cout << "ToJs signals data ready for processing" << endl;;
    // }
}


void postSmodemStart(void) {
    // hooks or whatever

    // soapy->ringbusCallbackCatchType(0x01000000);
    // soapy->ringbusCallbackCatchType(0x22000000);
    // soapy->ringbusCallbackCatchType(0x66000000);

    // tell Soapy which function to call when a ringbus matches
    soapy->registerRingbusCallback(&rbCallback);
}


// Start smodem slower, giving us time to do stuff

void startSmodemFirst(void) {
    cout << "startSmodem() FIRST" << endl;

    main_stream_ints();
}
_NAPI_BODY(startSmodemFirst,void,void);

void startSmodemSecond(void) {
    cout << "startSmodem() SECOND" << endl;

    main_stream_ints_part_2();

    blockUntilStarted();

    postSmodemStart();
}
_NAPI_BODY(startSmodemSecond,void,void);







void startSmodem(void) {
    cout << "startSmodem()" << endl;

    main_stream_ints();
    main_stream_ints_part_2();

    blockUntilStarted();

    postSmodemStart();
}
_NAPI_BODY(startSmodem,void,void);


void startArb(void) {
    cout << "startArb()" << endl;

    main_stream_ints();
    main_stream_ints_part_2();

}
_NAPI_BODY(startArb,void,void);


void enterDebugMode(void) {
    soapy->enterDebugMode();
}
_NAPI_BODY(enterDebugMode,void,void);




void stopSmodem(void) {

    device->deactivateStream(rxStream, 0, 0);
    device->closeStream(rxStream);

    SoapySDR::Device::unmake(device);
}
_NAPI_BODY(stopSmodem,void,void);


// used by ringbusFactory() when you want to debug or want a short cycle
void nullFactory() {
    cout << "nullFactory()" << endl;
}


// creates a CToJavascript() which is inherited from Napi::AsyncWorker
// When this class is notified, it pulls all values from a std vector
// and pushes them into javascript.  Once this is done javascript runtime will
// call the destructor automatically.  To allow continuous function
// we pass a std::function<> which gets called after the Async job is done
// this std::function<> just calls back to the factory which makes a new object
// and the cycle repeats.  Currently no way to stop (FIXME see CToJavascript::requestShutdown)
void ringbusFactory() {
    // cout << "ringbusFactory()" << endl;
    std::function<void()> factory = nullFactory;

    napi_value fromRef;

    napi_get_reference_value(rbenv, *ringbusCallbackRef, &fromRef);

    Function fromRefFunction = Function(rbenv, fromRef);

    auto wk = new CToJavascript<uint32_t>(fromRefFunction, mutfruit, toJsRingbusWords, ringbusReadyCondition, ringbusFactory);
    wk->Queue();

}

size_t timesRegisterRingbusCallbackCalled = 0;

void registerRingbusCallback(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    cout << "registerRingbusCallback()" << endl;

    // this function can only be called once because if the way we chain callbacks of the CToJavascript()
    if( timesRegisterRingbusCallbackCalled > 0 ) {
        Napi::TypeError::New(env, "Illegal to call registerRingbusCallback() more than once.").ThrowAsJavaScriptException();
    }

    rbenv = env; // copy to global for ringbusFactory() usage

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
    std::function<void()> factory = ringbusFactory;


    ringbusCallbackRef = new napi_ref();

    napi_create_reference(env, ringbusCallback, 1, ringbusCallbackRef);

    ringbusFactory();

    timesRegisterRingbusCallbackCalled++;

}

static std::vector<std::string> getPropNames(Napi::Env env, Napi::Object g) {
    auto all = g.GetPropertyNames();

    uint32_t len;
    auto status = napi_get_array_length(env,
                                  all,
                                  &len);
    (void)status;

    std::vector<std::string> ret;

    napi_value val;
    for(uint32_t i = 0; i < len; i++) {

        status = napi_get_element(env, all, i, &val);

        Napi::String val2(env, val);

        std::string asstr = val2.As<Napi::String>();

        ret.emplace_back(asstr);

        // cout << asstr << "\n";

        
    }

    return ret;
}

static void printPropNames(Napi::Env env, Napi::Object g) {
    auto names = getPropNames(env, g);

    for(auto x : names) {
        cout << x << "\n";
    }
}


void printEnv(Napi::Env env) {
    Napi::Object g =  env.Global();

    printPropNames(env,g);

    // auto names = getPropNames(env, g);

    // for(auto x : names) {
    //     cout << x << "\n";
    // }
}


_NAPI_BODY(getErrorFeedbackBusParseCritical,uint32_t,uint32_t);




static Napi::Object Init(Napi::Env env, Napi::Object exports) {

    // cout << "Napi::Object\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n\n";

    /// 
    /// Check for process.env.NAPI_SMODEM_ROLE
    ///
    Napi::Object g = env.Global();
    napi_status status;
    (void)status;
    napi_value process;
    status = napi_get_named_property(env, g, "process", &process);
    napi_value proc_env;
    status = napi_get_named_property(env, process, "env", &proc_env);
    // printPropNames(env, Napi::Object(env,proc_env));

    napi_value env_role;
    status = napi_get_named_property(env, proc_env, "NAPI_SMODEM_ROLE", &env_role);

    auto check_env_role = Napi::Value(env, env_role);
    napi_valuetype check_type = check_env_role.Type();

    if( check_type == 0 ) {
        // cout << "ttttt " << check_type << "\n";
        cout << "\n";
        cout << "  You must set process.env.NAPI_SMODEM_ROLE before including sjs.node\n\n";
        return {};
    }

    Napi::String val2(env, env_role);
    g_role = val2.As<Napi::String>();

    cout << "sjs.node Init() called as '" << g_role << "'\n";

    preSmodemSetGlobals(); // do not call this unless g_role is set


    if( g_role == "arb" ) {
        _NAPI_BIND_NAME_SINGLE(exports,startArb);
    } else {
        _NAPI_BIND_NAME_SINGLE(exports,startSmodem);
        _NAPI_BIND_NAME_SINGLE(exports,startSmodemFirst);
        _NAPI_BIND_NAME_SINGLE(exports,startSmodemSecond);
        _NAPI_BIND_NAME_SINGLE(exports,enterDebugMode);
        _NAPI_BIND_NAME_SINGLE(exports,ringbusCallbackCatchType);
        exports.Set("registerRingbusCallback", Napi::Function::New(env, registerRingbusCallback)); // not named __wrapped
    }

    _NAPI_BIND_NAME_SINGLE(exports,getErrorFeedbackBusParseCritical);
    _NAPI_BIND_NAME_SINGLE(exports,stopSmodem);

    exports.Set("dispatchMockRpc", Napi::Function::New(env, dispatch__wrapped));


    siglabs::bind::higgs::init(env, exports, g_role);


    siglabs::bind::txSchedule::init(env, exports);
    siglabs::bind::schedule::init(env, exports);
    siglabs::bind::data::init(env, exports);
    if( g_role != "arb" ) {
        siglabs::bind::flushHiggs::init(env, exports);
        siglabs::bind::demodThread::init(env, exports);
        siglabs::bind::evmThread::init(env, exports);
        siglabs::bind::higgsSNR::init(env, exports, g_role);
    }
    siglabs::bind::dsp::init(env, exports);
    if( g_role != "arb" ) {
        siglabs::bind::re::init(env, exports);
    }
    siglabs::bind::settings::init(env, exports);
    siglabs::bind::event::init(env, exports);
    siglabs::bind::airPacket::init(env, exports);


    return exports;
}


NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
