#include <rapidcheck.h>
#include "driver/AirPacket.hpp"

// #include "schedule.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"
#include "common/convert.hpp"
#include <boost/functional/hash.hpp>
#include "cplx.hpp"

// instead of including data
std::vector<std::vector<uint32_t>> get_data();
std::vector<uint32_t> get_demodpacket0();
std::vector<uint32_t> get_demodpacket1();

using namespace std;

#define __ENABLED




#define BUFSIZE 2000

int main() {

    rc::check("unpack step 1 works", [](void) {
        // return;

        static int runs = 0;
        // RC_PRE(runs < 2);
        runs++;

        std::vector<uint32_t> src;

        // src.push_back(0x00000000);
        src.push_back(0x00010001);
        src.push_back(0x00010002);
        src.push_back(0x00020001);
        src.push_back(0x00020002);
        // src.push_back(0xffff0001);
        // src.push_back(0xffff0001);

        const unsigned osz = 1024*4;

        double dst[osz];
        for(unsigned i = 0; i < osz; i++ ) {
            dst[i] = 42.4;
        }

        int step = 1;

        cf64_ci32_unpack_vector(src.data(), dst, 12, src.size(), step);

        for(unsigned i = 0; i < osz; i++ ) {
            if(dst[i] != 42.4) {
                // cout << "output " << i << " changed to " << dst[i] << "\n";
            }
            // osz = 
        }

        std::vector<double> expected = {0.000244141, 0.000244141, 0.000244141*2, 0.000244141};

        for(unsigned i = 0; i < 4; i++ ) {
            double delta = abs(expected[i] - dst[i]);
            RC_ASSERT(delta < 0.00001);
        }

    });


    rc::check("unpack works", [](void) {

        // static int runs = 0;
        // RC_PRE(runs < 1);
        // runs++;


        // uint32_t src[10]
        std::vector<uint32_t> src;

        // src.push_back(0x00000000);
        for( int i = 0; i < 32; i++ ) {
            src.push_back(0x00010001);
            src.push_back(0x00010002);
            src.push_back(0x00020001);
            src.push_back(0x00020002);
        }
        // src.push_back(0xffff0001);
        // src.push_back(0xffff0001);

        const unsigned osz = 1024*4;

        double dst[osz];
        for(unsigned i = 0; i < osz; i++ ) {
            dst[i] = 42.4;
        }

        int step = 2;

        cf64_ci32_unpack_vector(src.data(), dst, 12, src.size(), step);

        for(unsigned i = 0; i < osz; i++ ) {
            if(dst[i] != 42.4) {
                // cout << "output " << i << " changed to " << dst[i] << "\n";
            }
            // osz = 
        }

        std::vector<double> expected = {
0.000244141,
0.000244141,
0.000244141,
2*0.000244141,

0.000244141,
0.000244141,
0.000244141,
2*0.000244141,
};

        // exit(0);

        for(unsigned i = 0; i < expected.size(); i++ ) {
            double delta = abs(expected[i] - dst[i]);
            RC_ASSERT(delta < 0.00001);
        }


        // uint32_t sample = 0x04000800;
        // uint32_t sout;
        // bool saturation;
        // gain_ishort(0.75, sample, sout, saturation);

        // cout << "Gained to " << HEX32_STRING(sout) << "\n";
// 
        // RC_ASSERT(saturation == false);
        // RC_ASSERT(sout == 0x03000600);
        // exit(0);

    });



  return 0;
}

