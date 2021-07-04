#include <rapidcheck.h>
#include "driver/AirPacket.hpp"

// #include "schedule.h"

#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"
#include "random.h"

// instead of including data
// std::vector<std::vector<uint32_t>> get_data();
// std::vector<uint32_t> get_demodpacket0();
// std::vector<uint32_t> get_demodpacket1();
std::vector<uint32_t> get_slicedpacket0();
std::vector<std::vector<uint32_t>> get_airpacket18();

using namespace std;

#define __ENABLED

int global_correct;


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

void proto_trimSlicedData(sliced_buffer_t &sliced_words, unsigned erase) {
    std::vector<std::vector<uint32_t>> &erase_from = sliced_words;
    // cout << "TRIMMING SLICED for " << erase << endl;
    // cout << "before " << erase_from[0].size() << endl;
    erase_from[0].erase(erase_from[0].begin(), erase_from[0].begin()+erase);
    erase_from[1].erase(erase_from[1].begin(), erase_from[1].begin()+erase);
    // cout << " after " << erase_from[0].size() << endl;
}

void proto_remove_subcarrier_data2(std::vector<std::vector<std::pair<uint32_t, uint32_t>>> &erase_from, unsigned erase) {
    for(unsigned j = 0; j < erase_from.size(); j++) {
        erase_from[j].erase(erase_from[j].begin(), erase_from[j].begin()+erase);
    }
}


#define DATA_TONE_NUM 32
void proto_demodGotPacket(std::vector<uint32_t> &p, uint32_t got_at) {
    cout << "Found packet at " << got_at << endl;
    for( auto x : p ) {
        cout << HEX32_STRING(x) << endl;
    }

    if( p[0] == 0x54000045 && p[20] == 0x37363534 ) {
        global_correct = 1;
    }
}

