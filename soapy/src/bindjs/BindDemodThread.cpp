
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"
#include "AirPacket.hpp"
#include "AirMacBitrate.hpp"


#include "DemodThread.hpp"

using namespace std;

namespace siglabs {
namespace bind {
namespace demodThread {


// _NAPI_MEMBER(soapy->flushHiggs->,drain_higgs,bool);
// _NAPI_MEMBER(soapy->demodThread->,print,bool);
_NAPI_MEMBER(soapy->demodThread->,times_sync_word_detected,size_t);
_NAPI_MEMBER(soapy->demodThread->,pending_dump,unsigned);
_NAPI_MEMBER(soapy->demodThread->,incoming_data_type,unsigned);
_NAPI_MEMBER(soapy->demodThread->,dump_name,string);
_NAPI_MEMBER(soapy->demodThread->,retransmit_to_peer_id,size_t);
_NAPI_MEMBER(soapy->demodThread->,dropSlicedData,bool);


_NAPI_MEMBER(soapy->demodThread->macBitrate->,mac_print1,bool);
_NAPI_MEMBER(soapy->demodThread->macBitrate->,mac_print2,bool);
_NAPI_BOUND(soapy->demodThread->macBitrate->,resetBer,void,void);
_NAPI_BOUND(soapy->demodThread->macBitrate->,getBer,double,void);



#define BIND_PATH soapy->demodThread->airRx->
#define BIND_AIR_INSTANCE_INCLUDE_GLOBAL
#include "BindAirInstanced.hpp"


void init(Napi::Env env, Napi::Object& exports) {

  napi_status status;
  (void)status;

  Object subtree = Object::New(env);

  // _NAPI_MEMBER_INIT(subtree, print);
  _NAPI_MEMBER_INIT(subtree, times_sync_word_detected);
  _NAPI_MEMBER_INIT(subtree, pending_dump);
  _NAPI_MEMBER_INIT(subtree, incoming_data_type);
  _NAPI_MEMBER_INIT(subtree, dump_name);
  _NAPI_MEMBER_INIT(subtree, retransmit_to_peer_id);
  _NAPI_MEMBER_INIT(subtree, dropSlicedData);

  _NAPI_MEMBER_INIT(subtree, mac_print1);
  _NAPI_MEMBER_INIT(subtree, mac_print2);
  _NAPI_GENERIC_INIT(subtree, resetBer);
  _NAPI_GENERIC_INIT(subtree, getBer);


  Object airtree = Object::New(env);

  #define SUBTREE_OBJECT airtree
  #define BIND_AIR_INSTANCE_INCLUDE_SCOPE
  #include "BindAirInstanced.hpp"

  status = napi_set_named_property(env, subtree, "air", airtree);


  exports.Set("demodThread", subtree);
}


}
}
}