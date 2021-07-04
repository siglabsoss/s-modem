
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "TxSchedule.hpp"
#include "EventDsp.hpp"
#include "RadioEstimate.hpp"
#include "AttachedEstimate.hpp"
#include "HiggsFineSync.hpp"

using namespace std;

#include "BindEventDsp.hpp"
namespace siglabs {
namespace bind {
namespace dsp {
















// CToJavascript stuff
static std::mutex mutfruit;
static std::condition_variable readyCondition;
static std::vector<std::vector<uint32_t>> toJsWords;
static Napi::Env env_saved = NULL;
static napi_ref* callbackRef;



// called by soapy when an event happens
static void soapyHasZmq(const std::vector<uint32_t>& full_zmq) {
    std::lock_guard<std::mutex> lk(mutfruit);
    toJsWords.push_back(full_zmq);
    readyCondition.notify_one();
}

// not published to js
static void setCallback(void) {
    soapy->dspEvents->registerZmqCallback(&soapyHasZmq);
}


// creates a CToJavascript() which is inherited from Napi::AsyncWorker
// When this class is notified, it pulls all values from a std vector
// and pushes them into javascript.  Once this is done javascript runtime will
// call the destructor automatically.  To allow continuous function
// we pass a std::function<> which gets called after the Async job is done
// this std::function<> just calls back to the factory which makes a new object
// and the cycle repeats.  Currently no way to stop (FIXME see CToJavascript::requestShutdown)
static void workFactory() {
    // cout << "workFactory()" << endl;
    // std::function<void()> factory = nullFactory;

    napi_value fromRef;

    napi_get_reference_value(env_saved, *callbackRef, &fromRef);

    Function fromRefFunction = Function(env_saved, fromRef);

    auto wk = new CToJavascript<std::vector<uint32_t>>(fromRefFunction, mutfruit, toJsWords, readyCondition, workFactory);
    wk->Queue();
}




static size_t timesRegisterCallbackCalled = 0;

static void registerCallback(const Napi::CallbackInfo& info) {
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

_NAPI_BOUND(soapy->dspEvents->,zmqCallbackCatchType,void,uint32_t);









static void debugJoint__wrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();


    bool call_notok = info.Length() != 5
    || !info[0].IsArray()
    || !info[1].IsNumber()
    || !info[2].IsNumber()
    || !info[3].IsNumber()
    || !info[4].IsNumber();

    if(call_notok) {
        Napi::TypeError::New(env, "debugJoint() expects arguments of (array, size_t, size_t, size_t, size_t)").ThrowAsJavaScriptException();
        return;
    }

    uint32_t len;
    auto status = napi_get_array_length(env,
                          info[0],
                          &len);
    (void)status;

    std::vector<size_t> arg0;
    arg0.reserve(len);

    // cout << "Array len " << len << '\n';

    napi_value val;
    for(uint32_t i = 0; i < len; i++) {

        status = napi_get_element(env, info[0], i, &val);

        Napi::Number val2(env, val);

        uint32_t val3 = val2.Uint32Value();

        arg0.push_back(val3);

        // cout << HEX32_STRING(val3)  << endl;
    }



    uint8_t arg1 = info[1].As<Napi::Number>().Int64Value();
    size_t arg2 = info[2].As<Napi::Number>().Int64Value();
    uint32_t arg3 = info[3].As<Napi::Number>().Int64Value();
    uint32_t arg4 = info[4].As<Napi::Number>().Int64Value();

    soapy->dspEvents->debugJoint(arg0, arg1, arg2, arg3, arg4);
}


static napi_value scheduleJointFromTunData__wrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();


    bool call_notok = info.Length() != 5
    || !info[0].IsArray()
    || !info[1].IsNumber()
    || !info[2].IsNumber()
    || !info[3].IsNumber()
    || !info[4].IsNumber();

    if(call_notok) {
        Napi::TypeError::New(env, "scheduleJointFromTunData() expects arguments of (array, size_t, size_t, size_t, size_t)").ThrowAsJavaScriptException();
        return NULL;
    }

    uint32_t len;
    auto status = napi_get_array_length(env,
                          info[0],
                          &len);
    (void)status;

    std::vector<size_t> arg0;
    arg0.reserve(len);

    // cout << "Array len " << len << '\n';

    napi_value val;
    for(uint32_t i = 0; i < len; i++) {

        status = napi_get_element(env, info[0], i, &val);

        Napi::Number val2(env, val);

        uint32_t val3 = val2.Uint32Value();

        arg0.push_back(val3);

        // cout << HEX32_STRING(val3)  << endl;
    }



