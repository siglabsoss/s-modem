#pragma once

#include "AirMacInterface.hpp"
#include <chrono>


class AirMacBitrateTx : public CookedUserTxInterface {
public:
    void setSeed(const uint32_t s);
    std::vector<uint32_t> generate(const uint32_t count, const uint32_t arg = 0);
    std::vector<uint32_t> getCache(void); // do not call test only

private:
    uint32_t seed = 0x44;
    std::vector<uint32_t> cache;
    bool cacheValid = false;
    void updateCache(const uint32_t count);
};


class AirMacBitrateRx : public CookedUserRxInterface {
public:
    void setSeed(const uint32_t s);
    void got(const std::vector<uint32_t>& packet);

    void resetBer(void);
    double getBer(void);

    bool mac_print1 = true;
    bool mac_print2 = true;

private:
    uint32_t seed = 0x44;
    std::vector<uint32_t> cache;
    bool cacheValid = false;
    void updateCache(const uint32_t count);
    void updateDataRate(const std::chrono::milliseconds ms, const uint32_t size);
    uint32_t last_seq = 0;
    unsigned bits_correct = 0;
    unsigned bits_wrong = 0;


private:
    std::vector<std::pair<uint64_t, uint32_t>> data_rate;

    uint32_t got_count = 0;
};

