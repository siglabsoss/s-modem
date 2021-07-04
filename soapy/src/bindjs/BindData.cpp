#include "BindData.hpp"

#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "EventDsp.hpp"
#include "FlushHiggs.hpp"
// #include "RadioEstimate.hpp"
#include "DemodThread.hpp"
// #include "DispatchMockRpc.hpp"


using namespace std;
// using namespace SoapySDR;

namespace siglabs {
namespace bind {
namespace data {


napi_value getRxData(const Napi::CallbackInfo& info) {

  Napi::Env env = info.Env();

  // cout << "getRxData()\n";

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }

  bool valid;
  got_data_t packet;

  std::tie(valid, packet) = soapy->demodThread->getDataToJs();

  if( !valid ) {
      return NULL;
  }

  std::vector<uint32_t> data;
  uint32_t x;
  air_header_t header;

  std::tie(data, x, header) = packet;

  uint32_t header_length;
  uint8_t  header_seq;
  uint8_t  header_flags;

  std::tie(header_length, header_seq, header_flags) = header;


  // for(auto x : data) {
  //   cout << HEX32_STRING(x) << '\n';
  // }

  napi_value ret;
  auto status = napi_create_array_with_length(env, 5, &ret);
  (void)status;

  napi_value words;
  
  char* copy_result;
  status = napi_create_buffer_copy(env, data.size()*4, data.data(), (void**)&copy_result, &words);
  
  status = napi_set_named_property(env, ret, "0", words);

  napi_value got_at = Napi::Number::New(env, x);

  status = napi_set_named_property(env, ret, "1", got_at);


  napi_value n_header_length = Napi::Number::New(env, header_length);
  napi_value n_header_seq    = Napi::Number::New(env, header_seq);
  napi_value n_header_flags  = Napi::Number::New(env, header_flags);

  status = napi_set_named_property(env, ret, "2", n_header_length);
  status = napi_set_named_property(env, ret, "3", n_header_seq);
  status = napi_set_named_property(env, ret, "4", n_header_flags);


  return ret;
}

napi_value getRxData2(const Napi::CallbackInfo& info) {

  Napi::Env env = info.Env();

  // cout << "getRxData()\n";

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }

  std::vector<uint32_t> data;
  data.push_back(0xffffffff);
  data.push_back(1);
  data.push_back(2);
  data.push_back(3);

  for(auto x : data) {
    cout << HEX32_STRING(x) << '\n';
  }

  napi_value ret;
  auto status = napi_create_array_with_length(env, 2, &ret);
  (void)status;

  napi_value words;
  
  char* copy_result;
  status = napi_create_buffer_copy(env, data.size()*4, data.data(), (void**)&copy_result, &words);
  
  status = napi_set_named_property(env, ret, "0", words);

  uint32_t x = 0xdead0;

  napi_value got_at = Napi::Number::New(env, x);

  status = napi_set_named_property(env, ret, "1", got_at);



  return ret;


  // std::vector<unsigned char> data = soapy->flushHiggs->getDataHistory();

  // std::vector<unsigned char> data = {0,1,2,3,0xff,0xff,0xff,0xff,0,0,0,0,0xdd,0xdd,0xdd,0xdd};

  // char* copy_result;


  // return ret;
}


void queueTxData(const Napi::CallbackInfo& info) {
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

    soapy->txTun2EventDsp.enqueue(arr);
}

void sendZerosToHiggs(uint32_t count) {

    if( count == 0) {
        return;
    }

    std::vector<uint32_t> zrs;
    zrs.resize(count, 0);

    soapy->sendToHiggs(zrs);
}
_NAPI_BODY(sendZerosToHiggs,void,uint32_t);

void sendToHiggs(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    cout << "sendToHiggs()" << endl;

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

    cout << "Array len " << len << '\n';

    napi_value val;
    for(uint32_t i = 0; i < len; i++) {

        status = napi_get_element(env, info[0], i, &val);

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


    soapy->sendToHiggs(arr);
}





void init(Napi::Env env, Napi::Object& exports) {

  // napi_status status;
  // Object subtree = Object::New(env);
  // exports.Set("data", subtree);


  exports.Set("sendZerosToHiggs", Napi::Function::New(env, sendZerosToHiggs__wrapped));
  exports.Set("sendToHiggs", Napi::Function::New(env, sendToHiggs));
  exports.Set("getRxData", Napi::Function::New(env, getRxData));
  exports.Set("queueTxData", Napi::Function::New(env, queueTxData));

}






}
}
}