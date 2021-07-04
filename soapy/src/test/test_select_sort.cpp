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



void _selectionSort(uint32_t* const a, const unsigned n) {
    unsigned i, j;
    uint32_t min, temp;
    if( n == 0 ) {
       return; // nothing to sort
    }

    for (i = 0; i < n - 1; i++) {
        min = i;
        for (j = i + 1; j < n; j++) {
            if (a[j] < a[min]) {
                min = j;
            }
        }

        if(i == min) {
            continue;
        }

        temp = a[i];
        a[i] = a[min];
        a[min] = temp;
    }
}


void generate_input_data(
    std::vector<uint32_t> &input_data,
    const uint32_t data_length,
    const uint32_t low = 0u,
    const uint32_t high = 0xffffffffu) {
    for(uint32_t i = 0; i < data_length; i++) {
        const uint32_t a_word = *rc::gen::inRange(low, high);
        input_data.push_back(a_word);
    }

}




int main() {

    bool pass = true;

    pass &= rc::check("Test sort", []() {

        std::vector<uint32_t> original;

        const unsigned data_length = *rc::gen::inRange(0u, 256u);

        // // calculate a split point
        // const unsigned split = *rc::gen::inRange(10u, data_length);


        generate_input_data(original, data_length, 0, 15);

        RC_ASSERT(data_length == original.size());


        std::vector<uint32_t> std_sort = original;

        std::sort(std_sort.begin(), std_sort.end());


        uint32_t onstack[original.size()];

        for(unsigned i = 0; i < original.size(); i++) {
            onstack[i] = original[i];
        }

        _selectionSort(onstack, original.size());


        std::vector<uint32_t> back_as_vector;

        for(unsigned i = 0; i < original.size(); i++) {
            back_as_vector.push_back(onstack[i]);
        }

        RC_ASSERT(back_as_vector == std_sort);



        // proof that std::sort works, not needed
        // uint32_t prev = 0;
        // for(unsigned i = 0; i < std_sort.size(); i++) {
        //     RC_ASSERT(prev <= std_sort[i]);
        //     prev = std_sort[i];
        // }





        // for(const auto w : original ) {
        //     cout << HEX32_STRING(w) << "\n";
        // }

     });

    // pass &= rc::check("Test op_sel", []() {

    //     const uint32_t data = *rc::gen::inRange(0u, 0xffffffffu);

    //     cout << HEX32_STRING(data) << "\n";

    //     const uint32_t dmode = (data >> 16) & 0xff; // 8 bits
    //     const uint32_t data_16 = data & 0xffff; // 16 bits

    //     const uint32_t op   = dmode&0x7;        // 3 bits
    //     const uint32_t high = (dmode&0x8) >> 3; // 1 bit
    //     const uint32_t sel  = (dmode&0xf0)>>4; // 4 bits

    //     const uint32_t op_sel = dmode&(0x7|0xf0); // op and sel put together


    //     const uint32_t op_sel2 = op|(sel<<4);

    //     RC_ASSERT(op_sel == op_sel2);

    //  });


    return !pass;
}
