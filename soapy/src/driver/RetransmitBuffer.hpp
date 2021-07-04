#pragma once

#include <stdint.h>
#include <vector>
#include <map>
#include <chrono>
#include <mutex>

typedef std::tuple<uint8_t,std::vector<std::vector<uint32_t>>> retransmit_buffer_record_t;
typedef std::map<std::chrono::steady_clock::time_point, retransmit_buffer_record_t> retransmit_buffer_map_t;


class RetransmitBuffer
{
public:

    const unsigned delete_after_us = (1E6 * 4);
    
private:

    retransmit_buffer_map_t buffer;

    std::vector<std::vector<uint32_t>> need_retransmit;

    mutable std::mutex m;

public:
    RetransmitBuffer();
    void scheduleRetransmit(const uint8_t seq, const std::vector<uint32_t>& indices);

    unsigned firstLength() const;
    unsigned packetsReady() const;
    std::vector<uint32_t> getPacket();

    void sentOverAir(const uint8_t seq, const std::vector<std::vector<uint32_t>> bunch);
    void pruneOld(retransmit_buffer_map_t &m, std::chrono::steady_clock::time_point now) const;
    void printTable() const;
};
