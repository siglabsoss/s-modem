#pragma once


#include "driver/HiggsSoapyDriver.hpp"
#include "driver/SubcarrierEstimate.hpp"

#include "driver/HiggsSettings.hpp"


typedef std::tuple<uint32_t, uint8_t, uint8_t> air_header_t;
class AirPacketInbound;
class EventDsp;
class VerifyHash;

namespace siglabs {
namespace smodem{
class RadioDemodTDMA;
}
namespace rb{
class DispatchGeneric;
}
}

#include <stdint.h>
#include <fstream>
#include <mutex>

#ifndef _BUFFER_TYPES
#define _BUFFER_TYPES
typedef std::vector<std::vector<uint32_t>> sliced_buffer_t;
typedef std::vector<std::vector<std::pair<uint32_t, uint32_t>>> iq_buffer_t;
#endif

typedef std::tuple<std::vector<uint32_t>, uint32_t, air_header_t> got_data_t;


// class EventDsp;

class RadioEstimate {
public:
    // pointers to parent objects
    HiggsDriver *soapy;
    EventDsp *dsp;
    HiggsSettings *settings;
    siglabs::smodem::RadioDemodTDMA *demod;
    siglabs::rb::DispatchGeneric *dispatchOp;

    // which id (for zmq etc)
    // are we estimating here
    size_t peer_id;
    size_t array_index; // this is the index of the array in HiggsEventFsm

    // tunables
    static constexpr size_t coarse_bump = 80;
    static constexpr size_t coarse_estimates = 1280 / coarse_bump;
    static constexpr size_t coarse_wait = 3;

    // book keeping
    uint32_t cfo_update_counter;
    double cfo_estimated_sent;
    double sfo_estimated_sent;
    bool coarse_ok;
    size_t times_cfo_sent;
    size_t times_sfo_sent;
    size_t times_sto_sent;
    size_t times_residue_phase_sent;
    size_t times_eq_sent;
    size_t times_coarse_estimated;
    // int32_t demod_est_common_phase;

    bool applied_sfo;
    double sfo_estimated;
    double cfo_estimated;
    int coarse_state;
    size_t saved_times_coarse_estimated;
    double coarse_delta;
    bool trigger_coarse;
    bool coarse_finished;
    uint32_t delay_cfo;
    bool print_first_residue_phase_sent; // for printing
    uint32_t delay_residue_phase;
    bool prev_coarse_ok;
    bool should_run_background;
    bool should_mask_data_tone_tx_eq;
    bool should_mask_all_data_tone;
    bool should_print_estimates;
    bool use_sto_eq_method = false;
    int old_demod_print_counter_limit;
    int old_demod_print_counter;
    bool negate_sfo_updates = false;

    double sfo_tracking_0;
    double sfo_tracking_1;

    bool _eq_use_filter;
    bool _print_on_new_message;


    



    // double sfo_tracking_0;
    // double sfo_tracking_1;




    size_t times_sfo_estimated;
    size_t times_sto_estimated;
    size_t times_cfo_estimated;

    // thease are "previous copyies" of the counters
    size_t times_sto_estimated_p;
    size_t times_sfo_estimated_p;
    size_t times_cfo_estimated_p;

    size_t times_dsp_run_channel_est;

    int margin;
    size_t _prev_sto_est;
    size_t _prev_sfo_est;
    size_t _prev_cfo_est;

    size_t times_wait_tdma_state_0 = 0;


    std::vector<double> residue_phase_history;
    double residue_phase_trend = 0;

    // double frac_sto = 1.0;
    double sto_delta = 0.0;
    bool eq_to_sto_allowed = false; // are we in state where this estimation is valud?
    std::chrono::steady_clock::time_point last_eq_to_sto;
    // bool fractional_eq_allowed = false;

    // New from Zhen
    double sto_estimated;
    double residue_phase_estimated;
    double residue_phase_estimated_pre;

