#include <rapidcheck.h>
#include "driver/AirPacket.hpp"
#include "common/SafeQueue.hpp"
#include "common/convert.hpp"
#include "driver/VerifyHash.hpp"
#include "FileUtils.hpp"

// #include "schedule.h"


#include <algorithm>
#include <iostream>
#include <iomanip>
#include <vector>
#include "cpp_utils.hpp"
#include "../data/packet_vectors.hpp"

#include "random.h"

using namespace std;
using namespace siglabs::file;



void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length) {
    for(int i = 0; i < data_length; i++) {
        uint32_t a_word = *rc::gen::inRange(0u, 0xffffffffu);
        input_data.push_back(a_word);
    }

}


// xorshift32 needs an initial value.  this value should be anything, but must not be 0
const uint32_t initial = 1;


int main() {

    bool pass = true;

    // xorshift32 should be able to operate 2 ways
    //  1) hash the data all at once
    //  2) hash the data in pieces, where the pieces can be any size
    // xorshift32 must get the same result for the same data using method 1 and 2
    pass &= rc::check("Test whole vs test piece", []() {

        std::vector<uint32_t> words;

        const unsigned data_length = *rc::gen::inRange(10u, 256u);

        // calculate a split point
        const unsigned split = *rc::gen::inRange(10u, data_length);


        generate_input_data(words, data_length);

        RC_ASSERT(data_length == words.size());

        uint32_t hash_at_once = xorshift32(initial, words.data(), words.size());


        uint32_t hash_part_a;
        uint32_t hash_part_b;

        // we calculate in 2 pieces here (but n pieces would work)
        // we pass the initial value the first time
        // but we pass the hash_part_a as the initial value for the second piece

        hash_part_a = xorshift32(initial, words.data(), split);
        hash_part_b = xorshift32(hash_part_a, words.data()+split, data_length-split);

        // cout << hash_part_b << endl;

        RC_ASSERT(hash_at_once == hash_part_b);

     });


    return !pass;
}


void example() {

    std::vector<uint32_t> words = {0xdeadbeef, 0, 1, 2, 3};

    // uint32_t state[2];
    // state[0] = 0;
    // state[1] = 0;

    // hash_words(state, words.data(), words.size());

    uint32_t res0 = 0;
    uint32_t res1 = 0;
    uint32_t res2 = 0;

    res0 = xorshift32(0, words.data(), words.size());

    cout << HEX32_STRING(res0) << endl;


    res1 = xorshift32(0,    words.data(),  2);
    res2 = xorshift32(res1, words.data()+2, 3);

    cout << HEX32_STRING(res2) << endl;

}

