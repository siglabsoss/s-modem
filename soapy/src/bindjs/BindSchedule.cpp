
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "TxSchedule.hpp"
#include "ScheduleData.hpp"

using namespace std;

#include "BindSchedule.hpp"
namespace siglabs {
namespace bind {
namespace schedule {
// using namespace SoapySDR;





void setTimeslots(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  // cout << "queueTxData()" << endl;

  if (info.Length() != 1) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
    return;
  }

  uint32_t len;
  auto status = napi_get_array_length(env,
                                info[0],
                                &len);
  (void)status;

  std::vector<uint32_t> arr;
  arr.reserve(len);

  // cout << "Array len " << len << '\n';

  napi_value val;
  for(uint32_t i = 0; i < len; i++) {

    status = napi_get_element(env, info[0], i, &val);

    Napi::Number val2(env, val);

    uint32_t val3 = val2.Uint32Value();

    arr.push_back(val3);

      // cout << HEX32_STRING(val3)  << endl;
  }

  soapy->txSchedule->scheduleData->setTimeslot(arr);

  // soapy->txTun2EventDsp.enqueue(arr);
}


_NAPI_BOUND(soapy->txSchedule->scheduleData->,setEarlyFrames,void,int);
_NAPI_BOUND(soapy->txSchedule->scheduleData->,setPacketSize,void,uint32_t);
_NAPI_BOUND(soapy->txSchedule->scheduleData->,setSecondSleep,void,double);


void init(Napi::Env env, Napi::Object& exports) {

  napi_status status;
  (void)status;

  Object parent(env, exports.Get("txSchedule"));



  Object subtree = Object::New(env);

  // cout << "object:::::::::::: " << exports.Get("txSchedule").IsObject() << "\n";


  _NAPI_GENERIC_INIT(subtree, setEarlyFrames);
  _NAPI_GENERIC_INIT(subtree, setPacketSize);
  _NAPI_GENERIC_INIT(subtree, setSecondSleep);
  status = napi_set_named_property(env, subtree, "setTimeslots", Napi::Function::New(env, setTimeslots));



  
  // status = napi_set_named_property(env, subtree, "setTimeslots", Napi::Function::New(env, setTimeslots));

  // parent.Set("schedule", subtree);

  napi_set_named_property(env, parent, "scheduleData", subtree);

}


}
}
}