
#include "BindCommonInclude.hpp"
#include "BindHiggsInclude.hpp"

#include "TxSchedule.hpp"
#include "EventDsp.hpp"
#include "RadioEstimate.hpp"
#include "RadioDemodTDMA.hpp"
#include "AttachedEstimate.hpp"

using namespace std;

#include "BindEventDsp.hpp"
namespace siglabs {
namespace bind {
namespace re {


#define ATTACHED_INDEX (99)



template <unsigned I>
RadioEstimate* getBound() {
    if( I == ATTACHED_INDEX ) {
        return soapy->dspEvents->attached;
    } else {
        return soapy->dspEvents->radios[I];
    }
}


template <unsigned I>
napi_value getAllScSaved(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }


  RadioEstimate* const re = getBound<I>(); // pull correct object for template

  // copy of class, this is done without any mutex as I am lazy
  const std::vector<uint32_t> our_copy = re->all_sc_saved;

  napi_value ret;
  auto status = napi_create_array_with_length(env, our_copy.size(), &ret);
  (void)status;

  napi_value words;
  
  char* copy_result;
  status = napi_create_buffer_copy(env, our_copy.size()*4, our_copy.data(), 0, &words);
  (void)copy_result;
  
  return words;
}

template <unsigned I>
napi_value getChannelAngle(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }


  RadioEstimate* const re = getBound<I>(); // pull correct object for template

  // copy of vector, this function handles mutex correctly
  const std::vector<double> our_copy = re->getChannelAngle();

  napi_value ret;
  auto status = napi_create_array_with_length(env, our_copy.size(), &ret);
  (void)status;

  unsigned i = 0;
  for(auto w : our_copy) {
    status = napi_set_element(env, ret, i, Napi::Number::New(env,w));
    i++;
  }
  return ret;
}

template <unsigned I>
napi_value getChannelAngleSent(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }


  RadioEstimate* const re = getBound<I>(); // pull correct object for template

  // copy of vector, this function handles mutex correctly
  const std::vector<double> our_copy = re->getChannelAngleSent();

  napi_value ret;
  auto status = napi_create_array_with_length(env, our_copy.size(), &ret);
  (void)status;

  unsigned i = 0;
  for(auto w : our_copy) {
    status = napi_set_element(env, ret, i, Napi::Number::New(env,w));
    i++;
  }
  return ret;
}

template <unsigned I>
napi_value getChannelAngleSentTemp(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }


  

  RadioEstimate* const re = getBound<I>(); // pull correct object for template

  // copy of vector, this function handles mutex correctly
  const std::vector<double> our_copy = re->getChannelAngleSentTemp();

  napi_value ret;
  auto status = napi_create_array_with_length(env, our_copy.size(), &ret);
  (void)status;

  unsigned i = 0;
  for(auto w : our_copy) {
    status = napi_set_element(env, ret, i, Napi::Number::New(env,w));
    i++;
  }
  return ret;
}

template <unsigned I>
napi_value getEqualizerCoeffSent(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() != 0) {
    Napi::TypeError::New(env, "Invalid argument count").ThrowAsJavaScriptException();
  }


  

  RadioEstimate* const re = getBound<I>(); // pull correct object for template

  // copy of vector, this function handles mutex correctly
  const std::vector<uint32_t> our_copy = re->getEqualizerCoeffSent();

  napi_value ret;
  auto status = napi_create_array_with_length(env, our_copy.size(), &ret);
  (void)status;

  unsigned i = 0;
  for(auto w : our_copy) {
    status = napi_set_element(env, ret, i, Napi::Number::New(env,w));
    i++;
  }
  return ret;
}


