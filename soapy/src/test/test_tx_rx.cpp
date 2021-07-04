#include <rapidcheck.h>
#include "driver/AirPacket.hpp"
#include "cpp_utils.hpp"
#include "schifra_galois_field.hpp"
#include "schifra_galois_field_polynomial.hpp"
#include "schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra_reed_solomon_encoder.hpp"
#include "schifra_reed_solomon_decoder.hpp"
#include "schifra_error_processes.hpp"
#include "schifra_reed_solomon_bitio.hpp"

void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length,
                         bool random) {
    uint32_t data;
    for(uint32_t i = 0; i < data_length; i++) {
        data = random ?  *rc::gen::inRange(0u, 0xffffffffu) : i;
        input_data.push_back(data);
    }
}

void print_data(std::vector<uint32_t> &input_data) {
    std::cout << "[";
    for (uint32_t i = 0; i < input_data.size(); i++) {
        if (i == input_data.size() - 1) {
            std::cout << input_data[i];
        } else {
            std::cout << input_data[i] << ",";
        }
    }
    std::cout << "] " << "(len = " << input_data.size() << ")" << std::endl;
}

int main() {
    rc::check("Test TX RX w/ Reed Solomon Encoding", []() {
        AirPacketOutbound tx;
        uint32_t val;
        std::vector<uint32_t> input_data;
        std::vector<uint32_t> transformed_vector;


        std::uint32_t code_length = 255;
        std::uint32_t fec_length = 32;
        int code_bad;
        code_bad = tx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        // RC_ASSERT(code_bad==0);
        // tx.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
        // tx.setModulationIndex(MAPMOV_MODE_REED_SOLOMON);


        generate_input_data(input_data, 34, true);
        uint8_t seq;
        transformed_vector = tx.transform(input_data, seq, 0);
        // std::cout << std::hex;
        // print_data(transformed_vector);
        auto sliced_words = tx.emulateHiggsToHiggs(transformed_vector);

        uint32_t final_counter = sliced_words[0][sliced_words[0].size()-1];

        for(int i = 0; i < 128; i++) {
            final_counter++;
            sliced_words[0].push_back(final_counter);
            sliced_words[1].push_back(0);
        }


        // #define DATA_TONE_NUM 32
        AirPacketInbound airRx;
        airRx.setTestDefaults();
        airRx.set_subcarrier_allocation(MAPMOV_SUBCARRIER_128);
        airRx.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
        // WARNING this class was hand designed around 32 full iq subcarriers in mind
        // it may act weird with a value other than 32 for DATA_TONE_NUM
        // airRx.data_tone_num = DATA_TONE_NUM;

        // code_bad = airRx.set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
        // RC_ASSERT(code_bad==0);


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

        std::cout << "Found length: " << found_length << "\n";

        std::vector<uint32_t> output_data;
        unsigned do_erase_sliced = 0;
        if( found_something ) {
            std::cout << "Found something" << std::endl;
            uint32_t force_detect_index = found_at_index+(32*8);
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

    return 0;
}