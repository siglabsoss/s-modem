#include "NullBufferSplit.hpp"

#include <cstring>

// #include <iostream>
// using namespace std;

NullBufferSplit::NullBufferSplit(std::function< void(std::string&) > lambda): cap(lambda) {
}

// void NullBufferSplit::debug(void) {
//     std::string a("foo");
//     cap(a);
// }

void NullBufferSplit::addData(const char* const d, const size_t len) {
    addData((const unsigned char*)d, len);
}

void NullBufferSplit::addData(const unsigned char* const d, const size_t len) {
    vec.reserve(vec.size()+len);
    for(auto i = 0u; i < len; i++) {
        // cout << (unsigned) d[i] << endl;
        vec.push_back(d[i]);
    }

    dispatch();
}

void NullBufferSplit::dispatch(void) {
    size_t prev_slice = 0;
    size_t largest_slice = 0;
    bool largest_slice_set = false;
    for(size_t i = 0; i < vec.size(); i++) {
        if(vec[i] == '\0') {
            // cout << "found  null" << endl;
            bool validLength = prev_slice != i;
            if( validLength ) {
                size_t thisLen = i-prev_slice;
                const char* start = (const char*)vec.data()+prev_slice;

                // cout << "This len is " << thisLen << " i is " << i << endl;

                std::string pass;
                pass.resize(thisLen);
                std::memcpy(&pass.front(),start,thisLen);

                // pass.replace(pass.begin(), pass.begin()+thisLen, start);
                // pass.replace(0, thisLen, start);
                cap(pass);
            }

            largest_slice = i;
            largest_slice_set = true;
            prev_slice = i+1;
        }

    }

    if( largest_slice_set ) {
        // trim vec
        // cout << "erasing till: " << largest_slice << endl;
        vec.erase(vec.begin(), vec.begin()+largest_slice);
    }

}