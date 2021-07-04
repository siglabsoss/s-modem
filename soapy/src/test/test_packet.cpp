#include <rapidcheck.h>
#include "driver/AirPacket.hpp"
#include <boost/functional/hash.hpp>
// #include "schedule.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"
#include "random.h"
#include "schedule.h"

// instead of including data
// std::vector<std::vector<uint32_t>> get_data();
// std::vector<uint32_t> get_demodpacket0();
// std::vector<uint32_t> get_demodpacket1();
// std::vector<uint32_t> get_slicedpacket0();
// std::vector<std::vector<uint32_t>> get_airpacket18();

using namespace std;

#define __ENABLED


unsigned get_bit_pair_sliced(const std::vector<uint32_t> &slc, unsigned subcarrier, unsigned time, unsigned enabled_subcarriers);


uint32_t extract_single(const std::vector<std::vector<uint32_t>> &data, int j, int i) {
    uint32_t demod_word = 0;

    for(int k = i; k < i+16; k++)
    {
        uint32_t demod_bits = data[j][k];
        demod_word = (demod_word<<2 | demod_bits);
    }
    return demod_word;

}

uint32_t extract_single2(const std::vector<uint32_t> &slc, int j, int i, unsigned enabled_subcarriers) {
    uint32_t demod_word = 0;

    for(int k = i; k < i+16; k++)
    {
        uint32_t demod_bits = get_bit_pair_sliced(slc, j, k, 128);
        // uint32_t demod_bits = data[j][k];
        demod_word = (demod_word<<2 | demod_bits);
    }
    return demod_word;
}

uint32_t extract_single3(const std::vector<uint32_t> &slc, int i) {

    uint32_t tword = ~slc[i];

    uint32_t uword = 0;
    for(unsigned int i = 0; i < 16; i++) {
        uint32_t bits = (tword>>(i*2))&0x3;
        bits = ((bits&0x1)<<1) | ((bits&0x2)>>1);
        uword = (uword << 2) | bits;
    }

    return uword;
}

// uint32_t bit_mangle_sliced(const uint32_t in) {

//     const uint32_t tword = ~in;

//     uint32_t uword = 0;
//     for(unsigned int i = 0; i < 16; i++) {
//         const uint32_t bits = (tword>>(i*2))&0x3;
//         const uint32_t bitrev = ((bits&0x1)<<1) | ((bits&0x2)>>1);
//         uword = (uword << 2) | bitrev;
//     }

//     return uword;
// }

#define SLICED_DATA_BEV_WRITE_SIZE_WORDS 9

