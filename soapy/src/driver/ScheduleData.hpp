#pragma once

#include <stdint.h>
#include <mutex>
#include <vector>

class AirPacketOutbound;

class ScheduleData
{
public:
    ScheduleData(AirPacketOutbound* _airTx);
    // virtual ~ScheduleData();
    int pickTimeslot();         // how often
    double pickSecondSleep();
    uint32_t pickPacketSize();  // how much
    int pickEarlyFrames();      // how early

    void setEarlyFrames(int);     
    void setPacketSize(uint32_t);  
    void setSecondSleep(double);
    void setTimeslot(const std::vector<uint32_t>& v);

    int early = 3000;
    std::vector<uint32_t> timeslot = {12,22,32,42};
    double sleep = 0.005;
    int timeslot_index = 0;
    int max_frames = 4*512;
    
    AirPacketOutbound* airTx;
private:
    mutable std::mutex _global_mutex;
};
