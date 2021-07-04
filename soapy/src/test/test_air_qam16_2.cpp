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

    std::vector<uint32_t> data = {
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xafffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xffffffff,
0xa0000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0xfaaaaaaa,
0xaaaaaaaa,
0xaaaaaaaa,
0xaaaaaaaa,
0xaaaaaaaa,
0xaaaaaaaa,
0xaaaaaaaa,
0xaaaaaaaa,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x00000000,
0x43d240c2,
0x4bd240c2,
0x47d240c2,
0x4fd240c2,
0x403240c2,
0x483240c2,
0x443240c2,
0x4c3240c2,
0x06800000,
0x01f3e000,
0x06d240c2,
0x4ed240c2,
0x41d240c2,
0x49d240c2,
0x45d240c2,
0x4dd240c2,
0x433240c2,
0x4b3240c2,
0x473240c2,
0x4f3240c2,
0x40b240c2,
0x48b240c2,
0x44b240c2,
0x4cb240c2,
0x423240c2,
0x4a3240c2,
0x463240c2,
0x4e3240c2,
0x413240c2,
0x493240c2,
0x453240c2,
0x4d3240c2,
0x43b240c2,
0x4bb240c2,
0x47b240c2,
0x4fb240c2,
0x407240c2,
0x487240c2,
0x447240c2,
0x4c7240c2,
0x42b240c2,
0x4ab240c2,
0x46b240c2,
0x4eb240c2,
0x41b240c2,
0x49b240c2,
0x45b240c2,
0x4db240c2,
0x437240c2,
0x4b7240c2,
0x477240c2,
0x4f7240c2,
0x40f240c2,
0x48f240c2,
0x44f240c2,
0x4cf240c2,
0x427240c2,
0x4a7240c2,
0x467240c2,
0x4e7240c2,
0x417240c2,
0x497240c2,
0x457240c2,
0x4d7240c2
    };

    // for( auto w : data) {
    //     cout << HEX32_STRING(w) << endl;
    // }

    std::vector<uint32_t> dout;

    unsigned start = 0;//16-8;

    for(unsigned i = start; i < data.size(); i ++) {
        unsigned j = i-start;
        unsigned lookup = ((j/16)*16) + 15-(j%16);

        lookup += start;
// 
        // std::cout << "j: " << j << " i: " << i << " lookup: " << lookup << '\n';

        auto w2 = bitMangleSlicedQam16(data[lookup]<<4);
        dout.push_back(w2);
    }

    for( auto w : dout) {
        cout << HEX32_STRING(w) << endl;
    }


    return 0;
}

void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length) {
    for(int i = 0; i < data_length; i++) {
        uint32_t a_word = *rc::gen::inRange(0u, 0xffffffffu);
        input_data.push_back(a_word);
    }

}