    uint8_t arg1 = info[1].As<Napi::Number>().Int64Value();
    size_t arg2 = info[2].As<Napi::Number>().Int64Value();
    uint32_t arg3 = info[3].As<Napi::Number>().Int64Value();
    uint32_t arg4 = info[4].As<Napi::Number>().Int64Value();

    size_t ret0, ret1;
    std::tie(ret0,ret1) = soapy->dspEvents->scheduleJointFromTunData(arg0, arg1, arg2, arg3, arg4);

    napi_value ret;
    status = napi_create_array_with_length(env, 2, &ret);



    napi_value a,b;

    status = napi_create_int64(env, ret0, &a);
    status = napi_create_int64(env, ret1, &b);

    status = napi_set_element(env, ret, 0, a);
    status = napi_set_element(env, ret, 1, b);

    return ret;
}























static void sendVectorTypeToPartnerPC(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();



    bool call_notok = info.Length() != 3 || !info[0].IsNumber() || !info[1].IsNumber() || !info[2].IsArray();

    if(call_notok) {
        Napi::TypeError::New(env, "sendVectorTypeToPartnerPC() expects arguments of (size_t, size_t, array)").ThrowAsJavaScriptException();
        return;
    }

    // cout << "call ok!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";

    size_t arg0 = info[0].As<Napi::Number>().Int64Value();
    size_t arg1 = info[1].As<Napi::Number>().Int64Value();

    uint32_t len;
    auto status = napi_get_array_length(env,
                          info[2],
                          &len);
    (void)status;

    std::vector<uint32_t> arg2;
    arg2.reserve(len);

    // cout << "Array len " << len << '\n';

    napi_value val;
    for(uint32_t i = 0; i < len; i++) {

        status = napi_get_element(env, info[2], i, &val);

        Napi::Number val2(env, val);

        uint32_t val3 = val2.Uint32Value();

        arg2.push_back(val3);

        // cout << HEX32_STRING(val3)  << endl;
    }

    soapy->dspEvents->sendVectorTypeToPartnerPC(arg0, arg1, arg2);
}




static void sendVectorTypeToMultiPC(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();



    bool call_notok = info.Length() != 3 || !info[0].IsArray() || !info[1].IsNumber() || !info[2].IsArray();

    if(call_notok) {
        Napi::TypeError::New(env, "sendVectorTypeToMultiPC() expects arguments of (array, size_t, array)").ThrowAsJavaScriptException();
        return;
    }
    uint32_t len;



    auto status = napi_get_array_length(env,
                          info[0],
                          &len);
    (void)status;

    std::vector<size_t> arg0;
    arg0.reserve(len);

    // cout << "Array len " << len << '\n';

    napi_value val;
    for(uint32_t i = 0; i < len; i++) {

        status = napi_get_element(env, info[0], i, &val);

        Napi::Number val2(env, val);

        uint32_t val3 = val2.Uint32Value();

        arg0.push_back(val3);

        // cout << HEX32_STRING(val3)  << endl;
    }




    // cout << "call ok!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";

    // size_t arg0 = info[0].As<Napi::Number>().Int64Value();
    size_t arg1 = info[1].As<Napi::Number>().Int64Value();

    status = napi_get_array_length(env,
                          info[2],
                          &len);
    (void)status;

    std::vector<uint32_t> arg2;
    arg2.reserve(len);

    // cout << "Array len " << len << '\n';

    // napi_value val;
    for(uint32_t i = 0; i < len; i++) {

        status = napi_get_element(env, info[2], i, &val);

        Napi::Number val2(env, val);

        uint32_t val3 = val2.Uint32Value();

        arg2.push_back(val3);

        // cout << HEX32_STRING(val3)  << endl;
    }

    soapy->dspEvents->sendVectorTypeToMultiPC(arg0, arg1, arg2);
}


static void setPartnerEq__wrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();


    bool call_notok = info.Length() != 2 || !info[0].IsNumber() || !info[1].IsArray();

    if(call_notok) {
        Napi::TypeError::New(env, "setPartnerEq() expects arguments of (size_t, array)").ThrowAsJavaScriptException();
        return;
    }

    auto arg0 = info[0].As<Napi::Number>().Uint32Value();

    uint32_t len;
    auto status = napi_get_array_length(env,
                                  info[1],
                                  &len);
    (void)status;

