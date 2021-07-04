#include <rapidcheck.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "driver/AirPacket.hpp"
#include "cpp_utils.hpp"
#include "schifra_galois_field.hpp"
#include "schifra_galois_field_polynomial.hpp"
#include "schifra_sequential_root_generator_polynomial_creator.hpp"
#include "schifra_reed_solomon_encoder.hpp"
#include "schifra_reed_solomon_decoder.hpp"
#include "schifra_error_processes.hpp"
#include "schifra_reed_solomon_bitio.hpp"
#include "driver/ReedSolomon.hpp"
#include "vector_helpers.hpp"
#include "FileUtils.hpp"

using namespace std;
using namespace siglabs::file;


sliced_buffer_t load_pad(std::string s, unsigned pad = 90000) {
    sliced_buffer_t sliced_words;
    sliced_words.resize(2);


    std::vector<uint32_t> packet;

    packet = file_read_hex(s);


    sliced_words[0].resize(packet.size());
    sliced_words[1] = packet;


    std::vector<uint32_t> zrs;
    zrs.resize(pad);
    VEC_APPEND(sliced_words[0],zrs);
    VEC_APPEND(sliced_words[1],zrs);

    return sliced_words;
}


std::vector<uint32_t> check_subcarrier3(
    AirPacketInbound* airRx,
    sliced_buffer_t& sliced_words,
    uint32_t& found_length) {

    if( sliced_words[0].size() <= 16 || sliced_words[1].size() < 16 ) {
        cout << "sliced_words is very small in check_subcarrier2()!!!!\n";
        // check_subcarrier2
    }

    bool found_something = false;
    unsigned do_erase;
    uint32_t found_at_frame;
    unsigned found_at_index; // found_i
    air_header_t found_header;
    std::vector<uint32_t> output_data;

    airRx->demodulateSlicedSimple(
        sliced_words,
        output_data,
        found_something,
        found_length,
        found_at_frame,
        found_at_index,
        found_header,
        do_erase
        );

    // RC_ASSERT(found_something);


    if( found_something ) {
        cout << "Found something" << endl;

        // std::cout << std::endl << "output_data:" << std::endl;
        // for( uint32_t w : output_data ) {
        //     std::cout << HEX32_STRING(w) << std::endl;
        // }
    }

    return output_data;
}


void remove_tail(std::vector<uint32_t>& vec, const uint32_t value) {
    unsigned j = 0;
    for (auto i = vec.rbegin(); i != vec.rend(); ++i ) { 
        if(*i == value) {
            j++;
        } else {
            break;
        }
    }

    if( j != 0) {
        vec.resize(vec.size()-j);
    }
}



// this one is cut short because I didn't know cs22 needs to flush
bool test_640lin_qpsk(void) {
    // packet = file_read_hex("../../../../sim/data/mapmov_640_lin_3_sliced.hex");
    auto vv = load_pad("../../../../sim/data/mapmov_640_lin_3_sliced.hex");

    AirPacketInbound airRx;
    airRx.print_settings_did_change = false;
    // airRx = new AirPacketInbound();

    airRx.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    airRx.set_subcarrier_allocation(MAPMOV_SUBCARRIER_640_LIN);


    // airRx.print1 = true;
    // airRx.print2 = true;
    // airRx.print1 = true;

    uint32_t found_length;

    auto demod = check_subcarrier3(&airRx, vv, found_length);


    auto orig = file_read_hex("../../../../sim/data/mapmov_640_lin_3.hex");

    orig.erase(orig.begin(),orig.begin()+16); // remove fb header

    const unsigned words = airRx.sliced_word_count;
    const unsigned sync_len = words*32;

    orig.erase(orig.begin(), orig.begin()+sync_len);

    unsigned int check_first = 64;

    bool pass = true;
    for(unsigned i = 0; i < check_first; i++) {
        if(orig[i] -= demod[i]) {
            cout << "Did not match at " << i << "\n";
            pass = false;
            break;
        }
    }

    cout << "test_640lin_qpsk pass: " << (int)pass<<"\n";

    // bool pass = orig == demod;

    // cout << "pass: " << (int)pass<<"\n";

    // for(int i = 0; i < orig.size(); i++) {
    //     std::cout << HEX32_STRING(orig[i]) << "\n";
    // }

    // cout << "\n\n";

    // for(int i = 0; i < demod.size(); i++) {
    //     std::cout << HEX32_STRING(demod[i]) << "\n";
    // }


    return pass;
}




bool test_640lin_qam16(void) {
    bool pass = true;

    auto vv = load_pad("../../../../sim/data/mapmov_640_lin_qam16_1_sliced.hex");

    AirPacketInbound airRx;
    airRx.print_settings_did_change = false;
    // airRx = new AirPacketInbound();

    airRx.set_modulation_schema(FEEDBACK_MAPMOV_QAM16);
    airRx.set_subcarrier_allocation(MAPMOV_SUBCARRIER_640_LIN);


    // airRx.print1 = true;
    // airRx.print2 = true;
    // airRx.print1 = true;
    // airRx.print10 = true;
    // airRx.print15 = true;
    // airRx.print4 = 298;

    uint32_t found_length;

    auto demod = check_subcarrier3(&airRx, vv, found_length);

    cout << "found length: " << HEX32_STRING(found_length) << "\n";

    pass &= found_length == 0x12c;

    auto orig = file_read_hex("../../../../sim/data/mapmov_640_lin_qam16_1.hex");

    orig.erase(orig.begin(),orig.begin()+16); // remove fb header

    const unsigned words = airRx.sliced_word_count;
    const unsigned sync_len = words*32;

    orig.erase(orig.begin(), orig.begin()+sync_len);

    remove_tail(orig, 0x0);

    pass &= orig == demod;

    cout << "test_640lin_qam16 pass: " << (int)pass<<"\n";

    return pass;
}


bool test_320_qpsk(void) {
    bool pass = true;

    auto vv = load_pad("../../../../sim/data/mapmov_320_qpsk_1_sliced.hex");

    AirPacketInbound airRx;
    airRx.print_settings_did_change = false;
    // airRx = new AirPacketInbound();

    airRx.set_modulation_schema(FEEDBACK_MAPMOV_QPSK);
    airRx.set_subcarrier_allocation(MAPMOV_SUBCARRIER_320);


    // airRx.print1 = true;
    // airRx.print2 = true;
    // airRx.print1 = true;
    // airRx.print10 = true;
    // airRx.print15 = true;
    // airRx.print4 = 298;

    uint32_t found_length;

    auto demod = check_subcarrier3(&airRx, vv, found_length);

    cout << "found length: " << HEX32_STRING(found_length) << "\n";

    pass &= found_length == 0x00000259;

    auto orig = file_read_hex("../../../../sim/data/mapmov_320_qpsk_1.hex");

    orig.erase(orig.begin(),orig.begin()+16); // remove fb header

    const unsigned words = airRx.sliced_word_count;
    const unsigned sync_len = words*32;

    orig.erase(orig.begin(), orig.begin()+sync_len);

    remove_tail(orig, 0x0);

    pass &= orig == demod;

    cout << "test_320_qpsk pass: " << (int)pass<<"\n";

    return pass;
}




int main() {
    bool pass = true;

    int runs = 0;

    pass &= test_640lin_qam16();
    pass &= test_640lin_qpsk();
    pass &= test_320_qpsk();


    return !pass;
}