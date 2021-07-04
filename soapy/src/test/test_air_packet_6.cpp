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

#define __ENABLED



int foo() {
    return 0;
}

#include "driver/ReedSolomon.hpp"

#include "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/verilator/inc/vector_helpers.hpp"




VerifyHash *verifyHash = 0;








// "_comment": "This will cause both sides to zmq the hash of every packet"
#define GET_DATA_HASH_VALIDATE_ALL() false


size_t write_num = 0;



int mockwrite(const unsigned char* const data, size_t size) {
    size_t start = 30 + 2;
    size_t stop = start + 2;
    cout << "mockwrite ";
    for(size_t i = start; i < size && i < stop; i++) {
        cout << HEX8_STRING((int)data[i]) << " ";
    }
    cout << "\n";
    write_num++;

    return 0;
}


bool print = false;


AirPacketInbound* airRx = 0;



sliced_buffer_t sliced_words;

void check_subcarrier() {

    bool found_something;
    bool found_something_again;
    unsigned do_erase;
    uint32_t found_at_frame;
    unsigned found_at_index; // found_i
    uint32_t found_length;
    air_header_t found_header;

    airRx->demodulateSlicedHeader(
        sliced_words,
        found_something,
        found_length,
        found_at_frame,
        found_at_index,
        found_header,
        do_erase
        );
}

void check_subcarrier2(bool print_output = true) {

    if( sliced_words[0].size() <= 16 || sliced_words[1].size() < 16 ) {
        cout << "sliced_words is very small in check_subcarrier2()!!!!\n";
        // check_subcarrier2
    }

    bool found_something = false;
    unsigned do_erase;
    uint32_t found_at_frame;
    unsigned found_at_index; // found_i
    uint32_t found_length;
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

        if( print_output ) {
            std::cout << std::endl << "output_data:" << std::endl;
            for( uint32_t w : output_data ) {
                std::cout << HEX32_STRING(w) << std::endl;
            }
        }

    }
}




// #include "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/dheader_1154.txt"




int main() {


    sliced_words.resize(2);


    std::vector<uint32_t> packet;

    // packet = file_read_hex("/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/data/mapmov_640_lin_1_sliced.hex");
    // packet = file_read_hex("/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/data/mapmov_640_ota_1_sliced.hex");
    // packet = file_read_hex("/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/data/mapmov_512_lin_1_sliced.hex");
    // packet = file_read_hex("/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/sim/data/mapmov_640_lin_3_sliced.hex");
    // packet = file_read_hex("../../../../sim/data/mapmov_640_lin_3_sliced.hex");


    packet = file_read_hex("../../../..//sim/data/mapmov_640_lin_qam16_1_sliced.hex");


    sliced_words[0].resize(packet.size());
    sliced_words[1] = packet;

    // {
    //     // #include "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/dheader_224.txt"
    //     #include "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/capdata_3/dheader_183.txt"
    //     VEC_APPEND(sliced_words[0],a);
    //     VEC_APPEND(sliced_words[1],b);
    // }

    // {
    //     // /mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/capdata_5/dheader_427.txt
    //     #include "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/capdata_5/dheader_427.txt"
    //     VEC_APPEND(sliced_words[0],a);
    //     VEC_APPEND(sliced_words[1],b);
    // }

    // {
    //     #include "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/dheader_225.txt"
    //     VEC_APPEND(sliced_words[0],a);
    //     VEC_APPEND(sliced_words[1],b);
    // }

    // {
    //     #include "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/dheader_226.txt"
    //     VEC_APPEND(sliced_words[0],a);
    //     VEC_APPEND(sliced_words[1],b);
    // }

    std::vector<uint32_t> zrs;
    zrs.resize(1000000);
    zrs.resize(1000);
    VEC_APPEND(sliced_words[0],zrs);
    VEC_APPEND(sliced_words[1],zrs);

    // for(const auto w : b) {
    //     cout << HEX32_STRING(w) << "\n";
    // }

    airRx = new AirPacketInbound();

    airRx->set_modulation_schema(FEEDBACK_MAPMOV_QAM16);
    airRx->set_subcarrier_allocation(MAPMOV_SUBCARRIER_640_LIN);

    // airRx->print1 = true;
    airRx->print2 = true;
    airRx->print4 = 16;
    // airRx->print3 = true;
    // airRx->print6 = true;
    // airRx->print7 = true;

    // airRx->print8 = true;
    // airRx->print9 = true;
    airRx->print10 = true;
    airRx->print12 = 0;
    airRx->print13 = true;



    uint32_t code_length = 255;
    uint32_t fec_length = 48; //32;

    int code_bad;
    // code_bad = airRx->set_code_type(AIRPACKET_CODE_REED_SOLOMON, code_length, fec_length);
    (void)code_bad;



    check_subcarrier2(false);

    return 0;

    // foo();
}