#pragma once

#include "csocket.hpp"
#include <unistd.h>
#include <string>
#include <vector>
#include <chrono>
#include "common/SafeQueue.hpp"

typedef struct{
    /// ringbus ttl
    uint32_t ttl;
    /// ringbus data
    uint32_t data;
}__attribute__((packed, aligned(1))) ringbus_cmd_t;

typedef struct{
    uint32_t udp_packet_sequence;
    /// ringbus ttl
    uint32_t data;
}__attribute__((packed, aligned(1))) ringbus_cmd_reply_t;

class HiggsRing
{
  int sockfd_ring;
  CSocket s;
public:
  HiggsRing(const std::string& higgs_ip, uint32_t tx_port, uint32_t rx_port);
  ~HiggsRing();
  void send(uint32_t ttl, uint32_t data);
  void get(uint32_t &data, uint32_t &error);
  void sendMulti(std::vector<uint32_t> v);
  void injectFlags(bool,bool);
private:
    mutable std::mutex _mutex;
    std::chrono::steady_clock::time_point last_send;
    static constexpr int64_t minimum_gap = 1200; // in microseconds

    // for hash
    mutable std::mutex _hash_mutex;
    std::chrono::steady_clock::time_point init_timepoint;
    std::vector<std::tuple<uint64_t, uint32_t, uint32_t, uint32_t>> hash_expected;
    void saveIdealHash(const uint32_t ttl, const uint32_t data);
    void hashCompare(const uint32_t word);
    bool _check_hashes = false;
    bool _check_hash_print = true;
public:
    void handleHashReply(const uint32_t word);
};