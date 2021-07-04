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

    // rc::check("Testing Reed Solomon within AirPacket framework", []() {

        sliced_buffer_t sliced_words;

        sliced_words.resize(2);
        // #include "fix_vertical_demod.hpp"
        #include "fix_vertical_demod_2.hpp"
        // #include "fix_vertical_demod_3.hpp"
        sliced_words[0] = a;
        sliced_words[1] = b;

        // uint32_t final_counter = sliced_words[0][sliced_words[0].size()-1];

        // for(int i = 0; i < 128; i++) {
        //     final_counter++;
        //     sliced_words[0].push_back(final_counter);
        //     sliced_words[1].push_back(0);
        // }


        #define DATA_TONE_NUM 32
        AirPacketInbound airRx;
        airRx.setTestDefaults();
        // WARNING this class was hand designed around 32 full iq subcarriers
        // in mind it may act weird with a value other than 32 for DATA_TONE_NUM
        airRx.data_tone_num = DATA_TONE_NUM;


        std::vector<uint32_t> potential_words;
        bool found_something = false;
        bool found_something_again = false;
        unsigned do_erase;
        uint32_t found_at_frame;
        unsigned found_at_index; // found_i
        uint32_t found_length;
        air_header_t found_header;

        airRx.demodulateSlicedHeader(
            sliced_words,
            potential_words,
            found_something,
            found_length,
            found_at_frame,
            found_at_index,
            found_header,
            do_erase
            );

        // RC_ASSERT(found_something);

        std::vector<uint32_t> output_data;
        unsigned do_erase_sliced = 0;
        if( found_something ) {
            cout << "Found something" << endl;


            /// replace above demodulate with this
            uint32_t force_detect_index = found_at_index+(32*8);
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


            int i = 0;
            std::cout << std::endl << "output_data:" << std::endl;
            for( uint32_t w : output_data ) {
                std::cout << HEX32_STRING(w) << std::endl;
                if( i > 128 ) {
                    break;
                }
                i++;
            }

        }

        // RC_ASSERT( input_data == output_data );

    // });



  return 0;
}

void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length) {
    for(int i = 0; i < data_length; i++) {
        uint32_t a_word = *rc::gen::inRange(0u, 0xffffffffu);
        input_data.push_back(a_word);
    }

}
