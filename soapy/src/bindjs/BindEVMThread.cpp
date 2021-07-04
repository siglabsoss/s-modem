
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "DemodThread.hpp"
#include "EVMThread.hpp"

using namespace std;

namespace siglabs {
namespace bind {
namespace evmThread {


// _NAPI_MEMBER(soapy->flushHiggs->,drain_higgs,bool);
_NAPI_MEMBER(soapy->evmThread->,evm,double);
_NAPI_MEMBER(soapy->evmThread->,print_dump,bool);
// _NAPI_MEMBER(soapy->evmThread->,times_sync_word_detected,size_t);
// _NAPI_MEMBER(soapy->evmThread->,pending_dump,unsigned);
// _NAPI_MEMBER(soapy->evmThread->,dump_name,string);
// _NAPI_MEMBER(soapy->evmThread->,retransmit_to_peer_id,size_t);




napi_value getSourceData__wrapped(const Napi::CallbackInfo& info) {

  Napi::Env env = info.Env();

  // cout << "getRxData()\n";

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }

  bool valid;
  std::vector<uint32_t> data;

  std::tie(valid, data) = soapy->evmThread->getDataToJs();

  if( !valid ) {
      return NULL;
  }

  napi_value ret;
  auto status = napi_create_array_with_length(env, data.size(), &ret);
  (void)status;

  unsigned i = 0;
  for(const auto w : data) {
    status = napi_set_element(env, ret, i, Napi::Number::New(env,w));
    i++;
  }
  return ret;


  // std::vector<uint32_t> data;
  // uint32_t x;
  // air_header_t header;

  // std::tie(data, x, header) = packet;

  // uint32_t header_length;
  // uint8_t  header_seq;
  // uint8_t  header_flags;

  // std::tie(header_length, header_seq, header_flags) = header;


  // for(auto x : data) {
  //   cout << HEX32_STRING(x) << '\n';
  // }

  // napi_value ret;
  // auto status = napi_create_array_with_length(env, 5, &ret);
  // (void)status;

  // napi_value words;
  
  // char* copy_result;
  // status = napi_create_buffer_copy(env, data.size()*4, data.data(), (void**)&copy_result, &words);
  
  // status = napi_set_named_property(env, ret, "0", words);

  // napi_value got_at = Napi::Number::New(env, x);

  // status = napi_set_named_property(env, ret, "1", got_at);


  // napi_value n_header_length = Napi::Number::New(env, header_length);
  // napi_value n_header_seq    = Napi::Number::New(env, header_seq);
  // napi_value n_header_flags  = Napi::Number::New(env, header_flags);

  // status = napi_set_named_property(env, ret, "2", n_header_length);
  // status = napi_set_named_property(env, ret, "3", n_header_seq);
  // status = napi_set_named_property(env, ret, "4", n_header_flags);


  // return ret;
}



void init(Napi::Env env, Napi::Object& exports) {

  napi_status status;
  (void)status;

  Object subtree = Object::New(env);

  _NAPI_MEMBER_INIT(subtree, evm);
  _NAPI_MEMBER_INIT(subtree, print_dump);

  _NAPI_GENERIC_INIT(subtree, getSourceData);
  // _NAPI_MEMBER_INIT(subtree, times_sync_word_detected);
  // _NAPI_MEMBER_INIT(subtree, pending_dump);
  // _NAPI_MEMBER_INIT(subtree, dump_name);
  // _NAPI_MEMBER_INIT(subtree, retransmit_to_peer_id);

  exports.Set("evmThread", subtree);
}


}
}
}