template <unsigned I>
void setAllEqMask(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();
    // cout << "zmqPublishInternal()" << endl;

    bool call_notok = info.Length() != 1 || !info[0].IsArray();

    if(call_notok) {
        Napi::TypeError::New(env, "setAllEqMask() expects arguments of (array)").ThrowAsJavaScriptException();
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

    RadioEstimate* const re = getBound<I>(); // pull correct object for template

    re->setAllEqMask(arr);

    // soapy->sendToHiggs(arr);
    // soapy->zmqPublishInternal(arg0, arr);
}
// void HiggsDriver::zmqPublishInternal(const std::string& header, const std::vector<uint32_t>& vec) {



template <unsigned I>
napi_value predictScheduleFrame(const Napi::CallbackInfo& info) {
    int error;
    
    RadioEstimate* const re = getBound<I>(); // pull correct object for template

    const epoc_frame_t est = re->predictScheduleFrame(error, false, false);

    Napi::Env env = info.Env();
    napi_value ret;
    auto status = napi_create_array_with_length(env, 2, &ret);
    (void)status;



    napi_value a,b;

    status = napi_create_uint32(env, est.epoc,  &a);
    status = napi_create_uint32(env, est.frame, &b);

    status = napi_set_element(env, ret, 0, a);
    status = napi_set_element(env, ret, 1, b);

    return ret;
}

template <unsigned I>
napi_value predictLifetime32(const Napi::CallbackInfo& info) {
    int error;
    
    RadioEstimate* const re = getBound<I>(); // pull correct object for template

    const uint32_t lifetime32 = re->predictLifetime32(error, true);

    Napi::Env env = info.Env();
    // napi_value ret;
    // auto status = napi_create_array_with_length(env, 2, &ret);

    if(0) {
        cout << "l32: " << lifetime32 << "\n";
    }



    napi_value a;

    auto status = napi_create_uint32(env, lifetime32,  &a);
    // status = napi_create_uint32(env, lifetime32.frame, &b);

    // status = napi_set_element(env, ret, 0, a);
    // status = napi_set_element(env, ret, 1, b);

    (void)status;
    return a;
}






#define SCIDX (DATA_SUBCARRIER_INDEX)

#define TDMA_BODY(rpath, ridx) \
namespace r##ridx##tdma { \
_NAPI_BOUND(rpath demod->td->,reset,void,void); \
\
_NAPI_MEMBER(rpath demod->td->,found_dead,bool); \
_NAPI_MEMBER(rpath demod->td->,sent_tdma,bool); \
_NAPI_MEMBER(rpath demod->td->,lifetime_tx,uint32_t); \
_NAPI_MEMBER(rpath demod->td->,lifetime_rx,uint32_t); \
_NAPI_MEMBER(rpath demod->td->,fudge_rx,uint32_t); \
_NAPI_MEMBER(rpath demod->td->,needs_fudge,bool); \
}





// _NAPI_MEMBER(soapy->dspEvents->radios[ridx]->,demod_est_common_phase, int32_t); \                      x
// _NAPI_MEMBER(soapy->dspEvents->radios[ridx]->,demod_special_phase, int32_t); \                      x


