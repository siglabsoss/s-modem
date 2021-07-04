#include <rapidcheck.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include "cpp_utils.hpp"
#include "driver/AirPacket.hpp"
#include "duplex_schedule.h"
#include "vector_helpers.hpp"

using namespace std;

#define __ENABLED

/*
 * Duplex schedule has need for a non power of 2 mod
 * We use a "cache" technique to save calculating almost every
 * mod value.  In the case we really do need to calculate mod, we just 
 * set a flag and re-calculate
 * This test covers both correctness, and performance.
 * If we are doing mod very frequently this test will fail
 *
 * 
 * This test requires many runs to be complete, so you can use
 *
 *   make -j6 test_duplex_1 && RC_PARAMS="max_success=400000" ./test_duplex_1
 *
 */

int main() {

    bool pass = true;

    pass &= rc::check("Test Cached Phase Calculation", []() {

        duplex_timeslot_t duplex;
        
        duplex.role = DUPLEX_ROLE_RX;
        duplex.lt_period = 5;
        duplex.lt_phase = 0;
        duplex.lt_target = 0;
        duplex.lt_target_valid = false;
        duplex.lifetime_p = 0;

        bool print = false;

        
        // start value
        uint32_t lifetime = *rc::gen::inRange(0u, 0xffffffu);

        const uint32_t run_time = 128*30;

        std::vector<bool> expensive;

        if( print ) {
            cout << "starting at " << lifetime << "\n";
        }

        
        for(uint32_t i = 0; i < run_time; i++) {
            // duplex_progress = lifetime % DUPLEX_FRAMES;
            
            bool was_expensive = duplex_calculate_phase(&duplex, lifetime);
            if( print ) {
                cout << std::endl;
            }

            expensive.push_back(was_expensive);

            uint32_t ideal_phase = (lifetime / DUPLEX_FRAMES) % duplex.lt_period;

            RC_ASSERT(duplex.lt_phase == ideal_phase);
            
            
            if( i == 333 ) {

                uint32_t bump = *rc::gen::inRange(0u, 0xfffu);
                bool add = true;

                if( *rc::gen::inRange(0u, 0x2u) ) {
                    lifetime += bump;
                } else {
                    lifetime -= bump;
                    add = false;
                }

                if(print) {
                    if( add ) {
                        cout << "adding ";
                    } else {
                        cout << "subbing ";
                    }
                    cout << bump << " at i: " << i << " lifetime: " << lifetime << "\n";
                }
            }
            
            lifetime++;
        }

        bool e_prev = false;
        uint32_t e_sequential = 0;
        uint32_t e_total = 0;

        for(size_t j = 0; j < expensive.size(); j++) {
            bool e = expensive[j];
            if( e ) {
                e_total++;
            }

            if( e_prev && e ) {
                e_sequential++;
            }

            e_prev = e;
        }

        if( print ) {
            cout << "e total       : " << e_total << "\n";
            cout << "e e_sequential: " << e_sequential << "\n";
            cout << "\n";
        }

        RC_ASSERT(e_sequential < 3);

    });


    /// assumptions: DUPLEX_ROLE_TX_0 + n is a radio (linear order)
    /// this is not randomzied, does exhaustive search
    /// 
    /// The purpose of this is to test that calls to get_duplex_mode()
    /// and calls to duplex_dl_pilot_transmitter() return matching data
    /// (they are not calculated from the same source for performance)
    pass &= rc::check("duplex_dl_pilot_transmitter sanity", []() {

        // same range as if we did progress = i % 128
        // uint32_t progress = *rc::gen::inRange(0u, (unsigned)DUPLEX_FRAMES);


        const unsigned radio_c = 4; // count

        // [0] is radio
        // [0] is schedule progress
        std::vector<std::vector<uint32_t>> allmodes;
        allmodes.resize(radio_c);


        // loop over and get a vector of vector for the transmitters
        // this should be refactored and used in other tests

        for(uint32_t j = 0; j < radio_c; j++) {

            duplex_timeslot_t oneduplex;
            init_duplex(&oneduplex, DUPLEX_ROLE_TX_0 + j );

            for(uint32_t i = 0; i < DUPLEX_FRAMES; i++) {
                // if you don't know what to pass to get_duplex_mode(), copy the second param
                const uint32_t duplex_mode = get_duplex_mode(&oneduplex, i, i);
                allmodes[j].push_back(duplex_mode);
            }
        }

        // modes that must match if we got non zero
        const std::vector<uint32_t> answer = {
            DUPLEX_SEND_DL_PILOT_0,
            DUPLEX_SEND_DL_PILOT_1,
            DUPLEX_SEND_DL_PILOT_2,
            DUPLEX_SEND_DL_PILOT_3};

        // check all phases for a non zero result, then verify afainst mode
        for(uint32_t i = 0; i < DUPLEX_FRAMES; i++) {
            auto mode = duplex_dl_pilot_transmitter(i);

            // if we got a valid index out of this
            // we need to search
            bool must_be_in_zone = mode != -1;

            // cout << "i " << i << " must be in zone " << must_be_in_zone << "\n";

            if( must_be_in_zone ) {

                bool ok = false;
                for(const auto radio : allmodes) {
                    const auto mode = radio[i];
                    if(VECTOR_FIND(answer, mode)) {
                        ok = true;
                    }
                }
                RC_ASSERT(ok);

            }

        }
    });


    // not a real check
    pass &= rc::check("duplex_duty_per_radio print", []() {

        std::vector<uint32_t> a = {DUPLEX_ROLE_RX, DUPLEX_ROLE_TX_0};

        for(const auto role : a ) {
            duplex_timeslot_t dup;
            init_duplex(&dup, role);


            uint32_t res0 = duplex_duty_per_radio(&dup, false);

            // cout << "Res0 " << res0 << "\n";


        }



        // same range as if we did progress = i % 128
        // uint32_t progress = *rc::gen::inRange(0u, (unsigned)DUPLEX_FRAMES);
    });

        // not a real check
    pass &= rc::check("duplex_tx_map_t print", []() {

        std::vector<uint32_t> a = {DUPLEX_ROLE_RX, DUPLEX_ROLE_TX_0};

        for(const auto role : a ) {
            duplex_timeslot_t dup;
            init_duplex(&dup, role);

            
            bool map[DUPLEX_FRAMES];


            duplex_build_tx_map(&dup, map, false);

            for(uint32_t i = 0; i < DUPLEX_FRAMES; i++) {
                // cout << (map[i]?1:0) << " " << "\n";
                (void)map[i];
            }

            // cout << "Res0 " << res0 << "\n";
            // cout << "\n\n\n";


        }



        // same range as if we did progress = i % 128
        // uint32_t progress = *rc::gen::inRange(0u, (unsigned)DUPLEX_FRAMES);
    });




    pass &= rc::check("duplex_duty_frame_done in wrap", []() {

        // std::vector<uint32_t> a = {DUPLEX_ROLE_RX, DUPLEX_ROLE_TX_0};
        std::vector<uint32_t> a = {DUPLEX_ROLE_TX_0};

        const bool print = false;

        for(const auto role : a ) {
            duplex_timeslot_t dup;
            init_duplex(&dup, role);

            const uint32_t resTx = duplex_duty_per_radio(&dup, false); // 85
            
            bool map[DUPLEX_FRAMES];
            duplex_build_tx_map(&dup, map, false);


            uint32_t start_lifetime = *rc::gen::inRange(43u, 49u);
            uint32_t duration_frames = *rc::gen::inRange(0u, 800u);
            uint32_t linear_end = start_lifetime + duration_frames;

            uint32_t intm = (duration_frames+(start_lifetime-43));

            bool expect_wrap = intm > resTx;


            if( print ) {
                cout << "intm: " << intm << " ";
                cout << "start_lifetime: " << start_lifetime << " ";
                cout << "duration_frames: " << duration_frames << " ";
                cout << "linear_end: " << linear_end << " ";
                cout << "expect_wrap: " << (int)expect_wrap << "\n";
            }


            uint32_t res0 = duplex_duty_frame_done(start_lifetime, duration_frames, map) - start_lifetime;

            if( expect_wrap ) {
                if( print ) {
                    cout << "must not match:\n";
                }
                RC_ASSERT(res0 != duration_frames);
            } else {
                if( print ) {
                    cout << "must match:\n";
                }
                RC_ASSERT(res0 == duration_frames);
            }

            if( print ) {
                cout << "Res0 " << res0 << "\n";
            }

        }



        // same range as if we did progress = i % 128
        // uint32_t progress = *rc::gen::inRange(0u, (unsigned)DUPLEX_FRAMES);
    });



    /// test that adding a full duty cycles worth of frames (85) gives us a result
    /// that is exactly 128 frames larger
    pass &= rc::check("duplex_duty_frame_done add duty cycle gives full schedule", []() {

        // std::vector<uint32_t> a = {DUPLEX_ROLE_RX, DUPLEX_ROLE_TX_0};
        std::vector<uint32_t> a = {DUPLEX_ROLE_TX_0};

        const bool print = false;

        for(const auto role : a ) {
            duplex_timeslot_t dup;
            init_duplex(&dup, role);

            const uint32_t resTx = duplex_duty_per_radio(&dup, false); // 85
            
            bool map[DUPLEX_FRAMES];
            duplex_build_tx_map(&dup, map, false);


            uint32_t start_lifetime = *rc::gen::inRange(43u, 128u); // same range as i % 128

            uint32_t dead_start = 0;
            if( start_lifetime < 43 ) {
                dead_start = 43 - start_lifetime;
            }

            if( print ) {
            //     // cout << "intm: " << intm << " ";
                cout << "dead_start: " << dead_start << "\n";
                cout << "start_lifetime: " << start_lifetime << " ";
            }

            uint32_t duration_frames = *rc::gen::inRange(1u, 2000u);




            if( print ) {
                // cout << "intm: " << intm << " ";
                cout << "start_lifetime: " << start_lifetime << " ";
                cout << "duration_frames: " << duration_frames << " ";
                // cout << "linear_end: " << linear_end << " ";
                // cout << "expect_wrap: " << (int)expect_wrap << "\n";
            }


            uint32_t res0 = duplex_duty_frame_done(start_lifetime, duration_frames, map);
            uint32_t res1 = duplex_duty_frame_done(start_lifetime, duration_frames + resTx, map);

            RC_ASSERT((res0 + DUPLEX_FRAMES) == res1);


            // cout << "Res0 " << res0 << "\n";
            // cout << "Res1 " << res1 << "\n";

        }



        // same range as if we did progress = i % 128
        // uint32_t progress = *rc::gen::inRange(0u, (unsigned)DUPLEX_FRAMES);
    });



    /// test that adding a full duty cycles worth of frames (85) gives us a result
    /// that is exactly 128 frames larger
    pass &= rc::check("duplex_duty_frame_done large values", []() {

        // std::vector<uint32_t> a = {DUPLEX_ROLE_RX, DUPLEX_ROLE_TX_0};
        std::vector<uint32_t> a = {DUPLEX_ROLE_TX_0};

        const bool print = false;

        for(const auto role : a ) {
            duplex_timeslot_t dup;
            init_duplex(&dup, role);

            const uint32_t resTx = duplex_duty_per_radio(&dup, false); // 85
            
            bool map[DUPLEX_FRAMES];
            duplex_build_tx_map(&dup, map, false);

            uint32_t bump = DUPLEX_FRAMES * (*rc::gen::inRange(0u,8u)); // same range as i % 128


            uint32_t start_lifetime = *rc::gen::inRange(bump+43u, bump+60u); // same range as i % 128

            // if( print ) {
            // //     // cout << "intm: " << intm << " ";
            //     // cout << "dead_start: " << dead_start << "\n";
            //     cout << "start_lifetime: " << start_lifetime << " ";
            // }

            uint32_t duration_frames = *rc::gen::inRange(1u, 50u);
            uint32_t linear_end = start_lifetime + duration_frames;




            if( print ) {
                // cout << "intm: " << intm << " ";
                cout << "start_lifetime: " << start_lifetime << " ";
                cout << "duration_frames: " << duration_frames << " "<< "\n";;
                // cout << "linear_end: " << linear_end << " ";
                // cout << "expect_wrap: " << (int)expect_wrap << "\n";
            }


            uint32_t res0 = duplex_duty_frame_done(start_lifetime, duration_frames, map);
            // uint32_t res1 = duplex_duty_frame_done(start_lifetime, duration_frames + resTx, map);

            RC_ASSERT((res0)==(linear_end));


            // cout << "Res0 " << res0 << "\n";
            // cout << "Res1 " << res1 << "\n";

        }



        // same range as if we did progress = i % 128
        // uint32_t progress = *rc::gen::inRange(0u, (unsigned)DUPLEX_FRAMES);
    });




    return !pass;
}

