#pragma once

#include <stdint.h>

class SubcarrierEstimate{
public:
    int32_t phase;
    int bits_correct;
    int bits_wrong;
    int print_once;

    SubcarrierEstimate();
    double correct();
    double correctShort();
};