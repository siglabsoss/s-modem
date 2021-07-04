#pragma once

#include <iostream>
#include <chrono>


class discovery_t {
public:
    std::string ip;
    uint32_t port0;
    uint32_t port1;
    uint32_t id;
    std::string name;
    std::chrono::steady_clock::time_point last_seen; // this is not serialized and is for the server only

    discovery_t();
    discovery_t(const std::string ip, const uint32_t port0, const uint32_t port1, const uint32_t id, const std::string name);
    discovery_t(const std::string s);

    std::string stringify() const;

    size_t hash() const;
    std::string connect_str(const size_t i) const;

};
