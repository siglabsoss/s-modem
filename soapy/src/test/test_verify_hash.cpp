#include <rapidcheck.h>
#include "driver/AirPacket.hpp"
#include "driver/VerifyHash.hpp"
#include <boost/functional/hash.hpp>
#include "vector_helpers.hpp"
// #include "schedule.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"
#include "random.h"
#include "schedule.h"

// instead of including data
// std::vector<std::vector<uint32_t>> get_data();
// std::vector<uint32_t> get_demodpacket0();
// std::vector<uint32_t> get_demodpacket1();
// std::vector<uint32_t> get_slicedpacket0();
// std::vector<std::vector<uint32_t>> get_airpacket18();

using namespace std;

#define __ENABLED


std::vector<std::vector<uint32_t>> air_side() {
    std::vector<std::vector<uint32_t>> a = {{0x000000d0,
   0x260235dc,
   0xa1c2b5dc,
   0x431df5dc,
   0xcd0ee5dc,
   0x063935dc,
   0x7bc755dc,
   0xc3e3c5dc}
,  
   {0x000000d1,
   0x1512f5dc,
   0x83fcd5dc,
   0xd8aa25dc,
   0xb61f55dc,
   0x8108d5dc,
   0xb60725dc,
   0xfc5a15dc,
   0x4f1d75dc,
   0x131005dc,
   0xbf6b55dc,
   0xafa1d5dc}
,  
   {0x000000d2,
   0x6a9c85dc,
   0x957e15dc,
   0x0fdea5dc,
   0xd4f095dc,
   0x5ac585dc,
   0x22e305dc,
   0x109425dc,
   0xd6e385dc,
   0xc94805dc,
   0xf94065dc}
,  
   {0x000000d3,
   0xf0e935dc,
   0x62be25dc,
   0x950cf5dc,
   0xd4f715dc,
   0xcf9ac5dc,
   0xcaabb5dc,
   0x7e5ce5dc}
,  
   {0x000000d4,
   0x2f4455dc,
   0x9ef7e5dc,
   0x833a25dc,
   0xb5d3d5dc,
   0x2a5155dc,
   0xd4ed65dc,
   0xb05ee5dc,
   0x74aa85dc,
   0xe617b5dc,
   0xc234c5dc,
   0x934225dc}
,  
   {0x000000d5,
   0xf341f5dc,
   0x506935dc,
   0x35e6a5dc,
   0xc1cbc5dc,
   0x0dfe75dc,
   0x98d205dc,
   0x68e135dc,
   0x527675dc,
   0x05cd55dc,
   0x9d8275dc}
,  
   {0x000000d6,
   0xf560f5dc,
   0xa1f375dc,
   0xebfcc5dc,
   0xc97db5dc,
   0x923fb5dc,
   0x7ba1e5dc,
   0x8dc555dc}
,  
   {0x000000d7,
   0xb80205dc,
   0x31bce5dc,
   0x163625dc,
   0x34e5b5dc,
   0xcb0a15dc,
   0xac7d45dc,
   0xcfb215dc,
   0xe65875dc,
   0x12ce25dc,
   0xaacba5dc,
   0x4c2435dc}
,  
   {0x000000d8,
   0x86cb55dc,
   0x325ea5dc,
   0xdb7e05dc,
   0x67c555dc,
   0x6df195dc,
   0x02bac5dc,
   0xd75d05dc,
   0x2c8f75dc,
   0xa99f05dc,
   0x2f6ac5dc}
,  
   {0x000000d9,
   0x89c325dc,
   0x4efc25dc,
   0xbe7085dc,
   0xd1f565dc,
   0x621175dc,
   0x713e05dc,
   0xb8b2b5dc}
,  
   {0x000000da,
   0x054395dc,
   0x55c9e5dc,
   0x8839c5dc,
   0x759625dc,
   0xe6a485dc,
   0x6b6d55dc,
   0x3ca755dc,
   0xc28105dc,
   0x6b8115dc,
   0xb31705dc,
   0x7d53d5dc}
,  
   {0x000000db,
   0x5e8695dc,
   0xc8a8e5dc,
   0x8fef65dc,
   0x28f1a5dc,
   0x7a6305dc,
   0x75e195dc,
   0x73c975dc,
   0xbf2725dc,
   0x935685dc,
   0x0a6f45dc}
,  
   {0x000000dc,
   0x9510a5dc,
   0x2133d5dc,
   0xf37925dc,
   0xd10e25dc,
   0xd67325dc,
   0xa0b9e5dc,
   0x92f145dc}};

    return a;
}

