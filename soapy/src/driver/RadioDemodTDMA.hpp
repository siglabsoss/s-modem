#pragma once

#include <cstdint>
#include <stddef.h>
#include <vector>
#include <functional>
#include "driver/HiggsTDMA.hpp"
#include "driver/HiggsStructTypes.hpp"

#ifndef _BUFFER_TYPES
#define _BUFFER_TYPES
typedef std::vector<std::vector<uint32_t>> sliced_buffer_t;
typedef std::vector<std::vector<std::pair<uint32_t, uint32_t>>> iq_buffer_t;
#endif

typedef std::function<void(const size_t peer, const raw_ringbus_t* const rb)> ringbus_peer_cb_t;
typedef std::function<void(const size_t, const uint32_t)> advance_tdma_cb_t;
typedef std::function<void(const size_t, const uint32_t, const uint32_t)> set_partner_tdma_cb_t;


#ifndef DATA_TONE_NUM
#define DATA_TONE_NUM 32
#endif

namespace siglabs {
namespace smodem {

class RadioDemodTDMA {
public:
    RadioDemodTDMA(const size_t, const size_t, const unsigned, const unsigned);
    void setIndex(const unsigned index);
    void setAdvanceCallback(const advance_tdma_cb_t);
    void setSetPartnerTDMACallback(const set_partner_tdma_cb_t);
    void setRingbusCallback(const ringbus_peer_cb_t);

    void alternateDemodTDMA(const uint32_t buffer_index, const uint32_t expected_subcarrier, const bool do_erase);
    // void alignSchedule(const uint32_t tx, const uint32_t rx);
    // void alignSchedule2(HiggsTDMA& td);
    // void alignSchedule3();
    int alignSchedule4();
    int alignSchedule5();
    // int alignSchedule6();
    void partnerOp(const std::string s, const uint32_t _sel, const uint32_t _val);
private:
    void alternateDspRunDemod();
    void dumpBuffers();
public:
    void parseDemodWords(const uint32_t buffer_index);
    void resetTdmaAndLifetime();
    void resetDemodPhase();
    void resetTdmaIndexDefault(void);
    void run(const size_t);
    static uint32_t get_demod_2_bits(const int32_t data);
    // bool sync_multiple_word_stateful(uint32_t &state, const uint32_t demod_bits, const uint32_t subcarrier_index, const int data_index);
    void erasePrevDeltas();
    void found_sync(const int found_i);
    void print_found_demod_final(int found_i);
    size_t getBuffSize(unsigned i);
    void setDemodEnabled(bool _v);
    bool getDemodEnabled() const;

    size_t array_index; // this is the index of the array in HiggsEventFsm
    size_t peer_id;
    unsigned data_tone_num;
    unsigned data_subcarrier_index;
    unsigned subcarrier_chunk;
    // uint32_t demod_word_state = 0;

    iq_buffer_t demod_buf_accumulate;
    std::vector<HiggsTDMA> tdma;
    std::vector<std::pair<uint32_t, uint32_t>> demod_words[DATA_TONE_NUM];
    int32_t tdma_phase = -1;
    bool _print_fine_schedule = true;
    int print_count_after_beef = 0;
    int first_dprint = 0;
    int times_matched_tdma_6 = 0;
    int times_eq_sent = 0;
    bool demod_enabled = true;  // Global enable if we run or dump data at input
    bool track_demod_against_rx_counter = false;
    bool print_all_coarse_sync = false;
    uint32_t track_record_rx = 0;
    int print_found_sync = 0;
    HiggsTDMA *td = 0;
    bool should_print_demod_word = false;
    int print_sr_count = 0;
    unsigned last_mode_sent = 0;
    unsigned last_mode_data_sent = 0;

    // list of indices of subcarriers that we use
    std::vector<size_t> keep_demod_sc;
    std::vector<std::pair<std::uint32_t,std::uint32_t>> history_tx_rx_deltas;


    // "setPartnerTDMA", for example, is named after a member of dsp (EventDsp)
    // however we can abstract that away so this class does not need to know
    // anything about ringbus,soapy,radioestimate,eventdsp and can avoid
    // having pointers to them
    // this class also does not know about settings
    // advance_tdma_cb_t advancePartnerSchedule = 0;
    set_partner_tdma_cb_t setPartnerTDMA = 0;
    ringbus_peer_cb_t ringbusPeer = 0;


};

}
}