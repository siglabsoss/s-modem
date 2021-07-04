#include <rapidcheck.h>
#include "driver/AirPacket.hpp"
#include "common/SafeQueue.hpp"
#include "common/convert.hpp"
#include "driver/VerifyHash.hpp"

// #include "schedule.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"

// instead of including data
std::vector<std::vector<uint32_t>> get_data();
std::vector<uint32_t> get_demodpacket0();
std::vector<uint32_t> get_demodpacket1();


using namespace std;


std::vector<unsigned char> rCharPacket() {
    const auto len = *rc::gen::inRange(60u, 1500u);
    std::vector<unsigned char> v1;
    for(auto i = 0u; i < len; i++) {
        const unsigned char c = *rc::gen::inRange(0u, 256u); // not inclusive
        v1.push_back(c);
    }
    return v1;
}

SafeQueue<std::vector<uint32_t>> txTun2EventDsp;

std::vector<uint32_t> tunGetPacket() {
    return txTun2EventDsp.dequeue();
}

unsigned tunPacketsReady() {
    return txTun2EventDsp.size();
}

unsigned tunFirstLength() {
    if( txTun2EventDsp.size() ) {
        auto peek = txTun2EventDsp.peek();
        return peek.size();
    }
    cout << "warning tunFirstLength() was called when no packets were ready" << endl;
    return 0;
}






std::vector<std::vector<unsigned char>> demodGotPacket(std::vector<uint32_t> &p, bool hash) {
    cout << "Got Packet at frame " << endl;

    std::vector<std::vector<unsigned char>> out;

    int erase = 0;
    while(erase != -1 ) {


        // std::function<uint32_t(const std::vector<uint32_t>&)> lambda;

        auto v1 = hash ? packetToBytesHashMode(p,erase,&VerifyHash::getHashForWords) : packetToBytes(p, erase);
        // auto v1 =  packetToBytes(p, erase);


        if(  erase != -1) {

            if( v1.size() ) {
                out.push_back(v1);
            }


            if( erase > 0 ) {
                p.erase(p.begin(), p.begin()+erase);
            }

        } else {
            // cout << "Erase was -1" << endl;
            // cout << "v1.size() " << v1.size() << endl;
            // cout << "erase actually was " << erase << endl;
            break;
            // there is a crash where we get stuck in here
            // but why? and how?
            // the break might fix this? but why
        }
        // break; // FIXME only gets first packet
    }


    return out;
}


#include "../data/packet_vectors.hpp"


void test_errors_packetToBytes() {
    rc::check("early exit", []() {
        std::vector<unsigned char> mod = v1;
        std::vector<uint32_t> con = charVectorPacketToWordVector(mod);

        unsigned orig_len = con.size();

        const unsigned sub = *rc::gen::inRange(0u, 6u); // not inclusive

        con.resize(con.size()-sub);

        // cout << "con len " << con.size() << endl;

        int erase = 99999;

        packetToBytes(con, erase);

        // cout << "erase: " << erase << endl;

        RC_ASSERT(erase != 99999);

        if( sub == 0 ) {
            RC_ASSERT(erase == orig_len);
        } else {
            RC_ASSERT(erase == -1);
        }


    });
}

void test_errors_packetToBytesHash() {
    rc::check("early exit hash", []() {
        std::vector<unsigned char> mod = v1;
        std::vector<uint32_t> con = charVectorPacketToWordVector(mod);

        auto packet_hash = VerifyHash::getHashForWords(con);

        con.insert(con.begin(), packet_hash);



        unsigned orig_len = con.size();

        // const unsigned sub = *rc::gen::inRange(0u, 6u); // not inclusive
        unsigned sub = 1;

        con.resize(con.size()-sub);

        // cout << "con len " << con.size() << endl;

        int erase = 99999;

        packetToBytesHashMode(con, erase,&VerifyHash::getHashForWords);
        // cout << "erase: " << erase << endl;

        RC_ASSERT(erase != 99999);

        if( sub == 0 ) {
            RC_ASSERT(erase == orig_len);
        } else {
            RC_ASSERT(erase == -1);
        }


    });
}