std::vector<std::vector<uint32_t>> zmq_side() {
    std::vector<std::vector<uint32_t>> a = 
    {
{
   0x000000d0,
   0x260235dc,
   0xa1c2b5dc,
   0x431df5dc,
   0xcd0ee5dc,
   0x063935dc,
   0x7bc755dc,
   0xc3e3c5dc,
},
{
   0x000000d1,
   0x1512f5dc,
   0x83fcd5dc,
   0xd8aa25dc,
   0xb61f55dc,
   0x8108d5dc,
   0xb60725dc,
   0xfc5a15dc,
   0x4f1d75dc,
   0x131005dc,
   0xbf6b55dc,
   0xe32965dc,
},
{
   0x000000d2,
   0x6a9c85dc,
   0x957e15dc,
   0x0fdea5dc,
   0xd4f095dc,
   0x5ac585dc,
   0x22e305dc,
   0x109425dc,
   0xd6e385dc,
   0xc94805dc,
   0xf94065dc,
},
{
   0x000000d3,
   0xf0e935dc,
   0x62be25dc,
   0x950cf5dc,
   0xd4f715dc,
   0xcf9ac5dc,
   0xcaabb5dc,
   0x7e5ce5dc,
},
{
   0x000000d4,
   0x2f4455dc,
   0x9ef7e5dc,
   0x833a25dc,
   0xb5d3d5dc,
   0x2a5155dc,
   0xd4ed65dc,
   0xb05ee5dc,
   0x74aa85dc,
   0xe617b5dc,
   0xc234c5dc,
   0x1e9f05dc,
},
{
   0x000000d5,
   0xf341f5dc,
   0x506935dc,
   0x35e6a5dc,
   0xc1cbc5dc,
   0x0dfe75dc,
   0x98d205dc,
   0x68e135dc,
   0x527675dc,
   0x05cd55dc,
   0x9d8275dc,
},
{
   0x000000d6,
   0xf560f5dc,
   0xa1f375dc,
   0xebfcc5dc,
   0xc97db5dc,
   0x923fb5dc,
   0x7ba1e5dc,
   0x8dc555dc,
},
{
   0x000000d7,
   0xb80205dc,
   0x31bce5dc,
   0x163625dc,
   0x34e5b5dc,
   0xcb0a15dc,
   0xac7d45dc,
   0xcfb215dc,
   0xe65875dc,
   0x12ce25dc,
   0xaacba5dc,
   0x839595dc,
},
{
   0x000000d8,
   0x86cb55dc,
   0x325ea5dc,
   0xdb7e05dc,
   0x67c555dc,
   0x6df195dc,
   0x02bac5dc,
   0xd75d05dc,
   0x2c8f75dc,
   0xa99f05dc,
   0x2f6ac5dc,
},
{
   0x000000d9,
   0x89c325dc,
   0x4efc25dc,
   0xbe7085dc,
   0xd1f565dc,
   0x621175dc,
   0x713e05dc,
   0xb8b2b5dc,
},
{
   0x000000da,
   0x054395dc,
   0x55c9e5dc,
   0x8839c5dc,
   0x759625dc,
   0xe6a485dc,
   0x6b6d55dc,
   0x3ca755dc,
   0xc28105dc,
   0x6b8115dc,
   0xb31705dc,
   0xefac35dc,
},
{
   0x000000db,
   0x5e8695dc,
   0xc8a8e5dc,
   0x8fef65dc,
   0x28f1a5dc,
   0x7a6305dc,
   0x75e195dc,
   0x73c975dc,
   0xbf2725dc,
   0x935685dc,
   0x0a6f45dc,
},
{
   0x000000dc,
   0xeaac15dc,
   0x2133d5dc,
   0xf37925dc,
   0xd10e25dc,
   0xd67325dc,
   0xa0b9e5dc,
   0x92f145dc,
}};
    return a;
}




