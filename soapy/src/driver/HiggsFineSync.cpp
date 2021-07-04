#include "HiggsFineSync.hpp"
#include <future>

using namespace std;
#include "vector_helpers.hpp"

#include "driver/EventDsp.hpp"
#include "driver/RadioEstimate.hpp"
#include "driver/AttachedEstimate.hpp"

#define SENDING_IDLE 0
#define SENDING_NORMAL 1
#define SENDING_LOW 2




static void _handle_sfo_cfo_callback_new(struct bufferevent *bev, void *_dsp) {
     // cout << "here" << endl;
    EventDsp *dsp = (EventDsp*) _dsp;

    // dsp->handle_sfo_cfo_callback(bev);

    // auto futureCallback = std::async(
    //     std::launch::async, 
    //     [](EventDsp* _this, struct bufferevent *bev) {
    //         _this->handle_sfo_cfo_callback(bev);
    //     }, dsp, bev
    // );

    dsp->handle_sfo_cfo_callback(bev);

    // cout << "_handle_sfo_event" << endl;
}

static void _handle_sfo_cfo_event_new(bufferevent* d, short kind, void* v) {
    cout << "_handle_sfo_cfo_event" << endl;
}




// just sfo for now
static void _handle_cfo_timer_new(int fd, short kind, void *_soapy) {
    HiggsDriver *soapy = (HiggsDriver*) _soapy;
    EventDsp *dsp = soapy->dspEvents;
    dsp->handleCfoTimer();
}

// 2 here is dsp->radios.size()
static RadioEstimate* pointer_for_index(EventDsp *dsp, const unsigned last) {
    return last >= dsp->radios.size() ? dsp->attached : dsp->radios[last];
}

static void _handle_sfo_timer_new(int fd, short kind, void *_soapy) {
        HiggsDriver *soapy = (HiggsDriver*) _soapy;
        EventDsp *dsp = soapy->dspEvents;
        HiggsFineSync *higgsFineSync = soapy->higgsFineSync;

        // cout << "_handle_sfo_timer" << endl;

        // This static is "ok" I guess. It will handle N radios
        static unsigned last = 0;
        ///
        /// was
        ///

        // dsp->radios[last]->dspRunSfo_v1();

        auto sfoFuture = std::async(
            std::launch::async, 
            [](RadioEstimate* _re) {
                 _re->dspRunSfo_v1();
            }, pointer_for_index(dsp, last)
        );

        ///
        /// above last is used as the ptr
        /// even though that is still running
        /// we can bump and check our peer

        switch(last) {
            case 0:
                if( dsp->should_inject_r1 ) {
                    last = 1;
                    break;
                }
                if( dsp->should_inject_attached ) {
                    last = 2;
                    break;
                }
                break;
            case 1:
                if( dsp->should_inject_attached ) {
                    last = 2;
                    break;
                }
                if( dsp->should_inject_r0 ) {
                    last = 0;
                    break;
                }
                break;
            case 2:
                if( dsp->should_inject_r0 ) {
                    last = 0;
                    break;
                }
                if( dsp->should_inject_r1 ) {
                    last = 1;
                    break;
                }
                break;
            default:
                break;
        }
        // last ++;
        last = last % (dsp->radios.size()+1); // not needed if switch above is perfect
        // cout << "last: " << last << "\n";

        // look at next and trigger or not

        // after we run, look at peer,
        // if he is also full, run in 100 us
        // we dont need to do this on tx side as we only have one peer to run
        if( dsp->should_inject_r0 || dsp->should_inject_r1 ) {
            if( pointer_for_index(dsp, last)->sfo_buf.size() >= (unsigned)dsp->_sfo_symbol_num   ) {
                struct timeval tt = { .tv_sec = 0, .tv_usec = 100 };
                evtimer_add(higgsFineSync->process_sfo_timer, &tt);
            }
        }
}




///////////////////////////////////////////////////////////////
//
//  Above this line are static functions which comprise events for this class
//

HiggsFineSync::HiggsFineSync(HiggsDriver* s):
    
    //
    // class wide initilizers
    //
    HiggsEvent(s)
    // status(s->status)

    //
    // function starts:
    //
{
    settings = (soapy->settings);
    setupBuffers();
}

void HiggsFineSync::stopThreadDerivedClass() {
    // whatever
}


void HiggsFineSync::setupBuffers() {
    // shared set of pointers that we overwrite and then copy out from
    // (copied to BevPair class)
    struct bufferevent * rsps_bev[2];

    int ret = bufferevent_pair_new(evbase, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_DEFER_CALLBACKS | BEV_OPT_THREADSAFE , rsps_bev);
    assert(ret>=0);
    (void)ret;
    
    sfo_cfo_buf = new BevPair();
    sfo_cfo_buf->set(rsps_bev).enableLocking();

    if(true) {

        const auto sfo_cfo_watermark = SFO_CFO_PACK_SIZE * 4 * 512;

        // set 0 for write because nobody writes to out
        bufferevent_setwatermark(sfo_cfo_buf->out, EV_READ, sfo_cfo_watermark, 0);

        // bufferevent_set_max_single_read(sfo_cfo_buf->out, (1024*3)*4 );


        // only get callbacks on read side
        bufferevent_setcb(sfo_cfo_buf->out, _handle_sfo_cfo_callback_new, NULL, _handle_sfo_cfo_event_new, soapy->dspEvents);
        bufferevent_enable(sfo_cfo_buf->out, EV_READ | EV_WRITE | EV_PERSIST);
    }


    process_sfo_timer = evtimer_new(evbase, _handle_sfo_timer_new, soapy);

    process_cfo_timer = evtimer_new(evbase, _handle_cfo_timer_new, soapy);

}


void HiggsFineSync::dump_sfo_cfo_buffer() {
    std::unique_lock<std::mutex> lock(soapy->dspEvents->_handle_sfo_cfo_callback_running_mutex);

    struct evbuffer *buf = bufferevent_get_input(sfo_cfo_buf->out);

    size_t len = evbuffer_get_length(buf);

    cout << "dump_sfo_cfo_buffer dumped " << len << " bytes\n";

    evbuffer_drain(buf, len);
}
