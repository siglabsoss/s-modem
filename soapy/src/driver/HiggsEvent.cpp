#include "driver/HiggsEvent.hpp"

#include "HiggsSoapyDriver.hpp"

#include "HiggsDriverSharedInclude.hpp"

// also gets feedback_bus.h from riscv
// #include "feedback_bus_tb.hpp"

// #include "vector_helpers.hpp"


// #include "schedule.h"

#include <math.h>
#include <cmath>



#include <stdio.h>

using namespace std;


static void
_example_clock_event(int fd, short kind, void *userp)
{
    /* Print current time. */
    time_t t = time(NULL);
    int now_s = t % 60;
    int now_m = (t / 60) % 60;
    int now_h = (t / 3600) % 24;
    /* The trailing \r will make cursor return to the beginning
     * of the current line. */
    printf("%02d:%02d:%02d\n", now_h, now_m, now_s);
    fflush(stdout);

    /* Next clock update in one second. */
    /* (A more prudent timer strategy would be to update clock
     * on the next second _boundary_.) */


    struct timeval timeout = { .tv_sec = 10, .tv_usec = 0 };
    HiggsEvent* event = (HiggsEvent*)userp;
    evtimer_add(event->_example_event, &timeout);
}



/////////////////////////////




HiggsEvent::HiggsEvent(HiggsDriver* s, bool _run_timer):soapy(s),_thread_should_terminate(false),run_timer(_run_timer) {


    cout << "HiggsEvent() ctons" << endl;
    
    evthread_use_pthreads();
    // construct here or in thread?

    // must be called before event base is created
    // event_enable_debug_mode();

    evbase = event_base_new();


    // Example event
    if( run_timer ) {
        _example_event = evtimer_new(evbase, _example_clock_event, this);
        _example_clock_event(-1, EV_TIMEOUT, this);
    }

    // pass this
    _thread = std::thread(&HiggsEvent::threadMain, this);

    

    usleep(1000);
}


// sets the scheduling policy and priority for the thread
// read the links for a better understanding.  Not all combinations of policys and priorities
// are valid
void HiggsEvent::setThreadPriority(int policy, int priority) {

    // http://man7.org/linux/man-pages/man7/sched.7.html
    // http://man7.org/linux/man-pages/man3/pthread_setschedparam.3.html
    // https://stackoverflow.com/questions/18884510/portable-way-of-setting-stdthread-priority-in-c11

    // int resmax = sched_get_priority_max(SCHED_FIFO);
    // int resmin = sched_get_priority_min(SCHED_FIFO);
    // cout << "fifo min: " << resmin << " max: " << resmax << "\n";
    // resmax = sched_get_priority_max(SCHED_RR);
    // resmin = sched_get_priority_min(SCHED_RR);
    // cout << "RR min: " << resmin << " max: " << resmax << "\n";

    thread_priority.sched_priority = priority;
    thread_schedule_policy = policy;

    if(pthread_setschedparam(_thread.native_handle(), policy, &thread_priority)) {
        std::cout << "Failed to set Thread scheduling : " << std::strerror(errno) << "\n";
    }
}

void HiggsEvent::threadMain() {
    usleep(200);

    printf("event dispatch thread running\n");

    auto retval = event_base_loop(evbase, EVLOOP_NO_EXIT_ON_EMPTY);

    cout << "!!!!!!!!!!!!!!!!! HiggsEvent::threadMain() exiting with ret: " << retval << endl;
}

void HiggsEvent::stopThread() {
    cout << "fixme stopThread()" << endl;
    _thread_should_terminate = true;
    stopThreadDerivedClass();
}


BevPair::BevPair() : in(0), out(0) {
    
}


BevPair& BevPair::set(struct bufferevent * rsps_bev[2]) {
    in = rsps_bev[0];
    out = rsps_bev[1];
    return *this;
}

BevPair& BevPair::enableLocking() {
    evbuffer_enable_locking(bufferevent_get_input(in), NULL);
    evbuffer_enable_locking(bufferevent_get_output(in), NULL);
    evbuffer_enable_locking(bufferevent_get_input(out), NULL);
    evbuffer_enable_locking(bufferevent_get_output(out), NULL);

    return *this;
}