int main() {

    if(1) {
        rc::check("example data", []() {
        VerifyHash dut;
        dut.do_print = false;
        dut.do_print_partial = false;

        auto zmq = zmq_side();
        auto air = air_side();

        // int expected_sent = 0;
        // int expected_got = 0;

        // std::vector<uint32_t> sent;
        // std::vector<uint32_t> got;

        int i = 0;
        for( auto x: zmq ) {
            const bool add  = *rc::gen::inRange(0, 100) > 40;
            // cout << "zmq " << " " << HEX32_STRING(x[1]) << " " << x.size() << endl;
            if( add ) {
                dut.gotHashListFromPeer(x);
            }
            i++;
        }

        i = 0;
        for( auto x: air ) {
            const bool add  = *rc::gen::inRange(0, 100) > 40;
            // cout << "air " << " " << HEX32_STRING(x[0]) << " " << x.size() << endl;

            std::vector<uint32_t> strip = x;

            uint8_t seq = x[0]&0xff;
            strip.erase(strip.begin());

            if( seq == 215 ) {
                // cout << endl << endl << endl << endl << endl;
                strip[5] ^= 0xfff00000;
            }

            // for( auto y : strip ) { 
            //     cout << HEX32_STRING(y) << endl;
            // }

            air_header_t header = std::make_tuple(0, seq, 0);

            if( add ) {

                auto retransmit = dut.gotHashListOverAir(strip, header);

                auto p0 = std::get<0>(retransmit);
                auto p1 = std::get<1>(retransmit);
                auto p2 = std::get<2>(retransmit);

                cout << "Retransmit " << (int) p0 << ", " << (int)p1 << ", ";
                for( auto x : p2 ) {
                    cout << " (" << x << ")";
                }
                cout << endl;

            }





             // p2.size() << endl;
            i++;
        }


        // std::vector<uint32_t> h1;

        // uint32_t s1 = 1;

        // std::vector<uint32_t> h2;
        // uint32_t s2 = 2;

        // std::vector<uint32_t> h3;
        // uint32_t s3 = 3;

        // for(int i = 0; i < 3; i++) {
        //     const uint32_t hh = *rc::gen::inRange(0u, 0xffffffffu);
        //     h1.push_back(hh);
        // }

        // for(int i = 0; i < 0; i++) {
        //     const uint32_t hh = *rc::gen::inRange(0u, 0xffffffffu);
        //     h2.push_back(hh);
        // }

        // for(int i = 0; i < 3; i++) {
        //     const uint32_t hh = *rc::gen::inRange(0u, 0xffffffffu);
        //     h3.push_back(hh);
        // }

        // h1.insert(h1.begin(), s1);
        // h2.insert(h2.begin(), s2);
        // h3.insert(h3.begin(), s3);

        // dut.gotHashListFromPeer(h1);
        // dut.gotHashListFromPeer(h2);
        // dut.gotHashListFromPeer(h3);

        // air_header_t header = std::make_tuple(0, s1, 0);

        // auto retransmit = dut.gotHashListOverAir(h1, header);

        });
    }



    if(0) {
        rc::check("pull out of bound?", []() {
        VerifyHash dut;

        std::vector<uint32_t> h1;
        uint32_t s1 = 1;

        std::vector<uint32_t> h2;
        uint32_t s2 = 2;

        std::vector<uint32_t> h3;
        uint32_t s3 = 3;

        for(int i = 0; i < 3; i++) {
            const uint32_t hh = *rc::gen::inRange(0u, 0xffffffffu);
            h1.push_back(hh);
        }

        for(int i = 0; i < 0; i++) {
            const uint32_t hh = *rc::gen::inRange(0u, 0xffffffffu);
            h2.push_back(hh);
        }

        for(int i = 0; i < 3; i++) {
            const uint32_t hh = *rc::gen::inRange(0u, 0xffffffffu);
            h3.push_back(hh);
        }

        h1.insert(h1.begin(), s1);
        h2.insert(h2.begin(), s2);
        h3.insert(h3.begin(), s3);

        dut.gotHashListFromPeer(h1);
        dut.gotHashListFromPeer(h2);
        dut.gotHashListFromPeer(h3);

        air_header_t header = std::make_tuple(0, s1, 0);

        auto retransmit = dut.gotHashListOverAir(h1, header);

        });
    }

    if(0) {
        rc::check("compare directly", []() {
        VerifyHash dut;

        std::vector<uint32_t> h1;
        const uint32_t s1 = *rc::gen::inRange(0u, 256u);

        std::vector<uint32_t> h2;
        const uint32_t s2 = *rc::gen::inRange(0u, 256u);


        const uint32_t sz1 = *rc::gen::inRange(0u, 40u);
        const uint32_t sz2 = *rc::gen::inRange(0u, 40u);


        for(int i = 0; i < sz1; i++) {
            const uint32_t hh = *rc::gen::inRange(0u, 0xffffffffu);
            h1.push_back(hh);
        }

        for(int i = 0; i < sz2; i++) {
            const uint32_t hh = *rc::gen::inRange(0u, 0xffffffffu);
            h2.push_back(hh);
        }

        h1.insert(h1.begin(), s1);
        h2.insert(h2.begin(), s2);

        auto packed = VerifyHash::compare(h1, h2);


        });
    }


    if(0) {
        rc::check("pull out of bound?", []() {
                RC_ASSERT( 0xdc050045);
        });
    }



  return 0;
}