    std::vector<uint32_t> arr;
    arr.reserve(len);

    // cout << "Array len " << len << '\n';

    napi_value val;
    for(uint32_t i = 0; i < len; i++) {

        status = napi_get_element(env, info[1], i, &val);

        Napi::Number val2(env, val);

        uint32_t val3 = val2.Uint32Value();

        arr.push_back(val3);

        // cout << HEX32_STRING(val3)  << endl;
    }

    soapy->dspEvents->setPartnerEq(arg0, arr);
}





_NAPI_BOUND(soapy->dspEvents->,changeState,int,uint32_t);
_NAPI_BOUND(soapy->dspEvents->,resetCoarseState,void,void);
_NAPI_BOUND(soapy->dspEvents->,setPartnerSfoAdvance,void,uint32_t,uint32_t,uint32_t,bool);
_NAPI_BOUND(soapy->dspEvents->,nodeIdForRadioIndex,uint32_t,size_t);
_NAPI_BOUND(soapy->dspEvents->,radioIndexForNodeId,size_t,uint32_t);
_NAPI_BOUND(soapy->dspEvents->,subcarrierForBufferIndex,uint32_t,size_t);
_NAPI_BOUND(soapy->dspEvents->,generateSchedule,void,void);
_NAPI_BOUND(soapy->dspEvents->,getPeerMaskForFrame,int32_t,uint32_t);
_NAPI_BOUND(soapy->dspEvents->,updateSettingsFlags,void,void);
_NAPI_BOUND(soapy->dspEvents->,ringbusPeerLater,void,size_t, uint32_t, uint32_t, size_t);
_NAPI_BOUND(soapy->dspEvents->,resetPartnerEq,void,size_t);
_NAPI_BOUND(soapy->dspEvents->,resetPartnerBs,void,size_t,string);
_NAPI_BOUND(soapy->dspEvents->,setPartnerTDMA,void,size_t, uint32_t, uint32_t);
_NAPI_BOUND(soapy->dspEvents->,setPartnerCfo,void,size_t, double, bool);
_NAPI_BOUND(soapy->dspEvents->,setPartnerPhase,void,size_t, double);
_NAPI_BOUND(soapy->dspEvents->,sendPartnerNoop,void,size_t);
// _NAPI_BOUND(soapy->dspEvents->,resetPartnerScheduleAdvance,void,size_t);
// _NAPI_BOUND(soapy->dspEvents->,advancePartnerSchedule,void,size_t, uint32_t);
// _NAPI_BOUND(soapy->dspEvents->,setPartnerScheduleMode,void,size_t, uint32_t, uint32_t);
// _NAPI_BOUND(soapy->dspEvents->,setPartnerScheduleModeIndex,void,size_t, uint32_t, uint32_t);
_NAPI_BOUND(soapy->dspEvents->,pingPartnerFbBus,void,size_t);
_NAPI_BOUND(soapy->dspEvents->,sendCustomEventToPartner,void,size_t, uint32_t, uint32_t);
_NAPI_BOUND(soapy->dspEvents->,dspDispatchAttachedRb,void,uint32_t);
_NAPI_BOUND(soapy->dspEvents->,allPeersConnected,bool,void);
_NAPI_BOUND(soapy->dspEvents->,dumpTunBuffer,unsigned,void);
_NAPI_BOUND(soapy->dspEvents->,wordsInTun,size_t,void);
_NAPI_BOUND(soapy->dspEvents->,tickleAll,void,void);
_NAPI_BOUND(soapy->dspEvents->,op,void,uint32_t,string,uint32_t,uint32_t);
_NAPI_BOUND(soapy->dspEvents->,forceZeros,void,size_t,bool);
_NAPI_BOUND(soapy->dspEvents->,test_fb,void,void);
_NAPI_BOUND(soapy->dspEvents->,suppressFeedbackEqUntil,void,uint32_t,bool);


