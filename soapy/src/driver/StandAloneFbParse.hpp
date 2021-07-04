#pragma once

#include <cstdint>

#include <iomanip>
#include <vector>
#include <functional>
#include <cmath>
#include <atomic>

#include "feedback_bus.h"

typedef std::function<void(void)> fb_error_cb_t;
typedef std::function<void(const feedback_frame_t *v)> fb_raw_cb_t;
typedef std::function<void(uint32_t, uint32_t, const std::vector<uint32_t>&, const std::vector<uint32_t>&)> fb_cb_t;

class EmulateMapmov;

class StandAloneFbParse {
public:

    StandAloneFbParse();

    std::atomic<unsigned> enabled_subcarriers;
    std::atomic<uint16_t> subcarrier_allocation;

    EmulateMapmov* emulate_mapmov = 0;

    bool parse_feedback_bus = true;
    unsigned fbp_progress = 0;
    int fbp_jamming = -1;
    unsigned fbp_error_count = 0;
    fb_error_cb_t fbp_error_cb = 0;
    fb_raw_cb_t fbp_raw_cb = 0;
    fb_cb_t fbp_cb = 0;


    std::vector<uint32_t> fb_data;
    std::vector<uint32_t> fb_history;

    bool keep_history = false;

    bool enable_decode_mapmov = false; // should the decoder understand mapmov packets?
    bool enable_expand_mapmov = false; // should the decoder pass a "mapped moved" packet to callback?


    bool print1 = false; // print when adding
    bool print2 = false; // print math sub equations
    bool print3 = false; // print every word
    bool print4 = false; // print Adv and why we are breaking
    bool print5 = false; // print before calling cb if we got a UD
    bool fbp_print_jamming = true;  // print when jamming


    // seems like we have a bug, if there are no trailing zeros
    // doParseFeedbackBus() won't parse the last packet
    // this hack just adds zeros which you must do at the right time in order
    // to get the last packet
    void customParseFeedbackPadZeros(const unsigned count = 1);

    void addWords(const std::vector<uint32_t>& words);

    ///
    /// fbp prefix means "feedback bus parse"
    void parse(void);

    /// calls all of the correct user functions based on what's enabled
    ///
    void dispatchFb(const feedback_frame_t *v, const uint32_t ud_len = 0);
    void registerRawCb(fb_raw_cb_t cb);
    void registerCb(fb_cb_t cb);
    void registerFbError(fb_error_cb_t cb);

    void set_both(const unsigned enabled_sc, const uint16_t allocation_value);

    void set_mapmov(const uint16_t allocation_value, const uint32_t constellation);
    void disable_mapmov(void);


}; // class
