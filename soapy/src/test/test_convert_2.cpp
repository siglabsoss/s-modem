#include <rapidcheck.h>
#include "driver/AirPacket.hpp"
#include "common/SafeQueue.hpp"
#include "common/convert.hpp"
#include "driver/VerifyHash.hpp"

// #include "schedule.h"


#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"
#include "../data/packet_vectors.hpp"

using namespace std;


int main() {

    bool pass = true;

     pass &= rc::check("Test Char", []() {


        std::vector<std::vector<unsigned char>> all = {v1,v2,v3,v4,v5,v6,v7,v8,v9,v10,v11,v12};

        auto rcopy = v12;

        const auto bump = *rc::gen::inRange(0u, 140u);

        rcopy[3] += bump;
        for(int i = 0; i < bump; i++) {
            const unsigned char c = *rc::gen::inRange(0u, 256u); // not inclusive
            rcopy.push_back(c);
        }

        all.push_back(rcopy);

        int idx = 0;
        for(auto x : all) {


            // cout << "idx: " << idx << endl;

            std::vector<uint32_t> constructed1 = charVectorPacketToWordVector(x);
            // cout << HEX32_STRING(constructed1[0]) << endl;

            int erase;
            auto got1 = packetToBytes(constructed1, erase);

            RC_ASSERT(got1 == x);
            idx++;
        }



        

        // std::vector<uint32_t> constructed2 = charVectorPacketToWordVector(v2);


        // auto got2 = packetToBytes(constructed2, erase);

        // RC_ASSERT(got2 == v2);

        // auto packetToBytes;
      
    });

  return !pass;
}

