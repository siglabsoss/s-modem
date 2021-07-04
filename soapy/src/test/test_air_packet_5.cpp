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

bool handleRecovery(
    int& erase,
    std::vector<uint32_t> &p,
    unsigned& consumed,
    std::function<uint32_t(const std::vector<uint32_t>&)> wrapHash,
    const size_t full_len
    ) {

    // taken from VerifyHash.cpp (not exactly? (convert.cpp (it was packetToBytes())))
    // an enformenet of MTU in worst case packet drop
    constexpr unsigned maxlen = 375; //(0xfff);

    // consumed is the number of words we've eaten
    // if we just found a 0 hash, it means tha tthis entire IP packet is corrupted
    // however we will be able to recover the future
    // in this case we need 2 packets ahead because this one is dropped
    // but we would need at least a full one in the future (how about partials?)
    // (this can be reduced or just check end of input)
    constexpr uint32_t minlook = 6; // don't look for anthing this soon, this would be a short ass packet
    constexpr uint32_t window = 2*maxlen;

    // can also be seen as a flag meaning "recovery failed"
    bool bbreak = true;

    if( (minlook + window) < p.size() ) {
        cout << "There are " << p.size() << " unconsidered words remaining" << "\n";

        bool recovered = false;
        for(unsigned look_start = minlook; (look_start < window) && ((look_start+window) < p.size()) ; look_start++ ) {
            
            unsigned look_end =  look_start + window;

            // look_start = minlook;

            std::vector<uint32_t> look;
            look.assign(p.begin()+look_start,p.begin()+look_end);

            int look_erase = 0;

            auto chars2 = packetToBytesHashMode(look,look_erase,wrapHash);

            bool hashok = chars2.size() != 0;

            if( hashok ) {
                cout << "HASH MATCH!\n";
                cout << "i " << look_start << "\n";
                cout << "    chars2 " << chars2.size() << "\n";
                cout << "    look_erase " << look_erase << "\n";
                cout << "\n";

                const int fudge_erase = look_start;
                p.erase(p.begin(), p.begin()+fudge_erase);
                consumed += (unsigned)fudge_erase;

                erase = fudge_erase; // this will change the flag from -1
                // to a new value
                // while this value is not used, it is checked against -1
                // so setting it to anything positive would also work

                recovered = true; // we recovered
                bbreak = false; // tell parent loop to NOT break (demodGotPacket())
                return bbreak;
            }
        }

        // this is what happenes when we are still at -1 by this point
        // aka we searched but didn't find anything
        if( !recovered ) {
            cout << "demodGotPacket() tried to recover but failed" << " " << full_len << " "<< consumed << " " <<  p.size() << "\n";
            bbreak = true;
            return bbreak;
        }

    } else {
        bbreak = true;
        return bbreak;
    }

    return bbreak;
}


#ifndef food


void demodGotPacket(std::vector<uint32_t> &p) {
    ///
    /// Got the packet
    /// you need put it in the tun at this point, it will be destroyed after this function exits
    ///

    auto full_len = p.size();

    if( print) {
        cout << "demodGotPacket() " << full_len << "\n";
    }

    if( p.size() == 0 ) {
        cout << "Got Packet got EMPTY PACKET, decode probably failed!\n";
        return;
    }


    std::function<uint32_t(const std::vector<uint32_t>&)> wrapHash;

    // f = []() -> uint32_t { }

    wrapHash = [&](const std::vector<uint32_t>& x) {
     // return (return verifyHash->getHashForWords(x);
        // return (uint32_t)0;
        return verifyHash->getHashForWords(x);
    };


    const bool verify_hash = GET_DATA_HASH_VALIDATE_ALL();
    std::vector<uint32_t> hashes;

    unsigned consumed = 0;
    unsigned loop = 0;

    int erase = 0;
    while(erase != -1) {
        if( print) {
            cout << "loop: " << loop << "\n";
        }

        // auto v1 = packetToBytes(p, erase);
        auto v1 = packetToBytesHashMode(p,erase,wrapHash);

        if( erase != -1 ) {

            if( v1.size() ) {
                auto retval = mockwrite(v1.data(), v1.size() );
                if( retval < 0 ) {
                    cout << "demodGotPacket() write !!!!!!!!!!!!!!!!!!!!!!!!!!! returned " << retval << endl;
                }
            } else {
                cout << "hash mismatch\n";
            }

            uint32_t hash = verifyHash->getHashForChars(v1);

            if( verify_hash ) {
                hashes.emplace_back(hash);
            }



            if( erase > 0 ) {
                p.erase(p.begin(), p.begin()+erase);
                consumed += (unsigned)erase;
                // cout << "consumed " << consumed << "\n";
            }

        } else {
            // erase is -1
            cout << "Erase was " << erase << endl;

            const bool breakDesired = handleRecovery(erase, p, consumed, wrapHash, full_len);
            if( breakDesired ) {
                break;
            }

        }
        // break; // FIXME only gets first packet
        loop++;
    }
}

#endif

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

void check_subcarrier2() {

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

        std::cout << std::endl << "output_data:" << std::endl;
        for( uint32_t w : output_data ) {
            std::cout << HEX32_STRING(w) << std::endl;
        }

    }
}




// #include "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/dheader_1154.txt"

#include <bitset>


int main2() {

    auto path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/total3.txt";

    const auto orig = file_read_hex(path);

    cout << std::bitset<32>(0xaaaaaaaa) << "\n";

    for(unsigned i = 0; i < orig.size()-40; i+=40) {
        for(unsigned j = 0; j < 40; j++) {
            const auto w = orig[i+j];
            cout << std::bitset<32>(w);
        }
        cout << "\n";
    }


    // for(const auto w : orig ) {
    //     cout << HEX32_STRING(w) << "\n";
    // }


    return 0;

    // foo();
}

unsigned hammingDistance(uint32_t x, uint32_t y) {
    return bitset<32>(x^y).count();
}


int main() {

    auto path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/productionjs/total3.txt";

    // cout << hammingDistance(0xff000000, 0xff000000) << "\n";

    // return 0;

    const auto orig = file_read_hex(path);


    // const uint32_t mask = 0xaaaaaaaa;
    // const uint32_t mask = 0x55555555;
    // const uint32_t mask = 0x33333333;
    const uint32_t mask = 0xcccccccc;

    // cout << std::bitset<32>(0xaaaaaaaa) << "\n";

    bool p_match = false;
    bool pp_match = false;

    for(unsigned i = 0; i < orig.size()-40; i+=40) {
        unsigned distance = 0;
        for(unsigned j = 0; j < 39; j++) {
            const auto w  = orig[i+j] & mask;
            const auto w2 = orig[i+j+1] & mask;

            distance += hammingDistance(w,w2);
           // ? cout << std::bitset<32>(w);
        }
        if( distance < 300 ) {

            if( p_match && pp_match) {
                cout << "i: " << i/40 << " " << distance << "\n";
            }
            pp_match = p_match;
            p_match = true;
        } else {
            pp_match = p_match;
            p_match = false;
        }

        // cout << "\n";
    }


    // for(const auto w : orig ) {
    //     cout << HEX32_STRING(w) << "\n";
    // }


    return 0;

    // foo();
}