  // these actually come from HiggsFineSync
_NAPI_BOUND(soapy->higgsFineSync->,dump_sfo_cfo_buffer,void,void);



_NAPI_MEMBER(soapy->dspEvents->,_rx_should_insert_sfo_cfo,bool);
// _NAPI_MEMBER(soapy->dspEvents->,estimate_filter_mode,int);
_NAPI_MEMBER(soapy->dspEvents->,init_drain_udp_payload,bool);
// _NAPI_MEMBER(soapy->dspEvents->,init_drain_sfo_cfo,bool);
_NAPI_MEMBER(soapy->dspEvents->,init_drain_all_sc,bool);
_NAPI_MEMBER(soapy->dspEvents->,init_drain_coarse,bool);
_NAPI_MEMBER(soapy->dspEvents->,drain_gnurado_stream,bool);
_NAPI_MEMBER(soapy->dspEvents->,drain_gnurado_stream_2,bool);
_NAPI_MEMBER(soapy->dspEvents->,init_drain_for_gnuradio,bool);
_NAPI_MEMBER(soapy->dspEvents->,init_drain_for_gnuradio_2,bool);
_NAPI_MEMBER(soapy->dspEvents->,init_drain_for_inbound_fb_bus,bool);
_NAPI_MEMBER(soapy->dspEvents->,dsp_state,size_t);
_NAPI_MEMBER(soapy->dspEvents->,dsp_state_pending,size_t);
_NAPI_MEMBER(soapy->dspEvents->,_do_wait,size_t);
_NAPI_MEMBER(soapy->dspEvents->,fuzzy_recent_frame,uint32_t);
_NAPI_MEMBER(soapy->dspEvents->,_cfo_symbol_num,unsigned);
_NAPI_MEMBER(soapy->dspEvents->,_sfo_symbol_num,unsigned);
_NAPI_MEMBER(soapy->dspEvents->,outstanding_fsm_ping,bool);
_NAPI_MEMBER(soapy->dspEvents->,times_tried_attached_feedback_bus,uint32_t);
_NAPI_MEMBER(soapy->dspEvents->,valid_count_default_stream,size_t);
_NAPI_MEMBER(soapy->dspEvents->,valid_count_all_sc_stream,size_t);
_NAPI_MEMBER(soapy->dspEvents->,valid_count_fine_sync,size_t);
_NAPI_MEMBER(soapy->dspEvents->,valid_count_default_stream_p,size_t);
_NAPI_MEMBER(soapy->dspEvents->,valid_count_all_sc_stream_p,size_t);
_NAPI_MEMBER(soapy->dspEvents->,valid_count_fine_sync_p,size_t);
_NAPI_MEMBER(soapy->dspEvents->,_debug_loopback_fine_sync_counter,bool);
_NAPI_MEMBER(soapy->dspEvents->,enableRepeat,bool);
_NAPI_MEMBER(soapy->dspEvents->,grc_shows_eq_filter,bool);
_NAPI_MEMBER(soapy->dspEvents->,downsample_gnuradio,uint32_t);
_NAPI_MEMBER(soapy->dspEvents->,blockFsm,int);
_NAPI_MEMBER(soapy->dspEvents->,fsm_print_peers,bool);


void init(Napi::Env env, Napi::Object& exports) {

  napi_status status;
  (void)status;
  
  Object subtree = Object::New(env);

  
  // status = napi_set_named_property(env, subtree, "changeState", Napi::Function::New(env, changeState__wrapped));
  _NAPI_GENERIC_INIT(subtree, changeState);
  _NAPI_GENERIC_INIT(subtree, resetCoarseState);
  _NAPI_GENERIC_INIT(subtree, setPartnerSfoAdvance);
  _NAPI_GENERIC_INIT(subtree, nodeIdForRadioIndex);
  _NAPI_GENERIC_INIT(subtree, radioIndexForNodeId);
  _NAPI_GENERIC_INIT(subtree, subcarrierForBufferIndex);
  _NAPI_GENERIC_INIT(subtree, generateSchedule);
  _NAPI_GENERIC_INIT(subtree, getPeerMaskForFrame);
  _NAPI_GENERIC_INIT(subtree, updateSettingsFlags);
  _NAPI_GENERIC_INIT(subtree, ringbusPeerLater);
  _NAPI_GENERIC_INIT(subtree, resetPartnerEq);
  _NAPI_GENERIC_INIT(subtree, resetPartnerBs);
  _NAPI_GENERIC_INIT(subtree, setPartnerTDMA);
  _NAPI_GENERIC_INIT(subtree, setPartnerCfo);
  _NAPI_GENERIC_INIT(subtree, setPartnerPhase);
  _NAPI_GENERIC_INIT(subtree, sendPartnerNoop);
  // _NAPI_GENERIC_INIT(subtree, resetPartnerScheduleAdvance);
  // _NAPI_GENERIC_INIT(subtree, advancePartnerSchedule);
  // _NAPI_GENERIC_INIT(subtree, setPartnerScheduleMode);
  // _NAPI_GENERIC_INIT(subtree, setPartnerScheduleModeIndex);
  _NAPI_GENERIC_INIT(subtree, pingPartnerFbBus);
  _NAPI_GENERIC_INIT(subtree, sendCustomEventToPartner);
  _NAPI_GENERIC_INIT(subtree, dspDispatchAttachedRb);
  _NAPI_GENERIC_INIT(subtree, allPeersConnected);
  _NAPI_GENERIC_INIT(subtree, dumpTunBuffer);
  _NAPI_GENERIC_INIT(subtree, wordsInTun);
  _NAPI_GENERIC_INIT(subtree, tickleAll);
  _NAPI_GENERIC_INIT(subtree, op);
  _NAPI_GENERIC_INIT(subtree, forceZeros);
  _NAPI_GENERIC_INIT(subtree, test_fb);
  _NAPI_GENERIC_INIT(subtree, suppressFeedbackEqUntil);

  // Hand crafted
  _NAPI_GENERIC_INIT(subtree, setPartnerEq);
  
  // these actually come from HiggsFineSync
  _NAPI_GENERIC_INIT(subtree, dump_sfo_cfo_buffer);



  _NAPI_MEMBER_INIT(subtree, _rx_should_insert_sfo_cfo);
  // _NAPI_MEMBER_INIT(subtree, estimate_filter_mode);
  _NAPI_MEMBER_INIT(subtree, init_drain_udp_payload);
  // _NAPI_MEMBER_INIT(subtree, init_drain_sfo_cfo);
  _NAPI_MEMBER_INIT(subtree, init_drain_all_sc);
  _NAPI_MEMBER_INIT(subtree, init_drain_coarse);
  _NAPI_MEMBER_INIT(subtree, drain_gnurado_stream);
  _NAPI_MEMBER_INIT(subtree, drain_gnurado_stream_2);
  _NAPI_MEMBER_INIT(subtree, init_drain_for_gnuradio);
  _NAPI_MEMBER_INIT(subtree, init_drain_for_gnuradio_2);
  _NAPI_MEMBER_INIT(subtree, init_drain_for_inbound_fb_bus);
  _NAPI_MEMBER_INIT(subtree, dsp_state);
  _NAPI_MEMBER_INIT(subtree, dsp_state_pending);
  _NAPI_MEMBER_INIT(subtree, _do_wait);
  _NAPI_MEMBER_INIT(subtree, fuzzy_recent_frame);
  _NAPI_MEMBER_INIT(subtree, _cfo_symbol_num);
  _NAPI_MEMBER_INIT(subtree, _sfo_symbol_num);
  _NAPI_MEMBER_INIT(subtree, outstanding_fsm_ping);
  _NAPI_MEMBER_INIT(subtree, times_tried_attached_feedback_bus);
  _NAPI_MEMBER_INIT(subtree, valid_count_default_stream);
  _NAPI_MEMBER_INIT(subtree, valid_count_all_sc_stream);
  _NAPI_MEMBER_INIT(subtree, valid_count_fine_sync);
  _NAPI_MEMBER_INIT(subtree, valid_count_default_stream_p);
  _NAPI_MEMBER_INIT(subtree, valid_count_all_sc_stream_p);
  _NAPI_MEMBER_INIT(subtree, valid_count_fine_sync_p);
  _NAPI_MEMBER_INIT(subtree, _debug_loopback_fine_sync_counter);
  _NAPI_MEMBER_INIT(subtree, enableRepeat);
  _NAPI_MEMBER_INIT(subtree, grc_shows_eq_filter);
  _NAPI_MEMBER_INIT(subtree, downsample_gnuradio);
  _NAPI_MEMBER_INIT(subtree, blockFsm);
  _NAPI_MEMBER_INIT(subtree, fsm_print_peers);



  napi_set_named_property(env, subtree, "registerCallback", Napi::Function::New(env, registerCallback));
  _NAPI_GENERIC_INIT(subtree, zmqCallbackCatchType);
  _NAPI_GENERIC_INIT(subtree, debugJoint);
  _NAPI_GENERIC_INIT(subtree, scheduleJointFromTunData);

  napi_set_named_property(env, subtree, "sendVectorTypeToPartnerPC", Napi::Function::New(env, sendVectorTypeToPartnerPC));
  napi_set_named_property(env, subtree, "sendVectorTypeToMultiPC", Napi::Function::New(env, sendVectorTypeToMultiPC));

  exports.Set("dsp", subtree);
}


}
}
}