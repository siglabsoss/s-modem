#pragma once

#include <chrono>
#include <map>
#include <iterator>
#include <vector>
#include <mutex>

#include "HiggsSoapyDriver.hpp"

#define STATUS_OK 0
#define STATUS_WARN 1
#define STATUS_ERROR 2
#define STATUS_STALE 3

class HiggsDriver;


// just the key and a timeout, changes infrequenctly
typedef std::map<std::string, double> stat_timeout_t;

/// access this with
///  std::get<0>(x)  // get status
///  std::get<1>(x)  // get message
///  std::get<2>(x)  // get time last set
///  std::get<3>(x)  // flag came from over the network
///  std::get<4>(x)  // we will automatically print when warning or worse
///
typedef std::tuple<unsigned, std::string, std::chrono::steady_clock::time_point, bool, bool> status_code_t;

// a string key, pointing to a <unsigned, time>  pair
// this is because we update them at the same time always
typedef std::map<std::string, status_code_t > stat_map_t;
// typedef std::map<std::string, double> stat_timeout_t;
// typedef std::map<std::string, double> stat_timeout_t;

class StatusCode {
public:
    // if no timeout is set, we use this value
    const double default_timeout = 0.0;

    // if invalid path is asked for we return this
    const unsigned default_error_state = STATUS_STALE;
    std::chrono::steady_clock::time_point default_error_timestamp;

    bool broadcast_to_smodem;
    bool broadcast_to_dashboard;

    HiggsDriver* soapy;

    stat_timeout_t _timeout;
    stat_map_t _status;
    
    // std::vector<std::string> path;
    // uint32_t uuid;
    // dash_type_t type;
    // bool first_run;

    StatusCode(bool forward_to_smodem, bool forward_to_dashboard, HiggsDriver* pointer);

    void create(const std::string &path, const unsigned &status, const double &timeout, const bool print_warnings=true);
    void create(const std::string &path, const double &timeout, const bool print_warnings=true);

    void set(const std::string &path, const unsigned &status, const std::string &message = "");
    status_code_t get(const std::string &path, int* error);
    unsigned getCode(const std::string &path, int* error);
    void _updateTimeout(stat_map_t::iterator it1, stat_timeout_t::iterator it2, std::chrono::steady_clock::time_point& now);
    std::string getNiceText(const std::string &path);
    // mutable std::mutex m;
    std::string getEnumText(unsigned s, bool shortt = true);

    std::string getPeriodicPrint();
};
