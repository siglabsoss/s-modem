#include <rapidcheck.h>

#include <vector>
#include <algorithm>
#include <iostream>
#include <unistd.h>

#include "schedule.h"
#include "duplex_schedule.h"
#include "ringbus.h"
#include "cpp_utils.hpp"

using namespace std;

#define PONG_BUFFERS (5)

int main() {

    if(0) {
        epoc_frame_t a;
        a.epoc = 2;
        a.frame = 0x400;

        uint64_t pure = schedule_to_pure_frames(a);

        cout << "pure: " << pure << "\n";
        exit(0);
    }

    // non testing example playground to understand how cs11 feeds back to s-modem
    // trying to understand if adding playground to detect if changing cs11 away 
    // from schedule will add any problems
    rc::check("CS11 counter feedback Example", []() {

        bool print1 = false; // print when sending each "ringbus"
        bool print2 = false; // print long output thing, each ringbus

        schedule_t _schedule;
        schedule_t* schedule = &_schedule;
        schedule_all_on(schedule);
        schedule->epoc_time = 0;


        std::vector<uint32_t> rb0;

        auto ring_block_send_eth = [&](const uint32_t a) {
            rb0.push_back(a);
        };

        std::vector<uint32_t> esst;

        epoc_frame_t epoc_estimated = {0,0};

        auto consumeContinuousEpoc = [&](const uint32_t word) {
            uint32_t cmd_type = word & 0xff000000;
            uint32_t cmd_data = word & 0x00ffffff;
            switch(cmd_type) {
                case TX_PROGRESS_REPORT_PCCMD:
                    epoc_estimated.frame = cmd_data;
                    // cout << "Continuous Progress: " << epoc_estimated.frame << ", " << epoc_estimated.frame/512 << endl;
                    break;
                case TX_EPOC_REPORT_PCCMD:
                    epoc_estimated.frame = 0;
                    epoc_estimated.epoc = cmd_data;
                    // epoc_valid = true; // gets set to false very infrequently
                    cout << "Continuous Epoc Second: " << epoc_estimated.epoc << endl;
                    break;
                default:
                    cout << "illegal type passed to consumeContinuousEpoc" << endl;
                    break;
            }

            esst.push_back(epoc_estimated.epoc);
            esst.push_back(epoc_estimated.frame);
        };


        uint32_t duplex_progress = 0;
        uint32_t lifetime_32 = 1; // this shouldn't be 0, as schedule_get_timeslot2() has a bug where it will bump epoc by 1 erroneously
        uint32_t progress_report_remaining = 100000;
        (void) duplex_progress;

        uint32_t runs = 90000;

        for(uint32_t i = 0; i < runs; i++ ) {

            uint32_t progress, timeslot, can_tx, epoc, accumulated_progress;

            // schedule, an object which this function modifies
            // lifetime_32 global counter owned by main.c
            // progress how may samples into the timeslot are we
            // accumulated_progress how many samples into the second are we
            // can_tx are we allowed to tx based on the schedule
            
            // this is expensive to run, we can save output and all but progress would be able to be cached
            schedule_get_timeslot2(schedule,
                            lifetime_32,
                            &progress,
                            &accumulated_progress,
                            &timeslot,
                            &epoc,
                            &can_tx);

            duplex_progress = lifetime_32 % DUPLEX_FRAMES;

            uint32_t frame_phase = (accumulated_progress % PONG_BUFFERS);


            ////////////////////////////////////////////////////////////////////////////////////



            // fires 24 times per second
            if( (accumulated_progress & (1024-1)) == 0 ) {

                if( progress_report_remaining > 0 ) {
                    if( accumulated_progress == 0 ) {
                        ring_block_send_eth(TX_EPOC_REPORT_PCCMD | (epoc&0xffffff) );
                        if(print1) {
                            cout << "sending epoc second " << epoc << " at lifetime_32 " << lifetime_32 << "\n";
                        }
                    } else {
                        ring_block_send_eth(TX_PROGRESS_REPORT_PCCMD | (accumulated_progress) );
                        if( print1 ) {
                            cout << "sending delta " << accumulated_progress << " at lifetime_32 " << lifetime_32 << "\n";
                        }
                    }
                    progress_report_remaining--;
                }
            }


            lifetime_32++;
        } // for


        // for(const auto w : rb0) {
        //     cout << HEX32_STRING(w) << "\n";
        // }

        if( print2 ) {

            for(const auto w : rb0) {
                consumeContinuousEpoc(w);
            }

            for(const auto w : esst) {
                cout << HEX32_STRING(w) << "\n";
            }
        }
        

        usleep(100);
        exit(0);
    });

    

  return 0;
}


