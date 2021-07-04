#include <rapidcheck.h>
#include "driver/AirPacket.hpp"

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

#define __ENABLED





uint32_t extract_single(const std::vector<std::vector<uint32_t>> &data, int j, int i) {
    uint32_t demod_word = 0;

    for(int k = i; k < i+16; k++)
    {
        uint32_t demod_bits = data[j][k];
        demod_word = (demod_word<<2 | demod_bits);
    }
    return demod_word;

}


void extract_word(uint32_t demod_bits, int j, int i, uint32_t &dout, bool &dout_valid) {
    static uint32_t demod_word = 0;

    demod_word = (demod_word << 2) | demod_bits;

    bool ret = false;
    dout_valid = false;
    // if ((demod_word&0xffff0000) == 0xdead0000 ||
    //     demod_word == 0xcafebabe || demod_word == 0xdeadbeef || demod_word == 0x7d5d7f53 || demod_word == 0xdeadbabe || demod_word == 0xbeefbabe) {
    //     std::cout << "R" << 0 << " Found sync word: " << HEX32_STRING(demod_word)
    //             <<  " for subcarrier: " << j
    //               << " at index: " << i << std::endl;

    //     // found_subcarriers[subcarrier_index]++;
    // }

    if((demod_word&0xffff0000) == 0xdead0000 ) {
        dout_valid = true;
        dout = demod_word;
    }
}


void stateful_demod(const std::vector<std::vector<uint32_t>> &as_bits, std::vector<uint32_t> &dout, bool &dout_valid, unsigned &erase) {
    static bool found_sync = 0;
    static uint32_t found_at = 0;

    const unsigned detect_sync_thresh = 18;
    const uint32_t sync_word = 0xbeefbabe;

    uint32_t sync_detects = 0;
    unsigned run = as_bits[0].size();

    for(unsigned i = 0; i < run; i++) {
        sync_detects = 0;
        for(unsigned j = 0; j < 32; j++ ) {
            auto ex_single = extract_single(as_bits, j, i);
            if( ex_single == sync_word )
            {
                sync_detects++;
            }
        }
        if( sync_detects > detect_sync_thresh ) {
            found_sync = 1;
            found_at = i;
            cout << "found " << found_at << " with confidence " << sync_detects <<  endl;
            break;
        }
    }


    if( found_sync == 0 ) {
        cout << "didn't find this call" << endl;
    } else {
        for(unsigned i = found_at; i < run; i += 16) {
            for(unsigned j = 0; j < 32; j++ ) {
                auto ex_single = extract_single(as_bits, j, i);
                cout << HEX32_STRING(ex_single) << endl;
            }
        }
    }

}


void trickle_add( std::vector<std::vector<uint32_t>> &add_to, std::vector<std::vector<uint32_t>> &add_from, unsigned start, unsigned end) {
    for(unsigned j = 0; j < 32; j++) {
        for(unsigned i = start; i < end; i++) {
            add_to[j].push_back(add_from[j][i]);
        }
    }
}

void trickle_erase(std::vector<std::vector<uint32_t>> &add_to, unsigned erase) {

    for(unsigned j = 0; j < 32; j++) {
        // for(unsigned i = 0; i < erase; i++) {
            // add_to[j].push_back(add_from[j][i]);
        add_to[j].erase(add_to[j].begin(), add_to[j].begin()+erase);
        // }
    }
}




bool sync_multiple_word(uint32_t demod_bits, int j, int i) {
    static uint32_t demod_word = 0;

    demod_word = (demod_word << 2) | demod_bits;

    bool ret = false;
    if ((demod_word&0xffff0000) == 0xdead0000 ||
        demod_word == 0xcafebabe || demod_word == 0xdeadbeef || demod_word == 0x7d5d7f53 || demod_word == 0xdeadbabe || demod_word == 0xbeefbabe) {
        std::cout << "R" << 0 << " Found sync word: " << HEX32_STRING(demod_word)
                <<  " for subcarrier: " << j
                  << " at index: " << i << std::endl;

        // found_subcarriers[subcarrier_index]++;
    }

    if(demod_word == 0xbeefbabe) {
        ret = true;
    }

    return ret;
}

bool sss(uint32_t demod_bits, int j, int i, bool reset = false) {
    static uint32_t demod_word = 0;
    static int runs = 0;

    if(reset) {
        demod_word = 0;
        runs = 0;
    }

    demod_word = (demod_word << 2) | demod_bits;

    runs++;
    if( runs == 16 ) {
        cout << HEX32_STRING(demod_word) << endl;
        runs = 0;
        demod_word = 0;
    }


}

unsigned bit_set(uint32_t v) {
    unsigned int c; // c accumulates the total bits set in v
    for (c = 0; v; c++)
    {
      v &= v - 1; // clear the least significant bit set
    }

    return c;
}


