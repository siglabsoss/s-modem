#include "DashTie.hpp"

#include <iostream>
#include <sstream>

#include "common/constants.hpp"

using namespace std;

DashTie::DashTie() :
    p(0),
    uuid(0),
    first_run(true)
    {}

// template <typename T>
// static T* _cast(void* p) {
//     T* as_d = (T*)p;
//     return as_d;
// }

void DashTie::load() {
    switch(type) {
        case dash_type_t::dash_type_double:
        {
            double* as_d = (double*)p;
            value.x_double = *as_d;
        }
        break;
        case dash_type_t::dash_type_uint32_t:
        {
            uint32_t* as_d = (uint32_t*)p;
            value.x_uint32_t = *as_d;
        }
        break;
        case dash_type_t::dash_type_int32_t:
        {
            int32_t* as_d = (int32_t*)p;
            value.x_int32_t = *as_d;
        }
        break;
        case dash_type_t::dash_type_size_t:
        {
            size_t* as_d = (size_t*)p;
            value.x_size_t = *as_d;
        }
        break;
        case dash_type_t::dash_type_bool:
        {
            bool* as_d = (bool*)p;
            value.x_bool = *as_d;
        }
        break;
        case dash_type_t::dash_type_vec_double:
        {
            std::vector<double>* as_d = (std::vector<double>*)p;
            vec_x_double = *as_d;
        }
        break;
        default:
            std::cout << "unknown type in load() " << type << std::endl;
        break;
    }
    // this->valuee.x_double = 33.4;
    // std::cout << this->valuee.x_double << std::endl;
}

bool DashTie::hasChanged() {
    switch(type) {
        case dash_type_t::dash_type_double:
        {
            double* as_d = (double*)p;
            return value.x_double != *as_d;
        }
        break;
        case dash_type_t::dash_type_uint32_t:
        {
            uint32_t* as_d = (uint32_t*)p;
            return value.x_uint32_t != *as_d;
        }
        break;
        case dash_type_t::dash_type_int32_t:
        {
            int32_t* as_d = (int32_t*)p;
            return value.x_int32_t != *as_d;
        }
        break;
        case dash_type_t::dash_type_size_t:
        {
            size_t* as_d = (size_t*)p;
            return value.x_size_t != *as_d;
        }
        break;
        case dash_type_t::dash_type_bool:
        {
            bool* as_d = (bool*)p;
            return value.x_bool != *as_d;
        }
        break;
        case dash_type_t::dash_type_vec_double:
        {
            std::vector<double>* as_d = (std::vector<double>*)p;
            bool eq =
            std::equal(vec_x_double.begin(), vec_x_double.end(), as_d->begin());

            return !eq;
        }
        break;
        default:
            return false;
            std::cout << "unknown type in hasChanged()" << std::endl;
        break;
    }
    // this->valuee.x_double = 33.4;
    // std::cout << this->valuee.x_double << std::endl;
}

std::string DashTie::getPathStr() const {
    std::stringstream ss;

    uint32_t i = 0;
    for( auto folder : path ) {
        if(i != 0) {
            ss << ".";
        }
        ss << folder;
        i++;
    }

    return ss.str();
}





std::string DashTie::buildString() const {
    const DashTie *d = this;
    std::stringstream ss;

    ss << "[{\"u\":" << d->uuid << "},";
    

    ss << "{\"$set\":";


    ss << "{";

    ss << "\"";

    uint32_t i = 0;
    for( auto folder : d->path ) {
        if(i != 0) {
            ss << ".";
        }
        ss << folder;
        i++;
    }

    ss << "\":";

    switch(d->type) {
        case dash_type_t::dash_type_double:
        {
            double* as_d = (double*)d->p;
            ss << *as_d;
        }
        break;
        case dash_type_t::dash_type_uint32_t:
        {
            uint32_t* as_d = (uint32_t*)d->p;
            ss << *as_d;
        }
        break;
        case dash_type_t::dash_type_size_t:
        {
            size_t* as_d = (size_t*)d->p;
            ss << *as_d;
        }
        break;
        case dash_type_t::dash_type_bool:
        {
            bool* as_d = (bool*)d->p;
            ss << ( (*as_d) ? "true" : "false" );
        }
        break;
        case dash_type_t::dash_type_vec_double:
        {
            std::vector<double>* as_d = (std::vector<double>*)p;
            ss << "[";
            auto j = 0;
            for(auto t : *as_d ) {
                if(j != 0) { ss << ","; }
                ss << t;
                j++;
            }
            ss << "]";
        }
        break;
        default:
            cout << "unknown type buildString " << d->type << endl;
            ss << "0";
        break;
    }

    // for( auto folder : d->path ) {
    //     ss << "}";
    // }

    ss << "}}]";

#ifdef DEBUT_PRINT_JSON_OUTPUT
     cout << "ss built: " << ss.str() << endl;
#endif

    return ss.str();

}
