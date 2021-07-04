
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "TxSchedule.hpp"
#include "EventDsp.hpp"
#include "RadioEstimate.hpp"
#include "AttachedEstimate.hpp"
#include "HiggsFineSync.hpp"

using namespace std;

#include "BindHiggsDriver.hpp"
namespace siglabs {
namespace bind {
namespace higgs {





// send a ringbus to our attached radio
void ringbusLater(uint32_t fpga, uint32_t word, uint32_t us) {
    auto attached_id = soapy->this_node.id;

    raw_ringbus_t rb0 = {fpga, word};
    soapy->dspEvents->ringbusPeerLater(attached_id, &rb0, us);
}
_NAPI_BODY(ringbusLater,void,uint32_t,uint32_t,uint32_t);


void zmqPublishInternal__wrapped(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // cout << "zmqPublishInternal()" << endl;

    bool call_notok = info.Length() != 2 || !info[0].IsString() || !info[1].IsArray();

    if(call_notok) {
        Napi::TypeError::New(env, "zmqPublishInternal() expects arguments of (size_t, array)").ThrowAsJavaScriptException();
        return;
    }

    auto arg0 = info[0].As<Napi::String>();


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

    // info[0].As<Napi::Number>().Uint32Value();
    // auto f = val2::Value;

    // ().Uint32Value()

    // uint32_t valConv = val->Int32Value();

    // cout << HEX32_STRING(val) << endl;


    // soapy->sendToHiggs(arr);
    soapy->zmqPublishInternal(arg0, arr);
}
// void HiggsDriver::zmqPublishInternal(const std::string& header, const std::vector<uint32_t>& vec) {


napi_value getTxPeers__wrapped(const Napi::CallbackInfo& info) {

    Napi::Env env = info.Env();

    // cout << "getRxData()\n";

    if (info.Length() != 0) {
        Napi::TypeError::New(env, "getTxPeers() Invalid argument count").ThrowAsJavaScriptException();
    }

    std::vector<uint32_t> data = soapy->getTxPeers();



    napi_value ret;
    auto status = napi_create_array_with_length(env, data.size(), &ret);
    (void)status;

    unsigned i = 0;
    for(const auto w : data) {
        status = napi_set_element(env, ret, i, Napi::Number::New(env,w));
        i++;
    }
    return ret;
}


// _NAPI_BOUND(soapy->dspEvents->,changeState,int,uint32_t);

_NAPI_BOUND(soapy->,prepareEnvironment,void,void);
_NAPI_BOUND(soapy->,enableThreadByRole,void,string);
_NAPI_BOUND(soapy->,specialSettingsByRole,void,string);
_NAPI_BOUND(soapy->,discoverSocketInodes,void,void);
_NAPI_BOUND(soapy->,bindRxStreamSocket,int32_t,void);
_NAPI_BOUND(soapy->,releaseRxStreamSocket,void,void);
_NAPI_BOUND(soapy->,printAllRunThread,void,void);
_NAPI_BOUND(soapy->,connectTxStreamSocket,int32_t,void);
_NAPI_BOUND(soapy->,connectGrcUdpSocket,int32_t,void);
_NAPI_BOUND(soapy->,connectGrcUdpSocket2,int32_t,void);
// _NAPI_BOUND(soapy->,cs00ControlStream,void,bool);
_NAPI_BOUND(soapy->,cs00TurnstileAdvance,void,uint32_t);
_NAPI_BOUND(soapy->,cs31TriggerSymbolSync,void,uint32_t);
// _NAPI_BOUND(soapy->,cs01TriggerFineSync,void,void);
_NAPI_BOUND(soapy->,cs11TriggerMagAdjust,void,uint32_t);
_NAPI_BOUND(soapy->,cs00SetSfoAdvance,void,uint32_t, uint32_t);
_NAPI_BOUND(soapy->,setDSA,void,uint32_t);
_NAPI_BOUND(soapy->,requestFeedbackBusJam,void,void);
_NAPI_BOUND(soapy->,zmqSubcribes,void,void);
_NAPI_BOUND(soapy->,updateDashboard,void,string);
_NAPI_BOUND(soapy->,zmqUpdateConnections,void,void);
// _NAPI_BOUND(soapy->,zmqPublish,void,string,string);
_NAPI_BOUND(soapy->,zmqRbPeerTag,string,size_t);
_NAPI_BOUND(soapy->,discoveryCheckIdentity,void,void);
_NAPI_BOUND(soapy->,thisNodeReady,bool,void);
_NAPI_BOUND(soapy->,flushHiggsReady,bool,void);
_NAPI_BOUND(soapy->,zmqThreadReady,bool,void);
_NAPI_BOUND(soapy->,boostThreadReady,bool,void);
_NAPI_BOUND(soapy->,dspThreadReady,bool,void);
_NAPI_BOUND(soapy->,txScheduleReady,bool,void);
_NAPI_BOUND(soapy->,demodThreadReady,bool,void);
_NAPI_BOUND(soapy->,txTunBufferLength,size_t,void);
_NAPI_BOUND(soapy->,cs31CoarseSync,void,uint32_t,uint32_t);
_NAPI_BOUND(soapy->,cs32SFOCorrection,void,uint32_t);
_NAPI_BOUND(soapy->,cs32SFOCorrectionShift,void,uint32_t);
_NAPI_BOUND(soapy->,getTxNumber,int32_t,void);



// _NAPI_MEMBER(soapy->dspEvents->,_rx_should_insert_sfo_cfo,bool);

_NAPI_MEMBER(soapy->,higgsRunBoostIO,bool);
_NAPI_MEMBER(soapy->,higgsRunThread,bool);
_NAPI_MEMBER(soapy->,higgsRunOOOThread,bool);
_NAPI_MEMBER(soapy->,higgsRunRbPrintThread,bool);
_NAPI_MEMBER(soapy->,higgsRunZmqThread,bool);
_NAPI_MEMBER(soapy->,higgsRunTunThread,bool);
_NAPI_MEMBER(soapy->,higgsRunDemodThread,bool);
_NAPI_MEMBER(soapy->,higgsRunDspThread,bool);
_NAPI_MEMBER(soapy->,higgsRunFlushHiggsThread,bool);
_NAPI_MEMBER(soapy->,higgsRunTxScheduleThread,bool);
_NAPI_MEMBER(soapy->,higgsRunHiggsFineSyncThread,bool);
_NAPI_MEMBER(soapy->,higgsAttachedViaEth,bool);
_NAPI_MEMBER(soapy->,rx_sock_fd,int);
_NAPI_MEMBER(soapy->,rx_sock_timeout_us,uint32_t);
_NAPI_MEMBER(soapy->,grc_udp_sock_fd,int);
_NAPI_MEMBER(soapy->,grc_udp_sock_fd_2,int);
_NAPI_MEMBER(soapy->,rxFormat,uint32_t);
_NAPI_MEMBER(soapy->,tx_sock_fd,int);
_NAPI_MEMBER(soapy->,activationsPerSetup,uint32_t);
_NAPI_MEMBER(soapy->,discovery_server_message_count,size_t);
_NAPI_MEMBER(soapy->,zmq_should_print_received,bool);
// _NAPI_MEMBER(soapy->,zmq_should_listen,bool);
_NAPI_MEMBER(soapy->,tun_fd,int);
_NAPI_MEMBER(soapy->,connected_node_count,size_t);
_NAPI_MEMBER(soapy->,accept_remote_feedback_bus_higgs,bool);
_NAPI_MEMBER(soapy->this_node.,id,uint32_t);



void init(Napi::Env env, Napi::Object& exports, const std::string role) {

    napi_status status;
    (void)status;
    // Object subtree = Object::New(env);

  
  // _NAPI_GENERIC_INIT(subtree, changeState);

    _NAPI_GENERIC_INIT(exports, prepareEnvironment);
    _NAPI_GENERIC_INIT(exports, enableThreadByRole);
    _NAPI_GENERIC_INIT(exports, specialSettingsByRole);
    _NAPI_GENERIC_INIT(exports, discoverSocketInodes);
    _NAPI_GENERIC_INIT(exports, bindRxStreamSocket);
    _NAPI_GENERIC_INIT(exports, releaseRxStreamSocket);
    _NAPI_GENERIC_INIT(exports, printAllRunThread);
    _NAPI_GENERIC_INIT(exports, connectTxStreamSocket);
    _NAPI_GENERIC_INIT(exports, connectGrcUdpSocket);
    _NAPI_GENERIC_INIT(exports, connectGrcUdpSocket2);
    _NAPI_GENERIC_INIT(exports, zmqSubcribes);
    _NAPI_GENERIC_INIT(exports, updateDashboard);
    _NAPI_GENERIC_INIT(exports, zmqUpdateConnections);
    // _NAPI_GENERIC_INIT(exports, zmqPublish);
    _NAPI_GENERIC_INIT(exports, zmqRbPeerTag);
    _NAPI_GENERIC_INIT(exports, discoveryCheckIdentity);
    _NAPI_GENERIC_INIT(exports, thisNodeReady);
    _NAPI_GENERIC_INIT(exports, flushHiggsReady);
    _NAPI_GENERIC_INIT(exports, zmqThreadReady);
    _NAPI_GENERIC_INIT(exports, boostThreadReady);
    _NAPI_GENERIC_INIT(exports, dspThreadReady);
    _NAPI_GENERIC_INIT(exports, txScheduleReady);
    _NAPI_GENERIC_INIT(exports, demodThreadReady);
    _NAPI_GENERIC_INIT(exports, txTunBufferLength);
    _NAPI_GENERIC_INIT(exports, cs31CoarseSync);
    _NAPI_GENERIC_INIT(exports, cs32SFOCorrection);
    _NAPI_GENERIC_INIT(exports, cs32SFOCorrectionShift);
    _NAPI_GENERIC_INIT(exports, getTxNumber);

    // Hand crafted
    _NAPI_GENERIC_INIT(exports, zmqPublishInternal);
    _NAPI_GENERIC_INIT(exports, getTxPeers);



    if( role != "arb" ) {
        // _NAPI_GENERIC_INIT(exports, cs00ControlStream);
        _NAPI_GENERIC_INIT(exports, cs00TurnstileAdvance);
        _NAPI_GENERIC_INIT(exports, cs31TriggerSymbolSync);
        // _NAPI_GENERIC_INIT(exports, cs01TriggerFineSync);
        _NAPI_GENERIC_INIT(exports, cs11TriggerMagAdjust);
        _NAPI_GENERIC_INIT(exports, cs00SetSfoAdvance);
        _NAPI_GENERIC_INIT(exports, setDSA);
        _NAPI_GENERIC_INIT(exports, requestFeedbackBusJam);

        exports.Set("ringbusLater", Napi::Function::New(env, ringbusLater__wrapped));
    }




  // _NAPI_MEMBER_INIT(subtree, _rx_should_insert_sfo_cfo);

    _NAPI_MEMBER_INIT(exports, higgsRunBoostIO);
    _NAPI_MEMBER_INIT(exports, higgsRunThread);
    _NAPI_MEMBER_INIT(exports, higgsRunOOOThread);
    _NAPI_MEMBER_INIT(exports, higgsRunRbPrintThread);
    _NAPI_MEMBER_INIT(exports, higgsRunZmqThread);
    _NAPI_MEMBER_INIT(exports, higgsRunTunThread);
    _NAPI_MEMBER_INIT(exports, higgsRunDemodThread);
    _NAPI_MEMBER_INIT(exports, higgsRunDspThread);
    _NAPI_MEMBER_INIT(exports, higgsRunFlushHiggsThread);
    _NAPI_MEMBER_INIT(exports, higgsRunTxScheduleThread);
    _NAPI_MEMBER_INIT(exports, higgsRunHiggsFineSyncThread);
    _NAPI_MEMBER_INIT(exports, higgsAttachedViaEth);
    _NAPI_MEMBER_INIT(exports, rx_sock_fd);
    _NAPI_MEMBER_INIT(exports, rx_sock_timeout_us);
    _NAPI_MEMBER_INIT(exports, grc_udp_sock_fd);
    _NAPI_MEMBER_INIT(exports, grc_udp_sock_fd_2);
    _NAPI_MEMBER_INIT(exports, rxFormat);
    _NAPI_MEMBER_INIT(exports, tx_sock_fd);
    _NAPI_MEMBER_INIT(exports, activationsPerSetup);
    _NAPI_MEMBER_INIT(exports, discovery_server_message_count);
    _NAPI_MEMBER_INIT(exports, zmq_should_print_received);
    // _NAPI_MEMBER_INIT(exports, zmq_should_listen);
    _NAPI_MEMBER_INIT(exports, tun_fd);
    _NAPI_MEMBER_INIT(exports, connected_node_count);
    _NAPI_MEMBER_INIT(exports, accept_remote_feedback_bus_higgs);
    _NAPI_MEMBER_INIT(exports, id);



  // napi_set_named_property(env, subtree, "registerCallback", Napi::Function::New(env, registerCallback));
  // _NAPI_GENERIC_INIT(subtree, zmqCallbackCatchType);

  // napi_set_named_property(env, subtree, "sendVectorTypeToPartnerPC", Napi::Function::New(env, sendVectorTypeToPartnerPC));

  // exports.Set("dsp", subtree);
}


}
}
}