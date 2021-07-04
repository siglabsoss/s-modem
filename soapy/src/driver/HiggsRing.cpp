#include "HiggsRing.hpp"
#include <iostream>
#include "common/constants.hpp"
#include "cpp_utils.hpp"
#include "common/ReadbackHash.hpp"
#include "ringbus2_pre.h"

using namespace std;

HiggsRing::HiggsRing(const std::string& higgs_ip, uint32_t tx_port, uint32_t rx_port) {
  s.create_socket(higgs_ip.c_str(), tx_port, rx_port);
  s.bind_socket();

  std::cout << "HiggsRing() connect to " << higgs_ip << std::endl;

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 100000;
  if (setsockopt(s.sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      std::cout << "Error setting ringbus socket timeout" << std::endl;
  }

  init_timepoint = std::chrono::steady_clock::now();
}

HiggsRing::~HiggsRing() {
    if(s.sock_fd) {
        close(s.sock_fd);
    }
}

void HiggsRing::injectFlags(bool f0, bool f1) {
    _check_hashes = f0;
    _check_hash_print = f1;

    // cout << "_check_hashes " << _check_hashes << " _check_hash_print " << _check_hash_print << "\n";
}

// may call usleep
void HiggsRing::send(uint32_t ttl, uint32_t data) {

    // std::cout << "SENDING ttl: " << ttl << ", data: " << HEX32_STRING(data) << "\n";

    // do "work" now, before checking time
    ringbus_cmd_t a = {ttl, data};
    socklen_t addrlen = sizeof(s.tx_address);

    // grab lock
    std::unique_lock<std::mutex> lock(_mutex);

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    auto since_last_send = std::chrono::duration_cast<std::chrono::microseconds>( 
                now - last_send
                ).count();

    // std::cout << since_last_send << " us since last ringbus send\n";

    if( (since_last_send >= 0) && (since_last_send < minimum_gap) ) {
        auto sleep_for = minimum_gap-since_last_send;
        // std::cout << "sleeping for " <<  sleep_for << "\n";
        usleep(sleep_for);
    }

    
    sendto(s.sock_fd, &a, sizeof(a), 0, (struct sockaddr *)&s.tx_address, addrlen);

    last_send = std::chrono::steady_clock::now();

    lock.unlock();
    if(_check_hashes) {
        saveIdealHash(ttl, data);
    }
}

// only used when running against mockjs
void HiggsRing::sendMulti(std::vector<uint32_t> v) {
    socklen_t addrlen = sizeof(s.tx_address);

    char* asChar = (char*) v.data();
    auto len = v.size()*4;

    sendto(s.sock_fd, asChar, len, 0, (struct sockaddr *)&s.tx_address, addrlen);
}


void HiggsRing::get(uint32_t &data, uint32_t &error) {
  // uint32_t ack;

  ringbus_cmd_reply_t d;

  socklen_t addrlen = sizeof(s.rx_address);

  if(recvfrom(s.sock_fd, &d, sizeof(d), MSG_WAITALL,
              (struct sockaddr *)&s.rx_address, &addrlen) < 0){
      error = 1;
  }
  else{
      error = 0;
      data = d.data;
  }
}








void HiggsRing::saveIdealHash(const uint32_t ttl, const uint32_t data) {

    uint32_t ttl2 = (((ttl+1) + (RING_BUS_LENGTH*2)) % RING_BUS_LENGTH);;

    auto now = std::chrono::steady_clock::now();
    size_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>( 
        now - init_timepoint
        ).count();

    uint32_t hash = siglabs::hash::hashReadback(ttl2, data);

    if(_check_hash_print) {
        cout << "saveIdealHash ttl " << ttl2 << ", data " << HEX32_STRING(data) << ", hash " << HEX32_STRING(hash) << "\n";
    }

    std::unique_lock<std::mutex> lock(_hash_mutex);

    hash_expected.emplace_back(elapsed_us, hash, ttl, data);
}

void HiggsRing::hashCompare(const uint32_t word) {
    auto now = std::chrono::steady_clock::now();
    uint64_t now_age = std::chrono::duration_cast<std::chrono::microseconds>( 
        now - init_timepoint
        ).count();

    constexpr uint64_t max_age = 1E6*1;

    std::unique_lock<std::mutex> lock(_hash_mutex);

    if(_check_hash_print) {
        cout << "Started with " << hash_expected.size() << " ideal ringbus to search\n";
    }

    bool search_expired = true;
    while(search_expired) {
        search_expired = false;
        for(auto it = hash_expected.begin(); it != hash_expected.end(); ++it) {
            uint32_t ideal_hash;
            uint64_t ideal_age;
            uint32_t ideal_ttl;
            uint32_t ideal_data;
            std::tie(ideal_age, ideal_hash, ideal_ttl, ideal_data) = *it;

            if( (now_age - ideal_age) > max_age ) {

                cout << "!!!!!!!!!!!!!!\n";
                cout << "!!!!!!!!!!!!!!\n";
                cout << "!!!!!!!!!!!!!!  did not get ringbus hash of " << HEX32_STRING(word) << ", ttl " << ideal_ttl << ", data " << HEX32_STRING(ideal_data) << "\n";

                hash_expected.erase(it);
                search_expired = true;
                break;
            }
            // uint ideal_word;
        }
    }

    if(_check_hash_print) {
        cout << "After prune there were " << hash_expected.size() << " ideal ringbus to search\n";
    }

    bool hash_found = false;
    uint32_t ideal_hash;
    uint64_t ideal_age;
    uint32_t ideal_ttl;
    uint32_t ideal_data;
    for(auto it = hash_expected.begin(); it != hash_expected.end(); ++it) {
        std::tie(ideal_age, ideal_hash, ideal_ttl, ideal_data) = *it;
        if( word == ideal_hash ) {
            hash_expected.erase(it);
            hash_found = true;
            break;
        }

        // uint ideal_word;
    }

    if( hash_found ) {
        if(_check_hash_print) {
            cout << "Correct ringbus hash found " << HEX32_STRING(word) << ", ttl " << ideal_ttl << ", data " << HEX32_STRING(ideal_data) << "\n";
        }
    } else {
        // cout << "!!!!!!!!!!!!!!\n";
        // cout << "!!!!!!!!!!!!!!\n";
        if(_check_hash_print) {
            cout << "Got hash for ringbus we didn't send " << HEX32_STRING(word) << "\n";
        }
    }

}
void HiggsRing::handleHashReply(const uint32_t word) {
    if(_check_hashes) {
        hashCompare(word);
    }
}
