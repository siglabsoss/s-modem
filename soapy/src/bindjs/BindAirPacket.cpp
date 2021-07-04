
#include "BindCommonInclude.hpp"
// #include "BindHiggsInclude.hpp"

// #include "TxSchedule.hpp"
// #include "ScheduleData.hpp"
// #include "FlushHiggs.hpp"
#include "AirPacket.hpp"

using namespace std;

// #include "BindFlushHiggs.hpp"
namespace siglabs {
namespace bind {
namespace airPacket {

// static AirPacket ap;
static AirPacketOutbound airTx;
static AirPacketInbound airRx;



_NAPI_BOUND(airTx.,set_code_type, int,  uint32_t ,  uint32_t ,  uint32_t );
_NAPI_BOUND(airTx.,set_code_puncturing, void,  uint32_t );
_NAPI_BOUND(airTx.,set_modulation_schema,void,uint32_t );
_NAPI_BOUND(airTx.,set_subcarrier_allocation, void,  uint32_t );
_NAPI_BOUND(airTx.,getEffectiveSlicedWordCount, uint32_t, void) ;
_NAPI_BOUND(airTx.,getForceDetectIndex, uint32_t,  uint32_t ) ;
_NAPI_BOUND(airTx.,getMapMovVectorInfo, string,  size_t ) ;
_NAPI_BOUND(airTx.,get_rs, int, void);
_NAPI_BOUND(airTx.,get_interleaver, int, void);
_NAPI_BOUND(airTx.,getLengthForInput, uint32_t, uint32_t);

// _NAPI_BOUND (airTx.,getMapMovVectorInfo,uint32_t,void);
// _NAPI_BOUND (soapy->flushHiggs->,getLowLen,uint32_t,void);
// _NAPI_BOUND (soapy->flushHiggs->,dumpLow,void,void);
// _NAPI_MEMBER(soapy->flushHiggs->,drain_higgs,bool);
// _NAPI_MEMBER(soapy->flushHiggs->,allow_zmq_higgs,bool);
// _NAPI_MEMBER(soapy->flushHiggs->,print,bool);
// _NAPI_MEMBER(soapy->flushHiggs->,printSending,bool);



static napi_value getSizeInfo__wrapped(const Napi::CallbackInfo& info) {

    Napi::Env env = info.Env();

    const bool call_notok = info.Length() != 1
    || !info[0].IsNumber();

    if(call_notok) {
        Napi::TypeError::New(env, "getSizeInfo() expects arguments of (size_t)").ThrowAsJavaScriptException();
        return NULL;
    }

    const size_t arg0 = info[0].As<Napi::Number>().Int64Value();

    const auto ret_vector = airTx.getSizeInfo(arg0);

    napi_value ret;
    auto status = napi_create_array_with_length(env, ret_vector.size(), &ret);
    (void)status;

    unsigned i = 0;
    for(auto w : ret_vector) {
        status = napi_set_element(env, ret, i, Napi::Number::New(env,w));
        i++;
    }

    return ret;

}

// napi_value fullAge(const Napi::CallbackInfo& info) {

//   Napi::Env env = info.Env();

//   cout << "fullAge()" << endl;

//   if (info.Length() != 0) {
//     Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
//   }

//   uint32_t fill;
//   unsigned long age;
//   std::tie(fill, age) = soapy->flushHiggs->fillAge();

//   napi_value ret;
//   auto status = napi_create_array_with_length(env, 2, &ret);

//   napi_value a,b;

//   status = napi_create_int64(env, age,  &a);
//   status = napi_create_int64(env, fill, &b);

//   status = napi_set_element(env, ret, 0, a);
//   status = napi_set_element(env, ret, 1, b);

//   return ret;
// }

// napi_value getDataHistory(const Napi::CallbackInfo& info) {

//   Napi::Env env = info.Env();

//   cout << "getDataHistory()" << endl;

//   if (info.Length() != 0) {
//     Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
//   }

//   std::vector<unsigned char> data = soapy->flushHiggs->getDataHistory();

//   // std::vector<unsigned char> data = {0,1,2,3,0xff,0xff,0xff,0xff,0,0,0,0,0xdd,0xdd,0xdd,0xdd};

//   char* copy_result;
//   napi_value ret;
//   auto status = napi_create_buffer_copy(env, data.size(), data.data(), (void**)&copy_result, &ret);

//   return ret;
// }

void init(Napi::Env env, Napi::Object& exports) {

  napi_status status;
  (void)status;

  Object subtree = Object::New(env);

  _NAPI_GENERIC_INIT(subtree, set_code_type);
  _NAPI_GENERIC_INIT(subtree, set_code_puncturing);
  _NAPI_GENERIC_INIT(subtree, set_modulation_schema);
  _NAPI_GENERIC_INIT(subtree, set_subcarrier_allocation);
  _NAPI_GENERIC_INIT(subtree, getEffectiveSlicedWordCount);
  _NAPI_GENERIC_INIT(subtree, getForceDetectIndex);
  _NAPI_GENERIC_INIT(subtree, getMapMovVectorInfo);
  _NAPI_GENERIC_INIT(subtree ,getSizeInfo);
  _NAPI_GENERIC_INIT(subtree, get_interleaver);
  _NAPI_GENERIC_INIT(subtree, get_rs);
  _NAPI_GENERIC_INIT(subtree, getLengthForInput);


  exports.Set("airTx", subtree);
}


}
}
}