    std::vector<double> channel_angle; // sized 1024

    std::vector<double> pilot_angle;

    double channel_angle_filtering_buffer[1024][25];  // FIXME
    std::vector<std::vector<std::pair<double,double>>> eq_filter;
    size_t eq_filter_index;

    std::vector<uint32_t> eq_iir;
    int32_t eq_iir_gain;






    // std::vector<double> sfo_estimated_sent_array;
    // size_t sfo_estimated_sent_array_index;

    // this is the same value as the sent ones, but stored
    // as double.  when we actually send, sent is set to temp
    std::vector<double> channel_angle_sent; // sized 1024
    std::vector<double> channel_angle_sent_temp; // sized 1024
    std::vector<double> channel_angle_sent_frozen;
    
    /// 32 bit complex
    /// this is the actual value we zmq over (After masking)
    std::vector<uint32_t> equalizer_coeff_sent; 

    /// 32 bit complex
    /// this is the value we sent to eq_one

    std::vector<uint32_t> equalizer_coeff_one;

    // bool sfo_done_flag;
    // bool sto_done_flag;
    // bool cfo_done_flag;
    bool cheq_done_flag;
    bool cheq_onoff_flag;
    bool reset_flag;

    bool residue_phase_flag;  // aka "run residue phase"

    // bumped when remote fb bus replies
    uint32_t feedback_alive_count;


    sliced_buffer_t sliced_words;

    
    // buffers
    SafeQueue<uint32_t> sfo_buf;
    std::vector<uint32_t> cfo_buf_accumulate;
    SafeQueue<uint32_t> cfo_buf;
    // SafeQueue<uint32_t> all_sc_buf;
    SafeQueue<uint32_t> coarse_buf;
    std::vector<uint32_t> all_sc;
    static constexpr unsigned ALL_SC_CHUNK = 1024;

    std::vector<uint32_t> all_sc_saved;
    std::atomic<bool> save_next_all_sc;

    // tx based coarse buffers
    std::vector<bool> tx_coarse_est;
    std::vector<double> tx_coarse_est_mag;

    std::atomic<bool> pause_eq;

    // buffer of tracking for each data subcarrier
    

    // collection of objects for tracking BER
    std::vector<SubcarrierEstimate> demod_est;


    struct event_base *evbase_localfsm;
    struct event* localfsm_next;
    struct event* localfsm_next_timer;
    struct event* slow_poll_cpu_timer;

    size_t radio_state;
    size_t radio_state_pending;
    size_t fsm_event_pending; // same type as d0 of  custom_event_t
    custom_event_t most_recent_event;
 
    unsigned _sfo_symbol_num;
    unsigned _cfo_symbol_num;

    int _dashboard_eq_update_ratio;
    int _dashboard_eq_update_counter;
    bool _dashboard_only_update_channel_angle_r0;
    


    // Demod stuff added by janson
    // these need to go here because static global variables for r0 will get shared with r1
    std::ofstream output_data;
    
    // Received by remote attached radio
    std::vector<uint32_t> remote_rb_buffer;

    //! performance members
    uint32_t est_remote_perf;
    double cpu_load[8]; // FIXME num fpgas actiave
    std::vector<uint32_t> hold_perf;


    bool epoc_valid;
    // recoreded at
    std::chrono::steady_clock::time_point epoc_timestamp; // proposed_epoc_timestamp is copied here when we get 4x EPOC_REPLY_PCCMD
    epoc_frame_t epoc_estimated;
    bool epoc_recent_reply_frames_valid = false;
    int32_t epoc_recent_reply_frames;





    uint32_t cheq_background_counter;



    bool should_setup_fsm;

    std::chrono::steady_clock::time_point last_residue_timepoint;
    std::chrono::steady_clock::time_point init_timepoint;

    unsigned map_mov_packets_sent;
    unsigned map_mov_acks_received;

