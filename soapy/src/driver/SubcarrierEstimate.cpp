#include "driver/SubcarrierEstimate.hpp"
#include <iostream>

SubcarrierEstimate::SubcarrierEstimate() :
     phase(-1),
     bits_correct(0),
     bits_wrong(0),
     print_once(0)
     {}



double SubcarrierEstimate::correctShort() {
    double v = correct();
    int floor = v*1000000;
    return floor / 1000000.0;
}

double SubcarrierEstimate::correct() {
    if( bits_wrong == 0 && bits_correct == 0) {
        return 0.0;
    }

    return (double)bits_correct / (bits_wrong + bits_correct);
}