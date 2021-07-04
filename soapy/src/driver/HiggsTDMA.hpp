#pragma once

#include <stdint.h>


class HiggsTDMA {
public:
    bool found_dead = 0;
    bool sent_tdma = 0;
    uint32_t lifetime_tx = 0;
    uint32_t lifetime_rx = 0;
    uint32_t fudge_rx = 0;
    bool needs_fudge = 0;
    uint32_t epoc_tx = 0;
    void reset();
    bool needsUpdate() const;
};