    uint32_t eq_hash_state = 0;
    uint32_t eq_hash_word = 0;
    std::vector<std::pair<uint32_t, uint64_t>> eq_hash_expected;
    void handleEqHashReply(const uint32_t word);
    void eqHashCompare(const uint32_t word);
    void saveIdealEqHash(const std::vector<uint32_t>&);
    static uint32_t getEqHash(const std::vector<uint32_t>&);
    double calculateEqStoDelta(void) const;

    epoc_frame_t predictScheduleFrame(int &error, const bool useCalibrated = true, const bool print = true) const;
    uint32_t predictLifetime32(int &error, const bool print = false) const;
    void consumeContinuousEpoc(const uint32_t word);
    // void applyFillLevelToMember(const uint32_t word);
    // void handleFillLevelReply(const uint32_t word);
    epoc_frame_t getCalibratedEpoc() const;
    int32_t epoc_calibration = 0;
    std::pair<bool, int32_t> getEpocRecentReply(bool clear = true) const;
    void handleFillLevelReplyDuplex(const uint32_t word);


    void consumePerformance(const uint32_t word);
    void dispatchRemoteRb(const uint32_t word);
    void dispatchAttachedRb(const uint32_t word);
    void idleCheckRemoteRing();

    std::chrono::steady_clock::time_point last_cfo_residue;
    std::atomic<bool> enable_residue_to_cfo;
    std::atomic<uint64_t> residue_to_cfo_delay;
    std::atomic<double> residue_to_cfo_factor;
    void applyResidueToCfo();
    std::atomic<bool> pause_residue;

    std::vector<std::vector<double>> compareEq;
    void setupEqToSto();
    int calculateEqToSto(const bool print = false) const;
    void applyEqToSto2();
    void applyEqToSto();
    double fractionalStoToEq(const int index) const;
    void applyEqToSfo(const double delta);
    std::vector<std::pair<uint64_t,double>> eq_sfo_history;
    void calculateEqToSfo();


    // member calls, do all work here
    RadioEstimate(HiggsDriver *s, EventDsp *, size_t p, struct event_base* e, size_t ary_index, bool setup_fsm);
    

    void setup_localfsm();
    void init_localfsm();
    void tick_localfsm();

    void resetCoarseState();
    void resetTdmaState();

    void updateSfoEstimate(const double estimate);
    void dspRunSfo_v1(void);
    void dspRunSfo_v11(const std::vector<uint32_t>& sfo_buf_chunk);
    void dspRunCfo_v1(const std::vector<uint32_t>& cfo_buf_chunk);
    void dspRunResiduePhase(const std::vector<uint32_t>& cfo_buf_chunk);
    void dspRunChannelEst(void);
    void dspRunChannelEstEqAllSc(void);
    void dspRunChannelEstWithSTO(void);
    bool dspRunChannelEstBuffers(void);
    bool dspRunChannelEstBuffersComplexFilter(void);
    bool dspRunChannelEstBuffersComplexFilterIIR(void);
    void dspRunChannelEstEqAllScAllPilot(void);
    void updateChannelFilterIndex(void);
    void initChannelFilter(void);
    void tickleChannelAngle(void);
    void saveChannel();
    void dspRunStoEq(const double);
    void setRandomRotationEq(const bool send_to_partner = true);
    void resetBER();
    void monitorSendResidue();
    static void rotateVectorBySto(const std::vector<double>& input, std::vector<double>& output, const double m, const double sign);

    // void drainBev(const uint32_t seq, const uint32_t *read_from, const uint32_t len);
    // void drainDemodBuffers(SafeQueueTracked<uint32_t, uint32_t>(&drain_from) [DATA_TONE_NUM]);
    bool chooseCoarseSync();
    void runSoapyCoarseSyncFSM();
    void runSoapyCoarseSync();
    void dspSetupDemod();
    

    void triggerCoarse();

