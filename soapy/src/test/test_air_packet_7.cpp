#include <rapidcheck.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"
#include "driver/AirPacket.hpp"
#include "schifra_galois_field.hpp"
#include "schifra_galois_field_polynomial.hpp"
#include "schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra_reed_solomon_encoder.hpp"
#include "schifra_reed_solomon_decoder.hpp"
#include "schifra_error_processes.hpp"
#include "FileUtils.hpp"
#include "VerifyHash.hpp"
#include "convert.hpp"

using namespace std;
using namespace siglabs::file;

// #include "driver/ReedSolomon.hpp"


size_t transform_words_128(size_t in) {
    constexpr size_t enabled_subcarriers = 8;

    size_t div = in/enabled_subcarriers;

    size_t sub = (2*enabled_subcarriers)*div+(enabled_subcarriers-1);

    return sub - in;
}

unsigned bit_set(uint32_t v) {
    unsigned int c; // c accumulates the total bits set in v
    for (c = 0; v; c++)
    {
      v &= v - 1; // clear the least significant bit set
    }

    return c;
}




bool category_1(void) {

    bool pass = true;

    pass &= rc::check("check pack unpack header", []() {
        AirPacket ap;

        const unsigned length = *rc::gen::inRange(0, 0xfffff+1);
        const unsigned seq = *rc::gen::inRange(0, 0xff+1);
        const unsigned flags = *rc::gen::inRange(0, 0xf+1);


        auto p1 = AirPacket::packHeader(length, seq, flags);

        auto unp1 = AirPacket::unpackHeader(p1);

        RC_ASSERT( std::get<0>(unp1) == length );
        RC_ASSERT( std::get<1>(unp1) == seq );
        RC_ASSERT( std::get<2>(unp1) == flags );
    });


    pass &=     rc::check("check one bit flip", [](const uint32_t start_word) {

        std::vector<uint32_t> tun_packet;
        std::vector<uint32_t> tun_packet2;
        for(uint32_t i = 0; i < 1024; i++) {
            tun_packet.push_back (start_word | i);
            tun_packet2.push_back(start_word | i);
        }

        const unsigned pick = *rc::gen::inRange(0, 1024);
        const unsigned bit  = *rc::gen::inRange(0, 32);

        uint32_t overwrite = tun_packet2[pick] | (0x00000001 << bit);
            
        // force that we are changing a bit (We are lazy here)
        RC_PRE(overwrite != tun_packet2[pick]);

        // overwrite
        tun_packet2[pick] = overwrite;

        std::vector<uint32_t> transformed;
        std::vector<uint32_t> transformed2;
        new_subcarrier_data_load(transformed,  tun_packet,  128);
        new_subcarrier_data_load(transformed2, tun_packet2, 128);


        unsigned c1 = 0;
        for(auto x : tun_packet ) {
            c1 += bit_set(x);
        }

        unsigned c2 = 0;
        for(auto x : tun_packet2 ) {
            c2 += bit_set(x);
        }

        unsigned c3 = 0;
        for(auto x : transformed ) {
            c3 += bit_set(x);
        }

        unsigned c4 = 0;
        for(auto x : transformed2 ) {
            c4 += bit_set(x);
        }

        RC_ASSERT(c1+1 == c2);
        // cout << c2 - c1 << endl;

        RC_ASSERT(c1 == c3);
        RC_ASSERT(c2 == c4);


    });


    return pass;
}


int main() {


    bool pass = true;


    // pass &= rc::check("Testing Demod recovery when RS block fails", [&]() {

    // });


    pass &= rc::check("compare transforms", [](unsigned din) {

        // AirPacket ap;

        TransformWords128 tt;

        // std::pair<bool, unsigned> 

        auto rett = tt.t(din);

        // is valid?
        RC_ASSERT(rett.first == true);

        RC_ASSERT(rett.second == transform_words_128(din));
    });


    pass &= category_1();



    return !pass;

    // foo();
}