#include "BevStream.hpp"

#include <math.h>
#include <cmath>


#include <stdio.h>
#include <iostream>
#include <unistd.h>
#include <cassert>

#include <event2/buffer.h>

namespace BevStream {

using namespace std;


/////////////////////////////

static void _handle_data_wrapper(struct bufferevent *bev, void *_cls)
{
    BevStream *cls = reinterpret_cast<BevStream*>(_cls);
    struct evbuffer *input = bufferevent_get_input(bev);
    size_t len = evbuffer_get_length(input);

    cls->gotData(bev,input,len);
    
    // cout << "_handle_data_wrapper" << endl;
}

static void _handle_event_wrapper(bufferevent* bev, short kind, void* v) {
     cout << "_handle_event_wrapper" << endl;
}





/////////////////////////////



BevStream::BevStream(bool defer_callbacks, bool print):
    _print(print)
    ,_defer_callbacks(defer_callbacks)
    ,_init_run(false) 
                      {

    if(_print) {
        cout << "BevStream() ctons" << endl;
    }
    
    evthread_use_pthreads();
    // construct here or in thread?

    // must be called before event base is created
    // event_enable_debug_mode();

    evbase = event_base_new();
}

BevStream::~BevStream() {
    if( _print ) {
        cout << "~BevStream()" << endl;
    }
    if (_thread.joinable()) {
        _thread.join();
    }
}

// init the function, this is called from pipe
// this can be called as many times as needed, it will only run once
void BevStream::init() {
    if( !_init_run ) {
        setupBuffers();

        _init_run = true;

        // pass this
        _thread = std::thread(&BevStream::threadMain, this);
    }
}

void BevStream::threadMain() {
    // HiggsDriver *soapy = (HiggsDriver*)userdata;
    // size_t i = (size_t)userdata;
    // char c = *((char*)userdata);
    // cout << i << endl;

    // dsp_evbase = event_base_new();
    if(_print) {
        cout << "event dispatch thread running" << endl;
    }

    auto retval = event_base_loop(evbase, EVLOOP_NO_EXIT_ON_EMPTY);

    if(_print) {
        cout << "!!!!!!!!!!!!!!!!! BevStream(" << name << ")::threadMain() exiting " << endl;
        cout << "event_base_dispatch: " << retval << endl;
    }
}

void BevStream::stopThread() {
    // cout << "fixme stopThread()" << endl;
    // _thread_should_terminate = true;
    stopThreadDerivedClass();

    event_base_loopbreak(evbase);
}



///
/// Setup of callbacks and buffer waterlevels
///
void BevStream::setupBuffers() {
    // bufferevent_options(udp_payload_in_bev, BE_VOPT_THREADSAFE);


    // shared set of pointers that we overwrite and then copy out from
    // (copied to BevPair class)
    struct bufferevent * rsps_bev[2];


    ////////////////////
    //
    // raw udp buffer
    //
    // could be replaced by reading fid directly
    // fixme
    //
    // This buffer is cross threads while the others do not
    // this allows us to have an entire thread dedicated to rx
    int flags = BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE;

    if( _defer_callbacks ) {
        flags |= BEV_OPT_DEFER_CALLBACKS;
    }

    int ret = bufferevent_pair_new(evbase, 
        flags, 
        rsps_bev);
    assert(ret>=0);

    pair = new BevPair2();
    pair->set(rsps_bev);

    // copy to member
    input = pair->in;
    input2 = bufferevent_get_output(pair->in);

    setBufferOptions(pair->in, pair->out); // this will take care of this:
    // bufferevent_setwatermark(bev->out, EV_READ, (1024+16)*4, 0);
    // bufferevent_set_max_single_read(bev->out, (1024+16)*4 );

    bufferevent_setcb(pair->out, _handle_data_wrapper, NULL, _handle_event_wrapper, this);
    bufferevent_enable(pair->out, EV_READ | EV_WRITE | EV_PERSIST);
    // bufferevent_enable(pair->in,  EV_WRITE | EV_PERSIST);

    // needed for evbuffer_reserve_space(), for some reason s-modem does not need this
    evbuffer_unfreeze(bufferevent_get_input(pair->in), 0); // works




    // cout << "setup finished " << name << " " << bufferevent_get_input(pair->in)->freeze_end << endl;


    // pair->enableLocking();


}

    BevStream* BevStream::pipe(BevStream* arg) {
        next = arg;
        arg->prev = this;

        this->init();
        this->next->init();

        return arg;
    }


}