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

template<class T>
void setup(T* air) {
    air->set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    air->set_subcarrier_allocation(MAPMOV_SUBCARRIER_128);
    
    std::uint32_t code_length = 255;
    std::uint32_t fec_length = 48;

    int code_bad;
    code_bad = air->set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
    RC_ASSERT(code_bad==0);

}

int main() {

    bool pass = true;

    pass &= rc::check("Testing Reed Solomon within AirPacket framework", []() {

        AirPacketOutbound tx;
        setup(&tx);
        
        

        std::vector<uint32_t> input_data;
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


        AirPacketInbound airRx;
        setup(&airRx);


        bool found_something = false;
        unsigned do_erase;
        uint32_t found_at_frame;
        unsigned found_at_index; // found_i
        uint32_t found_length;
        air_header_t found_header;
        std::vector<uint32_t> output_data;

        airRx.demodulateSlicedSimple(
            sliced_words,
            output_data,
            found_something,
            found_length,
            found_at_frame,
            found_at_index,
            found_header,
            do_erase
            );

        RC_ASSERT(found_something);


        if( found_something ) {
            cout << "Found something" << endl;

            std::cout << std::endl << "output_data:" << std::endl;
            for( uint32_t w : output_data ) {
                std::cout << HEX32_STRING(w) << std::endl;
            }

        }

        RC_ASSERT( input_data == output_data );
        // RC_ASSERT( input_data != output_data );

    });

  return !pass;
}

void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length) {
    for(int i = 0; i < data_length; i++) {
        uint32_t a_word = *rc::gen::inRange(0u, 0xffffffffu);
        input_data.push_back(a_word);
    }

}
