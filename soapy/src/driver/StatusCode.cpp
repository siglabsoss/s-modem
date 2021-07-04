#include "StatusCode.hpp"

#include <iostream>
#include <sstream>

#include "3rd/json.hpp"


// #define __STATUS_PRINT

// #include "common/constants.hpp"

using namespace std;

StatusCode::StatusCode(bool s, bool d, HiggsDriver* pointer) :
    default_error_timestamp(std::chrono::steady_clock::now()),
    broadcast_to_smodem(s),
    broadcast_to_dashboard(d),
    soapy(pointer)
    {

    }


void StatusCode::create(const std::string &path, const double &timeout, const bool print_warnings) {
    create(path, STATUS_STALE, timeout, print_warnings);
}

void StatusCode::create(const std::string &path, const unsigned &status, const double &timeout, const bool print_warnings) {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    // pair
    auto inner = std::make_tuple(status, "", now, false, print_warnings);

    // pair of pairs (path is not a pair)
    this->_status.insert(std::make_pair(path, inner));
    this->_timeout.insert(std::make_pair(path, timeout));

}

// void StatusCode::set(const std::string &path, const unsigned &status) {
// };

///
/// Call this with a path to set a status.  This is an "upsert" beca
/// 
void StatusCode::set(const std::string &path, const unsigned &status, const std::string &message) {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    // (this->ins.find(path) != this->ins.end()) && "Test");
    auto found_status = this->_status.find(path);
    bool existing_key =  found_status != this->_status.end();

    if( existing_key ) {
        // set
        auto update = std::make_tuple(status, message, now, false, true);
        found_status->second = update;
    } else {
        create(path, status, this->default_timeout);
    }
}

// sets error to 0 on success
// returns a pair of the status code, and the timepoint
// this could be used to calculate age, but we also internally calculate ate
// age is less useful when the timeout is set correctly (ie just use the wrapper below and
// check stale as your way of knowing the age)
status_code_t StatusCode::get(const std::string &path, int* error) {
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    // all calls to get() automatically check and set STALE when needed
    auto found_status = this->_status.find(path);
    auto found_timeout = this->_timeout.find(path);

    _updateTimeout(found_status, found_timeout, now);


    bool status_exists = found_status != this->_status.end();
    if( !status_exists ) {
        *error = 1;
        // return bunk object
        // set error, don't modify anything
        return std::make_tuple(default_error_state, "", default_error_timestamp, false, true);
    } else {
        *error = 0;
        return found_status->second;
    }

}

// wrapper that strips off the time of last update
unsigned StatusCode::getCode(const std::string &path, int* error) {
    status_code_t pair = get(path, error);
    auto naked_code = std::get<0>(pair);
    return naked_code;
}


void StatusCode::_updateTimeout(stat_map_t::iterator it1, stat_timeout_t::iterator it2, std::chrono::steady_clock::time_point& now) {

    bool status_exists = it1 != this->_status.end();
    bool timeout_exists = it2 != this->_timeout.end();



    if( status_exists && timeout_exists ) {

        auto status = std::get<0>(it1->second);
        auto message = std::get<1>(it1->second);
        auto timeout = it2->second;
        auto remote = std::get<3>(it1->second);
        auto auto_print = std::get<4>(it1->second);

        // calculate how long since we last set the status
        auto delta_t = std::chrono::duration_cast<std::chrono::microseconds>( 
            now - std::get<2>(it1->second)
        ).count() / 1E6;

#ifdef __STATUS_PRINT
        cout << "it1->first: " << it1->first << " " << endl;
        cout << "0: " <<  status << " " << endl;
        cout << "1: " << message << " " << endl;
        cout << "2: " << timeout << " " << endl;
        cout << "delta_t " << delta_t << endl;
#endif


        // timer is older, and we're not already stale
        if( timeout != 0.0 && delta_t > timeout && status != STATUS_STALE ) {
            // call make_tuple to make a new one
            // keep old message and force status to stale
            // now is the time it went stale
            it1->second = std::make_tuple(STATUS_STALE, message, now, remote, auto_print);
        }

    } else {
        return; // silent fail
    }
}

std::string StatusCode::getNiceText(const std::string &path) {

    int error;
    status_code_t found = get(path, &error);
    if(error) {
        return path + std::string(" not found");
    }

    auto status = std::get<0>(found);
    auto message = std::get<1>(found);
    auto remote = std::get<3>(found);

    std::stringstream ss;
    ss << "Path: " << path << endl;
    ss << "  Status: " << getEnumText(status) << endl;
    ss << "  Message: " << message << endl;
    ss << "  Remote: " << std::string(remote?"yes":"no") << endl;

    return ss.str();
}

// Call this at a moderate speed
// this will return warning or error text for status that were
// built with the auto_print option
std::string StatusCode::getPeriodicPrint() {
    std::stringstream ss;

    for (auto& kv : _status) {
        int error;
        auto fetched = get(kv.first, &error);

        if( !error ) {
            auto error_status = std::get<0>(fetched);
            auto auto_print = std::get<4>(fetched);
            
            if( auto_print && error_status >= STATUS_WARN ) {
                ss << getNiceText(kv.first); // this includes the endl
                ss << endl;
            }
        }
    }

    return ss.str();
}


std::string StatusCode::getEnumText(unsigned s, bool shortt) {

if( shortt ) {
switch(s) {
case STATUS_OK: return "OK";
case STATUS_WARN: return "WARN";
case STATUS_ERROR: return "ERROR";
case STATUS_STALE: return "STALE";
default: return "?";
}

} else {

switch(s) {
case STATUS_OK: return "STATUS_OK";
case STATUS_WARN: return "STATUS_WARN";
case STATUS_ERROR: return "STATUS_ERROR";
case STATUS_STALE: return "STATUS_STALE";
default: return "?";
}

} // if short
} // getEnumText