bool testPacket7LimitedSet (uint32_t i) {
    switch(i) {
        case 0xdead0020:
        case 0xdead002e:
        case 0xdead008e:
        case 0xdead0120:
        case 0xdead018c:
        case 0xdead018e:
        case 0xdead0220:
        case 0xdead048a:
        case 0xdead048c:
        case 0xdead048e:
        case 0xdead0218:
        case 0xdead029e:
        case 0xdead0422:
        case 0xdead0424:
            return true;
            break;
        default:
            return false;
            break;
    }
    return true;
}


int main() {

    if(0) {
        AirPacket ap;

        // std::vector<uint32_t> d;

        uint32_t source;
        source = 0x10000000; //0xcafebabe & (~0x00000000);
        source = 0xcafebabe;

        // new_subcarrier_data_sync(&d, source, 128);


        // ap.printWide(d, 128);
        // cout << endl;


        std::vector<uint32_t> mydata;
        for(int i = 0; i < 128; i++) {
            mydata.push_back(source);
        }

        // mydata[0] = 0x20000000;

        std::vector<uint32_t> out;

        bool done = false;
        int words_out = 0;
        int words_in = 0;
        int shift = 0;
        const int mask = 0x3;
        while(!done) {
            uint32_t w = 0;

            for(int i = 0; i < 16; i++) {
                w = w | ((mydata[words_in] >> shift) & mask);
                words_in++;
                if( words_in == 128 ) {
                    words_in = 0;
                    shift+=2;
                }
            }
            out.push_back(w);
            words_out++;

            if(words_out == 128) {
                done = true;
            }
            
        }

        ap.printWide(out, 128);
        exit(0);


        // std::vector<uint32_t> d2;
        // new_subcarrier_data_load(d2, mydata, 128);
        // ap.printWide(d2, 128);
        // cout << endl;

        // // cout << "d2 size was: " << d2.size() << endl;
        // // for( auto w : d2 ) {
        // //     cout << HEX32_STRING(w) << endl;
        // // }


        // // RC_ASSERT(
        // //     d == d2
        // //     );

        // exit(0);
    }




    rc::check("signel is identical?", [](uint32_t a_word) {
        int sc = 128;

        std::vector<uint32_t> d;
        new_subcarrier_data_sync(&d, a_word, sc);

        std::vector<uint32_t> mydata;
        for(int i = 0; i < sc; i++) {
            mydata.push_back(a_word);
        }

        std::vector<uint32_t> d2;
        new_subcarrier_data_load(d2, mydata, sc);

        RC_ASSERT(
            d == d2
            );

    });

    rc::check("two words are identical?", [](uint32_t a_word, uint32_t b_word) {
        int sc = 128;

        std::vector<uint32_t> d;
        new_subcarrier_data_sync(&d, a_word, sc);
        new_subcarrier_data_sync(&d, b_word, sc);



        std::vector<uint32_t> d2;

        std::vector<uint32_t> mydata;
        for(int i = 0; i < sc; i++) {
            mydata.push_back(a_word);
        }
        for(int i = 0; i < sc; i++) {
            mydata.push_back(b_word);
        }

        new_subcarrier_data_load(d2, mydata, sc);

        RC_ASSERT(
            d == d2
            );

    });

    rc::check("check loop", []() {

        std::vector<uint32_t> ires;
        for (int i = 30; i >= 0; i -= 2) {
            // std::cout << i << std::endl;
            ires.push_back(i);
        }

        std::vector<uint32_t> jres;
        for(int j = 0; j < 16; j++) {
            int shift = 30 - j*2;
            jres.push_back(shift);
        }

        RC_ASSERT(ires == jres);
    });

#ifdef __ENABLEDasdf
    rc::check("check len", []() {
        std::vector<uint32_t> packet = {
0x54000045,
0x0040a59f,
0xfe7e0140,
0x0004020a,
0x0204020a,
0xd80f0008,
0x0100810d,
0x5bb2d916,
0x00000000,
0x00049e4e,
0x00000000,
0x13121110,
0x17161514,
0x1b1a1918,
0x1f1e1d1c,
0x23222120,
0x27262524,
0x2b2a2928,
0x2f2e2d2c,
0x33323130,
0x37363534};
    
        int erase;
        auto v1 = packetToBytes(packet, erase);
        cout << "erase was " << erase << endl;

        for(x : v1) {
            cout << HEX32_STRING((int)x) << endl;
        }
        exit(0);

// uint32_t data = packet[0];
// int pktSize;

// int bufferSize = 0;
//     // if(pktSize == -1) {
//         bufferSize  = ((data >> 8) & 0xFF00) | (data >> 24);
//         pktSize = ceil(bufferSize/4.0) ;
//         std::cout << "Packet size: " << pktSize << std::endl;
//     // }



        // RC_ASSERT(ires == jres);
    });
#endif


#ifdef __ENABLED
    rc::check("check pack unpack header", []() {
        AirPacket ap;

        const unsigned length = *rc::gen::inRange(0, 0xfffff+1);
        const unsigned seq = *rc::gen::inRange(0, 0xff+1);
        const unsigned flags = *rc::gen::inRange(0, 0xf+1);


        auto p1 = AirPacket::packHeader(length, seq, flags);

        auto unp1 = AirPacket::unpackHeader(p1);

        RC_ASSERT( std::get<0>(unp1) == length );
        RC_ASSERT( std::get<1>(unp1) == seq );
        RC_ASSERT( std::get<2>(unp1) == flags );
    });
#endif



// bool RadioEstimate::packetToTun(uint32_t data) {
//     static std::vector<uint32_t> pkt;
//     static int wordCount = 0;
//     static int pktSize = -1;
//     static int bufferSize = 0;
//     if(pktSize == -1) {
//         bufferSize  = ((data >> 8) & 0xFF00) | (data >> 24);
//         pktSize = ceil(bufferSize/4.0) ;
//         std::cout << "Packet size: " << pktSize << std::endl;
//     }

//     buffer[4*wordCount]=(data >> 0) & 0xFF;
//     buffer[4*wordCount+1]=(data >> 8) & 0xFF;
//     buffer[4*wordCount+2]=(data >> 16) & 0xFF;
//     buffer[4*wordCount+3]=(data >> 24) & 0xFF;
//     wordCount++;
    
//     if(wordCount == pktSize) {
//         pktSize = -1;
//         wordCount = 0;
//         sync_received = 0;
//         write(tun_fd,buffer,bufferSize);
//         return false;
//     }
//     return true;
// }




    // rc::check("test reorder", []() {

    //     uint32_t start_word = 0;

    //     std::vector<uint32_t> rx_packet = get_demodpacket1();

    //     AirPacketInbound inbound;

    //     std::vector<uint32_t> final = inbound.unswizzleData(rx_packet);

    //     // for(auto x : final ) {
    //     //     cout << HEX32_STRING(x) << endl;
    //     // }

    //     std::vector<uint32_t> expected;

    //     uint32_t xx = 0xdead0000;

    //     for(uint32_t i = 0; i < 229; i++) {
    //         expected.push_back(i+xx);
    //     }

    //     RC_ASSERT(final == expected);

    //     bool is_equal = false;
    //     if ( expected.size() != final.size() ) {
    //         is_equal = false;
    //     } else {
    //         is_equal = std::equal ( expected.begin(), expected.end(), final.begin() );
    //     }

    //     RC_ASSERT(is_equal);

    //     // std::vector<uint32_t> transformed;
    //     // new_subcarrier_data_load(transformed,  tun_packet,  128);

    //     // AirPacket ap;

    //     // ap.printWide(transformed, 128);

    //     // std::vector<std::vector<uint32_t>> &data;
    //     // data.resize(32);
    //     // for(j = 0; j < 32; j++) {
    //     //     data[j]
    //     //     for(i = 0; i < 16; i++) {
    //     //         cout << "endl" << endl;
    //     //     }
    //     // }



    //     // exit(0);


      
    // });


    rc::check("in and out ultra", []() {
        return;

        uint32_t start_word = 0;

        std::vector<uint32_t> tun_packet;

        // tun_packet.push_back(0xbeefbabe);
        // tun_packet.push_back(0xbeefbabe);
        // tun_packet.push_back(0xbeefbabe);
        // tun_packet.push_back(0xbeefbabe);
        // tun_packet.push_back(0xbeefbabe);
        // tun_packet.push_back(0xbeefbabe);
        // tun_packet.push_back(0xbeefbabe);
        // tun_packet.push_back(0xbeefbabe);

        for(uint32_t i = 0; i < 1024; i++) {
            tun_packet.push_back(i);
        }

        std::vector<uint32_t> transformed;
        new_subcarrier_data_load(transformed,  tun_packet,  128);

        AirPacket ap;

        ap.printWide(transformed, 128);

        // std::vector<std::vector<uint32_t>> &data;
        // data.resize(32);
        // for(j = 0; j < 32; j++) {
        //     data[j]
        //     for(i = 0; i < 16; i++) {
        //         cout << "endl" << endl;
        //     }
        // }



        exit(0);


      
    });



    rc::check("in and outt", []() {

        std::vector<uint32_t> bits;
        const uint32_t word = 0x00000001;
        uint32_t data = 0;
        uint32_t data2 = 0;
        uint32_t temp1, temp2;
        uint32_t temp3;
        for(int i = 0; i < 16; i++) {
            uint32_t rhs = (word<<(32-i*2)) && 0xff;
            data2 = (data2 << 2) | rhs;
        }

        for (int i = 30; i >= 0; i -= 2) {
            // data = 0;
            temp1 = (word >> i)&0x1;
            temp2 = (word >> (i+1))&0x1;
            temp3 = (temp1) | (temp2<<1);
            // for (int j = 0; j < 16; j++){
            data = (data << 2) | temp3;
            // }
        }

        cout << "in " << HEX32_STRING(word) << " out " << HEX32_STRING(data) << " out " << HEX32_STRING(data2) << endl;


        exit(0);
    });


    rc::check("in and out", []() {

        std::vector<uint32_t> bits;
        uint32_t word = 0x00000001;
        uint32_t sync_frame = 0;
        uint32_t temp1, temp2;
        uint32_t temp3;
        for (int i = 30; i >= 0; i -= 2) {
            // sync_frame = 0;
            temp1 = (word >> i)&0x1;
            temp2 = (word >> (i+1))&0x1;
            temp3 = (temp1) | (temp2<<1);
            // for (int j = 0; j < 16; j++){
            sync_frame = (sync_frame << 2) | temp3;
            // }
        }

        cout << "in " << HEX32_STRING(word) << " out " << HEX32_STRING(sync_frame) << endl;


        exit(0);
    });


    rc::check("check one bit flip", [](const uint32_t start_word) {

        std::vector<uint32_t> tun_packet;
        std::vector<uint32_t> tun_packet2;
        for(uint32_t i = 0; i < 1024; i++) {
            tun_packet.push_back (start_word | i);
            tun_packet2.push_back(start_word | i);
        }

        const unsigned pick = *rc::gen::inRange(0, 1024);
        const unsigned bit  = *rc::gen::inRange(0, 32);

        uint32_t overwrite = tun_packet2[pick] | (0x00000001 << bit);
            
        // force that we are changing a bit (We are lazy here)
        RC_PRE(overwrite != tun_packet2[pick]);

        // overwrite
        tun_packet2[pick] = overwrite;

        std::vector<uint32_t> transformed;
        std::vector<uint32_t> transformed2;
        new_subcarrier_data_load(transformed,  tun_packet,  128);
        new_subcarrier_data_load(transformed2, tun_packet2, 128);


        unsigned c1 = 0;
        for(auto x : tun_packet ) {
            c1 += bit_set(x);
        }

        unsigned c2 = 0;
        for(auto x : tun_packet2 ) {
            c2 += bit_set(x);
        }

        unsigned c3 = 0;
        for(auto x : transformed ) {
            c3 += bit_set(x);
        }

        unsigned c4 = 0;
        for(auto x : transformed2 ) {
            c4 += bit_set(x);
        }

        RC_ASSERT(c1+1 == c2);
        // cout << c2 - c1 << endl;

        RC_ASSERT(c1 == c3);
        RC_ASSERT(c2 == c4);


    });


// >>> [x&3 for x in [0xf, 0x0, 0xa, 0xa, 0xf, 0xf, 0xf, 0xa, 0xa, 0xf, 0xa, 0xa, 0xa, 0xf, 0xf, 0xa]]
// [3, 0, 2, 2, 3, 3, 3, 2, 2, 3, 2, 2, 2, 3, 3, 2]
// >>> [bin(x&3) for x in [0xf, 0x0, 0xa, 0xa, 0xf, 0xf, 0xf, 0xa, 0xa, 0xf, 0xa, 0xa, 0xa, 0xf, 0xf, 0xa]]
// ['0b11', '0b0', '0b10', '0b10', '0b11', '0b11', '0b11', '0b10', '0b10', '0b11', '0b10', '0b10', '0b10', '0b11', '0b11', '0b10']
// >>> bin(0xcafebabe)
// '0b11001010111111101011101010111110'
// exit(0);

#ifdef __ENABLEDasdf
    rc::check("AJSD convert airpacket3", []() {
        FirstTransform trans;

        std::vector<uint32_t> tally;
        tally.resize(1000);

        std::vector<uint32_t> after_transform;

        bool valid;
        unsigned i_out;
        for(unsigned i = 0; i < 1000; i++) {

            trans.t(i, i_out, valid);

            RC_ASSERT(valid);

            cout << i_out << endl;
            after_transform.push_back(i_out);


        }


        for(auto x : after_transform) {
            tally[x]++;
        }


        for(uint32_t i = 0; i < 1000; i++) {

            cout << tally[i] << " ";
            if( i % 128 == 127 ) {
                cout << endl;
            }

        }

        cout << endl << endl;

        trans.build_cache(10000);

        unsigned j_out;

        for(unsigned j = 0; j < 256; j++) {
            trans.r(j, j_out, valid);
            if( valid ) {
                cout << "j: " << j << " jout: " << j_out << endl;
            }
        }


        exit(0);

    });
#endif













#ifdef __ENABLEDasdf
    rc::check("convert airpacket7", []() {
        AirPacket ap;

        // cout << demod_buf_accumulate.size() << endl;

        auto found_i = 443;

        auto demod_buf_accumulate = get_data();

        std::vector<uint32_t> dout;
        bool dout_valid;
        unsigned erase;

        stateful_demod(demod_buf_accumulate, dout, dout_valid, erase);

        // auto s_start = 460;
        // for(auto i = s_start; i < s_start+20; i++) {
        //     auto ex_single = extract_single(demod_buf_accumulate, 0, i);
        //     cout << HEX32_STRING(ex_single) << endl;
        // }

        exit(0);
    });
#endif



#ifdef __ENABLEDasdf
    rc::check("trickle data function works correctly", []() {
        AirPacketInbound inbound;

        std::vector<std::vector<uint32_t>> storage;
        storage.resize(32);

        auto demod_buf_accumulate = get_data();


        unsigned last_start = 0;
        unsigned last_end = 0;

        unsigned add_this_time;

        add_this_time = 10;

        last_end += add_this_time;
        trickle_add(storage, demod_buf_accumulate, last_start, last_end);
        last_start = last_end;


        add_this_time = 60;

        last_end += add_this_time;
        trickle_add(storage, demod_buf_accumulate, last_start, last_end);
        last_start = last_end;


        add_this_time = 1040 - 60 - 10;

        last_end += add_this_time;
        trickle_add(storage, demod_buf_accumulate, last_start, last_end);
        last_start = last_end;



        RC_ASSERT( storage == demod_buf_accumulate);

    });
#endif




#ifdef __ENABLEDasdf
    rc::check("AirPacketInbound streaming demod works?", []() {
        AirPacketInbound inbound;
        inbound.setTestDefaults();


        // inbound.target_buffer_fill = *rc::gen::inRange(2, 4);
        // inbound.target_buffer_fill = 64;

        std::vector<std::vector<uint32_t>> storage;
        storage.resize(32);

        auto demod_buf_accumulate = get_data();

        std::vector<uint32_t> found_words;
        std::vector<uint32_t> r1;
        std::vector<uint32_t> r2;

        bool found_words_valid;
        unsigned erase;

        int total = 1040;

        int s1 = 400;
        int s2 = 400;
        int s3 = total - s2 - s1;


        unsigned total_added = 0;

        unsigned last_start = 0;
        unsigned last_end = 0;

        unsigned add_this_time;

        bool found_r1, found_r2;
        found_r1 = false;
        found_r2 = false;

        // start us off with an entire copy
        // below we add another copy in pieces
        int first_add = 750 + *rc::gen::inRange(0, 250);
        trickle_add(storage, demod_buf_accumulate, 0, first_add);

        bool done = false;
        while(!done) {

            // how many simulated buffer fills do we do?
            add_this_time = *rc::gen::inRange(1, 100);
            // cout << "add this time " << add_this_time << endl;

            // will this buffer fill overrun our input?
            if( (total_added + add_this_time) > total) {
                add_this_time = total - total_added;
                done = true; // exit loop after this run
            }

            last_end += add_this_time; // book keeping
            trickle_add(storage, demod_buf_accumulate, last_start, last_end);
            last_start = last_end;


            // call our mock function
            inbound.demodulate(storage, found_words, found_words_valid, erase);
            // cout << "found " << found_words_valid << " erase " << erase << endl;

            if(erase) {
                // cout << "erasing: " << erase << endl;
                trickle_erase(storage, erase);
            }
            // cout << r1.size() << endl;

            if( found_words_valid ) {
                if( r1.size() == 0 ) {
                    // cout << "wrote r1 " << endl;
                    r1 = found_words;
                    found_r1 = true;
                } else {
                    // cout << "wrote r2 " << endl;
                    r2 = found_words;
                    found_r2 = true;
                }
                found_words = std::vector<uint32_t>();
            }

            total_added += add_this_time;
        }

        // unsigned c2 = 0;
        // for(auto x : r1 ) {
        //     cout << HEX32_STRING(x) << endl;
        // }

        RC_ASSERT(r1 == r2);

        RC_ASSERT(found_r1);
        RC_ASSERT(found_r2);

        // std::vector<uint32_t>::iterator it = std::find_if (r2.begin(), r2.end(), testPacket7IsBad);
// 
        // int num_items3 = std::count_if(r2.begin(), r2.end(), [](int i){return i % 3 == 0;});
        int num_passed = std::count_if(r2.begin(), r2.end(), testPacket7LimitedSet);
        RC_ASSERT(num_passed == 16);

        // cout << "passed " << num_passed << " words" << endl;


        // if( it == r2.end() ) {
        //     cout << "was end" << endl;
        // }
        // std::cout << "The first odd value is " << HEX32_STRING( *it ) << '\n';

        // exit(0);

    });
#endif







#ifdef __ENABLEDsadf
    rc::check("convert airpacket5", []() {
        AirPacket ap;

        // cout << demod_buf_accumulate.size() << endl;

        auto found_i = 61;

        auto demod_buf_accumulate = get_data();

        std::vector<uint32_t> unsorted;

        for(int j = 0; j < 32; j++) {
            auto len = demod_buf_accumulate[j].size();
            len = 320;
            for(int i = found_i; i < len; i++) {
                auto bit_value = demod_buf_accumulate[j][i];
                // cout << "a[" << j << "][" << i << "] = " << bit_value << endl;
                sync_multiple_word(bit_value, j, i);

                uint32_t val;
                bool valid;
                extract_word(bit_value, j, i, val, valid);
                if(valid) {
                    unsorted.push_back(val);
                }

            }
        }
        cout << endl << endl << endl;

        std::sort (unsorted.begin(), unsorted.end());
        for(auto x : unsorted) {
            cout << HEX32_STRING(x) << endl;
        }
        exit(0);

        uint32_t start = 0xdead0000;
        uint32_t end = start + 0x3ae;
        uint32_t range = end - start;

        std::vector<uint32_t> tally;
        tally.resize(range);

        for(uint32_t i = start; i < end; i++) {
            // auto &ref = tally[i-start];

            // if() {

            // }
        }


        for(auto x : unsorted) {
            auto idx = x - start;
            if( idx > 0xf0000 ) {
                cout << "idx " << idx << " too big " <<endl;
            }
            tally[idx]++;
        }   


        for(uint32_t i = start; i < end; i++) {
            auto prog = i-start;

            cout << tally[prog] << " ";
            if( prog % 128 == 127 ) {
                cout << endl;
            }

        }

        cout << endl << endl;



        exit(0);

        // RC_ASSERT(0);


        // assumes they are all the same (they should be, and are in test data)
        auto len = demod_buf_accumulate[0].size();
        len = 946;
        int shift_reg = 0;
        int sc_start = 27;
        int sc_end = sc_start-16;




        for(int i = found_i; i < len; i++) {
            for(int j = 0; j < 32; j++) {
                auto bit_value = demod_buf_accumulate[j][i];
                cout << bit_value << " ";
            }
            cout << endl;
        }


        // RC_ASSERT(0);






        for(int i = found_i; i < len; i++) {
            for(int j = sc_start; j > sc_end; j--) {
                // auto len = demod_buf_accumulate[j].size();
                auto bit_value = demod_buf_accumulate[j][i];

                // if(bit_value==0b01) {
                //     bit_value = 0x10;
                // } else if(bit_value == 0b10) {
                //     bit_value = 0x01;
                // }

                if(0) {
                    shift_reg = (shift_reg << 2) | bit_value;
                    cout << "[" << j << "][" << i << "] = " <<
                    HEX32_STRING(shift_reg) << endl;

                    if( j % 16 == 15 ) {
                        cout << "bit" << endl;
                    }
                }

                sss(bit_value, j, i);

            }
        }
        exit(0);

    });
#endif











#ifdef __ENABLEDsdfsdf
    rc::check("convert airpacket4", []() {
        AirPacket ap;

        // cout << demod_buf_accumulate.size() << endl;

        auto found_i = 873;

        auto demod_buf_accumulate = get_data();

        std::vector<uint32_t> unsorted;

        for(int j = 0; j < 32; j++) {
            auto len = demod_buf_accumulate[j].size();
            // len = 320;
            for(int i = found_i; i < len; i++) {
                auto bit_value = demod_buf_accumulate[j][i];
                // cout << "a[" << j << "][" << i << "] = " << bit_value << endl;
                // sync_multiple_word(bit_value, j, i);

                uint32_t val;
                bool valid;
                extract_word(bit_value, j, i, val, valid);
                if(valid) {
                    unsorted.push_back(val);
                }

            }
        }
        cout << endl << endl << endl;

        std::sort (unsorted.begin(), unsorted.end());
        for(auto x : unsorted) {
            cout << HEX32_STRING(x) << endl;
        }

        uint32_t start = 0xdead0000;
        uint32_t end = start + 0x3ae;
        uint32_t range = end - start;

        std::vector<uint32_t> tally;
        tally.resize(range);

        for(uint32_t i = start; i < end; i++) {
            // auto &ref = tally[i-start];

            // if() {

            // }
        }


        for(auto x : unsorted) {
            auto idx = x - start;
            if( idx > 0xf0000 ) {
                cout << "idx " << idx << " too big " <<endl;
            }
            tally[idx]++;
        }   


        for(uint32_t i = start; i < end; i++) {
            auto prog = i-start;

            cout << tally[prog] << " ";
            if( prog % 128 == 127 ) {
                cout << endl;
            }

        }

        cout << endl << endl;



        exit(0);

        // RC_ASSERT(0);


        // assumes they are all the same (they should be, and are in test data)
        auto len = demod_buf_accumulate[0].size();
        len = 946;
        int shift_reg = 0;
        int sc_start = 27;
        int sc_end = sc_start-16;




        for(int i = found_i; i < len; i++) {
            for(int j = 0; j < 32; j++) {
                auto bit_value = demod_buf_accumulate[j][i];
                cout << bit_value << " ";
            }
            cout << endl;
        }


        // RC_ASSERT(0);






        for(int i = found_i; i < len; i++) {
            for(int j = sc_start; j > sc_end; j--) {
                // auto len = demod_buf_accumulate[j].size();
                auto bit_value = demod_buf_accumulate[j][i];

                // if(bit_value==0b01) {
                //     bit_value = 0x10;
                // } else if(bit_value == 0b10) {
                //     bit_value = 0x01;
                // }

                if(0) {
                    shift_reg = (shift_reg << 2) | bit_value;
                    cout << "[" << j << "][" << i << "] = " <<
                    HEX32_STRING(shift_reg) << endl;

                    if( j % 16 == 15 ) {
                        cout << "bit" << endl;
                    }
                }

                sss(bit_value, j, i);

            }
        }
        exit(0);

    });
#endif



#ifdef __ENABLEDasdf
    rc::check("check new vs old", []() {
        AirPacket ap;

        std::vector<uint32_t> d;

        uint32_t source = 0x10000000;

        // f0aafff
        new_subcarrier_data_sync(&d, source, 128);

        // cout << "d was: " << d.size() << endl;
        // for( auto w : d ) {
        //     cout << HEX32_STRING(w) << endl;
        // }

        ap.printWide(d, 128);
        cout << endl;


        std::vector<uint32_t> mydata;
        for(int i = 0; i < 128*2; i++) {
            auto val = source;
            if( i % 2 == 1) {
                // val = 0;
            }
            mydata.push_back(val);
            // mydata.push_back(i);
        }

        // mydata[1] = 0x20000000;
        // mydata[128*2-1] = 0x20000000;


        // cout << endl
        // << endl
        // << endl
        // << endl;

        // std::vector<uint32_t> out;
        // for(int i = 0; i < 128; i++) {
        //     out.push_back(0xcafebabe);
        // }

        // RC_ASSERT(0);




        std::vector<uint32_t> d2;


        bool newway = true;

        if(!newway) {

            new_subcarrier_data_load(d2, mydata, 128);
        } else {

            AirPacketOutbound tx;
            tx.setTestDefaults();

            d2 = tx.transform(mydata, 0);


        }

        ap.printWide(d2, 128);
        cout << endl;

        // cout << "d2 size was: " << d2.size() << endl;
        // for( auto w : d2 ) {
        //     cout << HEX32_STRING(w) << endl;
        // }


        // RC_ASSERT(
        //     d == d2
        //     );
        exit(0);
    });
#endif


#ifdef USE_WITH_AIRPACKET2
    rc::check("convert", []() {
        AirPacket ap;

        std::vector<uint32_t> d;

        uint32_t source = 0x10000000; //0xcafebabe & (~0x00000000);

        // f0aafff
        new_subcarrier_data_sync(&d, source, 128);

        // cout << "d was: " << d.size() << endl;
        // for( auto w : d ) {
        //     cout << HEX32_STRING(w) << endl;
        // }

        ap.printWide(d, 128);
        cout << endl;


        std::vector<uint32_t> mydata;
        for(int i = 0; i < 128*2; i++) {
            mydata.push_back(source);
            // mydata.push_back(i);
        }

        mydata[1] = 0x20000000;
        mydata[128*2-1] = 0x20000000;


        // cout << endl
        // << endl
        // << endl
        // << endl;

        // std::vector<uint32_t> out;
        // for(int i = 0; i < 128; i++) {
        //     out.push_back(0xcafebabe);
        // }

        // RC_ASSERT(0);


        std::vector<uint32_t> d2;
        new_subcarrier_data_load(d2, mydata, 128);
        ap.printWide(d2, 128);
        cout << endl;

        // cout << "d2 size was: " << d2.size() << endl;
        // for( auto w : d2 ) {
        //     cout << HEX32_STRING(w) << endl;
        // }


        // RC_ASSERT(
        //     d == d2
        //     );
        exit(0);
    });
#endif







#ifdef asldkf
    rc::check("something", []() {

        AirPacketOutbound tx;
        tx.setTestDefaults();
        // tx. = 2; //;


        // AirPacket a;

        std::vector<uint32_t> a;// = {1,2,3,4,5,6};

        for(uint32_t i = 0; i < 8 * 10; i++) {
            a.push_back(i + 0xface0000);
        }

        auto v = tx.transform(a);

        for( auto w : v ) {
            cout << HEX32_STRING(w) << endl;
        }


        RC_ASSERT(
            1
            );
    });
#endif











#ifdef asdfadsf


    rc::check("demod", []() {
        // #include "data/airpacket0.h"

        // cout << demod_buf_accumulate.size() << endl;

        auto found_i = 229-16;

        auto demod_buf_accumulate = get_data();

        for(int j = 0; false && j < 32; j++) {
            auto len = demod_buf_accumulate[j].size();
            len = 320;
            for(int i = found_i; i < len; i++) {
                auto bit_value = demod_buf_accumulate[j][i];
                // cout << "a[" << j << "][" << i << "] = " << bit_value << endl;
                sync_multiple_word(bit_value, j, i);

            }
        }



        // RC_ASSERT(0);


        // assumes they are all the same (they should be, and are in test data)
        auto len = demod_buf_accumulate[0].size();
        len = 946;
        int shift_reg = 0;
        int sc_start = 27;
        int sc_end = sc_start-16;




        for(int i = found_i; i < len; i++) {
            for(int j = 0; j < 32; j++) {
                auto bit_value = demod_buf_accumulate[j][i];
                cout << bit_value << " ";
            }
            cout << endl;
        }


        RC_ASSERT(0);






        for(int i = found_i; i < len; i++) {
            for(int j = sc_start; j > sc_end; j--) {
                // auto len = demod_buf_accumulate[j].size();
                auto bit_value = demod_buf_accumulate[j][i];

                // if(bit_value==0b01) {
                //     bit_value = 0x10;
                // } else if(bit_value == 0b10) {
                //     bit_value = 0x01;
                // }

                if(0) {
                    shift_reg = (shift_reg << 2) | bit_value;
                    cout << "[" << j << "][" << i << "] = " <<
                    HEX32_STRING(shift_reg) << endl;

                    if( j % 16 == 15 ) {
                        cout << "bit" << endl;
                    }
                }

                sss(bit_value, j, i);

            }
        }





        RC_ASSERT(0);
    });
#endif









#ifdef __ENABLEDasdf

// dead04a4
// dead04a6
// dead04a8
// dead04aa
// dead04ac
// dead04ae
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
// 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 2 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 





    rc::check("reverse engineer bit pattern", []() {
        AirPacket ap;

        auto unsorted = get_demodpacket0();

        std::sort (unsorted.begin(), unsorted.end());
        for(auto x : unsorted) {
            cout << HEX32_STRING(x) << endl;
        }

        uint32_t start = 0xdead0000;
        uint32_t end = start + 0x500;
        uint32_t range = end - start;

        std::vector<uint32_t> tally;
        tally.resize(range);

        // for(uint32_t i = start; i < end; i++) {
            // auto &ref = tally[i-start];

            // if() {

            // }
        // }

        // exit(0);

        for(auto x : unsorted) {
            auto idx = x - start;
            if( idx > 0xf0000 ) {
                cout << "idx " << idx << " too big " <<endl;
            }
            tally[idx]++;
        }


        for(uint32_t i = start; i < end; i++) {
            auto prog = i-start;

            cout << tally[prog] << " ";
            if( prog % 128 == 127 ) {
                cout << endl;
            }

        }

        cout << endl << endl;



        exit(0);

        // RC_ASSERT(0);


        // assumes they are all the same (they should be, and are in test data)
        // auto len = demod_buf_accumulate[0].size();
        // len = 946;
        // int shift_reg = 0;
        // int sc_start = 27;
        // int sc_end = sc_start-16;




        // for(int i = found_i; i < len; i++) {
        //     for(int j = 0; j < 32; j++) {
        //         auto bit_value = demod_buf_accumulate[j][i];
        //         cout << bit_value << " ";
        //     }
        //     cout << endl;
        // }


        // // RC_ASSERT(0);






        // for(int i = found_i; i < len; i++) {
        //     for(int j = sc_start; j > sc_end; j--) {
        //         // auto len = demod_buf_accumulate[j].size();
        //         auto bit_value = demod_buf_accumulate[j][i];

        //         // if(bit_value==0b01) {
        //         //     bit_value = 0x10;
        //         // } else if(bit_value == 0b10) {
        //         //     bit_value = 0x01;
        //         // }

        //         if(0) {
        //             shift_reg = (shift_reg << 2) | bit_value;
        //             cout << "[" << j << "][" << i << "] = " <<
        //             HEX32_STRING(shift_reg) << endl;

        //             if( j % 16 == 15 ) {
        //                 cout << "bit" << endl;
        //             }
        //         }

        //         sss(bit_value, j, i);

        //     }
        // }
        // exit(0);

    });
#endif


  return 0;
}


// MAPMOV_MODE_128_CENTERED_REDUCED
