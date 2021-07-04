#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include "3rd/json.hpp"


enum dash_type_t
{
    dash_type_size_t,
    dash_type_double,
    dash_type_uint32_t,
    dash_type_int32_t,
    dash_type_bool,
    dash_type_vec_double
};

class DashTie {
public:
    void *p;

    std::vector<std::string> path;
    uint32_t uuid;
    dash_type_t type;
    bool first_run;

    union
    {
        std::int32_t x_int32_t;
        std::uint32_t x_uint32_t;
        std::uint64_t x_size_t;
        double x_double;
        bool x_bool;
    } value;

    std::vector<double> vec_x_double;

    DashTie();
    bool hasChanged();

    // void reset();

    void load();

    std::string getPathStr() const;
    std::string buildString() const;
};





// meant for EventDspClass
// javascript needs these
// ranges start including the given number, and go up till next number minus 1
#define TIE_FSM_RANGE 0
#define TIE_CONTROL_RANGE 1000000
#define TIE_SMODEM_RANGE 2000000
#define TIE_VECTOR_RANGE 3000000
#define TIE_LOG_RANGE    4000000

// number of vectors per radio index
#define TIE_VECTOR_PER_INDEX 1000

#define TIE_FSM_UUID 2000000
#define TIE_RESET_COLLECTIONS 1000002
#define TIE_META_HELLO 1000001




///////  Some Constants





///
///  Naked function
///



// see DashTie::buildString()
// dumb way to build original $set style for updates over the same object
template <typename T>
std::string dashTieBuildStringDumb(uint32_t uuid, std::vector<std::string> path, T& j) {
    std::stringstream ss;

    // bool skip_top = false;
    // if(path.size() == 0) {
    //     skip_top = true;
    // }

    ss << "[{\"u\":" << uuid << "},";
    

    ss << "{\"$set\":";


    ss << "{";

    ss << "\"";

    uint32_t i = 0;
    for( auto folder : path ) {
        if(i != 0) {
            ss << ".";
        }
        ss << folder;
        i++;
    }

    ss << "\":";

    ss << j;

    ss << "}}]";

// #ifdef DEBUT_PRINT_JSON_OUTPUT
     std::cout << "dumb ss built: " << ss.str() << std::endl;
// #endif

    return ss.str();
}

template <typename T>
std::string dashTieBuildStringUpdateDumb(uint32_t uuid, T& j) {
    std::stringstream ss;

    // bool skip_top = false;
    // if(path.size() == 0) {
    //     skip_top = true;
    // }

    ss << "[{\"u\":" << uuid << "},";

    ss << j;
    
    ss << "]";

#ifdef DEBUT_PRINT_JSON_OUTPUT
     std::cout << "dumb ss built: " << ss.str() << std::endl;
#endif

    return ss.str();

}