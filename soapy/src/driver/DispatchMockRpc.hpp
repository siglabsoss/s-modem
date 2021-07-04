#pragma once

// #include <functional>
// #include <vector>

#include <string>
#include "HiggsSettings.hpp"

class HiggsDriver;

class DispatchMockRpc {
public:
    DispatchMockRpc(HiggsSettings* s, HiggsDriver* soapy);

    void dispatch(const std::string& s);

    void handleRequest(const int u_val, const nlohmann::json &j);

    void handleRequest_2(const int u_val, const nlohmann::json &j);
    void handleRequest_1000s(const int u_val, const nlohmann::json &j);

    HiggsSettings* settings;
    HiggsDriver* soapy;
    bool print = false;

    void updateFlags();
};