void test_corrupt_packetToBytesHash() {
    rc::check("corrupt hash", []() {
        std::vector<unsigned char> mod = v1;
        std::vector<uint32_t> con = charVectorPacketToWordVector(mod);

        auto packet_hash = VerifyHash::getHashForWords(con);

        con.insert(con.begin(), packet_hash);

        unsigned orig_len = con.size();

        const unsigned index = *rc::gen::inRange(1u, orig_len); // not inclusive

        const uint32_t mask = *rc::gen::inRange(1u, 0xffffffff); // not inclusive

        con[index] ^= mask;

        

        // unsigned sub = 0;

        // con.resize(con.size()-sub);

        // cout << "con len " << con.size() << endl;

        int erase = 99999;

        auto ret = packetToBytesHashMode(con, erase, &VerifyHash::getHashForWords);
        // cout << "erase: " << erase << endl;

        RC_ASSERT(erase != 99999);

        // if( sub == 0 ) {
        RC_ASSERT(erase == orig_len);
        RC_ASSERT(ret == decltype(ret){} );

        // } else {
        //     RC_ASSERT(erase == -1);
        // }


    });
}


void test_two_packets_selectable(bool use_hash_mode = true, bool corrupt = false) {
    rc::check("Test Char", [use_hash_mode, corrupt]() {

        // txTun2EventDsp.resize(0);

    // const auto len = *rc::gen::inRange(60u, 1500u);

        

        // auto v1 = rCharPacket();
        // auto v2 = rCharPacket();

        std::vector<uint32_t> constructed1 = charVectorPacketToWordVector(v1);
        std::vector<uint32_t> constructed2 = charVectorPacketToWordVector(v2);

        cout << "Constructed1.size " << constructed1.size() << endl;
        cout << "Constructed2.size " << constructed2.size() << endl;

        txTun2EventDsp.enqueue(constructed1);
        txTun2EventDsp.enqueue(constructed2);

        // build this for later, ideal is only used in final comparison
        std::vector<std::vector<unsigned char>> ideal;
        ideal.push_back(v1);
        ideal.push_back(v2);


        std::vector<uint32_t> build_packet;

        const int output_limit_words = 20000;
        int input_total = 0;
        int total_packets = 0;
        bool more;
        unsigned packetsReady = tunPacketsReady();
        more = packetsReady > 0;

        while(more) {
            total_packets++;
            auto fromTun = tunGetPacket();

            auto packet_hash = VerifyHash::getHashForWords(fromTun);

            if( use_hash_mode ) {

                cout << "Packet hash was: "
                   << HEX32_STRING(packet_hash) << ", "
                   << packet_hash << "\n";
                // push hash on
                build_packet.push_back(packet_hash);
            }

            // string together packets
            for( auto x : fromTun ) {
                build_packet.push_back(x);
            }

            // output_progress += last_output;
            input_total += fromTun.size();

            more = false;

            cout << "pullAndPrepTunData " << txTun2EventDsp.size() << endl;

            if( txTun2EventDsp.size() > 0 ) {
                int pk_sz = tunFirstLength();

                // cout << "peek " << pk_sz << endl;

                if( (pk_sz + 1 + input_total) < output_limit_words ) {
                    more = true;
                }
            }

            if( more ) {
                // cout << "doing more" << endl;
            }
        }


        // std::function<uint32_t(const std::vector<uint32_t>&)> lambda;// = lambdas->front();
        // uint32_t VerifyHash::getHashForWords(const std::vector<uint32_t> &v) {

        const unsigned pick = *rc::gen::inRange(0u, 1u); // not inclusive
        if( corrupt ) {
            cout << "pick " << pick << "\n";

            const unsigned index = *rc::gen::inRange(2u, 18u) + (pick*21); // not inclusive

            cout << "corrupt index " << index << "\n";

            const uint32_t mask = *rc::gen::inRange(1u, 0xffffffff); // not inclusive

            build_packet[index] ^= mask;

        }

        // now for the rx side:

        auto got_back = demodGotPacket(build_packet, use_hash_mode);


        if( corrupt ) {

            if( pick == 0 ) {
                RC_ASSERT( got_back.size() == 1 );
                RC_ASSERT( got_back[0] == ideal[1] );
            } else {
                RC_ASSERT( got_back.size() == 1 );
                RC_ASSERT( got_back[0] == ideal[0] );
            }

        } else {

            RC_ASSERT( got_back.size() == ideal.size() );
            RC_ASSERT( got_back[0] == ideal[0] );
            RC_ASSERT( got_back[1] == ideal[1] );
        }



        // cout << "sz " << len << endl;


        // for( auto x : v1 ) {
        //     cout << (int)x << endl;
        // }

        // RC_ASSERT( input_data == output_data );
        cout << endl;
    });
}



int main() {

    test_errors_packetToBytes();
    test_errors_packetToBytesHash();
    test_corrupt_packetToBytesHash();
    test_two_packets_selectable(true);
    test_two_packets_selectable(false);
    test_two_packets_selectable(true, true);
    // exit(0);


     

  return 0;
}

