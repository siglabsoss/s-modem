#pragma once

#include <stdint.h>
#include <vector>
#include <map>
#include <chrono>
#include <boost/functional/hash_fwd.hpp>

namespace siglabs {
    namespace crc {
        class Crc32;
    }
}

typedef std::tuple<uint32_t, uint8_t, uint8_t> air_header_t;
// #include "driver/AirPacket.hpp"  // for the type

typedef std::map<std::chrono::steady_clock::time_point, std::vector<uint32_t>> verify_hash_internal_t;
typedef std::map<std::chrono::steady_clock::time_point, uint32_t> verify_hash_rate_t;

// bool needs_retransmit 
// uint8_t sequence number of packet that needs retransmitting
// list of sequence numbers that needs retransmitting
typedef std::tuple<bool, uint8_t, std::vector<uint32_t>> verify_hash_retransmit_t;

class VerifyHash
{
public:

    unsigned count_sent = 0;
    unsigned count_got = 0;
    unsigned bytes_sent = 0;
    unsigned bytes_got = 0;
    unsigned count_stale_expected = 0;
    unsigned count_stale_received = 0;
    double percent_correct = 0.0;
    double bytes_per_second = 0.0;
    const unsigned delete_after_us = (1E6 * 5);
    const unsigned young_cutoff_us = (1E3 * 500); // don't look at anything younger than 500 ms
    const double data_rate_average_seconds = 2.0;

    const unsigned report_after_interactions = 2;
    unsigned report_after_counter = 0;
    bool do_print = true;
    bool do_print_partial = true;
    siglabs::crc::Crc32* crc;
    
private:
    verify_hash_internal_t expected;
    verify_hash_internal_t received;
    verify_hash_rate_t data_rate;

public:
    VerifyHash();
    uint32_t getHashForWords(const std::vector<uint32_t> &v) const;
    uint32_t getHashForWords2(const std::vector<uint32_t> &v) const;
    uint32_t getHashForChars(const std::vector<unsigned char> &v) const;
    static uint32_t getLengthFromHash(const uint32_t w);
    void gotHashListFromPeer(const std::vector<uint32_t> &v);
    verify_hash_retransmit_t gotHashListOverAir(const std::vector<uint32_t> &v, const air_header_t &header);
    std::tuple<std::string,verify_hash_retransmit_t> getStatus(const std::chrono::steady_clock::time_point for_key = std::chrono::steady_clock::time_point());
    std::tuple<std::string,verify_hash_retransmit_t> rateControlledReport(const std::chrono::steady_clock::time_point for_key = std::chrono::steady_clock::time_point() );
    static std::tuple<bool, unsigned, unsigned, unsigned, unsigned, std::vector<uint32_t>> compare(const std::vector<uint32_t> &a, const std::vector<uint32_t> &b);
    void pruneOld(verify_hash_internal_t &m, const std::chrono::steady_clock::time_point now, unsigned &bump_counter, unsigned &bump_counter2, unsigned &bump_bytes);
    void resetCounters();
    void updateDataRate(const std::chrono::steady_clock::time_point now);
};