static void _handle_load_vector_vector(std::vector<std::vector<uint32_t>>& sliced_words, const uint32_t *word_p, const size_t read_words) {

    for(size_t i = 0; i < read_words; i++) {

        if( i % SLICED_DATA_BEV_WRITE_SIZE_WORDS == 0 ) {
            // this branch enters 1 times for every OFDM frame

            // save the frame in index 0
            sliced_words[0].push_back(word_p[i]);
            sliced_words[0].push_back(0);
            sliced_words[0].push_back(0);
            sliced_words[0].push_back(0);
            sliced_words[0].push_back(0);
            sliced_words[0].push_back(0);
            sliced_words[0].push_back(0);
            sliced_words[0].push_back(0);
            // cout << "frame: " << word_p[i] << endl;
        } else {
            // this branch enters 8 times for every OFDM frame
            size_t j = (i % SLICED_DATA_BEV_WRITE_SIZE_WORDS) - 1;

            sliced_words[1].push_back(word_p[i]);

            // cout << "data: " << HEX32_STRING(word_p[i]) << " <- " << j << endl;
        }
    }
    assert(sliced_words[0].size() == sliced_words[1].size());

    // cout << r.sliced_words[0].size() << endl;
    // cout << r.sliced_words[1].size() << endl;

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

size_t transform_words_128(size_t in) {
    constexpr size_t enabled_subcarriers = 8;

    size_t div = in/enabled_subcarriers;

    size_t sub = (2*enabled_subcarriers)*div+(enabled_subcarriers-1);

    return sub - in;
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

bool switch_bits = true;
bool switch_shift = false;
bool invert_bits = true;


unsigned get_bit_pair_sliced(const std::vector<uint32_t> &slc, unsigned subcarrier, unsigned time, unsigned enabled_subcarriers) {
    unsigned words_per_frame = enabled_subcarriers / 16;
    unsigned start = time * words_per_frame;

    // 128 / 2 / 16

    unsigned choose_word = subcarrier / 16;
    int choose_bit;
    choose_bit = (subcarrier - (choose_word * 16)) * 2;

    if( switch_shift ) {
        choose_bit = 32 - choose_bit;
    } else {

    }

    // cout << "start " << start << " choose_word " << choose_word << " choose_bit " << choose_bit << endl;

    uint32_t mask = 0x3;

    uint32_t result = (slc[start + choose_word] >> choose_bit) & 0x3;

    if( switch_bits ) {
        if( result == 1 ) {
            // cout << "switch" << endl;
            result = 2;
        } else if( result == 2 ) {
            result = 1;
        }
    }

    if( invert_bits ) {
        result = (~result)&0x3;
    }

    return (unsigned) result;
}


void demodGotPacket(std::vector<uint32_t> &p, uint32_t got_at) {
    ///
    /// Got the packet
    /// you need put it in the tun at this point, it will be destroyed after this function exits
    ///
    cout << "Got Packet at frame " << got_at << " aka " << (got_at % SCHEDULE_FRAMES) << endl;
    // for(auto x : p) {
    //     cout << HEX32_STRING(x) << endl;
    // }

    bool is_equal = false;
   
    boost::hash<std::vector<unsigned char>> char_hasher;


    int erase = 0;
    while(erase != -1 && (!is_equal) ) {

        auto v1 = packetToBytes(p, erase);
        if( v1.size() && erase != -1) {
            cout << HEX32_STRING(p[0]) << endl;

            if(true) {
                std::size_t h = char_hasher(v1);
                // cout << "xxxxxxxxxxx Writing " << v1.size() << " bytes to radio estimate tun char hash " << HEX32_STRING(h) << endl;
                // auto retval = write(soapy->tun_fd, v1.data(), v1.size() );

                // if( retval < 0 ) {
                //     cout << "demodGotPacket() write !!!!!!!!!!!!!!!!!!!!!!!!!!! returned " << retval << endl;
                // }
            }

            if( erase > 0 ) {
                p.erase(p.begin(), p.begin()+erase);
            }

        } else {
            cout << "Erase was -1" << endl;
            cout << "v1.size() " << v1.size() << endl;
            cout << "erase actually was " << erase << endl;
            break;
            // there is a crash where we get stuck in here
            // but why? and how?
            // the break might fix this? but why
        }
        // break; // FIXME only gets first packet
    }

}


int main() {

    if(0) {

    cout << "started" << endl;
    std::vector<uint32_t> data0 = _file_read_hex("../src/data/datapacket0.hex");

    // for(auto i = 0; i < data0.size(); i++) {
    //     cout << HEX32_STRING(data0[i]) << endl;
    // }

    demodGotPacket(data0, 0);



    // for(auto i = 0; i < 16; i++) {
    //     auto new_slice  = get_bit_pair_sliced(sliced0, 0, i, 128);
    //     cout << "new_slice " << new_slice << endl;
    // }

    // for(auto i = 0; i < 4; i++) {
    //     for( auto j = 0; j < 128; j++ ) {
    //         auto extract2 = extract_single2(sliced0, j, i*16, 128);
    //         cout << "extract2 " << HEX32_STRING(extract2) << endl;
    //     }
    // }


    AirPacket ap;

    // cout << demod_buf_accumulate.size() << endl;

    auto found_i = 341;


    // auto demod_buf_accumulate = get_airpacket18();
    }

    if(0) {

        cout << "started" << endl;
        std::vector<uint32_t> data0 = _file_read_hex("../src/data/datapacket0.hex");

        for(auto i = 0; i < data0.size(); i+=375) {
            cout << HEX32_STRING(data0[i]) << endl;
        }

        // demodGotPacket(data0, 0);
    }



    rc::check("pull out of bound?", []() {
         // there are 20 packets in here, each with the same size of 375 words
        std::vector<uint32_t> data0 = _file_read_hex("../src/data/datapacket0.hex");
        const int packets = 20;
        const int size = 375;
        const int minpull = 4;

        int this_fill = (rand() % (packets - minpull)) + minpull;
        // cout << "debugFillBuffers() adding " << this_fill << " packets" << endl;

        for(int i = 0; i < this_fill; i++) {

            std::vector<uint32_t> packet;
            packet.assign(
                data0.begin() + (i * size),
                data0.begin() + ((i+1) * size)
            );

            // cout << HEX32_STRING(packet[0]) << endl;
            RC_ASSERT( packet[0] == 0xdc050045);

            RC_ASSERT( ((i+1) * size) < data0.size() );

            // soapy->txTun2EventDsp.enqueue(tun_packet);

        }

    });







  return 0;
}
