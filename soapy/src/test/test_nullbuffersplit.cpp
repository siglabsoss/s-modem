#include <rapidcheck.h>
#include "driver/StatusCode.hpp"
#include "common/NullBufferSplit.hpp"



#include <vector>
#include <algorithm>
#include <iostream>
#include <unistd.h> // for usleep

using namespace std;

int main() {

    rc::check("basic", []() {

        size_t callbacks = 0;

        std::function<void(std::string)> cb = [&callbacks](std::string parsed) {
            std::cout << "cb len " <<  parsed.size() << ": " << parsed <<  std::endl;

            // for( auto x : parsed ) {
            //     cout << (unsigned) x << endl;
            // }
            RC_ASSERT(parsed == "ABCD");
            callbacks++;
        };


        NullBufferSplit *split = new NullBufferSplit(cb);

        // split->debug();

        std::string load("ABCD");
        split->addData(load.c_str(), load.size());

        unsigned char nullc = (unsigned char)0;

        split->addData(&nullc, 1);
        split->addData(&nullc, 1);
        split->addData(load.c_str(), load.size());
        split->addData(&nullc, 1);

        RC_ASSERT(callbacks == 2u);


        delete split;

        // RC_ASSERT(error == 0);
    });


    return 0;
}

