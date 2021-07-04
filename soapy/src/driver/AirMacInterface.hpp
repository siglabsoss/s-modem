#pragma once

#include <stdint.h>
#include <vector>



// Gets packets as the tx
class CookedUserTxInterface
{
public:
    virtual void setSeed(const uint32_t seed) = 0;
    virtual std::vector<uint32_t> generate(const uint32_t count, const uint32_t arg) = 0;
};



class CookedUserRxInterface
{
public:
    virtual void setSeed(const uint32_t seed) = 0;
    virtual void got(const std::vector<uint32_t>& packet) = 0;
};