    // bool tdmaCondition();
    void updatePartnerCfo();
    void updatePartnerSfo();
    void channel_angle_sent_temp_into_channel_angle_sent(void);
    void updatePartnerEq(const bool send_existing = false, const bool update_counter=true);
    void updatePartnerSto(const uint32_t sto_adjustment);
    size_t sfoState();
    size_t stoState();
    size_t cfoState();
    void continualBackgroundEstimates();
    uint32_t getStoAdjustmentForEstimated(const double est) const;
    void startBackground();
    void stopBackground();
    std::vector<uint32_t> maskChannelVector(const size_t option, const std::vector<uint32_t>& vec) const;
    void initialMask();
    void tickDataAndBackground();
    void resetFeedbackBackDetectRemote();
    void feedbackPingRemote();
    void tickleOutsiders();


    // bool demopilotangle();

    bool applyeqonce;


    //void tick_background_sync();
    void handleCustomEvent(const custom_event_t* const e);

    void sendEvent(const custom_event_t e);
    void tieAll();
    mutable std::mutex _channel_angle_sent_mutex;

    std::vector<double> getChannelAngle() const;
    std::vector<double> getChannelAngleSent() const;
    std::vector<double> getChannelAngleSentTemp() const;
    std::vector<uint32_t> getEqualizerCoeffSent() const;



    VerifyHash *verifyHash;

    uint32_t getPeerId() const;
    uint32_t getArrayIndex() const;

    unsigned getIndexForDemodTDMA() const;
    unsigned getScForTransmitTDMA() const;
    static std::vector<unsigned> getAllDemodTDMA();
    static std::vector<unsigned> getAllTransmitTDMA();
    void tickleAllTdma();
    void setPartnerTDMA(const uint32_t dmode, const uint32_t value);
    void partnerOp(const std::string s, const uint32_t _sel, const uint32_t _val = 0);
    static std::vector<uint32_t> defaultEqualizerCoeffSent(const double mag_coeff);
    void setMaskedSubcarriers(const std::vector<unsigned>& sc);
    void setTDMASc();
    void maskAllSc();
    void clearRecentEvent();
    unsigned save_tdma_mode_during_recheck = 0;
    bool save_tdma_should_mask_data_tone_tx_eq = false;
    void printSmallMagResidue(double, double, double, uint32_t );
    std::chrono::steady_clock::time_point last_print_small_mag;
    void freezeChannelAngleForStoEq();

    // named drop, but we keep a residue every N many
    unsigned drop_residue_counter = 0;
    // set to (0,1) to keep all, set to 2 keep every other, set to 3 keep every 3rd
    unsigned drop_residue_target = 2;

    // void __attribute__((deprecated)) scheduleOff();
    // void __attribute__((deprecated)) scheduleOn();

    mutable std::mutex _all_eq_mutex;
    std::vector<uint32_t> all_eq_mask;
    void setAllEqMask(const std::vector<uint32_t>&);
    std::vector<uint32_t> maskAllEq(const std::vector<uint32_t>& vec) const;

    // SafeQueue<std::pair<uint32_t,uint32_t>> rolling_data_queue;
    void handleDataToEq();

    bool use_all_pilot_eq_method = false;
    bool should_unmask_all_eq = true;


    void updateStoUsingSfo(void);
    bool adjust_sto_using_sfo = false;
    unsigned update_sto_sfo_delay = 2;
    unsigned update_sto_sfo_counter = 0;
    double update_sto_sfo_tol = 0.0008;
    double update_sto_sfo_bump = 0.005;


    void compensateFractionalSto(void);
    bool adjust_fractional_sto = false;
    unsigned fractional_sto_delay = 24;
    unsigned fractional_sto_counter = 0;

    bool print_pilot_angle_switch = false;

    bool sfo_sto_use_duplex = false;

    void setupEqOne(void);
    void sendEqOne(void);

    void printEqOne(void);
};
