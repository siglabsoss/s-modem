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

using namespace std;

#define __ENABLED

/*
 * Populates a standard vector with specified amount of random 32 bit values
 *
 * Attributes:
 *     input_data: A standard vector to populate random data
 *     data_length: Amount of random data to populate
 */
void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length);

int main() {

    bool pass = true;

    pass &= rc::check("Testing Reed Solomon within AirPacket framework", []() {

        AirPacketOutbound tx;
        uint32_t val;
        std::vector<uint32_t> input_data;
        tx.print_settings_did_change = false;
        tx.setTestDefaults();
        generate_input_data(input_data, 32);


        uint8_t seq;
        std::vector<uint32_t> d2 = tx.transform(input_data, seq, 0);

        auto sliced_words = tx.emulateHiggsToHiggs(d2);

        uint32_t final_counter = sliced_words[0][sliced_words[0].size()-1];

        for(int i = 0; i < 128; i++) {
            final_counter++;
            sliced_words[0].push_back(final_counter);
            sliced_words[1].push_back(0);
        }


        // #define DATA_TONE_NUM 32
        AirPacketInbound airRx;
        airRx.print_settings_did_change = false;
        airRx.setTestDefaults();
        // WARNING this class was hand designed around 32 full iq subcarriers
        // in mind it may act weird with a value other than 32 for DATA_TONE_NUM
        // airRx.data_tone_num = DATA_TONE_NUM;


        bool found_something = false;
        bool found_something_again = false;
        unsigned do_erase;
        uint32_t found_at_frame;
        unsigned found_at_index; // found_i
        uint32_t found_length;
        air_header_t found_header;

        airRx.demodulateSlicedHeader(
            sliced_words,
            found_something,
            found_length,
            found_at_frame,
            found_at_index,
            found_header,
            do_erase
            );

        RC_ASSERT(found_something);

        std::vector<uint32_t> output_data;
        unsigned do_erase_sliced = 0;
        if( found_something ) {
            cout << "Found something" << endl;


            /// replace above demodulate with this
            uint32_t force_detect_index = airRx.getForceDetectIndex(found_at_index);
            // cout << "force_detect_index " << force_detect_index << endl;
            uint32_t found_at_frame_again;
            unsigned found_at_index_again;
            
            airRx.demodulateSliced(
                sliced_words,
                output_data,
                found_something_again,
                found_at_frame_again,
                found_at_index_again,
                do_erase_sliced,
                force_detect_index,
                found_length);


            std::cout << std::endl << "output_data:" << std::endl;
            for( uint32_t w : output_data ) {
                std::cout << HEX32_STRING(w) << std::endl;
            }

        }

        RC_ASSERT( input_data == output_data );

    });

    pass &= rc::check("Testing vectorJoin and vectorSplit", []() {
        std::vector<std::vector<uint32_t>> a;

        for(unsigned i = 0; i < 5; i++) {
            std::vector<uint32_t> inner;
            for(unsigned j = 0; j < 2+i; j++) {
                // cout << "i: " << i << ", j: " << j << "\n";
                inner.push_back(j);
            }
            a.push_back(inner);
        }

        auto b = AirPacket::vectorJoin(a);

        // for(auto x : b) {
        //     cout << x << "\n";
        // }

        auto c = AirPacket::vectorSplit(b);

        for(unsigned i = 0; i < c.size(); i++) {
            // std::vector<uint32_t> inner;
            for(unsigned j = 0; j < c[i].size(); j++) {
                // cout << "i: " << i << ", j: " << c[i][j] << "\n";
                // inner.push_back(j);
            }
            // a.push_back(inner);
        }

        RC_ASSERT( a == c );

    });


    pass &= rc::check("Testing vectorJoin and vectorSplit zero", []() {
        std::vector<std::vector<uint32_t>> a;

        for(unsigned i = 0; i < 5; i++) {
            std::vector<uint32_t> inner;
            for(unsigned j = 0; j < 2+i; j++) {
                // cout << "i: " << i << ", j: " << j << "\n";
                inner.push_back(j);
            }
            a.push_back(inner);
            if( i == 2 ) {
                a.push_back({});
            }
        }

        auto b = AirPacket::vectorJoin(a);

        auto c = AirPacket::vectorSplit(b);

        RC_ASSERT( a == c );

    });


    pass &= rc::check("Testing vectorJoin and vectorSplit random", []() {
        std::vector<std::vector<uint32_t>> a;

        const uint32_t vectors = *rc::gen::inRange(1u, 35u);
        constexpr uint32_t max_len = 1504/4;

        for(unsigned i = 0; i < vectors; i++) {
            std::vector<uint32_t> inner;
            const uint32_t this_len = *rc::gen::inRange(0u, max_len);
            generate_input_data(inner, this_len);
            a.push_back(inner);
            inner.resize(0);
        }

        auto b = AirPacket::vectorJoin(a);

        auto c = AirPacket::vectorSplit(b);


        RC_ASSERT( a == c );

    });


  return !pass;
}

void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length) {
    for(uint32_t i = 0; i < data_length; i++) {
        const uint32_t a_word = *rc::gen::inRange(0u, 0xffffffffu);
        input_data.push_back(a_word);
    }

}




    // AirPacketOutbound foo;
    // foo.setModulationIndex(MAPMOV_MODE_128_CENTERED_LINEAR);  // For now it's ok that this says 128 but we are in 320 mode below
    // foo.set_modulation_schema(FEEDBACK_MAPMOV_QAM16);
    // foo.set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);
    // auto w = foo.getGainRingbus(4, 0x57000000);
    // for( auto x : w ) {
    //     cout << HEX32_STRING(x[0]) << " " << HEX32_STRING(x[1]) << "\n";
    // }
    // return 0;