#define RADIO_BODY(rpath, ridx) \
\
TDMA_BODY(rpath, ridx); \
\
namespace r##ridx { \
_NAPI_BOUND(rpath,getPeerId,uint32_t,void); \
_NAPI_BOUND(rpath,getArrayIndex,uint32_t,void); \
_NAPI_BOUND(rpath,sfoState,size_t,void); \
_NAPI_BOUND(rpath,stoState,size_t,void); \
_NAPI_BOUND(rpath,cfoState,size_t,void); \
_NAPI_BOUND(rpath,dspRunStoEq,void,double); \
_NAPI_BOUND(rpath,updatePartnerEq,void,bool,bool); \
_NAPI_BOUND(rpath,startBackground,void,void); \
_NAPI_BOUND(rpath,stopBackground,void,void); \
_NAPI_BOUND(rpath,updatePartnerCfo,void,void); \
_NAPI_BOUND(rpath,updatePartnerSfo,void,void); \
_NAPI_BOUND(rpath,initialMask,void,void); \
_NAPI_BOUND(rpath,setRandomRotationEq,void,void); \
_NAPI_BOUND(rpath,applyEqToSto,void,void); \
_NAPI_BOUND(rpath,calculateEqToSto,int,bool); \
_NAPI_BOUND(rpath,fractionalStoToEq,double,int); \
_NAPI_BOUND(rpath,resetCoarseState,void,void); \
_NAPI_BOUND(rpath,resetTdmaState,void,void); \
_NAPI_BOUND(rpath,partnerOp,void,string,uint32_t,uint32_t); \
_NAPI_BOUND(rpath,setPartnerTDMA,void,uint32_t,uint32_t); \
_NAPI_BOUND(rpath,freezeChannelAngleForStoEq,void,void); \
_NAPI_BOUND(rpath,sendEqOne,void,void); \
_NAPI_BOUND(rpath,printEqOne,void,void); \
_NAPI_BOUND(rpath,channel_angle_sent_temp_into_channel_angle_sent,void,void); \
\
_NAPI_MEMBER(rpath,peer_id, size_t); \
_NAPI_MEMBER(rpath,array_index, size_t); \
_NAPI_MEMBER(rpath,cfo_update_counter, uint32_t); \
_NAPI_MEMBER(rpath,cfo_estimated_sent, double); \
_NAPI_MEMBER(rpath,sfo_estimated_sent, double); \
_NAPI_MEMBER(rpath,coarse_ok, bool); \
_NAPI_MEMBER(rpath,times_cfo_sent, size_t); \
_NAPI_MEMBER(rpath,times_sfo_sent, size_t); \
_NAPI_MEMBER(rpath,times_sto_sent, size_t); \
_NAPI_MEMBER(rpath,times_residue_phase_sent, size_t); \
_NAPI_MEMBER(rpath,times_eq_sent, size_t); \
_NAPI_MEMBER(rpath,times_coarse_estimated, size_t); \
_NAPI_MEMBER(rpath,applied_sfo, bool); \
_NAPI_MEMBER(rpath,sfo_estimated, double); \
_NAPI_MEMBER(rpath,cfo_estimated, double); \
_NAPI_MEMBER(rpath,coarse_state, int); \
_NAPI_MEMBER(rpath,saved_times_coarse_estimated, size_t); \
_NAPI_MEMBER(rpath,coarse_delta, double); \
_NAPI_MEMBER(rpath,trigger_coarse, bool); \
_NAPI_MEMBER(rpath,coarse_finished, bool); \
_NAPI_MEMBER(rpath,delay_cfo, uint32_t); \
_NAPI_MEMBER(rpath,print_first_residue_phase_sent, bool); \
_NAPI_MEMBER(rpath,delay_residue_phase, uint32_t); \
_NAPI_MEMBER(rpath,prev_coarse_ok, bool); \
_NAPI_MEMBER(rpath,should_run_background, bool); \
_NAPI_MEMBER(rpath,should_mask_data_tone_tx_eq, bool); \
_NAPI_MEMBER(rpath,should_mask_all_data_tone, bool); \
_NAPI_MEMBER(rpath,should_print_estimates, bool); \
_NAPI_MEMBER(rpath,old_demod_print_counter_limit, int); \
_NAPI_MEMBER(rpath,old_demod_print_counter, int); \
_NAPI_MEMBER(rpath,sfo_tracking_0, double); \
_NAPI_MEMBER(rpath,sfo_tracking_1, double); \
_NAPI_MEMBER(rpath,_eq_use_filter, bool); \
_NAPI_MEMBER(rpath,_print_on_new_message, bool); \
_NAPI_MEMBER(rpath,times_sfo_estimated, size_t); \
_NAPI_MEMBER(rpath,times_sto_estimated, size_t); \
_NAPI_MEMBER(rpath,times_cfo_estimated, size_t); \
_NAPI_MEMBER(rpath,times_sto_estimated_p, size_t); \
_NAPI_MEMBER(rpath,times_sfo_estimated_p, size_t); \
_NAPI_MEMBER(rpath,times_cfo_estimated_p, size_t); \
_NAPI_MEMBER(rpath,times_dsp_run_channel_est, size_t); \
_NAPI_MEMBER(rpath,margin, int); \
_NAPI_MEMBER(rpath,_prev_sto_est, size_t); \
_NAPI_MEMBER(rpath,_prev_sfo_est, size_t); \
_NAPI_MEMBER(rpath,_prev_cfo_est, size_t); \
_NAPI_MEMBER(rpath,times_wait_tdma_state_0, size_t); \
_NAPI_MEMBER(rpath,residue_phase_trend, double); \
_NAPI_MEMBER(rpath,sto_estimated, double); \
_NAPI_MEMBER(rpath,residue_phase_estimated, double); \
_NAPI_MEMBER(rpath,residue_phase_estimated_pre, double); \
_NAPI_MEMBER(rpath,eq_filter_index, size_t); \
_NAPI_MEMBER(rpath,cheq_done_flag, bool); \
_NAPI_MEMBER(rpath,cheq_onoff_flag, bool); \
_NAPI_MEMBER(rpath,reset_flag, bool); \
_NAPI_MEMBER(rpath,residue_phase_flag, bool); \
_NAPI_MEMBER(rpath,feedback_alive_count, uint32_t); \
_NAPI_MEMBER(rpath,radio_state, size_t); \
_NAPI_MEMBER(rpath,radio_state_pending, size_t); \
_NAPI_MEMBER(rpath,fsm_event_pending, size_t); \
_NAPI_MEMBER(rpath,_sfo_symbol_num, unsigned); \
_NAPI_MEMBER(rpath,_cfo_symbol_num, unsigned); \
_NAPI_MEMBER(rpath,_dashboard_eq_update_ratio, int); \
_NAPI_MEMBER(rpath,_dashboard_eq_update_counter, int); \
_NAPI_MEMBER(rpath,_dashboard_only_update_channel_angle_r0, bool); \
_NAPI_MEMBER(rpath,epoc_valid, bool); \
_NAPI_MEMBER(rpath,epoc_recent_reply_frames_valid, bool); \
_NAPI_MEMBER(rpath,epoc_recent_reply_frames, int32_t); \
_NAPI_MEMBER(rpath,cheq_background_counter, uint32_t); \
_NAPI_MEMBER(rpath,should_setup_fsm, bool); \
_NAPI_MEMBER(rpath,map_mov_packets_sent, unsigned); \
_NAPI_MEMBER(rpath,map_mov_acks_received, unsigned); \
_NAPI_MEMBER(rpath,eq_hash_state, uint32_t); \
_NAPI_MEMBER(rpath,eq_hash_word, uint32_t); \
_NAPI_MEMBER(rpath,epoc_calibration, int32_t); \
_NAPI_MEMBER(rpath,save_next_all_sc, bool); \
_NAPI_MEMBER(rpath,enable_residue_to_cfo, bool); \
_NAPI_MEMBER(rpath,residue_to_cfo_delay, uint64_t); \
_NAPI_MEMBER(rpath,residue_to_cfo_factor, double); \
_NAPI_MEMBER(rpath,pause_residue, bool); \
_NAPI_MEMBER(rpath,sto_delta, double); \
_NAPI_MEMBER(rpath,drop_residue_target, unsigned); \
_NAPI_MEMBER(rpath,use_sto_eq_method, bool); \
_NAPI_MEMBER(rpath,pause_eq, bool); \
_NAPI_MEMBER(rpath,eq_to_sto_allowed, bool); \
_NAPI_MEMBER(rpath,use_all_pilot_eq_method, bool); \
_NAPI_MEMBER(rpath,should_unmask_all_eq, bool); \
_NAPI_MEMBER(rpath,eq_iir_gain, int32_t); \
_NAPI_MEMBER(rpath,adjust_sto_using_sfo, bool); \
_NAPI_MEMBER(rpath,update_sto_sfo_delay, unsigned); \
_NAPI_MEMBER(rpath,update_sto_sfo_counter, unsigned); \
_NAPI_MEMBER(rpath,update_sto_sfo_tol, double); \
_NAPI_MEMBER(rpath,update_sto_sfo_bump, double); \
_NAPI_MEMBER(rpath,adjust_fractional_sto, bool); \
_NAPI_MEMBER(rpath,sfo_sto_use_duplex, bool); \
_NAPI_MEMBER(rpath,applyeqonce, bool); \
\
_NAPI_MEMBER(rpath demod->,tdma_phase, int32_t); \
_NAPI_BOUND( rpath demod->,resetDemodPhase,void,void); \
_NAPI_BOUND( rpath demod->,resetTdmaAndLifetime,void,void); \
_NAPI_BOUND( rpath demod->,resetTdmaIndexDefault,void,void); \
_NAPI_BOUND( rpath demod->,getBuffSize,size_t,unsigned); \
_NAPI_MEMBER(rpath demod->,track_demod_against_rx_counter, bool); \
_NAPI_MEMBER(rpath demod->,first_dprint, int); \
_NAPI_MEMBER(rpath demod->,times_matched_tdma_6, int); \
_NAPI_MEMBER(rpath demod->,print_found_sync, int); \
_NAPI_MEMBER(rpath demod->,track_record_rx, uint32_t); \
_NAPI_MEMBER(rpath demod->,print_all_coarse_sync, bool); \
_NAPI_MEMBER(rpath demod->,_print_fine_schedule, bool); \
_NAPI_MEMBER(rpath demod->,should_print_demod_word, bool); \
_NAPI_MEMBER(rpath demod->,data_tone_num, unsigned); \
_NAPI_MEMBER(rpath demod->,data_subcarrier_index, unsigned); \
_NAPI_MEMBER(rpath demod->,print_count_after_beef, int); \
_NAPI_MEMBER(rpath demod->,print_sr_count, int); \
_NAPI_MEMBER(rpath demod->,subcarrier_chunk, unsigned); \
_NAPI_MEMBER(rpath demod->,demod_enabled, bool); \
}



