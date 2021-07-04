
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "TxSchedule.hpp"
#include "AirPacket.hpp"

using namespace std;

#include "BindTxSchedule.hpp"
namespace siglabs {
namespace bind {
namespace txSchedule {

namespace air {






#define BIND_AIR_AS_TX
#define BIND_PATH soapy->txSchedule->airTx->
#define BIND_AIR_INSTANCE_INCLUDE_GLOBAL
#include "BindAirInstanced.hpp"



void initAir(Napi::Env env, Napi::Object& airtree) {
  napi_status status;
  (void)status;


#define BIND_AIR_AS_TX
#define SUBTREE_OBJECT airtree
#define BIND_AIR_INSTANCE_INCLUDE_SCOPE
#include "BindAirInstanced.hpp"
}



}




_NAPI_BOUND(soapy->txSchedule->,debugFillBuffers, void, unsigned );
_NAPI_BOUND(soapy->txSchedule->,debugFillZmq, void, void);
_NAPI_BOUND(soapy->txSchedule->,tunFirstLength, unsigned, void);
_NAPI_BOUND(soapy->txSchedule->,dumpUserData, void, void);
_NAPI_BOUND(soapy->txSchedule->,tunPacketsReady, unsigned, void);
_NAPI_BOUND(soapy->txSchedule->,stopThreadDerivedClass, void, void);
_NAPI_BOUND(soapy->txSchedule->,setupFsm, void, void);
_NAPI_BOUND(soapy->txSchedule->,tickFsmSchedule, void, void);
_NAPI_BOUND(soapy->txSchedule->,setupPointers, void, void);
_NAPI_BOUND(soapy->txSchedule->,tickleAttachedProgress, void, void);
_NAPI_BOUND(soapy->txSchedule->,setLinkUp, void, bool);
_NAPI_BOUND(soapy->txSchedule->,debugFillBeamform, void, uint32_t, uint32_t, int);
_NAPI_BOUND(soapy->txSchedule->,debug, void, unsigned);
_NAPI_BOUND(soapy->txSchedule->,setupDuplexAsRole, void, string);
_NAPI_BOUND(soapy->txSchedule->,estimateBusy, uint32_t, uint32_t, uint32_t);

_NAPI_MEMBER(soapy->txSchedule->,print,bool);
_NAPI_MEMBER(soapy->txSchedule->,dsp_state,size_t);
_NAPI_MEMBER(soapy->txSchedule->,dsp_state_pending,size_t);
_NAPI_MEMBER(soapy->txSchedule->,_do_wait,size_t);
_NAPI_MEMBER(soapy->txSchedule->,role,string);
_NAPI_MEMBER(soapy->txSchedule->,doWarmup,bool);
_NAPI_MEMBER(soapy->txSchedule->,warmupDone,bool);
_NAPI_MEMBER(soapy->txSchedule->,requestPause,bool);
_NAPI_MEMBER(soapy->txSchedule->,exitDataMode,bool);
_NAPI_MEMBER(soapy->txSchedule->,debugBeamform,bool);
_NAPI_MEMBER(soapy->txSchedule->,errorDetected,size_t);
_NAPI_MEMBER(soapy->txSchedule->,flushZerosEnded,size_t);
_NAPI_MEMBER(soapy->txSchedule->,flushZerosStarted,size_t);
// _NAPI_MEMBER(soapy->txSchedule->,needs_pre_zero,bool);
_NAPI_MEMBER(soapy->txSchedule->,errorRecoveryAttempts,size_t);
_NAPI_MEMBER(soapy->txSchedule->,errorFlushZerosWaiting,size_t);
_NAPI_MEMBER(soapy->txSchedule->,debug_est_run,int);
_NAPI_MEMBER(soapy->txSchedule->,debug_est_run_target,int);
_NAPI_MEMBER(soapy->txSchedule->,warmup_epoc_est,int);
_NAPI_MEMBER(soapy->txSchedule->,warmup_epoc_est_target,int);
_NAPI_MEMBER(soapy->txSchedule->,debug_grow_size,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,periodic_schedule_last,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,fsm_us_track,uint64_t);
_NAPI_MEMBER(soapy->txSchedule->,debugBeamformSize,size_t);
_NAPI_MEMBER(soapy->txSchedule->,pending_valid,bool);
_NAPI_MEMBER(soapy->txSchedule->,pending_epoc,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,pending_ts,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,pending_seq,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,print1,bool);
_NAPI_MEMBER(soapy->txSchedule->,idle_print,int);
_NAPI_MEMBER(soapy->txSchedule->,debug_duplex,unsigned);
_NAPI_MEMBER(soapy->txSchedule->,debug_duplex_ahead,unsigned);
_NAPI_MEMBER(soapy->txSchedule->,debug_extra_gap,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,debug_duplex_size,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,debug_duplex_type,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,pending_lifetime,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,debug_counter_delta,int32_t);
_NAPI_MEMBER(soapy->txSchedule->,duplex_seq,uint32_t);
_NAPI_MEMBER(soapy->txSchedule->,duplex_debug_split,bool);
_NAPI_MEMBER(soapy->txSchedule->,duplex_skip_body,bool);






void init(Napi::Env env, Napi::Object& exports) {

  napi_status status;
  (void)status;
  Object subtree = Object::New(env);


  _NAPI_GENERIC_INIT(subtree, debugFillBuffers);
  _NAPI_GENERIC_INIT(subtree, debugFillZmq);
  _NAPI_GENERIC_INIT(subtree, tunFirstLength);
  _NAPI_GENERIC_INIT(subtree, dumpUserData);
  _NAPI_GENERIC_INIT(subtree, tunPacketsReady);
  _NAPI_GENERIC_INIT(subtree, stopThreadDerivedClass);
  _NAPI_GENERIC_INIT(subtree, setupFsm);
  _NAPI_GENERIC_INIT(subtree, tickFsmSchedule);
  _NAPI_GENERIC_INIT(subtree, setupPointers);
  _NAPI_GENERIC_INIT(subtree, tickleAttachedProgress);
  _NAPI_GENERIC_INIT(subtree, setLinkUp);
  _NAPI_GENERIC_INIT(subtree, debugFillBeamform);
  _NAPI_GENERIC_INIT(subtree, debug);
  _NAPI_GENERIC_INIT(subtree, setupDuplexAsRole);
  _NAPI_GENERIC_INIT(subtree, estimateBusy);


  _NAPI_MEMBER_INIT(subtree, print);
  _NAPI_MEMBER_INIT(subtree, dsp_state);
  _NAPI_MEMBER_INIT(subtree, dsp_state_pending);
  _NAPI_MEMBER_INIT(subtree, _do_wait);
  _NAPI_MEMBER_INIT(subtree, role);
  _NAPI_MEMBER_INIT(subtree, doWarmup);
  _NAPI_MEMBER_INIT(subtree, warmupDone);
  _NAPI_MEMBER_INIT(subtree, requestPause);
  _NAPI_MEMBER_INIT(subtree, exitDataMode);
  _NAPI_MEMBER_INIT(subtree, debugBeamform);
  _NAPI_MEMBER_INIT(subtree, errorDetected);
  _NAPI_MEMBER_INIT(subtree, flushZerosEnded);
  _NAPI_MEMBER_INIT(subtree, flushZerosStarted);
  // _NAPI_MEMBER_INIT(subtree, needs_pre_zero);
  _NAPI_MEMBER_INIT(subtree, errorRecoveryAttempts);
  _NAPI_MEMBER_INIT(subtree, errorFlushZerosWaiting);
  _NAPI_MEMBER_INIT(subtree, debug_est_run);
  _NAPI_MEMBER_INIT(subtree, debug_est_run_target);
  _NAPI_MEMBER_INIT(subtree, warmup_epoc_est);
  _NAPI_MEMBER_INIT(subtree, warmup_epoc_est_target);
  _NAPI_MEMBER_INIT(subtree, debug_grow_size);
  _NAPI_MEMBER_INIT(subtree, periodic_schedule_last);
  _NAPI_MEMBER_INIT(subtree, fsm_us_track);
  _NAPI_MEMBER_INIT(subtree, debugBeamformSize);
  _NAPI_MEMBER_INIT(subtree, pending_valid);
  _NAPI_MEMBER_INIT(subtree, pending_epoc);
  _NAPI_MEMBER_INIT(subtree, pending_ts);
  _NAPI_MEMBER_INIT(subtree, pending_seq);
  _NAPI_MEMBER_INIT(subtree, print1);
  _NAPI_MEMBER_INIT(subtree, idle_print);
  _NAPI_MEMBER_INIT(subtree, debug_duplex);
  _NAPI_MEMBER_INIT(subtree, debug_duplex_ahead);
  _NAPI_MEMBER_INIT(subtree, debug_extra_gap);
  _NAPI_MEMBER_INIT(subtree, debug_duplex_size);
  _NAPI_MEMBER_INIT(subtree, debug_duplex_type);
  _NAPI_MEMBER_INIT(subtree, pending_lifetime);
  _NAPI_MEMBER_INIT(subtree, debug_counter_delta);
  _NAPI_MEMBER_INIT(subtree, duplex_seq);
  _NAPI_MEMBER_INIT(subtree, duplex_debug_split);
  _NAPI_MEMBER_INIT(subtree, duplex_skip_body);
  


  Object airtree = Object::New(env);
  air::initAir(env, airtree);
  status = napi_set_named_property(env, subtree, "air", airtree);




  exports.Set("txSchedule", subtree);
}





}
}
}