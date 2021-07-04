#include <rapidcheck.h>
// #include "driver/PredictBuffer.hpp"

#include "ringbus.h"
#include "schedule.h"
#include "feedback_bus.h"

#include <vector>
#include <algorithm>
#include <iostream>

#include "cpp_utils.hpp"

using namespace std;


void generate_input_data(std::vector<uint32_t> &input_data, uint32_t data_length);
void generate_input_data(std::vector<uint32_t> &input_data,
                         uint32_t data_length) {
    for(uint32_t i = 0; i < data_length; i++) {
        const uint32_t a_word = *rc::gen::inRange(0u, 0xffffffffu);
        input_data.push_back(a_word);
    }

}

int32_t calculateFrameDelta(const uint32_t word);
int32_t calculateFrameDelta(const uint32_t word) {
    uint32_t dmode =      (word & 0x00ff0000) >> 16;
    int32_t frame_delta =  word & 0x0000ffff;

    // do this to sign extend
    frame_delta <<= 16;
    frame_delta >>= 16;

    int epoc_delta = (int8_t)dmode;

    // unwinds some values, I think this is just to save compute
    // in cs20
    while(0 < epoc_delta) {
        frame_delta += SCHEDULE_FRAMES;
        epoc_delta--;
    }

    // copy to class
    return frame_delta;
}


// goal is to report how many frames using a fucked up format
auto report_userdata_latency = [](
    const uint32_t epoc,
    const uint32_t accumulated_progress,
    const uint32_t ud_epoc,
    const uint32_t ud_frame
    ) {
        // userdata_needs_latency_report = 0;
        int8_t e_delta = (int8_t)(ud_epoc - epoc);
        int16_t frame_delta = (int16_t)((ud_frame) - accumulated_progress);
        uint32_t masked = ((e_delta<<16)&0xff0000) | (frame_delta&0xffff);
        return TX_FILL_LEVEL_PCCMD | masked;
    };