std::tuple<bool, uint32_t> proto_check_subcarrier(sliced_buffer_t sliced_words) {

    AirPacketInbound airRx;
    airRx.setTestDefaults();
    airRx.data_tone_num = DATA_TONE_NUM;

    int times_sync_word_detected = 0;

    std::vector<uint32_t> potential_words;
    bool found_something;
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


    if( found_something ) {
        times_sync_word_detected++;

        std::vector<uint32_t> potential_words_again;


        /// replace above demodulate with this
        uint32_t force_detect_index = found_at_index+(32*8);
        cout << "force_detect_index " << force_detect_index << endl;
        uint32_t found_at_frame_again;
        unsigned found_at_index_again;
        unsigned do_erase_sliced;
        airRx.demodulateSliced(
            sliced_words,
            potential_words_again,
            found_something,
            found_at_frame_again,
            found_at_index_again,
            do_erase_sliced,
            force_detect_index,
            found_length);

        // hack use first found at time
        proto_demodGotPacket(potential_words_again, found_at_frame);
    
        // print_found_demod_final(found_at_index);


       if(do_erase_sliced) {
            cout << "INSIDE ERASE!!!!!!!!1" << endl;
            proto_trimSlicedData(sliced_words, do_erase_sliced);
        }
    } else {
        // HACK fix this erase
        if(sliced_words[0].size() > 40000) {
            // 2000 must be a multiple of 8
            proto_trimSlicedData(sliced_words, sliced_words[0].size()-30000);
        }
    }


    if(do_erase) {
        // trickle_erase(storage, erase);
        // proto_remove_subcarrier_data2(demod_buf_accumulate, do_erase);
    }

    return std::make_tuple(found_something, found_length);

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


int main() {

    if(0) {

    cout << "started" << endl;
    std::vector<uint32_t> sliced0 = _file_read_hex("../src/data/slicedpacket18.hex");


    unsigned air_start = 309;
        unsigned air_start_frame = 42700753;
    unsigned sliced_packet_start = 42777585;
    // unsigned 

    /*
got 8 words s 42777553
ffffffff
ffffffff
ffffffff
ffffffff
00000000
00000000
00000000
0000c000
*/



    for(auto i = 0; i < 16; i++) {
        auto new_slice  = get_bit_pair_sliced(sliced0, 0, i, 128);
        cout << "new_slice " << new_slice << endl;
    }

    for(auto i = 0; i < 4; i++) {
        for( auto j = 0; j < 128; j++ ) {
            auto extract2 = extract_single2(sliced0, j, i*16, 128);
            cout << "extract2 " << HEX32_STRING(extract2) << endl;
        }
    }


    AirPacket ap;

    // cout << demod_buf_accumulate.size() << endl;

    auto found_i = 341;


    auto demod_buf_accumulate = get_airpacket18();
}



    if(0) {
    cout << "started" << endl;
    std::vector<uint32_t> sliced0 = _file_read_hex("../src/data/slicedpacket19.hex");


    // unsigned air_start = 309;
    // unsigned air_start_frame = 42700753;
    unsigned sliced_packet_start = 50047985;
    // unsigned 

    /*
got 8 words s 42777553
ffffffff
ffffffff
ffffffff
ffffffff
00000000
00000000
00000000
0000c000
*/

    bool search = false;

    if(search) {

        for(auto i = 0; i < 16; i++) {
            auto new_slice  = get_bit_pair_sliced(sliced0, 0, i, 128);
            cout << "new_slice " << new_slice << endl;
        }

        for(auto i = 0; i < 4; i++) {
            for( auto j = 0; j < 128; j++ ) {
                auto extract2 = extract_single2(sliced0, j, i*16, 128);
                cout << "extract2 " << HEX32_STRING(extract2) << endl;
            }
        }

    }
    uint32_t tword = ~sliced0[128];

    uint32_t uword = 0;
    for(unsigned int i = 0; i < 16; i++) {
        uword = (uword << 2) | (tword>>(i*2))&0x3;
    }
 
    cout << HEX32_STRING(tword) << endl;
    cout << HEX32_STRING(uword) << endl;
}

    // AirPacket ap;

    // // cout << demod_buf_accumulate.size() << endl;

    // auto found_i = 341;


    // auto demod_buf_accumulate = get_airpacket18();



    // generation code for test_mover_8/override/fpgas/cs/cs11/c/src/cs21test_buffer.h
    if(0) {
        uint32_t r_state = 0x21343234;
        std::vector<uint32_t> pulls;

        for(unsigned int i = 0; i < 1024 * 8; i++) {
            uint32_t pull1 = xorshift32(0, &r_state, 1);
            pulls.push_back(pull1);
            r_state = pull1;
        }

        // for( auto x : pulls) {
        //     cout << HEX32_STRING(x) << endl;
        // }

        for(unsigned int i = 0; i < 1024 * 8; i++) {
            cout << "0x" << HEX32_STRING(pulls[i]) << ",";
            if( i % 1024 == 1023) {
                for(unsigned int j = 0; j < 16; j++ ) {
                    cout << HEX32_STRING(0) << ",";
                }
                cout << endl;
            }
        }


        // simple_random_seed(0x3);
    }

    // FIXME this can be left on
    if(0) {
        cout << "started 20" << endl;
        std::vector<uint32_t> sliced0 = _file_read_hex("../src/data/slicedpacket20.hex");

        bool search = false;

        if(search) {

            for(auto i = 0; i < 16; i++) {
                auto new_slice  = get_bit_pair_sliced(sliced0, 0, i, 128);
                cout << "new_slice " << new_slice << endl;
            }

            for(auto i = 0; i < 4; i++) {
                for( auto j = 0; j < 128; j++ ) {
                    auto extract2 = extract_single2(sliced0, j, i*16, 128);
                    cout << "extract2 " << HEX32_STRING(extract2) << endl;
                }
            }

        }

        for(auto i = 0; i < 32; i++) {
            cout << "idx: " << i << " tfm " << transform_words_128(i) << endl;
            auto w = extract_single3(sliced0, i);
            cout << "w: " << HEX32_STRING(w) << endl;
        }

        uint32_t ideal;

        uint32_t incorrect = 0;

        for(auto i = 128; i < 128+20000; i++) {
            // cout << "idx: " << i << " tfm " << transform_words_128(i) << endl;
            auto lookup_i = transform_words_128(i);
            //auto lookup_i = i;//transform_words_128(i);
            auto w = extract_single3(sliced0, lookup_i);
            cout << "w: " << HEX32_STRING(w) << endl;

            ideal = 0xdead0000 + (i-128);

            if( w != ideal ) {
                cout << "FAILED, incrrect value" << endl;
                // exit(1);
                incorrect++;
            }

        }

        cout << "Found " << incorrect << " failed words" << endl;

        // uint32_t tword = ~sliced0[128];

        // cout << "t word " << HEX32_STRING(tword) << endl;

        // uint32_t uword = 0;
        // for(unsigned int i = 0; i < 16; i++) {
        //     uint32_t bits = (tword>>(i*2))&0x3;
        //     bits = ((bits&0x1)<<1) | ((bits&0x2)>>1);
        //     uword = (uword << 2) | bits;
        // }
     
        // // cout << HEX32_STRING(tword) << endl;
        // cout << HEX32_STRING(uword) << endl;
    }












if(0) {

    AirPacketInbound air;

    std::vector<std::vector<uint32_t>> as_iq;

     as_iq.resize(2);

     const std::vector<uint32_t> sliced2 = _file_read_hex("../src/data/slicedpacket20.hex");

     //

     unsigned out = 0;
     uint32_t counter = 0x4000;
     for(unsigned int i = 0; i < sliced2.size(); i+=8) {
         std::vector<uint32_t> prep = {
            counter, 
            sliced2[i+0],
            sliced2[i+1],
            sliced2[i+2],
            sliced2[i+3],
            sliced2[i+4],
            sliced2[i+5],
            sliced2[i+6],
            sliced2[i+7]
        };
        counter++;

         _handle_load_vector_vector(as_iq, prep.data(), prep.size());
     }

     // for(auto i = 0; i < as_iq[0].size(); i++ ) {
     //    cout << HEX32_STRING(as_iq[0][i]) << " " << HEX32_STRING(as_iq[1][i]) << endl;
     // }
        // for( auto x : as_iq[0] ) {
        //     cout << HEX32_STRING(x) << endl;
        // }

    bool force = true;
    uint32_t force_to = 128;

    std::vector<uint32_t> potential_words;
    bool found_something;
    unsigned do_erase;
    uint32_t found_at_frame;
    unsigned found_at_index; // found_i
    uint32_t force_length = 1000;


    air.demodulateSliced(
        as_iq,
        potential_words,
        found_something,
        found_at_frame,
        found_at_index,
        do_erase,
        force,
        force_to,
        force_length
        );

    assert(found_something);


    for(unsigned i = 0; i < 16; i++) {
        cout << HEX32_STRING(potential_words[i]) << endl;
    }

    cout << "-----------" << endl;


}



if(0) {
    std::vector<uint32_t> sliced6 = _file_read_hex("../src/data/slicedpacket20.hex");
    for(int j = 0; j < 128*2; j++) {
        auto w = extract_vertical_from_sliced(sliced6, j, 0, 128);
        cout << "w: " << HEX32_STRING(w) << endl;
    }
}



if(0) {
    std::vector<uint32_t> sliced6 = _file_read_hex("../src/data/slicedpacket20.hex");
    std::vector<std::vector<uint32_t>> as_iq = generate_counter_for_sliced_buf(sliced6, 400);
    int offset = 0;

    for(int j = 0; j < 128*2; j++) {
        auto w = extract_vertical_from_sliced(as_iq[1], 0, 0, 128);
        cout << "w: " << HEX32_STRING(w) << endl;
    }
}

// test if we can print stuff
if(0) {
    std::vector<uint32_t> sliced7 = _file_read_hex("../src/data/slicedpacket21.hex");
    std::vector<std::vector<uint32_t>> as_iq = generate_counter_for_sliced_buf(sliced7, 400);
    int offset = 3;

    for(int k = 0; k < 16*2; k+=16) {
        for(int j = 0; j < 128; j++) {
            auto w = extract_vertical_from_sliced(as_iq[1], j, offset+k, 128);
            cout << "w: " << HEX32_STRING(w) << endl;
        }
    }
}

// test demodulateSliced solo 
if(0) {
    AirPacketInbound airRx;
    airRx.setTestDefaults();
    airRx.data_tone_num = DATA_TONE_NUM;

    std::vector<uint32_t> sliced7 = _file_read_hex("../src/data/slicedpacket21.hex");
    std::vector<std::vector<uint32_t>> as_iq = generate_counter_for_sliced_buf(sliced7, 400);
    int offset = 3;

    uint32_t found_length = 32;

    std::vector<uint32_t> potential_words_again;


    /// replace above demodulate with this
    bool found_something;
    uint32_t force_detect_index = offset*8 + 16*2*8;
    cout << "force_detect_index " << force_detect_index << endl;
    uint32_t found_at_frame_again;
    unsigned found_at_index_again;
    unsigned do_erase_sliced;
    airRx.demodulateSliced(
            as_iq,
            potential_words_again,
            found_something,
            found_at_frame_again,
            found_at_index_again,
            do_erase_sliced,
            force_detect_index,
            found_length);

    for( auto x : potential_words_again) {
        cout << HEX32_STRING(x) << endl;
    }

    // for(int k = 0; k < 16*2; k+=16) {
    //     for(int j = 0; j < 128; j++) {
    //         auto w = extract_vertical_from_sliced(as_iq[1], j, offset+k, 128);
    //         cout << "w: " << HEX32_STRING(w) << endl;
    //     }
    // }
}



// test corrupting the header
if(0) {
    std::vector<uint32_t> sliced8 = _file_read_hex("../src/data/slicedpacket21.hex");
    std::vector<std::vector<uint32_t>> as_iq = generate_counter_for_sliced_buf(sliced8, 400);
    int offset = 3;

    int corrupt_offset = 1;

    as_iq[1][8*(16+offset)+corrupt_offset] ^= 0xFFFFFFFF;

    auto packed = proto_check_subcarrier(as_iq);

    auto found_something = std::get<0>(packed);
    auto found_length = std::get<1>(packed);

}



// exhaustive search to see if we can always read a clean header despite corruptions
if(0) {

    std::vector<uint32_t> sliced8 = _file_read_hex("../src/data/slicedpacket21.hex");
    

    rc::check("check vote circuit", [sliced8](unsigned din) {
        int offset = 3;

        std::vector<uint32_t> sliced9 = sliced8;

        std::vector<std::vector<uint32_t>> as_iq = generate_counter_for_sliced_buf(sliced9, 400);

        const unsigned corrupt_times = *rc::gen::inRange(0, 5);

        for( auto i = 0; i < corrupt_times; i++ ) {
            const unsigned corrupt_offset = *rc::gen::inRange(0, 127);

            const uint32_t corruption_flip = *rc::gen::inRange(0u, 0xffffffffu);

            as_iq[1][8*(16+offset)+corrupt_offset] ^= corruption_flip;
        }

        global_correct = 0;

        auto packed = proto_check_subcarrier(as_iq);

        auto found_something = std::get<0>(packed);
        auto found_length = std::get<1>(packed);

        RC_ASSERT(found_something);
        RC_ASSERT(found_length == 0x15);

        RC_ASSERT(global_correct == 1);


    });

}


// test if we can pad the front
if(1) {
    std::vector<uint32_t> sliced8 = _file_read_hex("../src/data/slicedpacket21.hex");
    

    rc::check("check vote circuit", [sliced8](unsigned din) {
        int offset = 3;

        std::vector<uint32_t> sliced9 = sliced8;

        const unsigned pad_front = *rc::gen::inRange(0, 127);

        for(unsigned i = 0; i < pad_front; i++) {
            for(unsigned j = 0; j < 8; j++) {
                const uint32_t word = *rc::gen::inRange(0u, 0xffffffffu);
                sliced9.insert(sliced9.begin(), word);
            }
        }

        std::vector<std::vector<uint32_t>> as_iq = generate_counter_for_sliced_buf(sliced9, 400);

        global_correct = 0;

        auto packed = proto_check_subcarrier(as_iq);

        auto found_something = std::get<0>(packed);
        auto found_length = std::get<1>(packed);

        RC_ASSERT(found_something);
        RC_ASSERT(found_length == 0x15);

        RC_ASSERT(global_correct == 1);


    });

}









if(0) {

    std::vector<uint32_t> sliced3 = _file_read_hex("../src/data/slicedpacket20.hex");
    unsigned limit_read = 8 * 40;
    sliced3.resize(limit_read);

    

    rc::check("compare transforms", [sliced3](unsigned din) {

        std::vector<uint32_t> sliced4;
        sliced4 = sliced3;


        AirPacketInbound air;

        // std::vector<uint32_t> sliced2 = sliced3;

        // for( x : sliced4 ) {
        //     cout << HEX32_STRING(x) << endl;
        // }
        const unsigned appends = *rc::gen::inRange(0, 15);

        // #ifdef asdfasdflkj
        cout << endl << "Inserting ";
        for(unsigned i = 0; i < appends; i++) {
            const unsigned pick = *rc::gen::inRange(0, 4);
            uint32_t word;
            switch(pick) {
                default:
                case 0:
                    word = 0x00000000;
                    break;
                case 1:
                    word = 0x55555555;
                    break;
                case 2:
                    word = 0xaaaaaaaa;
                    break;
                case 3:
                    word = 0xffffffff;
                    break;
            }

            cout << HEX32_STRING(word) << " ";

            for(unsigned j = 0; j < 8; j++ ) {
                sliced4.insert ( sliced4.begin() , word );
            }

        }
        cout << endl;
        // #endif

        // Inserting 00000000 aaaaaaaa 
        // Inserting aaaaaaaa 00000000 00000000 
        // Inserting 00000000 00000000 aaaaaaaa 



        // for(unsigned j = 0; j < 8; j++ ) {
        //     sliced4.insert ( sliced4.begin() , 0x00000000 );
        // }

        // for(unsigned j = 0; j < 8; j++ ) {
        //     sliced4.insert ( sliced4.begin() , 0x00000000 );
        // }

        // for(unsigned j = 0; j < 8; j++ ) {
        //     sliced4.insert ( sliced4.begin() , 0xaaaaaaaa );
        // }

        // sliced4.insert(sliced4.begin(), 0x00000000);
        // sliced4.insert(sliced4.begin(), 0xaaaaaaaa);


        // const unsigned pick = *rc::gen::inRange(0, 1024);
        

        std::vector<std::vector<uint32_t>> as_iq = generate_counter_for_sliced_buf(sliced4, 400);

        bool force = true;
        uint32_t force_to = 128;

        std::vector<uint32_t> potential_words;
        bool found_something;
        unsigned do_erase;
        uint32_t found_at_frame;
        unsigned found_at_index; // found_i
        uint32_t force_length = 1000;
        uint32_t found_length;
        air_header_t found_header;

        // cout << "enter" << endl;

        air.demodulateSlicedHeader(
            as_iq,
            potential_words,
            found_something,
            found_length,
            found_at_frame,
            found_at_index,
            found_header,
            do_erase
            );


        RC_ASSERT(found_something == true);

        cout << endl << endl << "found: " << found_at_frame << endl;
        
        RC_ASSERT(found_at_frame == 400+appends);

    });


}






    rc::check("compare transforms", [](unsigned din) {

        // AirPacket ap;

        TransformWords128 tt;

        // std::pair<bool, unsigned> 

        auto rett = tt.t(din);

        // is valid?
        RC_ASSERT(rett.first == true);

        RC_ASSERT(rett.second == transform_words_128(din));
    });


    const std::vector<uint32_t> sliced1 = _file_read_hex("../src/data/slicedpacket20.hex");
    rc::check("functions match", [sliced1]() {

        int i = *rc::gen::inRange(128, 128+20000);

        auto lookup_i = transform_words_128(i);
        // //auto lookup_i = i;//transform_words_128(i);
        auto w = extract_single3(sliced1, lookup_i);
        auto w2 = bitMangleSliced(sliced1[lookup_i]);

        RC_ASSERT(w == w2);
        // cout << "w: " << HEX32_STRING(w) << endl;
        // uint32_t ideal;
        // for(auto i = 128; i < 128+20000; i++) {
        //     // cout << "idx: " << i << " tfm " << transform_words_128(i) << endl;
        //     auto lookup_i = transform_words_128(i);
        //     //auto lookup_i = i;//transform_words_128(i);
        //     auto w = extract_single3(sliced1, lookup_i);
        //     cout << "w: " << HEX32_STRING(w) << endl;

        //     ideal = 0xdead0000 + (i-128);

        //     if( w != ideal ) {
        //         cout << "FAILED, incrrect value" << endl;
        //         // exit(1);
        //         // incorrect++;
        //     }

        // }


    });






  return 0;
}


// MAPMOV_MODE_128_CENTERED_REDUCED
