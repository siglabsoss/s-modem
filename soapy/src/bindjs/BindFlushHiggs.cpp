
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "TxSchedule.hpp"
#include "ScheduleData.hpp"
#include "FlushHiggs.hpp"

using namespace std;

#include "BindFlushHiggs.hpp"
namespace siglabs {
namespace bind {
namespace flushHiggs {


_NAPI_BOUND (soapy->flushHiggs->,getNormalLen,uint32_t,void);
_NAPI_BOUND (soapy->flushHiggs->,getLowLen,uint32_t,void);
_NAPI_BOUND (soapy->flushHiggs->,dumpLow,void,void);
_NAPI_MEMBER(soapy->flushHiggs->,drain_higgs,bool);
_NAPI_MEMBER(soapy->flushHiggs->,allow_zmq_higgs,bool);
_NAPI_MEMBER(soapy->flushHiggs->,print,bool);
_NAPI_MEMBER(soapy->flushHiggs->,printSending,bool);

_NAPI_MEMBER(soapy->flushHiggs->,print_when_almost_full,bool);
_NAPI_MEMBER(soapy->flushHiggs->,keep_data_history,bool);
_NAPI_MEMBER(soapy->flushHiggs->,pad_zeros_before_low_pri,bool);
_NAPI_MEMBER(soapy->flushHiggs->,drain_rate,double);
_NAPI_MEMBER(soapy->flushHiggs->,max_packet_burst,int);
_NAPI_MEMBER(soapy->flushHiggs->,do_force_boost,bool);
_NAPI_MEMBER(soapy->flushHiggs->,print_low_words,bool);
_NAPI_MEMBER(soapy->flushHiggs->,print_evbuf_to_socket_burst,bool);
_NAPI_MEMBER(soapy->flushHiggs->,last_fill_level,uint32_t);

napi_value fullAge(const Napi::CallbackInfo& info) {

  Napi::Env env = info.Env();

  cout << "fullAge()" << endl;

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }

  uint32_t fill;
  unsigned long age;
  std::tie(fill, age) = soapy->flushHiggs->fillAge();

  napi_value ret;
  auto status = napi_create_array_with_length(env, 2, &ret);
  (void)status;

  napi_value a,b;

  status = napi_create_int64(env, age,  &a);
  status = napi_create_int64(env, fill, &b);

  status = napi_set_element(env, ret, 0, a);
  status = napi_set_element(env, ret, 1, b);

  return ret;
}

napi_value getDataHistory(const Napi::CallbackInfo& info) {

  Napi::Env env = info.Env();

  cout << "getDataHistory()" << endl;

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }

  std::vector<unsigned char> data = soapy->flushHiggs->getDataHistory();

  // std::vector<unsigned char> data = {0,1,2,3,0xff,0xff,0xff,0xff,0,0,0,0,0xdd,0xdd,0xdd,0xdd};

  char* copy_result;
  napi_value ret;
  auto status = napi_create_buffer_copy(env, data.size(), data.data(), (void**)&copy_result, &ret);
  (void)status;

  return ret;
}



void init(Napi::Env env, Napi::Object& exports) {

  napi_status status;
  (void)status;

  Object flush = Object::New(env);

  _NAPI_MEMBER_INIT(flush, allow_zmq_higgs);
  _NAPI_MEMBER_INIT(flush, drain_higgs);
  _NAPI_MEMBER_INIT(flush, print);
  _NAPI_MEMBER_INIT(flush, printSending);
  _NAPI_MEMBER_INIT(flush, print_when_almost_full);
  _NAPI_MEMBER_INIT(flush, keep_data_history);
  _NAPI_MEMBER_INIT(flush, pad_zeros_before_low_pri);
  _NAPI_MEMBER_INIT(flush, drain_rate);
  _NAPI_MEMBER_INIT(flush, max_packet_burst);
  _NAPI_MEMBER_INIT(flush, do_force_boost);
  _NAPI_MEMBER_INIT(flush, print_low_words);
  _NAPI_MEMBER_INIT(flush, print_evbuf_to_socket_burst);
  _NAPI_MEMBER_INIT(flush, last_fill_level);



  
  status = napi_set_named_property(env, flush, "getNormalLen", Napi::Function::New(env, getNormalLen__wrapped));
  status = napi_set_named_property(env, flush, "getLowLen", Napi::Function::New(env, getLowLen__wrapped));
  status = napi_set_named_property(env, flush, "dumpLow", Napi::Function::New(env, dumpLow__wrapped));
  
  status = napi_set_named_property(env, flush, "fullAge", Napi::Function::New(env, fullAge));
  status = napi_set_named_property(env, flush, "getDataHistory", Napi::Function::New(env, getDataHistory));

  exports.Set("flushHiggs", flush);
}


}
}
}