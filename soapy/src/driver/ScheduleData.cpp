#include "ScheduleData.hpp"

#include "AirPacket.hpp"
#include <iostream>

using namespace std;

ScheduleData::ScheduleData(AirPacketOutbound* _airTx) :
airTx(_airTx)
{

}

int ScheduleData::pickTimeslot() {
    std::unique_lock<std::mutex> lock(_global_mutex);

    timeslot_index++;

    if( (unsigned)timeslot_index >= timeslot.size()  ) {
        timeslot_index = 0;
    }

    return timeslot[timeslot_index];

    // switch(timeslot_index) {
    //     default:
    //     case 0:
    //         return 12;
    //     case 1:
    //         return 22;
    //     case 2:
    //         return 32;
    //     case 3:
    //         return 42;
    // }
}
double ScheduleData::pickSecondSleep() {
    std::unique_lock<std::mutex> lock(_global_mutex);
    // return 0.1;
    return sleep;
}


uint32_t ScheduleData::pickPacketSize() {
    std::unique_lock<std::mutex> lock(_global_mutex);
    // uint32_t max_frames = 4*512; // is this too close to gap of 10? yes we need to factor in 3000 early

    return max_frames * airTx->sliced_word_count;
}

int ScheduleData::pickEarlyFrames() {
    std::unique_lock<std::mutex> lock(_global_mutex);
    return early;
}


void ScheduleData::setEarlyFrames(int v) {
    // cout << "setEarlyFrames " << v << "\n";
    std::unique_lock<std::mutex> lock(_global_mutex);
    early = v;
}

// this is in frames.
// pickPacketSize() internally multiplies by the number of words per frame
void ScheduleData::setPacketSize(uint32_t v) {
    // cout << "setPacketSize " << v << "\n";
    std::unique_lock<std::mutex> lock(_global_mutex);
    max_frames = v;
}
void ScheduleData::setSecondSleep(double v) {
    // cout << "setSecondSleep " << v << "\n";
    std::unique_lock<std::mutex> lock(_global_mutex);
    sleep = v;
}
void ScheduleData::setTimeslot(const std::vector<uint32_t>& v) {
    // cout << "setTimeslot " << v.size() << "\n";
    std::unique_lock<std::mutex> lock(_global_mutex);
    timeslot = v;
}