RADIO_BODY(soapy->dspEvents->radios[0]->, 0);
RADIO_BODY(soapy->dspEvents->radios[1]->, 1);
RADIO_BODY(soapy->dspEvents->radios[2]->, 2);
RADIO_BODY(soapy->dspEvents->radios[3]->, 3);
RADIO_BODY(soapy->dspEvents->attached->, 99);


// below copies this:
// status = napi_set_named_property(env, subtree_r0, "getPeerId", Napi::Function::New(env, r0::getPeerId__wrapped));

#define BODY_TDMA_LINE(ridx,name) \
status = napi_set_named_property(env, subtree_r##ridx##tdma, __STR(name), Napi::Function::New(env, r##ridx##tdma :: name##__wrapped));

#define MEMB_TDMA_LINE(ridx,name) \
_NAPI_MEMBER_INIT_NS(subtree_r##ridx##tdma, r##ridx##tdma ::, name);



#define BODY_LINE(ridx,name) \
status = napi_set_named_property(env, subtree_r##ridx, __STR(name), Napi::Function::New(env, r##ridx :: name##__wrapped));

#define MEMB_LINE(ridx,name) \
_NAPI_MEMBER_INIT_NS(subtree_r##ridx, r##ridx ::, name);

#define DEMOD_MEMB(ridx,name) \
_NAPI_MEMBER_INIT_NS(demod_r##ridx, r##ridx ::, name);

#define DEMOD_BOUND(ridx,name) \
status = napi_set_named_property(env, demod_r##ridx, __STR(name), Napi::Function::New(env, r##ridx :: name##__wrapped));



// r##ridx##tdma
#define INIT_TDMA(ridx) \
Object subtree_r##ridx##tdma = Object::New(env); \
\
BODY_TDMA_LINE(ridx,reset); \
\
MEMB_TDMA_LINE(ridx,found_dead); \
MEMB_TDMA_LINE(ridx,sent_tdma); \
MEMB_TDMA_LINE(ridx,lifetime_tx); \
MEMB_TDMA_LINE(ridx,lifetime_rx); \
MEMB_TDMA_LINE(ridx,fudge_rx); \
MEMB_TDMA_LINE(ridx,needs_fudge); \
\
status = napi_set_named_property(env, subtree_r##ridx, "td", subtree_r##ridx##tdma);




// MEMB_LINE(ridx,demod_est_common_phase); \                x
// MEMB_LINE(ridx,demod_special_phase); \                x


#define INIT_BODY(ridx) \
Object subtree_r##ridx = Object::New(env); \
\
status = napi_set_named_property(env, subtree_r##ridx, "getAllScSaved", Napi::Function::New(env, getAllScSaved<ridx>)); \
status = napi_set_named_property(env, subtree_r##ridx, "getChannelAngle", Napi::Function::New(env, getChannelAngle<ridx>)); \
status = napi_set_named_property(env, subtree_r##ridx, "getChannelAngleSent", Napi::Function::New(env, getChannelAngleSent<ridx>)); \
status = napi_set_named_property(env, subtree_r##ridx, "getChannelAngleSentTemp", Napi::Function::New(env, getChannelAngleSentTemp<ridx>)); \
status = napi_set_named_property(env, subtree_r##ridx, "getEqualizerCoeffSent", Napi::Function::New(env, getEqualizerCoeffSent<ridx>)); \
status = napi_set_named_property(env, subtree_r##ridx, "setAllEqMask", Napi::Function::New(env, setAllEqMask<ridx>)); \
status = napi_set_named_property(env, subtree_r##ridx, "predictScheduleFrame", Napi::Function::New(env, predictScheduleFrame<ridx>)); \
status = napi_set_named_property(env, subtree_r##ridx, "predictLifetime32", Napi::Function::New(env, predictLifetime32<ridx>)); \
\
\
BODY_LINE(ridx,getPeerId); \
BODY_LINE(ridx,getArrayIndex); \
BODY_LINE(ridx,sfoState); \
BODY_LINE(ridx,stoState); \
BODY_LINE(ridx,cfoState); \
BODY_LINE(ridx,dspRunStoEq); \
BODY_LINE(ridx,updatePartnerEq); \
BODY_LINE(ridx,startBackground); \
BODY_LINE(ridx,stopBackground); \
BODY_LINE(ridx,updatePartnerCfo); \
BODY_LINE(ridx,updatePartnerSfo); \
BODY_LINE(ridx,initialMask); \
BODY_LINE(ridx,setRandomRotationEq); \
BODY_LINE(ridx,applyEqToSto); \
BODY_LINE(ridx,calculateEqToSto); \
BODY_LINE(ridx,fractionalStoToEq); \
BODY_LINE(ridx,resetCoarseState); \
BODY_LINE(ridx,resetTdmaState); \
BODY_LINE(ridx,partnerOp); \
BODY_LINE(ridx,setPartnerTDMA); \
BODY_LINE(ridx,freezeChannelAngleForStoEq); \
BODY_LINE(ridx,sendEqOne); \
BODY_LINE(ridx,printEqOne); \
BODY_LINE(ridx,channel_angle_sent_temp_into_channel_angle_sent); \
\
MEMB_LINE(ridx,peer_id); \
MEMB_LINE(ridx,array_index); \
MEMB_LINE(ridx,cfo_update_counter); \
MEMB_LINE(ridx,cfo_estimated_sent); \
MEMB_LINE(ridx,sfo_estimated_sent); \
MEMB_LINE(ridx,coarse_ok); \
MEMB_LINE(ridx,times_cfo_sent); \
MEMB_LINE(ridx,times_sfo_sent); \
MEMB_LINE(ridx,times_sto_sent); \
MEMB_LINE(ridx,times_residue_phase_sent); \
MEMB_LINE(ridx,times_eq_sent); \
MEMB_LINE(ridx,times_coarse_estimated); \
MEMB_LINE(ridx,applied_sfo); \
MEMB_LINE(ridx,sfo_estimated); \
MEMB_LINE(ridx,cfo_estimated); \
MEMB_LINE(ridx,coarse_state); \
MEMB_LINE(ridx,saved_times_coarse_estimated); \
MEMB_LINE(ridx,coarse_delta); \
MEMB_LINE(ridx,trigger_coarse); \
MEMB_LINE(ridx,coarse_finished); \
MEMB_LINE(ridx,delay_cfo); \
MEMB_LINE(ridx,print_first_residue_phase_sent); \
MEMB_LINE(ridx,delay_residue_phase); \
MEMB_LINE(ridx,prev_coarse_ok); \
MEMB_LINE(ridx,should_run_background); \
MEMB_LINE(ridx,should_mask_data_tone_tx_eq); \
MEMB_LINE(ridx,should_mask_all_data_tone); \
MEMB_LINE(ridx,should_print_estimates); \
MEMB_LINE(ridx,old_demod_print_counter_limit); \
MEMB_LINE(ridx,old_demod_print_counter); \
MEMB_LINE(ridx,sfo_tracking_0); \
MEMB_LINE(ridx,sfo_tracking_1); \
MEMB_LINE(ridx,_eq_use_filter); \
MEMB_LINE(ridx,_print_on_new_message); \
MEMB_LINE(ridx,times_sfo_estimated); \
MEMB_LINE(ridx,times_sto_estimated); \
MEMB_LINE(ridx,times_cfo_estimated); \
MEMB_LINE(ridx,times_sto_estimated_p); \
MEMB_LINE(ridx,times_sfo_estimated_p); \
MEMB_LINE(ridx,times_cfo_estimated_p); \
MEMB_LINE(ridx,times_dsp_run_channel_est); \
MEMB_LINE(ridx,margin); \
MEMB_LINE(ridx,_prev_sto_est); \
MEMB_LINE(ridx,_prev_sfo_est); \
MEMB_LINE(ridx,_prev_cfo_est); \
MEMB_LINE(ridx,times_wait_tdma_state_0); \
MEMB_LINE(ridx,residue_phase_trend); \
MEMB_LINE(ridx,sto_estimated); \
MEMB_LINE(ridx,residue_phase_estimated); \
MEMB_LINE(ridx,residue_phase_estimated_pre); \
MEMB_LINE(ridx,eq_filter_index); \
MEMB_LINE(ridx,cheq_done_flag); \
MEMB_LINE(ridx,cheq_onoff_flag); \
MEMB_LINE(ridx,reset_flag); \
MEMB_LINE(ridx,residue_phase_flag); \
MEMB_LINE(ridx,feedback_alive_count); \
MEMB_LINE(ridx,radio_state); \
MEMB_LINE(ridx,radio_state_pending); \
MEMB_LINE(ridx,fsm_event_pending); \
MEMB_LINE(ridx,_sfo_symbol_num); \
MEMB_LINE(ridx,_cfo_symbol_num); \
MEMB_LINE(ridx,_dashboard_eq_update_ratio); \
MEMB_LINE(ridx,_dashboard_eq_update_counter); \
MEMB_LINE(ridx,_dashboard_only_update_channel_angle_r0); \
MEMB_LINE(ridx,epoc_valid); \
MEMB_LINE(ridx,epoc_recent_reply_frames_valid); \
MEMB_LINE(ridx,epoc_recent_reply_frames); \
MEMB_LINE(ridx,cheq_background_counter); \
MEMB_LINE(ridx,should_setup_fsm); \
MEMB_LINE(ridx,map_mov_packets_sent); \
MEMB_LINE(ridx,map_mov_acks_received); \
MEMB_LINE(ridx,eq_hash_state); \
MEMB_LINE(ridx,eq_hash_word); \
MEMB_LINE(ridx,epoc_calibration); \
MEMB_LINE(ridx,save_next_all_sc); \
MEMB_LINE(ridx,enable_residue_to_cfo); \
MEMB_LINE(ridx,residue_to_cfo_delay); \
MEMB_LINE(ridx,residue_to_cfo_factor); \
MEMB_LINE(ridx,pause_residue); \
MEMB_LINE(ridx,sto_delta); \
MEMB_LINE(ridx,drop_residue_target); \
MEMB_LINE(ridx,use_sto_eq_method); \
MEMB_LINE(ridx,pause_eq); \
MEMB_LINE(ridx,eq_to_sto_allowed); \
MEMB_LINE(ridx,use_all_pilot_eq_method); \
MEMB_LINE(ridx,should_unmask_all_eq); \
MEMB_LINE(ridx,eq_iir_gain); \
MEMB_LINE(ridx,adjust_sto_using_sfo); \
MEMB_LINE(ridx,update_sto_sfo_delay); \
MEMB_LINE(ridx,update_sto_sfo_counter); \
MEMB_LINE(ridx,update_sto_sfo_tol); \
MEMB_LINE(ridx,update_sto_sfo_bump); \
MEMB_LINE(ridx,adjust_fractional_sto); \
MEMB_LINE(ridx,sfo_sto_use_duplex); \
MEMB_LINE(ridx,applyeqonce); \
\
\
Object demod_r##ridx = Object::New(env); \
\
\
DEMOD_BOUND(ridx,getBuffSize); \
DEMOD_BOUND(ridx,resetDemodPhase); \
DEMOD_BOUND(ridx,resetTdmaAndLifetime); \
DEMOD_BOUND(ridx,resetTdmaIndexDefault); \
DEMOD_MEMB(ridx,tdma_phase); \
DEMOD_MEMB(ridx,track_demod_against_rx_counter); \
DEMOD_MEMB(ridx,first_dprint); \
DEMOD_MEMB(ridx,times_matched_tdma_6); \
DEMOD_MEMB(ridx,print_found_sync); \
DEMOD_MEMB(ridx,track_record_rx); \
DEMOD_MEMB(ridx,print_all_coarse_sync); \
DEMOD_MEMB(ridx,_print_fine_schedule); \
DEMOD_MEMB(ridx,data_tone_num); \
DEMOD_MEMB(ridx,data_subcarrier_index); \
DEMOD_MEMB(ridx,print_count_after_beef); \
DEMOD_MEMB(ridx,print_sr_count); \
DEMOD_MEMB(ridx,subcarrier_chunk); \
DEMOD_MEMB(ridx,demod_enabled); \
\
status = napi_set_named_property(env, subtree_r##ridx, "demod", demod_r##ridx); \
status = napi_set_named_property(env, subtree, __STR(ridx), subtree_r##ridx); \
\
INIT_TDMA(ridx); \




void init(Napi::Env env, Napi::Object& exports) {

  napi_value subtree;
  auto status = napi_create_object(env, &subtree);
  (void)status;

  INIT_BODY(0);
  INIT_BODY(1);
  INIT_BODY(2);
  INIT_BODY(3);
  INIT_BODY(99);

  exports.Set("r", subtree);
}


}
}
}