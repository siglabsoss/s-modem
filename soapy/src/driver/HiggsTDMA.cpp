#include "driver/HiggsTDMA.hpp"


#include <iostream>
void HiggsTDMA::reset() {
    std::cout << "HiggsTDMA reset\n";
    found_dead = 0;
    sent_tdma = 0;
    lifetime_tx = 0;
    lifetime_rx = 0;
    fudge_rx = 0;
    needs_fudge = 0;
    epoc_tx = 0;
}

bool HiggsTDMA::needsUpdate() const {
    return ((lifetime_tx == 0) && (lifetime_rx == 0)) ;
}