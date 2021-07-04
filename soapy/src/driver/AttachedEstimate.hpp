#pragma once

#include "driver/RadioEstimate.hpp"

#include <stdint.h>

// class RadioEstimate;

// class RadioEstimate;
class AttachedEstimate : public RadioEstimate {
public:
    using RadioEstimate::RadioEstimate;  // c++11 way to inherit constructors
    void dispatchAttachedRb(uint32_t word);
    void attachedFineSyncRaw(const uint32_t word);
    uint32_t fine_sync_raw_lower = 0;
};