int main() {

    bool pass = true;

    pass &= rc::check("my ctons", []() {

        const unsigned frame_pull = *rc::gen::inRange(0, SCHEDULE_FRAMES);

        epoc_frame_t a = (epoc_frame_t){0, frame_pull};
        // epoc_frame_t b = (epoc_frame_t){0, 50};

        RC_ASSERT(
            a.frame == frame_pull
            );

        // the final test only makes sense if we added SOMETHING
        // otherwise we can't check
        // RC_PRE(expected != 0 );


    });

    pass &= rc::check("add is the same", []() {

        const unsigned frame_start = *rc::gen::inRange(0, SCHEDULE_FRAMES);
        const unsigned epoc_second = *rc::gen::inRange(0, 999);

        epoc_frame_t aa = (epoc_frame_t){epoc_second, frame_start};
        epoc_frame_t bb = (epoc_frame_t){epoc_second, frame_start};

        const unsigned adjust = *rc::gen::inRange(0, 0xafff);

        auto aa_1 = adjust_frames_to_schedule(aa, adjust);
        auto bb_1 = add_frames_to_schedule(bb, adjust);

        RC_ASSERT(aa_1.epoc ==  bb_1.epoc);
        RC_ASSERT(aa_1.frame == bb_1.frame);
    });

    pass &= rc::check("sub is the same as add", []() {

        const unsigned frame_start = *rc::gen::inRange(0, SCHEDULE_FRAMES);
        const unsigned epoc_second = *rc::gen::inRange(800, 999);

        epoc_frame_t aa = (epoc_frame_t){epoc_second, frame_start};
        // epoc_frame_t bb = (epoc_frame_t){epoc_second, frame_start};


        bool print = false;

        if( print ) {
            cout << "a: " << aa.epoc << "\n";
            cout << "a: " << aa.frame << "\n";
        }


        const int64_t adjust = *rc::gen::inRange(-0xafff, 0xafff);

        if( print ) {
            cout << "adjust: " << adjust << "\n";
        }

        auto aa_1 = adjust_frames_to_schedule(aa, adjust);

        if( print ) {
            cout << "a1: " << aa_1.epoc << "\n";
            cout << "a1: " << aa_1.frame << "\n";
        }

        auto bb_1 = adjust_frames_to_schedule(aa_1, -adjust);

        if( print ) {
            cout << "b1: " << bb_1.epoc << "\n";
            cout << "b1: " << bb_1.frame << "\n";
        }

        RC_ASSERT(aa.epoc ==  bb_1.epoc);
        RC_ASSERT(aa.frame == bb_1.frame);
    });

    pass &= rc::check("pure", []() {
        const unsigned frame_start = *rc::gen::inRange(0, SCHEDULE_FRAMES);
        const unsigned epoc_second = *rc::gen::inRange(0, 1);
        epoc_frame_t a = (epoc_frame_t){epoc_second, frame_start};

        auto a_pure = schedule_to_pure_frames(a);

        const unsigned adjust = *rc::gen::inRange(0, 0xafff);

        // b will always be later than a
        auto b = adjust_frames_to_schedule(a, adjust);

        auto b_pure = schedule_to_pure_frames(b);

        RC_ASSERT(  (b_pure - a_pure)  == adjust );

        auto delta = schedule_delta_frames(b, a);

        RC_ASSERT(  (b_pure - a_pure)  == (long unsigned)delta );

    });

    // check that adding is the same as subtracting
    pass &= rc::check("add will rollover", []() {
    
        const unsigned frame_start = *rc::gen::inRange(0, SCHEDULE_FRAMES);
        const unsigned epoc_second = *rc::gen::inRange(10, 999999);
        epoc_frame_t a = (epoc_frame_t){epoc_second, frame_start};


        // test adding zero
        const auto zero_test = add_frames_to_schedule(a, 0);

        // if this fails, the test didn't start or maybe we did an initliaize with a value over 1?
        RC_ASSERT(a.epoc == zero_test.epoc);
        RC_ASSERT(a.frame == zero_test.frame);

        bool print = false;

        if( print ) {
            cout << "a: " << a.epoc << "\n";
            cout << "a: " << a.frame << "\n";
        }



        // THESE SHOULD BE THE SAME
        // auto val_b = add_frames_to_schedule(a, 1);
        const auto val_b = adjust_frames_to_schedule(a, 1);

        if( print ) {
            cout << "b: " << val_b.epoc << "\n";
            cout << "b: " << val_b.frame << "\n";
        }

        const auto val_c = adjust_frames_to_schedule(val_b, -1);

        if( print ) {
            cout << "c: " << val_b.epoc << "\n";
            cout << "c: " << val_b.frame << "\n";
        }

        RC_ASSERT( val_c.epoc == a.epoc );
        RC_ASSERT( val_c.frame == a.frame );
        RC_ASSERT( schedule_frame_equal(val_c,a) );

    });


    /// Does the schedule_to_pure_frames and pure_frames_to_epoc_frame functions work perfectly
    ///
    pass &= rc::check("pure and schedule convert", []() {

    
        const unsigned frame_start = *rc::gen::inRange(0, SCHEDULE_FRAMES);
        const unsigned epoc_second = *rc::gen::inRange(10, 999999999);
        const epoc_frame_t a = (epoc_frame_t){epoc_second, frame_start};

        const auto ideal = schedule_to_pure_frames(a);

        epoc_frame_t calc0 = pure_frames_to_epoc_frame(ideal);


        RC_ASSERT(a.epoc == calc0.epoc);
        RC_ASSERT(a.frame == calc0.frame);

    });

    /// Does the schedule_to_pure_frames work the same as adding frames to a zero schedule?
    pass &= rc::check("pure and add_frames are the same", []() {
        return; // FIXME
    
        const unsigned frame_start = *rc::gen::inRange(0, SCHEDULE_FRAMES);
        const unsigned epoc_second = *rc::gen::inRange(10, 16000);
        const epoc_frame_t a = (epoc_frame_t){epoc_second, frame_start};

        const auto ideal = schedule_to_pure_frames(a);

        epoc_frame_t calc0 = pure_frames_to_epoc_frame(ideal);

        epoc_frame_t b = (epoc_frame_t){0,0};

        epoc_frame_t calc1 = add_frames_to_schedule(b, ideal);



        RC_ASSERT(a.epoc == calc0.epoc);
        RC_ASSERT(a.frame == calc0.frame);

        RC_ASSERT(a.epoc == calc1.epoc);
        RC_ASSERT(a.frame == calc1.frame);
    });


    /// converter
    pass &= rc::check("Convert in and out", []() {

    
        // const unsigned frame_start = *rc::gen::inRange(0, SCHEDULE_FRAMES);
        // const unsigned epoc_second = *rc::gen::inRange(10, 16000);
        // const epoc_frame_t a = (epoc_frame_t){epoc_second, frame_start};

        // force to be multiple of 512
        const auto ideal = (*rc::gen::inRange(0, 2333)) * SCHEDULE_LENGTH;

        // SCHEDULE_FRAMES
        // epoc_frame_t

        bool print = false;

        if( print ) {
            cout << "ideal: " << ideal << "\n";
        }


        epoc_frame_t a = pure_frames_to_epoc_frame(ideal);

        if( print ) {
            cout << "a: " << a.epoc << "\n";
            cout << "a: " << a.frame << "\n";
        }

        epoc_timeslot_t b;
        b = schedule_frame_to_timeslot(a);

        if( print ) {
            cout << "b: " << b.epoc << "\n";
            cout << "b: " << b.timeslot << "\n";
        }


        epoc_frame_t c = schedule_timeslot_to_frame(b);

        if( print ) {
            cout << "c: " << c.epoc << "\n";
            cout << "c: " << c.frame << "\n";
        }

        RC_ASSERT(schedule_frame_equal(a,c));

    });

    /// testing 2 versions of set_mapmov_epoc_timeslot()
    pass &= rc::check("test set_mapmov_epoc_timeslot() overload", []() {

    

        // force to be multiple of 512
        const auto ideal = (*rc::gen::inRange(0, 2333)) * SCHEDULE_LENGTH;


        epoc_timeslot_t b;
        {
            const epoc_frame_t a = pure_frames_to_epoc_frame(ideal);
            b = schedule_frame_to_timeslot(a);
        }

        // generate 16 random values in a vector
        const std::vector<uint32_t> data0 = [](const uint32_t len) {
            std::vector<uint32_t> out;
            generate_input_data(out, len);
            return out;
        }(16);

        RC_ASSERT(data0.size() == 16u);

        // two copies
        auto data1 = data0; // through one overload  (typed one)
        auto data2 = data0; // through the other     (separate arguments one)

        set_mapmov_epoc_timeslot(data1, b);

        set_mapmov_epoc_timeslot(data2, b.epoc, b.timeslot);

        RC_ASSERT(data1 == data2);

    });

#ifdef TEST_OLD_CODE
    pass &= rc::check("test ringbus reporting delta", []() {



        // non zero so large negative below wont mess us up
        const uint32_t now_epoc  = *rc::gen::inRange(200, 4000);
        const uint32_t now_frame = *rc::gen::inRange(0, SCHEDULE_FRAMES-2);

        const int64_t start_adjust   = *rc::gen::inRange(-40000, 40000);
        const int64_t partner_adjust = *rc::gen::inRange(-60000, 60000);

        bool print = true;



        // cout << "st: " << start_adjust << "\n";

        // adjust_frames_to_schedule


        // when is now?
        const epoc_frame_t _now = (epoc_frame_t){now_epoc,now_frame};

        if( print ) {
            cout << "a: " << _now.epoc << "\n";
            cout << "a: " << _now.frame << "\n";
        }

        // mess with ud to get it in all possible places
        const epoc_frame_t now = adjust_frames_to_schedule(_now, start_adjust);

        if( print ) {
            cout << "b: " << now.epoc << "\n";
            cout << "b: " << now.frame << "\n";
        }


        // when is userdata
        const epoc_frame_t ud = adjust_frames_to_schedule(now, partner_adjust);

        if( print ) {
            cout << "c: " << ud.epoc << "\n";
            cout << "c: " << ud.frame << "\n";
        }

        const int64_t pure_now = schedule_to_pure_frames(now);
        const int64_t pure_ud = schedule_to_pure_frames(ud);
        const int64_t pure_delta = pure_ud - pure_now;

        if( print ) {
            cout << "pure now: " << pure_now << "\n";
            cout << "pure ud: " << pure_ud << "\n";
        }

        uint32_t val = report_userdata_latency(now.epoc, now.frame, ud.epoc, ud.frame);


        int32_t delta = calculateFrameDelta(val);

        // cout << HEX32_STRING(val) << "\n";
        // cout << "delta: " << (delta) << "\n";
        // cout << "pure : " << (pure_delta) << "\n";
        // cout << "\n";

        RC_ASSERT(delta == pure_delta);


    });
#endif

    pass &= rc::check("test new ringbus reporting delta", []() {



        // non zero so large negative below wont mess us up
        const uint32_t now_epoc  = *rc::gen::inRange(200, 4000);
        const uint32_t now_frame = *rc::gen::inRange(0, SCHEDULE_FRAMES-2);

        const int64_t start_adjust   = *rc::gen::inRange(-40000, 40000);
        const int64_t partner_adjust = *rc::gen::inRange(-60000, 60000);


        // when is now?
        const epoc_frame_t _now = (epoc_frame_t){now_epoc,now_frame};

        // mess with ud to get it in all possible places
        const epoc_frame_t now = adjust_frames_to_schedule(_now, start_adjust);


        // when is userdata
        const epoc_frame_t ud = adjust_frames_to_schedule(now, partner_adjust);

        const uint64_t pure_now = schedule_to_pure_frames(now);
        const uint64_t pure_ud = schedule_to_pure_frames(ud);

        const uint32_t pure_now32 = pure_now;
        const uint32_t pure_ud32 = pure_ud;
        // force that we didn't get anything 64 bit
        RC_ASSERT(pure_now32 == pure_now);
        RC_ASSERT(pure_ud32 == pure_ud);


        const int32_t pure_delta = (int32_t)pure_ud32 - (int32_t)pure_now32;

        // returns a - b
        uint32_t rb = schedule_report_delta_ringbus(pure_ud, pure_now32);


        int32_t delta = schedule_parse_delta_ringbus(rb);

        if( 0 ) {
            cout << "st: " << start_adjust << "\n";
            cout << "pure_now32: " << (pure_now32) << "\n";
            cout << "pure_ud32: " << (pure_ud32) << "\n";
            cout << HEX32_STRING(rb) << "\n";
            cout << "delta: " << (delta) << "\n";
            cout << "pure : " << (pure_delta) << "\n";
            cout << "\n";
        }

        // exit(0)

        RC_ASSERT(delta == pure_delta);


    });


    /// equality
    pass &= rc::check("test equality fn", []() {
        // sort of a waste of time to check

        const uint32_t bump = *rc::gen::inRange(0, 40); // sum of this must not exceed 25k

        epoc_frame_t a = pure_frames_to_epoc_frame(44+bump);
        epoc_frame_t b = pure_frames_to_epoc_frame(45+bump);
        epoc_frame_t c = pure_frames_to_epoc_frame(44+bump);



        RC_ASSERT(a.epoc == c.epoc);
        RC_ASSERT(a.frame == c.frame);

        // RC_ASSERT(a.epoc == calc1.epoc);
        RC_ASSERT(a.frame != b.frame);

        RC_ASSERT(schedule_frame_equal(a,c));
        RC_ASSERT(!schedule_frame_equal(a,b));
        RC_ASSERT(!schedule_frame_equal(b,c));

    });

/// equality
    pass &= rc::check("test equality fn2", []() {
        // sort of a waste of time to check

        const uint32_t bump = *rc::gen::inRange(0, 40); // sum of this must not exceed 25k

        epoc_timeslot_t a = (epoc_timeslot_t){0,44+bump};
        epoc_timeslot_t b = (epoc_timeslot_t){0,45+bump};
        epoc_timeslot_t c = (epoc_timeslot_t){0,44+bump};



        RC_ASSERT(a.epoc == c.epoc);
        RC_ASSERT(a.timeslot == c.timeslot);

        // RC_ASSERT(a.epoc == calc1.epoc);
        RC_ASSERT(a.timeslot != b.timeslot);

        RC_ASSERT(schedule_timeslot_equal(a,c));
        RC_ASSERT(!schedule_timeslot_equal(a,b));
        RC_ASSERT(!schedule_timeslot_equal(b,c));

    });


    return !pass;
}


