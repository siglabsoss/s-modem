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











int main() {

    ReedSolomon* reed_solomon = new ReedSolomon();

    verifyHash = new VerifyHash();

    const uint32_t code_length = 255;
    const uint32_t fec_length = 32;
    // const uint32_t words = (code_length-fec_length)/4;

    // auto path = "/mnt/overflow/work/software_parent_repo/higgs_sdr_rev2/libs/s-modem/soapy/src/data/rs_packet_0.hex";
    auto path = "../src/data/rs_packet_0.hex";

    const auto original = file_read_hex(path);

    cout << "original size " << original.size() << "\n";

    bool pass = true;

    unsigned checks = 0;


    pass &= rc::check("Testing Demod recovery when RS block fails", [&]() {

        auto input = original;

        const size_t corruption_start = *rc::gen::inRange(1u, (unsigned)(input.size() - 1u));

        const size_t corruption_end = *rc::gen::inRange(corruption_start, corruption_start + 80);

        // cout << "bail ";
        RC_PRE( (corruption_end+1) < input.size() );
        // cout << corruption_end << "\n";

        const uint32_t mask = *rc::gen::inRange(0u, 0xffffffffu);


        const unsigned st = corruption_start;
        const unsigned ed = corruption_end;
        for(unsigned i = st; i < ed; i++) {
            input[i] &= mask;
        }

        std::vector<std::pair<uint32_t,uint32_t>> failures;

        std::vector<uint32_t> decoded_message;

        cout << "\n\n\n";
        failures = reed_solomon->decode_message2(input, decoded_message, code_length, fec_length);

        RC_PRE( failures.size() == 2u  );

        uint32_t a,b;
        for( auto f : failures ) {
            std::tie(a,b) = f;
            cout << "a " << a << "\n";
            cout << "b " << b << "\n";
        }
        // RC_ASSERT( failures.size() < 4u );

        // if( true ) {
        //     for( unsigned i = 0; i < failures.size(); i++ ) {
        //         uint32_t f,b;
        //         std::tie(f,b) = failures[i];
        //         cout << "failed: " << f << " - " << b << "\n";
        //     }
        // }


        // best case is 37
        // this is without us adding corruption.
        // (there is already a corrupted block as it sits on disk)

        // one outcome 36.
        // this means we corrupted the body of a packet but did not get the hash/length combo

        // another outcome is 35
        // this means our block corrupted the tail of one packet, and then also the hash/length of another
        // this means we knock out 2 packets

        cout << "\n\n " << checks << " -----------------------------------------------------------------------------------------------\n";

        write_num = 0;
        demodGotPacket(decoded_message);


        cout << "\n\n\n                                                                             " << write_num << "\n\n\n";
        cout << "\n\n " << checks << " -----------------------------------------------------------------------------------------------\n";
        RC_ASSERT( write_num == 33u || write_num == 34u || write_num == 35u || write_num == 36u || write_num == 37u);

        // somehow I got a 33 here? weird

        // make -j test_air_packet_3 && RC_PARAMS="reproduce=BoCVlNHdp52ZgQUZt9GZgIXZj9mdlJXegcHal5GISNFIix2bjtGImFWasNX09tYrFqNYo9ABP0romgW2iXTB4NZ9b3hKWBBTHh8tgCIgACIgCAQJkBAAA8iCLwQDQcABFgQABIQDGIQCF8ABFQgABUQAVAxBCEACFIQBKoQACExBGEQDCEwBMA" ./test_air_packet_3


        checks ++;
    });

    // foo();
}