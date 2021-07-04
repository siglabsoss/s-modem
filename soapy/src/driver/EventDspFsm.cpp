#include "EventDsp.hpp"

#include "HiggsDriverSharedInclude.hpp"

#include "vector_helpers.hpp"

#include "schedule.h"

using namespace std;

#include "driver/FsmMacros.hpp"

#include "EventDspFsmStates.hpp"
#include "CustomEventTypes.hpp"
#include "DispatchMockRpc.hpp"

#include <signal.h> 
#include <sstream>
#include <ctime>
#include "common/convert.hpp"
#include "NullBufferSplit.hpp"
#include "driver/AttachedEstimate.hpp"
#include "driver/RadioEstimate.hpp"
#include "driver/TxSchedule.hpp"
#include "driver/RadioDemodTDMA.hpp"
#include "driver/DemodThread.hpp"
#include "AirPacket.hpp"

// only works for payload.peer == dsp->soapy->this_node.id
static void handle_rb_later_multi(int fd, short kind, void *_dsp) {
    EventDsp *dsp = (EventDsp*) _dsp;

    // struct event* tmp2;
    size_t max_pull = 700; // could be a bit larger
    size_t potential_pull = std::min(max_pull, dsp->pending_ring_data.size());
    size_t did_pull = 0;
    std::vector<uint32_t> send;
    bool first_loop = true;
    size_t peer = 0;
    {
        std::unique_lock<std::mutex> lock(dsp->_rb_later_mutex);

        for( auto i = 0u; i < potential_pull; i++ ) {
            ringbus_later_t payld;
            payld = dsp->pending_ring_data[i];

            if( first_loop ) {
                peer = payld.peer;
                first_loop = false;
            } else {
                if( payld.peer != peer ) {
                    break; // next message is not for attached rb, break
                }
            }

            send.push_back(payld.rb.ttl);
            send.push_back(payld.rb.data);

            did_pull++;
        }
        dsp->pending_ring_data.erase(dsp->pending_ring_data.begin(),dsp->pending_ring_data.begin()+did_pull);
        dsp->pending_ring_events.erase(dsp->pending_ring_events.begin(),dsp->pending_ring_events.begin()+did_pull);
        //LEAK:
        // event_free()
    }

    dsp->soapy->rb->sendMulti(send);

    // cout << "was able to batch send " << did_pull << " to ourselves, peer " << peer << endl;
}

static void handle_rb_later(int fd, short kind, void *_dsp)
{
    EventDsp *dsp = (EventDsp*) _dsp;

    ringbus_later_t payload;
    // struct event* tmp2;

    {
        std::unique_lock<std::mutex> lock(dsp->_rb_later_mutex);
        if( dsp->pending_ring_data.size() == 0 ) {
            return; // since we've added the multi version, it's possible this can be zero, so just bail
        }
        
        payload = dsp->pending_ring_data[0];
        // tmp2 = dsp->pending_ring_events[0];

        // if we are allowed, and this is a local rb, try the multi version
        if( payload.peer == dsp->soapy->this_node.id && dsp->soapy->GET_AGGREGATE_ATTACHED_RB() ) {
            lock.unlock(); // we have to unlock because this function takes its own lock
            handle_rb_later_multi(fd, kind, _dsp);
            return; // return so we can live another day
        }


        dsp->pending_ring_data.erase(dsp->pending_ring_data.begin());
        dsp->pending_ring_events.erase(dsp->pending_ring_events.begin());

        //LEAK:
        // event_free()
    }

    // cout << "handle_rb_later()" << endl;

    if(payload.peer == dsp->soapy->this_node.id) {
        // cout << "consumed local" << endl;
        // suprise, this message is actually destined for our attached higs
        dsp->soapy->rb->send(payload.rb.ttl, payload.rb.data);
    } else {
        // cout << "  peer: " << payload.peer << endl;
        dsp->soapy->zmqRingbusPeer(payload.peer, (char*)&payload.rb, sizeof(payload.rb));
    }


    // cout << "  ttl: " << payload.rb.ttl << endl;
    // cout << " data: " << HEX32_STRING(payload.rb.data) << endl;

}

static void handle_fsm_boot(int fd, short kind, void *_dsp)
{
    EventDsp *dsp = (EventDsp*) _dsp;
    cout << "handle_fsm_boot" << endl;

    dsp->dsp_state_pending    = dsp->dsp_state    = PRE_BOOT;
    dsp->tx_dsp_state_pending = dsp->tx_dsp_state = PRE_BOOT;

    // cout << "resetting estimate_filter_mode" << endl;
    // dsp->estimate_filter_mode = 0;

    // calls the event ON THE SAME STACK as us
    event_active(dsp->fsm_next, EV_WRITE, 0);
    event_active(dsp->tx_fsm_next, EV_WRITE, 0);
}

// call back for (rx main fsm)
/// There are two version of this function. when the timer event is made
/// we check init.json for the role and use that
static void handle_fsm_tick_rx(int fd, short kind, void *_dsp)
{
    // cout << "handle_fsm_tick" << endl;
    EventDsp *dsp = (EventDsp*) _dsp;
    dsp->tickFsmRx();
}

// call back for (tx main fsm)
/// There are two version of this function. when the timer event is made
/// we check init.json for the role and use that
static void handle_fsm_tick_tx(int fd, short kind, void *_dsp)
{
    // cout << "handle_fsm_tick" << endl;
    EventDsp *dsp = (EventDsp*) _dsp;
    dsp->tickFsmTx();
}

static void handle_radios_tick(int fd, short kind, void *_dsp)
{
    // cout << "handle_radios_tick" << endl;
    EventDsp *dsp = (EventDsp*) _dsp;


//     size_t radio_i = 0;
//     for(auto r : dsp->radios) {
//         // call new() in a loop and use std vector for pointers
//         // +1 is actually +4 bytes but because pointer arith
//         // r->drainBev(read_from[0], read_from+1, word_len-1);
// //        r->drainDemodBuffers(dsp->demod_buf);
//         r->dspRunDemod();
//     }

    auto futureCallback = std::async(
        std::launch::async, 
        [](EventDsp* _this) {

            _this->outsideFillDemodBuffers(_this->demod_buf);
            _this->tickAllRadios();
            _this->slowUpdateDashboard();

            auto t = RADIO_TICK_SLEEP;
            evtimer_add(_this->radios_tick_timer, &t);
        }, dsp
    );

    // dsp->outsideFillDemodBuffers(dsp->demod_buf);

    // dsp->tickAllRadios();

    // auto t = RADIO_TICK_SLEEP;
    // evtimer_add(dsp->radios_tick_timer, &t);
}

static void _handle_print_unhealth_status_codes(int fd, short kind, void *_dsp) {
    EventDsp *dsp = (EventDsp*) _dsp;

    cout << dsp->status->getPeriodicPrint();

    auto t4 = _250_MS_SLEEP;
    evtimer_add(dsp->print_unhealth_status_codes_timer, &t4); // fire once now
}


static void handle_poll_attached_rb(int fd, short kind, void *_dsp)
{
    // EventDsp *dsp = (EventDsp*) _dsp;
    // // cout << "handle_poll_attached_rb()" << endl;

    // const size_t max_run = RINGBUS_DISPATCH_BATCH_LIMIT;

    // size_t runs = 0;
    // bool more_work_left = false;

    // while(dsp->soapy->rb_buffer.size() > 0 ) {
    //     if(runs >= max_run) {
    //         more_work_left = true;
    //         break;
    //     }
    //     auto word = dsp->soapy->rb_buffer.dequeue();
        
    //     dsp->dspDispatchAttachedRb(word);
    //     runs++;
    // }

    // // do a tiny sleep to give other stuff a chance to run
    // if(more_work_left) {
    //     auto t = TINY_SLEEP;
    //     evtimer_add(dsp->poll_rb_timer, &t);
    // } else {
    //     // schedule us again in the future
    //     auto t = POLL_RB_SOCKET_TIME;
    //     evtimer_add(dsp->poll_rb_timer, &t);
    // }
}

void EventDsp::createStatus() {
    if(this->role != "arb") {
        createStatusRx();
        createStatusTx();
    }

}

void EventDsp::setupFsm() {

    generateSchedule();
    fsmSetup_fixme();
    createStatus();

    // block until this_node is ready
    // there are probably tons of things we should be blocking on
    while(!soapy->thisNodeReady()) {usleep(1);}


    if(this->role != "arb") {
        fsm_next       = event_new(evbase, -1, EV_PERSIST, handle_fsm_tick_rx, this);
        fsm_next_timer = evtimer_new(evbase, handle_fsm_tick_rx, this);

        tx_fsm_next       = event_new(evbase, -1, EV_PERSIST, handle_fsm_tick_tx, this);
        tx_fsm_next_timer = evtimer_new(evbase, handle_fsm_tick_tx, this);
    }


    radios_tick_timer = evtimer_new(evbase, handle_radios_tick, this);
    auto t1 = _50_MS_SLEEP;
    evtimer_add(radios_tick_timer, &t1); // fire once now
    
    fsm_init = evtimer_new(evbase, handle_fsm_boot, this);

    auto t2 = TINY_SLEEP;
    evtimer_add(fsm_init, &t2);

    times_blocked_fsm_start = 0;

    onetimeTieMasterFSM();

    poll_rb_timer = evtimer_new(evbase, handle_poll_attached_rb, this);
    auto t3 = _50_MS_SLEEP;
    evtimer_add(poll_rb_timer, &t3); // fire once now

    print_unhealth_status_codes_timer = evtimer_new(evbase, _handle_print_unhealth_status_codes, this);
    auto t4 = _250_MS_SLEEP;
    evtimer_add(print_unhealth_status_codes_timer, &t4); // fire once now

    // cout << "starting queue" << endl;

    // debug
    // raw_ringbus_t rb0 = {2, SFO_PERIODIC_ADJ_CMD | 0x40};
    // zmqRingbusPeerLater(1, &rb0, 10);

    // raw_ringbus_t rb1 = {2, SFO_PERIODIC_ADJ_CMD | 0x41};
    // zmqEventLater(1, &rb1, 300);

    // raw_ringbus_t rb2 = {2, SFO_PERIODIC_ADJ_CMD | 0x42};
    // zmqEventLater(1, &rb2, 700);

    // raw_ringbus_t rb3 = {2, SFO_PERIODIC_ADJ_CMD | 0x43};
    // zmqEventLater(1, &rb3, 1000*3000);

    // cout << "ending queue" << endl;
    // event_add(fsm_init);

}

// called by handleCustomEventCallback, which is called by the buffer event
void EventDsp::handleCustomEvent(const custom_event_t* const e) {
    cout << "############### handleCustomEvent() " << e->d0 << "\n";
    // if(role == "rx") {
    // }
    // if(role == "tx") {
    // }
    handleCustomEventRx(e);
    handleCustomEventTx(e);

    // this could be moved up, into the buffer event callback
    if(soapy->eventCallback) {
        // cout << "soapy->eventCallback\n";
        soapy->eventCallback(e->d0,e->d1);
    } else {
        cout << "handleCustomEvent() fired and soapy->eventCallback NOT SET\n";
    }
}

void EventDsp::clearRecentEvent() {
    most_recent_event.d0 = NOOP_EV;
}

void EventDsp::handleCustomEventRx(const custom_event_t* const e) {
    cout << "EventDsp::handleCustomEventRx() " << e->d0 << ", " << e->d1 << endl;

    switch(e->d0) {
        case FINISHED_TDMA_EV:
        case RADIO_ENTERED_BG_EV:
        case FSM_PING_EV:
        case FSM_PONG_EV:

            cout << "EventDspFsm got event " << e->d0 << " with d1 " << e->d1 << endl;

            // cout << "Radio " << this->array_index << " matched event" << endl;
            most_recent_event = *e; // this is how we tell the fsm we got an event
            // event_active(localfsm_next_timer, EV_WRITE, 0);
            // cout << "After event_active" << endl;

            break;
        case UDP_SEQUENCE_DROP_EV: {
            // udp dropped, we can reset FSMS or whatever here

            stringstream ss;
            ss << "UDP DROPPED " << e->d1;
            tickleLog("rx",  ss.str() );
            
            break;
        }

    }
}

void EventDsp::createStatusRx() {
    cout << "EventDsp::createStatusRx()" << endl;
    status->create("rx.attached.fb.datarate", 2);
}


void EventDsp::fsmSetup_fixme() {
    // usleep(500);
    ///////////////////////////////////////////////////////////////////
    // for our own radio
    // soapy->cs00SetSfoAdvance(0, 0);
    // usleep(500);
    
    // zhen
    // soapy->setDSA(0x0040);
    
    // ben
    // FIXME
    // soapy->setDSA(0x0200);
    // soapy->setDSA(0x0100);
    // soapy->setDSA(GET_ADC_DSA_GAIN());

    uint32_t dsa_setting = GET_ADC_DSA_GAIN_DB() & 0xfe; // make even
    constexpr uint32_t dsa_channel_a = 0x00010000; (void)dsa_channel_a;
    constexpr uint32_t dsa_channel_b = 0x00000000; (void)dsa_channel_b;
    constexpr uint32_t dsa_gain_addr = 0x00000200;
    uint32_t dsa_rb = dsa_setting | dsa_channel_b | dsa_gain_addr;

    cout << "setting DSA rb to " << HEX32_STRING(dsa_rb) << " based on setting of " << dsa_setting << endl;

    soapy->setDSA(dsa_rb);

}

void EventDsp::onetimeTieMasterFSM() {
    
    // std::vector<std::string> path0 = {"global", "times_blocked_fsm_start"};
    tieDashboard<uint32_t>(&times_blocked_fsm_start, TIE_FSM_UUID, "fsm", "times_blocked_fsm_start");

    tieDashboard<size_t>(
        &dsp_state, TIE_FSM_UUID, 
        "fsm", "state");

#ifdef FIXME_LATER_MOVED_TO_TX_SCHEDULE_CPP
    tieDashboard<unsigned>( &verifyHashTx->count_sent, TIE_FSM_UUID, 
        "data", "hash", "verify_tx", "count_sent");
    tieDashboard<unsigned>( &verifyHashTx->count_got, TIE_FSM_UUID, 
        "data", "hash", "verify_tx", "count_got");
    tieDashboard<unsigned>( &verifyHashTx->count_stale_expected, TIE_FSM_UUID, 
        "data", "hash", "verify_tx", "count_stale_expected");
    tieDashboard<unsigned>( &verifyHashTx->count_stale_received, TIE_FSM_UUID, 
        "data", "hash", "verify_tx", "count_stale_received");
    tieDashboard<double>( &verifyHashTx->percent_correct, TIE_FSM_UUID, 
        "data", "hash", "verify_tx", "percent_correct");

    tieDashboard<unsigned>( &verifyHashTx->bytes_sent, TIE_FSM_UUID, 
        "data", "hash", "verify_tx", "bytes_sent");
    tieDashboard<unsigned>( &verifyHashTx->bytes_got, TIE_FSM_UUID, 
        "data", "hash", "verify_tx", "bytes_got");
    tieDashboard<double>( &verifyHashTx->bytes_per_second, TIE_FSM_UUID, 
        "data", "hash", "verify_tx", "bytes_per_second");
#endif

    reset_hash_timer = std::chrono::steady_clock::now();
}

void EventDsp::shutdownThreadTx() {
    sendEvent(MAKE_EVENT(EXIT_DATA_MODE_EV, 1));
}

void EventDsp::slowUpdateDashboard() {
    // radio tie
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

#ifdef FIXME_LATER_MOVED_TO_TX_SCHEDULE_CPP
    tickle( &verifyHashTx->count_sent);
    tickle( &verifyHashTx->count_got);
    tickle( &verifyHashTx->count_stale_expected);
    tickle( &verifyHashTx->count_stale_received);
    tickle( &verifyHashTx->percent_correct);
    tickle( &verifyHashTx->bytes_sent);
    tickle( &verifyHashTx->bytes_got);
    tickle( &verifyHashTx->bytes_per_second);


#endif
    
    unsigned age = std::chrono::duration_cast<std::chrono::microseconds>( 
            now - reset_hash_timer
    ).count();

    if( age > (60*1E6) ) {
        reset_hash_timer = now;
        soapy->txSchedule->verifyHashTx->resetCounters();
    }

}



// returns 0 for ok
// may run on another thread
int EventDsp::changeState(const uint32_t desired) {
    cout << "changeState called with " << desired << "\n";

    std::unique_lock<std::mutex> lock(_change_pre_mutex);
    if( _change_pending ) {
        cout << "changeState() dropping request due to pending request\n";
        return 1;
    }

    _pending_change_state = desired;
    _change_pending = true;
    
    return 0;
}


// undoes state added by
//   handle_sfo_cfo_callback
//   sfo_cfo_buf
void EventDsp::resetCoarseState() {
    // don't need to reset this, this is a direct measurement of the hardware's state
    // fuzzy_recent_frame = 0;

    // this flag stops this same thread from inserting samples
    _rx_should_insert_sfo_cfo = false;

    // this dumps the existing samples in the buffer
    // assuming these two are on the same thread, this should not leave
    // anything in the buffer
    soapy->higgsFineSync->dump_sfo_cfo_buffer();

    for( auto r : radios ) {
        r->resetCoarseState();
    }






    // evbuffer_drain(buf, largest_read);
}

void EventDsp::executeChangeState() {
    std::unique_lock<std::mutex> lock(_change_pre_mutex);
    if( !_change_pending ) {
        return;
    }
    _change_pending = false;

    // dsp_state vs dsp_state_pending
    // dsp_state is the state of the code which we have actually run
    // dsp_state_pending is the state which set by next = GO_AFTER(...)
    // when we enter here, the code in dsp_state_pending was requested, but has not yet run
    // so we make decisions based on dsp_state;
    auto state = dsp_state;

    if( _pending_change_state == PRE_BOOT_2 ) {
        bool ok = false;
        switch(state) {
            case DID_BOOT:
            case DO_WAIT:
            case COARSE_SCHEDULE_0:
            case CORASE_SCHEDULE_DELAY_0:
            case CORASE_SYMBOL_WAIT_0:
            case WAIT_COARSE_0:
            case DO_TRIGGER_0:
            case WAIT_FOR_RADIO_TO_FINE_0:
            case WAIT_FOR_RADIO_0_EQ:
                ok = true;
                break;
            default:
                break;
        }

        if( ok ) {
            cout << "std::function executed (PRE_BOOT_2)\n";


            // cout << "Before: " << this->dsp_state_pending << "\n";
            this->dsp_state_pending = PRE_BOOT_2;
            // cout << "Before: " << this->dsp_state_pending << "\n";
        } else {
            cout << "cannot change into PRE_BOOT_2 from " << state << "\n";
        }
    } else {
        cout << "changeState() does not know how to change into " << _pending_change_state << "\n";
    }
}

void EventDsp::peerConnectedReply(uint32_t id) {
    if( !peer_state_got.size() ) {
        peer_state_got.resize(32,0);
    }

    // cout << "peerConnectedReply() called with " << id << "\n";

    if( id >= peer_state_got.size() ) {
        cout << "peerConnectedReply() called with illegally large argument " << id << "\n";
        return;
    }

    peer_state_got[id]++;
}

bool EventDsp::allPeersConnected() {
    // cout << "soapy->connected_node_count " << soapy->connected_node_count << "\n";

    if( !peer_state.size() ) {
        peer_state.resize(32,0);
    }

    if( !peer_state_sent.size() ) {
        peer_state_sent.resize(32,0);
    }

    if( !peer_state_got.size() ) {
        peer_state_got.resize(32,0);
    }

    const auto all = soapy->getTxPeers();
    // auto ids = std::vector<size_t>{tx_peer_0, tx_peer_1};

    const auto wait = DSP_WAIT_FOR_PEERS();

    if( wait == 0 ) {
        cout << "Weird behavior in allPeersConnected(), DSP_WAIT_FOR_PEERS() was set to 0\n";
        return true;
    }

    for(unsigned i = 0; i < all.size(); i++) {
        auto x = all[i];

        sendVectorTypeToPartnerPC(x, FEEDBACK_VEC_REQUEST_ID, {soapy->this_node.id});
    }

    bool ok = true;

    std::vector<size_t> found;

    for(unsigned i = 0; i < all.size(); i++) {
        auto x = all[i];

        if( peer_state_got[x] == 0) {
            ok = false;
        }
    }

    for(unsigned i = 0; i < peer_state_got.size(); i++) {
        if( peer_state_got[i] ) {
            found.push_back(i);
        }
    }

    if( fsm_print_peers ) {
        cout << "Looking for peers: ";
        for(auto x : all) {
            cout << x << ",";
        }

        cout << " found peers: ";
        for(auto x : found) {
            cout << x << ",";
        }

        cout << "\n";
    }




    // if( peer_state_sent == 0)


    // if( peer_state.find("foo") == peer_state.end() ) {
    //     peer_state["foo"] = 0;
    // } else {
    //     peer_state["foo"] = peer_state["foo"].get<int>() + 1;
    // }

    // cout << "foo " << peer_state["foo"] << "\n";


    // soapy->connected_node_count >= DSP_WAIT_FOR_PEERS()
    return ok;
}


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-value"
void EventDsp::tickFsmRx() {

    if( blockFsm > 0 ) {
        blockFsm--;
    }

    if( blockFsm == 0 ) {
        cout << "FSM Hand stalled at " << dsp_state << "\n";
        auto timm = _500_MS_SLEEP;
        evtimer_add(fsm_next_timer, &timm);
        return;
    }

    executeChangeState();

    dsp_state = dsp_state_pending;
    tickle(&dsp_state);
    auto __dsp_preserve = dsp_state;
    struct timeval tv = DEFAULT_NEXT_STATE_SLEEP;
    size_t __timer = __UNSET_NEXT__;
    size_t __imed = __UNSET_NEXT__;
    size_t next = __UNSET_NEXT__;

    uint32_t recent_frame = fuzzy_recent_frame;
    (void)recent_frame;

    switch(dsp_state) {
        case PRE_BOOT:
            cout << "tickFsmRx PRE_BOOT\n";
            if( this->role == "arb" ) {
                cout << endl << endl << "EventDsp::tickFSM detected non RX mode, shutting down" << endl << endl;
                next = GO_NOW(TX_STALL);
            } else {
            // std::string a("foo bar bax hello");
            // this->sendToMock(a);
                // our role is rx
                cout << endl << endl;
                cout << "node.csv encodes our id as                " << soapy->this_node.id << endl;
                cout << "init.json encodes rx radio's peer id as   " << GET_PEER_RX() << endl;


                if( soapy->this_node.id != GET_PEER_RX() ) {
                    // FIXME we can get rid of this error if we refactor node.csv
                    cout << endl << endl << endl << "----------------------------------------------------" << endl;
                    cout << " DEBUG_STALL: rx-side init.json does not match rx-side node.csv" << endl;
                    cout << "----------------------------------------------------" << endl;
                    next = GO_NOW(DEBUG_STALL);
                    break;
                    // assert( soapy->this_node.id == GET_PEER_RX() );
                    // exit(1);
                }

                if( GET_DEBUG_STALL() ) {
                    next = GO_NOW(DEBUG_STALL);
                    break;
                }

                cout << "EventDsp:: waiting for " << DSP_WAIT_FOR_PEERS() << " peers" << endl;
                // next = GO_AFTER(AFTER_PRE_BOOT, _3_SECOND_SLEEP);
                next = GO_AFTER(WAIT_FOR_SELF_SYNC, _1_MS_SLEEP);
                tickleAll(); // don't call this often
            }
        break;

        case WAIT_FOR_SELF_SYNC: {
            if( GET_RUNTIME_SELF_SYNC_DONE() ) {
                next = GO_AFTER(AFTER_PRE_BOOT, _1_MS_SLEEP);
            } else {
                // static int self_sync_wait_rx = 0;
                // cout << "RX Waiting for self sync " << self_sync_wait_rx++ << "\n";
                next = GO_AFTER(WAIT_FOR_SELF_SYNC, _300_MS_SLEEP);
            }
            break;
        }

        case AFTER_PRE_BOOT:
            {
                // cout << "sending event\n";
                // sendEvent(MAKE_EVENT(REQUEST_FINE_SYNC_EV,0));
                // usleep(1000);
                // sendEvent(MAKE_EVENT(RADIO_ENTERED_BG_EV,0));
                // sendEvent(MAKE_EVENT(REQUEST_FINE_SYNC_EV,1));
                bool do_block = GET_BLOCK_FSM();
                // tickleLog("rx", "hello mate");

                // cout << "Checking do_block value: " << do_block << endl;

                if( do_block ) {
                    if( false ) {
                    cout << "will loop" << endl;
                    cout << "double: " << settings->getDefault<double>(99, "runtime", "some_double") << endl;
                    
                    auto got = settings->vectorGet<uint32_t>({"net", "network", "discovery_port0", "fo"});
                    bool ok;
                    uint32_t value;
                    std::tie(ok, value) = got;
                    cout << "vector get " << value << " ok: " << ok << endl;

                    auto got2 = settings->vectorGet<std::string>({"exp", "our", "rolee"});
                    std::string value_s;
                    std::tie(ok, value_s) = got2;

                    cout << "vector get2 " << value_s << " ok: " << ok << endl;

                    auto role_or_default = settings->vectorGetDefault<std::string>("not found", {"exp", "our", "rolee"});

                    cout << "vector get3 " << role_or_default << endl;



                    auto bool_default = settings->vectorGetDefault<bool>(true, {"global","print","PRINT_PEER_RB"});

                    cout << "vector get4 " << bool_default << endl;

                }



                // times_blocked_fsm_start++;
                // tickle(&times_blocked_fsm_start);

                next = GO_AFTER(AFTER_PRE_BOOT,_1_SECOND_SLEEP);
            } else {
                // this used to be TINY_SLEEP
                // for some reason the WAIT_FOR_PEERS class was running many many times
                // which does not make any sense
                next = GO_AFTER(WAIT_FOR_PEERS, _1_SECOND_SLEEP);

				if(0) { // INTEGRATION_TEST
                	// next = GO_AFTER(WAIT_FOR_PEERS, _1_SECOND_SLEEP);
                	soapy->demodThread->dropSlicedData = false;
                	next = GO_AFTER(DEBUG_STALL, _1_SECOND_SLEEP);
				}
            }
        }
        break;


        case WAIT_FOR_PEERS:
            // cout << "WAIT_FOR_PEERS\n";
            if(allPeersConnected()) {
                cout << "EventDsp:: found " << soapy->connected_node_count << " peers" << endl;
                for(auto r : radios) {
                    sendPartnerNoop(r->peer_id);
                }

                    // event_base_dump_events(evbase, stdout);
                    
                    bool normal = true;

                    if( GET_FLOOD_PING_TEST() ) {
                        normal = false;
                        next = GO_AFTER(DEBUG_EARLY_EVENT_0, _1_SECOND_SLEEP);
                    }

                    if( GET_DEBUG_TX_LOCAL_EPOC() ) {
                        normal = false;
                        next = GO_AFTER(DEBUG_TX_LOCAL_EPOC_0, _1_SECOND_SLEEP);
                    }

                    if( normal ) {
                        next = GO_AFTER(TICKLE_PEERS, _1_SECOND_SLEEP);
                    }
                // next = GO_AFTER(DEBUG_PERF_0, _300_MS_SLEEP);
                // next = GO_AFTER(DEBUG_PEER_PERFORMANCE_0, _300_MS_SLEEP);


            } else {
                // cout << "next" << endl;
                next = GO_AFTER(WAIT_FOR_PEERS, _1_SECOND_SLEEP);
                // next = GO_AFTER(WAIT_FOR_PEERS, _1_MS_SLEEP);
            }
            break;

        // case DEBUG_PEER_PERFORMANCE_0: {
        //         cout << "sent performance to cs20" << endl;
        //         // dump
        //         raw_ringbus_t rb0 = {RING_ADDR_CS20, PERF_CMD | (1<<16) };
        //         zmqRingbusPeerLater(GET_PEER_0(), &rb0, 0);

        //         // reset
        //         raw_ringbus_t rb1 = {RING_ADDR_CS20, PERF_CMD | (2<<16) };
        //         zmqRingbusPeerLater(GET_PEER_0(), &rb1, 10000);

        //         // dump
        //         raw_ringbus_t rb2 = {RING_ADDR_CS20, PERF_CMD | (1<<16) };
        //         zmqRingbusPeerLater(GET_PEER_0(), &rb2, 11000);

        //         auto longish = (struct timeval){ .tv_sec = 13, .tv_usec = 0 };
        //         next = GO_AFTER(DEBUG_PEER_PERFORMANCE_0, longish);

        //         // next = GO_NOW(DEBUG_STALL);

        //     }
        //     break;

        // case DEBUG_PERF_0:
        //         // debug
        //         soapy->rb->send(RING_ADDR_CS31, PERF_CMD | (0x1<<16)  );
        //         next = GO_AFTER(DEBUG_PERF_1, _3_SECOND_SLEEP);
        //     break;

        // case DEBUG_PERF_1:
        //         // debug
        //         cout << "sent reset" << endl;
        //         soapy->rb->send(RING_ADDR_CS31, PERF_CMD | (0x2<<16)  );
        //         next = GO_AFTER(DEBUG_PERF_2, _1_SECOND_SLEEP);
        //     break;


        // case DEBUG_PERF_2:
        //         // debug
        //         cout << "second dump" << endl;
        //         soapy->rb->send(RING_ADDR_CS31, PERF_CMD | (0x1<<16)  );
        //         next = GO_AFTER(TICKLE_PEERS, _8_SECOND_SLEEP);
        //     break;


        case TICKLE_PEERS:
            cout << "TICKLE_PEERS" << endl;


            // for(auto r : radios) {
            //     auto id = r->peer_id;
            //     setPartnerSfoAdvance(id, 0, 0);

            //     // raw_ringbus_t rb0 = {RING_ADDR_ETH, MAPMOV_RESET_CMD};
            //     // zmqRingbusPeerLater(id, &rb0, 0);
            // }
            setPartnerSfoAdvance(GET_OUR_ID(), 0, 0);
            //FIXME: 
            // coarse_buf.get(coarse_buf.size()); // will dump data


            next = GO_AFTER(TICKLE_PEERS_1, _1_SECOND_SLEEP);


            break;

        case TICKLE_PEERS_1:
            // for(auto r : radios) {
            //     auto id = r->peer_id;
            //     setPartnerCfo(id, 0);
            // }
            setPartnerCfo(GET_OUR_ID(), 0);
            next = GO_AFTER(TICKLE_PEERS_2, _50_MS_SLEEP);
            break;
        case TICKLE_PEERS_2:
            for(auto r : radios) {
                auto id = r->peer_id;
                resetPartnerEq(id);
                // resetPartnerBs(id);
            }
            resetPartnerEq(GET_OUR_ID());
            // resetPartnerBs(GET_OUR_ID());
            next = GO_AFTER(TICKLE_PEERS_3, _50_MS_SLEEP);
            break;
        case TICKLE_PEERS_3:
            for(auto r : radios) {
                // auto id = r->peer_id;
                r->setPartnerTDMA(20, 0); // 20 is off mode
                r->setPartnerTDMA(21, 0); // 21 will reset  pending_data = 0;

                //ringbusPeerLater(r->peer_id, RING_ADDR_CS11, COOKED_DATA_TYPE_CMD | 0, 1);
            }
            setPartnerTDMA(GET_OUR_ID(), 20, 0);
            setPartnerTDMA(GET_OUR_ID(), 21, 0);

            next = GO_AFTER(TICKLE_PEERS_3_DOT_5, _50_MS_SLEEP);
            break;
        case TICKLE_PEERS_3_DOT_5:
            // do not aligned canned
            cout << "reset canned mode" << endl;

            for(auto r : radios) {
                auto id = r->peer_id;
                resetPartnerBs(id);
            }
            resetPartnerBs(GET_OUR_ID());


            // I think I got rid of this?...
            // setPartnerScheduleModeIndex(radios[0]->array_index, 1, 0);
            // setPartnerScheduleModeIndex(radios[1]->array_index, 1, 0);
            soapy->cs11TriggerMagAdjust(0);
            next = GO_AFTER(TICKLE_PEERS_3_DOT_55, _50_MS_SLEEP);
            break;

        case TICKLE_PEERS_3_DOT_55:
            // reset once here
            // then loop below this prevents weird timing?
            for(auto r : radios) {
                r->resetFeedbackBackDetectRemote();
            }

            next = GO_AFTER(TICKLE_PEERS_3_DOT_6, TINY_SLEEP);
            break;

        case TICKLE_PEERS_3_DOT_6:
            for(auto r : radios) {
                cout << "TICKLE_PEERS_3_DOT_6 r" << r->array_index << endl;
                
                r->feedbackPingRemote();

                // raw_ringbus_t rb0 = {RING_ADDR_CS20, REQUEST_EPOC_CMD};
                // zmqRingbusPeerLater(r->peer_id, &rb0, 0);

                // GET_DEBUG_TX_EPOC()
                // setPartnerSchedule(*r, schedule_off);
                // setPartnerSchedule(*r, schedule_on);
                // setPartnerSchedule(*r, schedule_toggle);
            }

            // setPartnerSchedule(*radios[0], schedule_off);

            next = GO_AFTER(TICKLE_PEERS_3_DOT_7, _1_SECOND_SLEEP);
            break;
            
        case TICKLE_PEERS_3_DOT_7: {
                uint32_t found = 0;

                cout << "TICKLE_PEERS_3_DOT_7: " << endl;
                for(auto r : radios) {
                    cout << "r" << r->array_index << "(peer " << r->peer_id << ") got fb status replies: " << r->feedback_alive_count << endl;
                    if(r->feedback_alive_count > 0) {
                        found++;
                    }
                }

                if( found < DSP_WAIT_FOR_PEERS() ) {
                    cout << "found       : " << found << endl;
                    cout << "need to find: " << DSP_WAIT_FOR_PEERS() << endl;

                    cout << "-------------------------------------------" << endl;
                    cout << "Feedback bus is down for 1 or more TX peers" << endl;
                    cout << "Try re-starting gnuradio app, then "          << endl;
                    cout << "try re-programming tx-bitfiles"              << endl;
                    cout << "-------------------------------------------" << endl;
                    cout << "" << endl;
                    cout << "" << endl;
                    next = GO_AFTER(TICKLE_PEERS_3_DOT_6, _50_MS_SLEEP);
                } else {
                    next = GO_AFTER(TICKLE_PEERS_4, _50_MS_SLEEP);
                }

            }
            break;

        // case TICKLE_PEERS_3_DOT_8:
        //     break;
        case TICKLE_PEERS_4:
            // setPartnerSchedule(*radios[0], schedule_on);
            // setPartnerSchedule(*radios[1], schedule_off);
            
            // FIXME DEBUG switch these
            next = GO_AFTER(PRE_BOOT_2, _1_SECOND_SLEEP);
            // cout<< "observe reset!!!!!"<<endl;
            // next = GO_AFTER(DEBUG_PEER_PERFORMANCE_0, _300_MS_SLEEP);
            

            // cout<< "observe reset!!!!!"<<endl;
            break;



        case PRE_BOOT_2:
            cout << "PRE_BOOT_2" << endl;
            // setPartnerSchedule(*radios[0], schedule_off);
            // setPartnerSchedule(*radios[1], schedule_off);

            // default is true
            // radios[0]->should_mask_data_tone_tx_eq = false;
            // radios[1]->should_mask_data_tone_tx_eq = true;

            // radios[0]->initialMask();
            // radios[1]->initialMask();

            // should set all to zero
            // this code was found to trigger s-modem issue #59
            
            
            if(GET_SFO_RESET_ZERO()) {
                cout << "RESETTING SFO for R0 R1\n";
                radios[0]->sfo_estimated = radios[1]->sfo_estimated = 0;
                radios[0]->updatePartnerSfo();
                radios[1]->updatePartnerSfo();
            }
            if(GET_CFO_RESET_ZERO()) {
                cout << "RESETTING CFO for R0 R1\n";
                radios[0]->cfo_estimated = radios[1]->cfo_estimated = 0;
                radios[0]->updatePartnerCfo();
                radios[1]->updatePartnerCfo();
            }

            // the idea was to always exit data mode if user restarted rx but not tx
            // however this goes wrong when both sides ARE restarted
            // sendCustomEventToPartner(radios[0]->peer_id, MAKE_EVENT(EXIT_DATA_MODE_EV, 0) );
            // sendCustomEventToPartner(radios[1]->peer_id, MAKE_EVENT(EXIT_DATA_MODE_EV, 0) );

            cout << "Are both off?" << endl;

            // next = GO_AFTER(DID_BOOT, _1_SECOND_SLEEP);
            next = GO_AFTER(SET_RANDOM_ROTATION, _2_SECOND_SLEEP);
            // next = GO_AFTER(SET_RANDOM_ROTATION, _50_MS_SLEEP);
            break;

        case SET_RANDOM_ROTATION: {

            if(this->role == "rx") {
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS31, COOKED_DATA_TYPE_CMD, 1000);
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS32, COOKED_DATA_TYPE_CMD, 2000);
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS22, COOKED_DATA_TYPE_CMD, 3000);
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS21, COOKED_DATA_TYPE_CMD, 4000);
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS11, COOKED_DATA_TYPE_CMD, 5000);

                
                // usleep(6000);
            }

            soapy->cs31CFOTurnoff(0);

            soapy->cs32SFOCorrection(0);

            // radios[0]->setRandomRotationEq();
            // radios[1]->setRandomRotationEq();
            attached->setRandomRotationEq();

            cout<<"Set random rotation (of each subcarrier) to all radios\n";


            const uint32_t rb_for_rx = soapy->demodThread->airRx->getRbForRxSlicer();

            // tell rx side to demod qam
            raw_ringbus_t rb0 = {RING_ADDR_RX_TAGGER, RX_DEMOD_MODE | rb_for_rx};
            ringbusPeerLater(soapy->this_node.id, &rb0, 0);


             if( GET_SFO_APPLY_INITIAL() ) {
                next = GO_AFTER(SET_SFO_INIT_0, _50_MS_SLEEP);
             } else if( GET_CFO_APPLY_INITIAL() ) {
                next = GO_AFTER(SET_CFO_INIT_0, _50_MS_SLEEP);
             } else {
                next = GO_AFTER(DID_BOOT, _50_MS_SLEEP);
             }

             }
             break;

        case SET_SFO_INIT_0:

             radios[0]->sfo_estimated = GET_SFO_INIT_T0();
             radios[0]->updatePartnerSfo();

             //soapy->cs31CoarseSync(0,0);
             soapy->cs32EQData(1);
             
             radios[1]->sfo_estimated = GET_SFO_INIT_T1();
             radios[1]->updatePartnerSfo();
             
             cout<<"Update SFO based on prior information"<<endl;

             if( GET_CFO_APPLY_INITIAL() ) {
                next = GO_AFTER(SET_CFO_INIT_0, _50_MS_SLEEP);
             } else {
                next = GO_AFTER(DID_BOOT, _50_MS_SLEEP);
             }

             break;

        case SET_CFO_INIT_0:

             radios[0]->cfo_estimated = GET_CFO_INIT_T0();
             radios[0]->updatePartnerCfo();
             
             radios[1]->cfo_estimated = GET_CFO_INIT_T1();
             radios[1]->updatePartnerCfo();
             
             cout<<"Update CFO based on prior information!!!!!!!"<<endl;

             next = GO_AFTER(DID_BOOT, _50_MS_SLEEP);

             break;

        case DID_BOOT:
            cout << "DID_BOOT" << endl;
            tickleAll();
            // next = GO_NOW(DO_WAIT);
            next = GO_AFTER(RX_SYNC_1, _2_SECOND_SLEEP);
            // _do_wait = 10000;
            break;

        case RX_SYNC_1:
            forceZeros(radios[0]->peer_id, false); // same as schedule_on
            // setPartnerSchedule(*radios[0], schedule_on);
            next = GO_NOW(RX_SYNC_2);
            break;

        case RX_SYNC_2: {
            // jsEval("console.log('hi world');");
            std::string eval = "\
            let current = sjs.getIntDefault(-1, 'hardware.tx.bootload.after.tx_channel.current');\
            sjs.current(current);\
            console.log('RX radio is transmitting');\
            \
            sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x57000010, 1);\
            sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x57010010, 1);\
            sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x57020010, 1);\
            sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x5703000f, 1);\
            sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x5704000f, 1);\
            \
            \
            for(let p0 of sjs.getTxPeers()) { \
                \
                /*let p0 = sjs.getIntDefault(-1, 'exp.peers.t0');*/ \
                sjs.zmq.id(p0).eval('sjs.safe();sjs.dsa(sjs.settings.getIntDefault(0, \"hardware.rx.bootload.after.set_dsa.attenuation\"))');\
                sjs.zmq.id(p0).eval('this.sjs.clog(\"SETTING DSA TO \" +sjs.settings.getIntDefault(0, \"hardware.rx.bootload.after.set_dsa.attenuation\"))');\
                \
                sjs.dsp.ringbusPeerLater(p0, hc.RING_ADDR_TX_FFT, 0x57000010, 1);\
                sjs.dsp.ringbusPeerLater(p0, hc.RING_ADDR_TX_FFT, 0x57010010, 1);\
                sjs.dsp.ringbusPeerLater(p0, hc.RING_ADDR_TX_FFT, 0x57020010, 1);\
                sjs.dsp.ringbusPeerLater(p0, hc.RING_ADDR_TX_FFT, 0x5703000f, 1);\
                sjs.dsp.ringbusPeerLater(p0, hc.RING_ADDR_TX_FFT, 0x5704000f, 1);\
            }\
            ";
            jsEval(eval);

            if( GET_DSP_LONG_RANGE() ) {
                cout << "RX Radio is using LONG RANGE for coarse sync\n";
                std::string eval2 = "\
                sjs.attached.setAllEqMask(sjs.masks.m0_neg); \
                sjs.attached.updatePartnerEq(false, false); \
                \
                sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x5700000e, 1);\
                sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x5701000f, 1);\
                sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x5702000f, 1);\
                sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x5703000f, 1);\
                sjs.dsp.ringbusPeerLater(sjs.id, hc.RING_ADDR_TX_FFT, 0x5704000f, 1);\
                \
                sjs.ringbus(hc.RING_ADDR_CS11, hc.TX_TONE_2_VALUE | 0x6000); \
                \
                ";
                jsEval(eval2);
            }

            // newest narrow band
            if( true ) {
                std::string eval3 = "\
                sjs.attached.setAllEqMask(sjs.masks.fine_sync); \
                sjs.attached.updatePartnerEq(false, false); \
                sjs.bs('tx', 0, 75, true); \
                \
                sjs.ringbus(hc.RING_ADDR_CS11, hc.TX_TONE_2_VALUE | 0x2000); \
                \
                ";
                jsEval(eval3);
            }

            // jsEval("sjs.demodThread.air.set_interleave(0)");

            // next = GO_NOW(DEBUG_STALL);
            next = GO_NOW(DO_WAIT);
            break;
        }






        case DO_WAIT:
            cout << "DO_WAIT" << endl;
            next = GO_AFTER(COARSE_SCHEDULE_0, _1_SECOND_SLEEP);
            // _do_wait--;
            // if(_do_wait == 0) {
            //     next = COARSE_SCHEDULE_0;
            // }
            break;
        case COARSE_SCHEDULE_0: {
            // cout << "COARSE_SCHEDULE_0" << endl;
            // cout << "Leaving DO_WAIT" << endl;
            // cout << recent_frame << " (" << (recent_frame % SCHEDULE_FRAMES) << ")" << endl;
            // if((recent_frame % SCHEDULE_FRAMES) < 1000) {
                // resetPartnerScheduleAdvance(*radios[0]);
                // resetPartnerScheduleAdvance(*radios[1]);

                // epoc_frame_t rx_time = pure_frames_to_epoc_frame(recent_frame);

                // use rough estimate of rx counter
                // to try and set both tx radios to the same ball park
                // we will sync perfectly in TDMA later
                for( auto r : radios) {
                    // r->partnerOp("set", 3, rx_time.epoc); // epoc
                    // r->partnerOp("set", 0, recent_frame); // litetime_32

                    r->setTDMASc();  // choose which sc to use
                    r->maskAllSc();  // mask other sc to TDMA of partner works
                }



                next = GO_NOW(CORASE_SCHEDULE_DELAY_0);
                // setPartnerSchedule(ids[0], schedule_toggle);
                // setPartnerSchedule(ids[1], schedule_toggle);
                // setPartnerSchedule(ids[0], schedule_bringup);
                forceZeros(radios[0]->peer_id, false); // same as schedule_on
                // setPartnerSchedule(*radios[0], schedule_on);
                cout << "Is anything back on?\n";
                cout << "Partner 0 in bringup\n";
            // } else {
                // cout << recent_frame << " (" << (recent_frame % SCHEDULE_FRAMES) << ")\n";
                // normally we would go slowly
                // but this schedules us to check very fast
                // next = GO_AFTER(COARSE_SCHEDULE_0, SMALLEST_SLEEP);
            // }
            }
            break;

        case DUPLEX_COARSE_0: {
                cout << "DUPLEX_COARSE_0\n";
                int ofdm_amount = GET_COARSE_SYNC_NUM();
                cout << "COARSE_SYNC_OFDM_NUM: " << ofdm_amount << "\n";
                for(const auto p : soapy->getTxPeers()) {
                    soapy->duplexTriggerSymbolSync(p, ofdm_amount);
                }
                // soapy->cs31TriggerSymbolSync(ofdm_amount);
                next = GO_AFTER(DUPLEX_COARSE_1, _2_SECOND_SLEEP);
            }
            break;

        case DUPLEX_COARSE_1:
            _rx_should_insert_sfo_cfo = true;

            

            if( GET_DSP_LONG_RANGE() ) {
                cout << "RX Radio is using LONG RANGE for fine sync\n";
                std::string eval = "\
                sjs.attached.setAllEqMask(sjs.masks.fine_sync); \
                sjs.attached.updatePartnerEq(false, false); \
                ";
                jsEval(eval);
            }

            // always true for narrow mode
            if( true ) {
                // std::string eval = "\
                // sjs.attached.setAllEqMask(sjs.masks.narrow_1); \
                // sjs.attached.updatePartnerEq(false, false); \
                // ";
                // jsEval(eval);


                // send this once
                // used for EQ_1 (only used in duplex mode)
                attached->sendEqOne();
            }



            for(const auto p : soapy->getTxPeers()) {
            
                sendCustomEventToPartner(
                        p,
                        MAKE_EVENT(DUPLEX_FINE_SYNC, 0)
                        );
            }

            


            next = GO_NOW(DEBUG_STALL);
            // DO_TRIGGER_0
            break;
        case DUPLEX_COARSE_2:
            break;

        


        case CORASE_SCHEDULE_DELAY_0:
            cout << "Waited for 1 period" << endl;
            resetCoarseState();
            // next = GO_AFTER(CORASE_SYMBOL_WAIT_0, _1_SECOND_SLEEP);
            next = GO_AFTER(DUPLEX_COARSE_0, _1_SECOND_SLEEP);

            
            break;
        case CORASE_SYMBOL_WAIT_0: {

            
            // if((recent_frame % SCHEDULE_FRAMES) < 200) {
                cout << "CORASE_SYMBOL_WAIT_0" << endl;
                int ofdm_amount = GET_COARSE_SYNC_NUM();
                cout << "COARSE_SYNC_OFDM_NUM: " << ofdm_amount << "\n";
                soapy->cs31TriggerSymbolSync(ofdm_amount);
                next = GO_AFTER(WAIT_COARSE_0, _1_MS_SLEEP);
            // }
            }
            break;
        // case COARSE_0:
        //     cout << "COARSE_0" << endl;
        //     // radios[0]->triggerCoarse();
        //     cs31TriggerSymbolSync();
        //     next = WAIT_COARSE_0;
        //     break;
        case WAIT_COARSE_0:
            // if(radios[0]->coarse_finished) {
            //     // next = FINISH_COARSE_0;
            //     next = DEBUG_STALL;
            //     _do_wait = 10000;
            // }
            optional_rb_word = checkGetCoarseRb();
            if( optional_rb_word.first ) {
                cout << "DSP RB: " << HEX32_STRING(optional_rb_word.second) << endl;

                coarse_est = (1280-optional_rb_word.second) + GET_COARSE_SYNC_TWEAK();

                cout << "coarse_est " << coarse_est << endl;

                if( GET_DEBUG_COARSE_SYNC_ON_RX() ) {
                    // for debugging with mockjs
                    //coarse_est -= 128; // works on latest

                    coarse_est -= 0; // works on latest
                    cout << "GET_DEBUG_COARSE_SYNC_ON_RX() " << coarse_est << endl;
                    // setPartnerSfoAdvance(soapy->this_node.id, coarse_est, 4);
                    soapy->cs31CoarseSync(coarse_est, 3);

                } else {
                    // mode 4 will protect large values
                    setPartnerSfoAdvance(radios[0], coarse_est, 4);
                }

                // tell radio to start sync
                // argument is radio index, not peer_id

                if( GET_RETRY_COARSE() ) {
                    next = GO_AFTER(DO_RESET_COARSE, _4_SECOND_SLEEP);
                } else if( GET_STALL_BEFORE_R0_FINE_SYNC() ) {
                    cout << "Going to DEBUG_STALL because exp.stall_before_r0_fine_sync was set\n";
                    next = GO_NOW(DEBUG_STALL); // used for debugging cfo bug
                } else {
                    next = GO_AFTER(DO_TRIGGER_0, _16_SECOND_SLEEP);
                }



                // _prev_sfo_est = radios[0]->times_sfo_estimated;
                _rx_should_insert_sfo_cfo = true;

                

                // _prev_sto_est
            }
            cout << "waiting for coarse\n";
            break;
        // case FINISH_COARSE_0:
        //     _do_wait--;
        //     if(_do_wait == 0) {
        //         next = SFO_CFO_0;
        //     } else {
        //         if( _do_wait < 5000 && !radios[0]->coarse_ok) {
        //             if(_do_wait == 1) {
        //                 cout << "Seems like previous Coarse sync did not go well!! (" << _do_wait << ")" << endl;
        //                 next = COARSE_0;
        //             }
        //         }
        //     }
        //     break;
        case DO_RESET_COARSE:
            cout << " ======================= DO_RESET_COARSE =======================\n";
            resetCoarseState();
            if( GET_RETRY_COARSE_RAND() ) {
                int pick = rand() % 1280;
                cout << "randomizing to " << pick << "\n";
                setPartnerSfoAdvance(radios[0], pick, 4);
            }
            next = GO_AFTER(PRE_BOOT_2, _1_SECOND_SLEEP);
            break;


        case DO_TRIGGER_0:
            /////////////////////// soapy->cs31CFOTurnoff(1);
            cout << "DO_TRIGGER_0" << endl;
            cout << " sent REQUEST_FINE_SYNC_EV" << endl;
            sendEvent(MAKE_EVENT(REQUEST_FINE_SYNC_EV,0));
            next = GO_NOW(WAIT_FOR_RADIO_TO_FINE_0);
            break;


        case WAIT_FOR_RADIO_TO_FINE_0:
            if(most_recent_event.d0 == RADIO_ENTERED_BG_EV &&
                most_recent_event.d1 == 0) {
                soapy->cs11TriggerMagAdjust(1);
                //soapy->cs11TriggerMagAdjust(0);
                cout << "leaving WAIT_FOR_RADIO_TO_FINE_0" << endl;
                    // clear
                    clearRecentEvent();
                    next = GO_AFTER(WAIT_FOR_RADIO_0_EQ, _1_MS_SLEEP );
                }
            break;

        case WAIT_FOR_RADIO_0_EQ:
         if(
            radios[0]->times_residue_phase_sent > 100 &&
            radios[0]->times_eq_sent > 2 ) {
                cout << "leaving WAIT_FOR_RADIO_0_EQ" << endl;
                if(GET_FSM_SKIP_R1()) {
                    // skip r1, go to schedule r0
                    next = GO_AFTER(FINE_SCHEDULE_0, _3_SECOND_SLEEP);
                } else {
                    // normal, goes on to go cfo/sfo of r1
                    next = GO_AFTER(TRANSITION_0_TO_1, _50_MS_SLEEP);
                }

                // next = GO_AFTER(PRE_COARSE_SCHEDULE_0, _8_SECOND_SLEEP);
         }
        break;

///////////////////////////////////////////////////////////////////

        case TRANSITION_0_TO_1:
            cout << "TRANSITION_0_TO_1 DOES NOT EXIST" << endl;
            next = GO_NOW(DEBUG_STALL);
            // radios[0]->enable_residue_to_cfo = false;
            // setPartnerSchedule(*radios[0], schedule_off);
            // setPartnerSchedule(*radios[1], schedule_on);
            // next = GO_AFTER(CORASE_SYMBOL_WAIT_1, _50_MS_SLEEP);
            break;



        case CORASE_SYMBOL_WAIT_1:
                // if((recent_frame % SCHEDULE_FRAMES) < 4000) {
                    cout << "CORASE_SYMBOL_WAIT_1" << endl;
                    soapy->cs31TriggerSymbolSync();
                    next = GO_AFTER(WAIT_COARSE_1, _1_SECOND_SLEEP);
                // } else {
                //     next = GO_AFTER(CORASE_SYMBOL_WAIT_1, _1_SECOND_SLEEP);
                // }
                break;
        case WAIT_COARSE_1:
            cout << "WAIT_COARSE_1 DOES NOT EXIST" << endl;
            next = GO_NOW(DEBUG_STALL);
            // optional_rb_word = checkGetCoarseRb();
            // if( optional_rb_word.first ) {
            //     cout << "DSP RB: " << HEX32_STRING(optional_rb_word.second) << endl;

            //     coarse_est = (1280-optional_rb_word.second) + GET_COARSE_SYNC_TWEAK();

            //     setPartnerSfoAdvance(*radios[1], coarse_est, 4);
            //     next = GO_AFTER(DO_TRIGGER_1, _1_SECOND_SLEEP);

            //     // now that 1 is synd'c, turn 0 back on
            //     // FIXME: consider sync of 1 before moving on
            //     setPartnerSchedule(*radios[0], schedule_on);

            // }
            break;


        case DO_TRIGGER_1:
            cout << "DO_TRIGGER_1" << endl;
            cout << " sent REQUEST_FINE_SYNC_EV to radio 1" << endl;
            sendEvent(MAKE_EVENT(REQUEST_FINE_SYNC_EV,1));

            clearRecentEvent(); // FIxME: force clear event
            next = GO_NOW(WAIT_FOR_RADIO_TO_FINE_1);
            break;


        case WAIT_FOR_RADIO_TO_FINE_1:
            if(most_recent_event.d0 == RADIO_ENTERED_BG_EV &&
                most_recent_event.d1 == 1) {
                cout << "FSM MATCHED EVENT FROM RADIO 1" << endl;
                    clearRecentEvent();
                    next = GO_AFTER(WAIT_FOR_RADIO_1_EQ, TINY_SLEEP);
                }
            break;

        case WAIT_FOR_RADIO_1_EQ:
         if(
            radios[1]->times_residue_phase_sent > 100 &&
            radios[1]->times_eq_sent > 2 ) {
                cout << "WAIT_FOR_RADIO_1_EQ WAIT_FOR_RADIO_0_EQ" << endl;
                // normal, goes on to go cfo/sfo of r1
                next = GO_AFTER(AFTER_FINE_1, _50_MS_SLEEP);

                // next = GO_AFTER(PRE_COARSE_SCHEDULE_0, _8_SECOND_SLEEP);
            }
        break;


        case AFTER_FINE_1:
            cout << "AFTER_FINE_1" << endl;
            if(GET_DEBUG_EQ_BEFORE_SCHEDULE()) {
                cout << "DEBUG leaving normal path, will not return.  exp.debug_eq_before_schedule" << endl;
                next = GO_NOW(DEBUG_EQ_ROT_PRE);
            } else {
                // normal
                next = GO_NOW(PRE_FINE_SCHEDULE_0);
            }
            break;

        case PRE_FINE_SCHEDULE_0:
            cout << "PRE_FINE_SCHEDULE_0\n";
            next = GO_AFTER(FINE_SCHEDULE_0, _5_MS_SLEEP);
            // allow radio 0 on data tone
            // true means radio 0 does NOT transmit tone
            // false means radio 0 does transmit tone
            // already false
            radios[0]->should_mask_data_tone_tx_eq = false;
            radios[0]->updatePartnerEq(false);
            radios[0]->enable_residue_to_cfo = true; // turned off in the 2:1 case
            break;

        case FINE_SCHEDULE_0:
            cout << "FINE_SCHEDULE_0\n";

            soapy->cs32SFOCorrection(1);

            cout << "cs32 sfo correction enabled!!!!\n";

            //soapy->cs32SFOCorrectionShift(14);

            // // while 0 sync's, we put 1 into mode 10
            // // this is so that it will mask it's subcarriers
            // // maybe we need a special mode for this?
            // radios[1]->setPartnerTDMA(10,0);
            
            // sendEvent(MAKE_EVENT(REQUEST_TDMA_EV,0));

            // // tickleLog("rx", "my hack 1");
            // // radios[0]->initialMask();
            // // FINISHED_TDMA_EV

            // next = GO_AFTER(WAIT_FINE_SCHEDULE_0, _1_MS_SLEEP);

            next = GO_AFTER(DEBUG_STALL, _1_MS_SLEEP);
            break;


        case WAIT_FINE_SCHEDULE_0:

            if(most_recent_event.d0 == FINISHED_TDMA_EV &&
                most_recent_event.d1 == 0) {
                cout << "GOT TO WAIT_FINE_SCHEDULE_0" << endl;
                cout << "RADIO 0 fine schedule synch'd" << endl;

                    // controls, allows demodThread to run
                    // IMPORTANT this will run when 1 is done and 2 is still starting
                    // do we care?
                    soapy->demodThread->dropSlicedData = false;

                    // clear
                    clearRecentEvent();

                    bool debug_state = false;

                    if(GET_TEST_COARSE_MODE()) {
                        next = GO_AFTER(TEST_COARSE_SYNC_DROP_0, TINY_SLEEP);
                        debug_state = true;
                    }

                    if(GET_DEBUG_TX_EPOC()) {
                        cout << "going to GET_DEBUG_TX_EPOC" << endl;
                        next = GO_AFTER(DEBUG_EPOC_SEND_0, TINY_SLEEP);
                        debug_state = true;
                    }

                    // normal
                    if(!debug_state) {
                        next = GO_AFTER(PRE_FINE_SCHEDULE_1, TINY_SLEEP);
                    }

                }

            break;

        case PRE_FINE_SCHEDULE_1:
            cout << "PRE_FINE_SCHEDULE_1\n";

            // radios[0]->should_mask_data_tone_tx_eq = false;
            // radios[1]->should_mask_data_tone_tx_eq = true;

            // radios[0]->initialMask();
            // radios[1]->initialMask();

            // pass true as first one to 
            radios[0]->should_mask_data_tone_tx_eq = true;
            radios[0]->updatePartnerEq(true, false);
            radios[1]->should_mask_data_tone_tx_eq = false;
            radios[1]->updatePartnerEq(true, false);

            // while 1 sync's, we put 0 into mode 10
            // this is so that it will mask it's subcarriers
            // maybe we need a special mode for this?
            radios[0]->setPartnerTDMA(10,0);

            next = GO_AFTER(FINE_SCHEDULE_1, _5_MS_SLEEP);
            break;

        case FINE_SCHEDULE_1:
            // debug turn off radio 0
            // setPartnerSchedule(*radios[0], schedule_off);
            sendEvent(MAKE_EVENT(REQUEST_TDMA_EV, 1));
            // FINISHED_TDMA_EV

            next = GO_AFTER(WAIT_FINE_SCHEDULE_1, _1_MS_SLEEP);
            break;


        case WAIT_FINE_SCHEDULE_1:

            if( GET_FSM_SKIP_R1() ) {
                if( GET_FSM_ARBITER_DATA() ) {
                    next = GO_AFTER(VISUAL_CHECK_COUNTERS_0, _1_SECOND_SLEEP);
                } else {
                    cout << "when exp.fsm_skip_r1 is set, we will be stuck in WAIT_FINE_SCHEDULE_1. going to DEBUG_STALL" << endl;
                    next = GO_AFTER(DEBUG_STALL, TINY_SLEEP);
                }
            }

            if(most_recent_event.d0 == FINISHED_TDMA_EV &&
                most_recent_event.d1 == 1) {
                cout << "RADIO 1 fine schedule synch'd" << endl;
                    // clear
                    clearRecentEvent();
                    next = GO_AFTER(BEAMFORM_SETUP, TINY_SLEEP);
                }

            break;

        case BEAMFORM_SETUP:
            cout << "entering BEAMFORM_SETUP" << endl;

            // mode 6
            radios[0]->setPartnerTDMA(6, 0);
            radios[1]->setPartnerTDMA(6, 0);

            radios[0]->should_mask_data_tone_tx_eq = false;
            radios[0]->updatePartnerEq(true, false);
            radios[1]->should_mask_data_tone_tx_eq = false;
            radios[1]->updatePartnerEq(true, false);

            // radios[0]->print_all_coarse_sync = true;


            // setPartnerSchedule(radios[0], schedule_missing_10 );
            // setPartnerSchedule(radios[1], schedule_missing_20 );



            // canned data align schedule
            // setPartnerScheduleModeIndex(radios[0]->array_index, 2, 0);
            // setPartnerScheduleModeIndex(radios[1]->array_index, 2, 0);
            if( GET_FSM_ARBITER_DATA() ) {
                next = GO_AFTER(VISUAL_CHECK_COUNTERS_0, _1_SECOND_SLEEP);
            } else {
                cout << "BEAMFORM_SETUP is going to DEBUG_STALL\n";
                next = GO_NOW(DEBUG_STALL);
            }
            
            break;

             // if(most_recent_event.d0 == RADIO_ENTERED_BG_EV &&
             //    most_recent_event.d1 == 0) {
             //    cout << "FSM MATCHED EVENT FROM RADIO 0" << endl;
             //        // clear
             //        most_recent_event.d0 = NOOP_EV;
             //        GO_AFTER(TRANSITION_0_TO_1, TINY_SLEEP);
             //    }



        //////////////////////////////////////////////////////////////////////////////

        case VISUAL_CHECK_COUNTERS_0:
            cout << "VISUAL_CHECK_COUNTERS_0 please observe sc 63 in gnuradio\n";

            // have radio1 move it's TDMA over to radio 0
            if( !GET_FSM_SKIP_R1() ) {
                setPartnerTDMASubcarrier(radios[1]->peer_id, radios[0]->getScForTransmitTDMA());
                soapy->cs11TriggerMagAdjust(2); // we need this for visual inspection
            }
            next = GO_AFTER(VISUAL_CHECK_COUNTERS_1, _5_SECOND_SLEEP);
            break;


        case VISUAL_CHECK_COUNTERS_1:
            cout << "VISUAL_CHECK_COUNTERS_1 turning TDMA off\n";

            if( !GET_FSM_SKIP_R1() ) {
                // restore radio1 tdma to original sc
                radios[1]->setTDMASc();

                // turn both TDMA to mode 20, which is off
                radios[1]->setPartnerTDMA(20, 0);
            }

            // turn both TDMA to mode 20, which is off
            radios[0]->setPartnerTDMA(20, 0);

            next = GO_AFTER(ARBITER_DATA_0, _1_MS_SLEEP);
            break;

        case ARBITER_DATA_0:

            if( !GET_FSM_SKIP_R1() ) {
                cout << "ARBITER_DATA_0 triggering both radios to enter data mode and 1/x mode 2\n";
                soapy->cs11TriggerMagAdjust(2);

                soapy->setPhaseCorrectionMode(3);

                // radios[0]->should_mask_data_tone_tx_eq = false;
                // radios[1]->should_mask_data_tone_tx_eq = false;

            } else {
                cout << "ARBITER_DATA_0 triggering radio[0] to enter data mode\n";
            }


            sendCustomEventToPartner(
                    radios[0]->peer_id,
                    MAKE_EVENT(REQUEST_MAPMOV_DEBUG, 1)
                    );
            radios[0]->should_mask_data_tone_tx_eq = false;

            if( !GET_FSM_SKIP_R1() ) {
                sendCustomEventToPartner(
                        radios[1]->peer_id,
                        MAKE_EVENT(REQUEST_MAPMOV_DEBUG, 1)
                        );
                radios[1]->should_mask_data_tone_tx_eq = false;
            }


            cout << "ARBITER_DATA_0 is going to DEBUG_STALL\n";
            next = GO_NOW(DEBUG_STALL);
            break;




        //////////////////////////////////////////////////////////////////////////////

        case DEBUG_EQ_ROT_PRE:
            radios[0]->should_mask_data_tone_tx_eq = true;
            radios[0]->should_mask_all_data_tone = true;
            radios[0]->updatePartnerEq(true, false);
            radios[1]->should_mask_data_tone_tx_eq = true;
            radios[1]->should_mask_all_data_tone = true;
            radios[1]->updatePartnerEq(true, false);

            if( GET_TDMA_FORCE_FIRST_ROW() ) {
                cout << "FORCING MODE 7" << endl;
                // forces radio to stay in first dmode
                radios[0]->setPartnerTDMA(7, 0);
                radios[1]->setPartnerTDMA(7, 0);
            }

            next = GO_AFTER(DEBUG_EQ_ROT_0, _700_MS_SLEEP);
            tickle(&radios[0]->should_mask_all_data_tone);
            tickle(&radios[1]->should_mask_all_data_tone);
            break;


        case DEBUG_EQ_ROT_0:
            cout << "Showing tone of R0!!!!!!!!!!!!!!!!!!!!" << endl;
            radios[0]->should_mask_data_tone_tx_eq = false;
            radios[0]->should_mask_all_data_tone = false;
            radios[0]->updatePartnerEq(true, false);
            radios[1]->should_mask_data_tone_tx_eq = true;
            radios[1]->should_mask_all_data_tone = true;
            radios[1]->updatePartnerEq(true, false);

            next = GO_AFTER(DEBUG_EQ_ROT_1, _3_SECOND_SLEEP);
            tickle(&radios[0]->should_mask_all_data_tone);
            tickle(&radios[1]->should_mask_all_data_tone);
            break;

        case DEBUG_EQ_ROT_1:
            cout << "Showing tone of R1!!!!!!!!!!!!!!!!!!!!" << endl;
            radios[0]->should_mask_data_tone_tx_eq = true;
            radios[0]->should_mask_all_data_tone = true;
            radios[0]->updatePartnerEq(true, false);
            radios[1]->should_mask_data_tone_tx_eq = false;
            radios[1]->should_mask_all_data_tone = false;
            radios[1]->updatePartnerEq(true, false);

            next = GO_AFTER(DEBUG_EQ_ROT_PRE, _3_SECOND_SLEEP);
            tickle(&radios[0]->should_mask_all_data_tone);
            tickle(&radios[1]->should_mask_all_data_tone);
            break;


        //////////////////////////////////////////////////////////////////////////////


        case TEST_COARSE_SYNC_DROP_0:
            cout << "TEST_COARSE_SYNC_DROP_0 onetime setup" << endl;
            ///
            /// meant to be used with only r0, testing for rx coarse sync
            /// enter this state 1ce to setup this debug mode, then never come back
            ///
            radios[0]->setPartnerTDMA(6, 0);
            radios[0]->demod->erasePrevDeltas(); // erase history_tx_rx_deltas
            debug_coarse_sync_counter = 0;

            next = GO_AFTER(TEST_COARSE_SYNC_DROP_1, _3_SECOND_SLEEP);
            
            ///
            /// set this to 100k for human inspection, while also making sure that PRINT_TDMA_SC is 0 
            ///
            // radios[0]->first_dprint = 100000;
            // radios[0]->first_dprint = 0;

            debug_coarse_sync_did_send = 0;
            debug_coarse_sync_times_matched_6 = radios[0]->demod->times_matched_tdma_6;
            break;

        case TEST_COARSE_SYNC_DROP_1:

            if(debug_coarse_sync_did_send > 0)  {
                if( radios[0]->demod->history_tx_rx_deltas.size() == 0 ) {
                    cout << "Test FAILED stage 1" <<
                    "" << endl;

                    if( radios[0]->demod->first_dprint == 0 ){
                        // radios[0]->first_dprint = 100000;
                    }

                } else {
                    cout << "Test passed iteration " << debug_coarse_sync_did_send << " with number of detections: " << radios[0]->demod->history_tx_rx_deltas.size() << endl;
                    cout << "Note if this number is stuck at low number this is actually a fail" << endl;
                }

            } else {

                if( radios[0]->demod->history_tx_rx_deltas.size() == 0 ) {
                    cout << "Test entered TEST_COARSE_SYNC_DROP_1 while nothing was being detected" <<
                    " test cannot start if TDMA mode 6 is not working correctly" << endl;
                }
            }

            cout << "r" << radios[0]->array_index << " TEST_COARSE_SYNC_DROP_1" << endl;
            next = GO_AFTER(TEST_COARSE_SYNC_DROP_1, _3_SECOND_SLEEP);
            if(radios[0]->demod->history_tx_rx_deltas.size() > 3 ) {
                for(auto ppair : radios[0]->demod->history_tx_rx_deltas ) {
                        auto frame = ppair.first;
                        auto delta_estimate = ppair.second;
                        cout << "frame " << frame << " d " << delta_estimate << endl;
                }
                radios[0]->demod->erasePrevDeltas();
                debug_coarse_sync_counter++;
            }
            if( debug_coarse_sync_did_send == 0 && debug_coarse_sync_counter > 3) {
                cout << "at time of first send, we have seen "
                << debug_coarse_sync_times_matched_6 << ", " << radios[0]->demod->times_matched_tdma_6 << endl;
                cout << "if this number is not moving test can't start" << endl;
            }
            if( debug_coarse_sync_counter > 3 ) {
                // reset this number
                debug_coarse_sync_times_matched_6 = radios[0]->demod->times_matched_tdma_6;


                next = GO_AFTER(TEST_COARSE_SYNC_DROP_2, TINY_SLEEP);
            }
            break;

        case TEST_COARSE_SYNC_DROP_2:
            debug_coarse_sync_did_send++;
            soapy->cs31TriggerSymbolSync();
            debug_coarse_sync_counter = 0;
            // re-latch this timer
            debug_coarse_sync_times_matched_6 = radios[0]->demod->times_matched_tdma_6;
            
            if(GET_TEST_COARSE_MODE_AND_ADJUST()) {
                // after doing a coarse measurement, try to advance the tx side
                // and keep sync
                next = GO_AFTER(TEST_COARSE_SYNC_DROP_3, _3_SECOND_SLEEP);

                // don't drain because we need the values
            } else {
                // to back and try to continuously measure without adjusting
                // DROP_4 gives us time to reset before checking counters again
                next = GO_AFTER(TEST_COARSE_SYNC_DROP_4, TINY_SLEEP);
                for(auto i = 0; i < 128; i++) { checkGetCoarseRb(); } // drain this
            }
            break;

            case TEST_COARSE_SYNC_DROP_4:
                radios[0]->demod->erasePrevDeltas();
                next = GO_AFTER(TEST_COARSE_SYNC_DROP_1, _2_SECOND_SLEEP);
                break;


        case TEST_COARSE_SYNC_DROP_3:
            optional_rb_word = checkGetCoarseRb();
            if( optional_rb_word.first ) {
                cout << "DSP RB: " << HEX32_STRING(optional_rb_word.second) << endl;

                coarse_est = (1280-optional_rb_word.second) + GET_COARSE_SYNC_TWEAK();

                cout << "dbg coarse_est " << coarse_est << endl;

                // mode 4 will protect large values
                setPartnerSfoAdvance(*radios[0], coarse_est, 4);

                // tell radio to start sync
                // argument is radio index, not peer_id
                next = GO_AFTER(TEST_COARSE_SYNC_DROP_1, _8_SECOND_SLEEP);

                // _prev_sfo_est = radios[0]->times_sfo_estimated;
                // _rx_should_insert_sfo_cfo = true;
            }
            break;


//////////////////////////////////////////////////////////////////////////////

        case DEBUG_EPOC_SEND_0: {
                cout << "DEBUG_EPOC_SEND_0----------" << endl;

                cout << "got to DEBUG_EPOC_SEND_0 setting should_print_demod_word" << endl;

                radios[0]->demod->should_print_demod_word = true;

                // auto our_peer = GET_PEER_RX();
                // this will arrive at the tx inside HiggsZmq.cpp: zmqHandleMessage()
                sendCustomEventToPartner(
                    radios[0]->peer_id,
                    MAKE_EVENT(REQUEST_MAPMOV_DEBUG, 1)
                    );

                // radios[0]->epoc_valid = false;
                // raw_ringbus_t rb0 = {RING_ADDR_CS20, REQUEST_EPOC_CMD};
                // zmqRingbusPeerLater(radios[0]->peer_id, &rb0, 0);


                // cout << "??????????????????????????????????????? will tx be ok???????????" << endl;
                // cout << "??????????????????????????????????????? will tx be ok???????????" << endl;
                // cout << "??????????????????????????????????????? will tx be ok???????????" << endl;
                // auto tv = (struct timeval){ .tv_sec = 60, .tv_usec = 0 };
                // next = GO_AFTER(REQUEST_TX_EXIT_DATA_MODE, tv);
                cout << "going to DEBUG_STALL\n";
                next = GO_AFTER(DEBUG_STALL, TINY_SLEEP);
            }
            break;

        case DEBUG_EPOC_SEND_1:

            if( radios[0]->epoc_valid ) {
                next = GO_AFTER(DEBUG_EPOC_SEND_2, TINY_SLEEP);
            } else {
                next = GO_AFTER(DEBUG_EPOC_SEND_1, TINY_SLEEP);
            }
            break;

        case DEBUG_EPOC_SEND_2: {
                cout << "R0 got epoc " << endl;
                cout << radios[0]->epoc_estimated.epoc << endl;
                cout << radios[0]->epoc_estimated.frame << endl;

                // radios[0]->setRandomRotationEq();
                // radios[1]->setRandomRotationEq();


                // auto tb = std::chrono::steady_clock::to_time_t(radios[0]->epoc_timestamp);
                // std::chrono::steady_clock::to_time_t

                time_t v = steady_clock_to_time_t(radios[0]->epoc_timestamp);

                cout <<
                 v
                 
                 << endl;
                cout << endl << endl;

                // radios[0]->should_mask_all_data_tone = true;
                // radios[0]->updatePartnerEq(false, false);

                next = GO_AFTER(DEBUG_EPOC_SEND_3, _50_MS_SLEEP);
            }
            break;

        case DEBUG_EPOC_SEND_3: {
                // send
                // std::vector<uint32_t> data;
                // subcarrier_data_sync(&data, 0xcafebabe, 16);
                // subcarrier_data_sync(&data, 0xdeadbeef, 16);
                // subcarrier_data_sync(&data, 0xdeadbeef, 16);
                // subcarrier_data_sync(&data, 0xdeadbeef, 16);

                // cout << "About to send " << data.size() << " words " << endl;
                // cout << "or " << data.size()*32 << " bits" << endl;
                // cout << "or " << data.size()*16 << " subcarriers worth " << endl;


                uint32_t epoc = radios[0]->epoc_estimated.epoc + 4;     // seq2
                uint32_t timeslot = 0x15; // seq

                // const int header = 16;
                // const double enabled_subcarriers = 128;

                // // assumes that 0 and 1023 are the boundaries
                // auto custom_size = header + ceil((data.size()*16)/enabled_subcarriers)*1024;
                // auto packet = 
                //     feedback_vector_mapmov_scheduled(
                //         FEEDBACK_VEC_TX_USER_DATA, 
                //         data, 
                //         custom_size, 
                //         // feedback_peer_to_mask(radios[0]->peer_id), 
                //         FEEDBACK_PEER_8,
                //         FEEDBACK_DST_HIGGS,
                //         timeslot,
                //         epoc
                //         );

                std::vector<uint32_t> dumb;
                dumb.resize(1);
                auto packet = feedback_vector_packet(
                            FEEDBACK_VEC_INVOKE_USER_DATA,
                            dumb,
                            feedback_peer_to_mask(radios[0]->peer_id),
                            FEEDBACK_DST_HIGGS);

                packet[5] = timeslot;
                packet[6] = epoc;

                soapy->zmqPublish(ZMQ_TAG_ALL, packet);


                next = GO_AFTER(DEBUG_EPOC_SEND_4, TINY_SLEEP);
            }
            break;

        case DEBUG_EPOC_SEND_4:
            next = GO_AFTER(DEBUG_EPOC_SEND_3, _8_SECOND_SLEEP);
            break;


//////////////////////////////////////////////////////////////////////////////


        case DEBUG_EARLY_EVENT_0: {
                // cout << "ENTERED DEBUG_EARLY_EVENT_0" << endl;
                // optional: set epoc or change it or something

                // we would use sendEvent() to send to either of our radio estimates
                // to talk to our peer hoever, use this
                // see CustomEventTypes.hpp
                auto our_peer = GET_PEER_RX();
                // this will arrive at the tx inside HiggsZmq.cpp: zmqHandleMessage()
                sendCustomEventToPartner(
                    radios[0]->peer_id,
                    MAKE_EVENT(FSM_PING_EV, our_peer)
                    );

                next = GO_AFTER(DEBUG_EARLY_EVENT_1, TINY_SLEEP);
                outstanding_fsm_ping = true;
                fsm_ping_sent = std::chrono::steady_clock::now();
            }

            break;
        case DEBUG_EARLY_EVENT_1:
            if(most_recent_event.d0 == FSM_PONG_EV) {
                if( outstanding_fsm_ping ) {
                    auto fsm_ping_got = std::chrono::steady_clock::now();
                    std::chrono::duration<double> update_elapsed_time = fsm_ping_got-fsm_ping_sent;
                    // std::chrono::steady_clock::time_point now = 


                    // std::chrono::steady_clock::to_time_t
                    auto b = 
                        chrono::duration_cast<chrono::microseconds>( 
                            fsm_ping_got - fsm_ping_sent
                        ).count();
                    auto c = 
                        chrono::duration_cast<chrono::milliseconds>( 
                            fsm_ping_got - fsm_ping_sent
                        ).count();


                    cout << "TX side got FSM_PING_EV round trip time was:" << endl;
                    cout << "    " << c << "ms" << "   " << b << "us" << endl;

                    auto our_peer = soapy->this_node.id;
                    // send a pong back
                    auto reply_to = most_recent_event.d1;

                    sendCustomEventToPartner(
                        reply_to,
                        MAKE_EVENT(FSM_PONG_EV, our_peer)
                        );



                    clearRecentEvent(); // clear
                    next = GO_NOW(DEBUG_EARLY_EVENT_0);
                    outstanding_fsm_ping = false;
                }
            } else {
                next = GO_NOW(DEBUG_EARLY_EVENT_1);
            }


            break;
        case DEBUG_EARLY_EVENT_2:
            break;

        case DEBUG_EARLY_EVENT_3:
            break;


//////////////////////////////////////////////////////////////////////////////


        case DEBUG_TX_LOCAL_EPOC_0: {

                // auto our_peer = GET_PEER_RX();
                // this will arrive at the tx inside HiggsZmq.cpp: zmqHandleMessage()
                sendCustomEventToPartner(
                    radios[0]->peer_id,
                    MAKE_EVENT(REQUEST_MAPMOV_DEBUG, 1)
                    );

                next = GO_AFTER(DEBUG_TX_LOCAL_EPOC_1, TINY_SLEEP);
                // outstanding_fsm_ping = true;
                // fsm_ping_sent = std::chrono::steady_clock::now();
            }
            break;

        case DEBUG_TX_LOCAL_EPOC_1:
            break;

        // confusing name but this state is a state the RX goes into
        // which will send a message to tx
        case REQUEST_TX_EXIT_DATA_MODE:
                sendCustomEventToPartner(
                    radios[0]->peer_id,
                    MAKE_EVENT(EXIT_DATA_MODE_EV, 1)
                    );

                next = GO_NOW(DEBUG_STALL);
            break;


        case DEBUG_STALL:
            break;
        case TX_STALL:
            break;

        break;
        default:
            cout << "Bad state (" << dsp_state << ") in eventdsp::tickfsm" << endl;
            break;
    }


    if( __dsp_preserve != dsp_state ) {
        cout << endl << "ERROR dsp_state changed during operation.  Expected: " 
        << __dsp_preserve
        << " Got: " << dsp_state
        << endl;
        usleep(100);
        assert(0);
    }

    if(__timer != __UNSET_NEXT__) {
        // cout << "timer mode" << endl;
        dsp_state_pending = __timer;
#ifdef USE_DYNAMIC_TIMERS
        tv = time_multiply(tv, GET_RUNTIME_SLOWDOWN()); // modify in place should be ok, I don't think we re-use tv.. do we?
#endif
        evtimer_add(fsm_next_timer, &tv);
    } else if(__imed != __UNSET_NEXT__) {
        // cout << "imed mode" << endl;
        dsp_state_pending = __imed;
        auto timm = SMALLEST_SLEEP;
        evtimer_add(fsm_next_timer, &timm);
    } else if(next != 0 && next != __UNSET_NEXT__) {
        cout << endl << "ERROR You cannot use:  ";
        cout << endl << "   next = " << endl << endl << "without GO_NOW() or GO_AFTER()      (inside EventDsp::tickFsm)" << endl << endl << endl;
        usleep(100);
        assert(0);
    } else {
        // cout << "default mode" << endl;
        // default
        dsp_state_pending = dsp_state;
        evtimer_add(fsm_next_timer, &tv);
        // cout << "using default next in tiskFsm" << endl;
    }
}
#pragma GCC diagnostic pop

// should be threadsafe
void EventDsp::stopThreadDerivedClass() {
    shutdownThread();
}

void EventDsp::shutdownThread() {
    if(this->role != "arb") {
        shutdownThreadRx();
        shutdownThreadTx();
    }
}
void EventDsp::shutdownThreadRx() {
    cout << "EventDsp::shutdownThreadRx " << endl;
}






void EventDsp::printSchedule(const std::string& name, const std::vector<uint32_t>& schedule) const {
    cout << name << ":" << endl;
    for(auto &n : schedule) {
        cout << n << ", ";
    }
    cout << endl;
}


// #define SCH_0_IF (i % 10) == 0 || (i % 10) > 2
// #define SCH_1_IF (i % 10) == 1 || (i % 10) > 2


#define SCH_0_IF i < 15 || i >= 20
#define SCH_1_IF i >= 15 || i >= 20


/// Build some copies of a 50 element schedule
/// This schedule is sent down to each side
/// Originally I was going to have masks be unique
/// to boards
/// but the S-Modem driver can handle the masks
/// allowing for idential .hex files and no unique ID's inside vex schdule object
void EventDsp::generateSchedule() {
    size_t i;

    schedule_on.resize(SCHEDULE_SLOTS);
    schedule_off.resize(SCHEDULE_SLOTS);
    schedule_0.resize(SCHEDULE_SLOTS);
    schedule_1.resize(SCHEDULE_SLOTS);
    schedule_toggle.resize(SCHEDULE_SLOTS);
    schedule_missing_5.resize(SCHEDULE_SLOTS);
    schedule_missing_6.resize(SCHEDULE_SLOTS);
    schedule_missing_10.resize(SCHEDULE_SLOTS);
    schedule_missing_20.resize(SCHEDULE_SLOTS);


    for(auto &n : schedule_on) {
        n = 0xffffffff;
    }

    for(auto &n : schedule_off) {
        n = 0x0;
    }

    i = 0;
    for(auto &n : schedule_0) {
        if( 
            SCH_0_IF
                           ) {
            n = 0xffffffff;
        } else {
            n = 0;
        }
        i++;
    }

    i = 0;
    for(auto &n : schedule_1) {
        if(
            SCH_1_IF
            ) {
            n = 0xffffffff;
        } else {
            n = 0;
        }
        i++;
    }

    i = 0;
    for(auto &n : schedule_toggle) {
        if( i % 2 == 0 ) {
            n = 0xffffffff;
        } else {
            n = 0;
        }
        i++;
    }

    schedule_bringup.resize(SCHEDULE_SLOTS);
    schedule_bringup_inverse.resize(SCHEDULE_SLOTS);

    i = 0;
    for(auto &n : schedule_bringup) {
        if( i < 2 ) {
            n = 0xffffffff;
        } else {
            n = 0;
        }
        i++;
    }

    i = 0;
    for(auto &n : schedule_bringup_inverse) {
        if( i < 2 ) {
            n = 0;
        } else {
            n = 0xffffffff;
        }
        i++;
    }

    i = 0;
    for(auto &n : schedule_missing_5) {
        if( i == 5 ) {
            n = 0;
        } else {
            n = 0xffffffff;
        }
        i++;
    }

    i = 0;
    for(auto &n : schedule_missing_6) {
        if( i == 6 ) {
            n = 0;
        } else {
            n = 0xffffffff;
        }
        i++;
    }

    i = 0;
    for(auto &n : schedule_missing_10) {
        if( i == 8 || i == 9 || i == 10 ) {
            n = 0;
        } else {
            n = 0xffffffff;
        }
        i++;
    }

    i = 0;
    for(auto &n : schedule_missing_20) {
        if( i == 18 || i == 19 || i == 20 ) {
            n = 0;
        } else {
            n = 0xffffffff;
        }
        i++;
    }



    // printSchedule("s0", schedule_0);
    // printSchedule("s1", schedule_1);
}

uint32_t EventDsp::timeslotForFrame(const uint32_t frame) const {
    const uint32_t sample = frame % SCHEDULE_FRAMES;

    const uint32_t i = sample / SCHEDULE_LENGTH;
    return i;
}

///
/// Returns a mask. Fixme not compatible with feedback bus type mask
/// Pass a frame in the rx clock domain, assumes that frame is already
/// modulo corrected for frame
/// 
int32_t EventDsp::getPeerMaskForFrame(const uint32_t frame) const {

    const uint32_t i = timeslotForFrame(frame);

    int32_t peer_index = 0;


    if( SCH_0_IF ) {
        peer_index |= 0x1<<0;
    }

    if( SCH_1_IF ) {
        peer_index |= 0x1<<1;
    }

    // cout << "for " << frame << " " << HEX32_STRING(peer_index) << endl;

    return peer_index;
}

void EventDsp::tickAllRadios() {
    unsigned i = 0;
    for( auto r : radios) {
        if( i == 0 && should_inject_r0 ) {
            r->tickDataAndBackground();
        }
        if( i == 1 && should_inject_r1 ) {
            r->tickDataAndBackground();
        }
        i++;
    }

    if( should_inject_attached ) {
        attached->tickDataAndBackground();
    }

}

void EventDsp::ringbusPeerLater(const size_t peer, const raw_ringbus_t& rb, const __suseconds_t future_us) {
    zmqRingbusPeerLater(peer, &rb, future_us);
}

void EventDsp::ringbusPeerLater(const size_t peer, const std::vector<uint32_t> rbv, const __suseconds_t future_us) {
    if(rbv.size() != 2) {
        cout << "ringbusPeerLater did NOT send because rb vector was of wrong size " << rbv.size() << '\n';
        return;
    }

    raw_ringbus_t rb0 = {rbv[0], rbv[1]};

    zmqRingbusPeerLater(peer, &rb0, future_us);
}

void EventDsp::ringbusPeerLater(const size_t peer, const uint32_t ttl, const uint32_t data, const __suseconds_t future_us) {
    raw_ringbus_t rb0 = {ttl, data};
    zmqRingbusPeerLater(peer, &rb0, future_us);
}



// alias, needed to make sense because this function now supports delivering locally as well
void EventDsp::ringbusPeerLater(const size_t peer, const raw_ringbus_t* rb, const __suseconds_t future_us) {
    zmqRingbusPeerLater(peer, rb, future_us);
}

void EventDsp::zmqRingbusPeerLater(const size_t peer, const raw_ringbus_t* rb, const __suseconds_t future_us) {

    ringbus_later_t payload;
    payload.peer = peer;
    payload.dsp = this;
    
    payload.rb.ttl  = rb->ttl;
    payload.rb.data = rb->data;



    // cout << "  peer: " << pending_ring_data[0].peer << endl;
    // cout << "  ttl: " << pending_ring_data[0].rb.ttl << endl;
    // cout << " data: " << pending_ring_data[0].rb.data << endl;

    // grab pointer because I think std vector copies it
    // void* payload_ptr = &pending_ring_data[pending_ring_data.size()-1];
    // void* payload_ptr = pending_ring_data.data() + (pending_ring_data.size()-1);

    struct event* tmp;
    tmp = evtimer_new(evbase, handle_rb_later, this);
    
    {
        std::unique_lock<std::mutex> lock(_rb_later_mutex);
        pending_ring_data.push_back(payload);
        pending_ring_events.push_back(tmp);
    }


    struct timeval tv = { .tv_sec = 0, .tv_usec = future_us };
    evtimer_add(tmp, &tv);



}

// returned a rb word with command bits masked off (all zeros)
// returns an optional value, if .first is set, then .second is valid
std::pair<bool, uint32_t> EventDsp::checkGetCoarseRb() {
    if(rb_buf_coarse_sync.size() > 0) {
        auto copy = rb_buf_coarse_sync[0];

        // erase the first element
        rb_buf_coarse_sync.erase(rb_buf_coarse_sync.begin() + 0);

        return std::pair<bool, uint32_t>(true, copy);
    }
    return std::pair<bool, uint32_t>(false, 0);
}

// attached higgs just sent us a rb message, and we are in s-modem rx
bool EventDsp::dspDispatchAttachedRb_RX(const uint32_t word) {
    bool handled = true;
    const uint32_t data = word & 0x00ffffff;
    const uint32_t cmd_type = word & 0xff000000;
    switch(cmd_type) {
        case COARSE_SYNC_PCCMD:
            rb_buf_coarse_sync.push_back(data);
            cout << "\n\ndispatchAttachedRb() coarse sync matched "
            << HEX32_STRING(cmd_type) << " " << HEX32_STRING(word) << "\n\n";
            break;
        case NODEJS_RCP_PCCMD:
            gotFromMock(data);
            break;

        // This is here to catch expected or debug ringbus
        // For these ringbus we do NOT print, instead we just have the original Ringbus: 
        case COARSE_DEBUG_PCCMD:
        case DEBUG_32_PCCMD:
        case GENERIC_PCCMD:
        case DEBUG_0_PCCMD:
        case COARSE_DEBUG_PROG_PCCMD:
        case FILL_REPLY_CMD:
        case UART_READOUT_PCCMD:
        case POWER_RESULT_PCCMD:
        case EXFIL_READOUT_PCCMD:
        case RX_COUNTER_SYNC_PCCMD:
            break;
        case RX_ERROR_PCCMD:
            cout << "Rx side cs00 code reported RX_ERROR_PCCMD\n";
            break;

        case RX_FINE_SYNC_OVERFLOW_PCCMD:
            if( role == "tx" ) {
                handleFineSyncMessage(data);
            }
            break;


        // Default puts up a warning
        default:
            handled = false;
            // cout << "dispatchAttachedRb_RX() Got Ringbus from Attached-Higgs "
            // << HEX32_STRING(word) << " with unknown type " << HEX32_STRING(cmd_type) << "\n";
            break;
    }
    return handled;
}

void EventDsp::handleFineSyncMessage(const uint32_t data) {
    switch(data) {
        case 0:
            cout << "Fine sync in CS32 overflowed\n";
            break;
        case 1:
            cout << "Fine sync in CS32 underflow for r0\n";
            break;
        case 2:
            cout << "Fine sync in CS32 underflow for r1\n";
            break;
        default:
            cout << "Fine sync in CS32 overflowed but unknown message??\n";
            break;
    }
}

/// attached higgs just sent us a rb message, and we are in s-modem tx side
/// returns true if we handled
bool EventDsp::dspDispatchAttachedRb_TX(const uint32_t word) {
    const uint32_t data = word & 0x00ffffff;
    const uint32_t cmd_type = word & 0xff000000;
    (void)data;

    bool do_forward = false;

    bool attached_consume = false;

    ///
    /// based on the type we either forward it or eat it
    ///  If we forard it, rx side will get it in HiggsZmq.cpp : zmqHandleMessage()
    ///  which then dumps it into fb_pc_bev() into handlePcPcVectorType()
    ///  which dumps into dispatchRemoteRb()
    switch(cmd_type) {
        case FEEDBACK_ALIVE:   // sent after a successful FEEDBACK_VEC_STATUS_REPLY
            cout << "Got attached alive!!!\n";
            do_forward = true;
            break;
        case GENERIC_READBACK_PCCMD:
            do_forward = true;
            break;
        case PERF_ETH_PCCMD:
        case PERF_02_PCCMD :
        case PERF_12_PCCMD :
        case PERF_00_PCCMD :
        case PERF_01_PCCMD :
        case PERF_11_PCCMD :
        case PERF_21_PCCMD :
        case PERF_31_PCCMD :
        case PERF_30_PCCMD :
        case TDMA_REPLY_PCCMD:
        case FEEDBACK_HASH_PCCMD:
        // case EPOC_REPLY_PCCMD: // sent to rx for now, may need to change routing, epoc reply
            do_forward = true;
            break;
        default:
            break;
    }

    switch(cmd_type) {
        case TDMA_REPLY_PCCMD:
        // case EPOC_REPLY_PCCMD:
        // case TX_FILL_LEVEL_PCCMD:
        case TX_UD_LATENCY_PCCMD:
        case FILL_REPLY_PCCMD:
        case TX_USERDATA_ERROR:
        case FEEDBACK_ALIVE:
        case TEST_STACK_RESULTS_PCCMD:
        case GENERIC_READBACK_PCCMD:
        case LAST_USERDATA_PCCMD:
        case TX_PARSE_GOT_VECTOR_PCCMD:
        case TX_FB_REPORT_PCCMD:
        case TX_FB_REPORT2_PCCMD:
        case CHECK_BOOTLOAD_PCCMD:
        case TX_REPORT_STATUS_PCCMD:
        case FINE_SYNC_RAW_PCCMD:
            attached_consume = true;
            break;
        case NODEJS_RCP_PCCMD:
            gotFromMock(data);
            break;
        default:
            break;
    }

    // FIXME: remove this once we figure it all out

    // this code assumes we always forward to rx side
    if(do_forward) {
        // this is the rx peer
        auto forward_to = GET_PEER_RX();

        vector<uint32_t> rings;
        // note: we forward the whole word, we do not mask it like other fn()
        rings.push_back(soapy->this_node.id); // who it came from
        rings.push_back(word);

        ///
        /// we are the tx side;  we received a ringbus from our attached-higgs
        /// here we forward that message over zmq/feedback bus to the rx computer
        transportRingToPartner(forward_to, rings);

        // vector<uint32_t> rings2;
        // rings2.push_back(soapy->this_node.id);
        // rings2.push_back(0);
        // rings2.push_back(1);
        // rings2.push_back(2);
        // rings2.push_back(3);
        // rings2.push_back(4);

        // transportRingToPartner(forward_to, rings2);

        // FIXME: we could have an event that fires in 5ms
        // and it would accumulate N ringbus at a time
        // this would save us on the 16 word header
    } // do_forward


// FIXME FIXMEEEE, I still don't understand how this works?

    if(attached_consume && attached) {
        // cout << "attached_consume && attached" << endl;


        // NOTE, this function is NOT in this file, see RadioEstimate
        attached->dispatchAttachedRb(word);
    }
    

    return attached_consume || do_forward;


} // dispatchAttachedRb_TX

///
/// called from inside an event to track
/// these should be very short or spawn events in-turn
/// this is just an arbiter for either a tx or rx side callback
/// called every time attached higgs sends us a rb
void EventDsp::dspDispatchAttachedRb(const uint32_t word) {
    // if( this->role == "rx") {

        // if we are rx also publish now
        
        // std::stringstream ss;
        // ss << "0x";
        // ss << HEX32_STRING(word);
        // tickleLog("rb", ss.str() );
    const bool rx_handled = dspDispatchAttachedRb_RX(word);
    const bool tx_handled = dspDispatchAttachedRb_TX(word);

    // cout << "rx_handled: " << rx_handled << " tx_handled " << tx_handled << " " << HEX32_STRING(word) << "\n";

    if( !rx_handled && !tx_handled ) {
        // const uint32_t data = word & 0x00ffffff;
        const uint32_t cmd_type = word & 0xff000000;
        cout << "dspDispatchAttachedRb() Got Ringbus from Attached-Higgs "
        << HEX32_STRING(word) << " with unknown type " << HEX32_STRING(cmd_type) << "\n";
    }

    // }
}

void EventDsp::gotFromMock(const size_t word) {
    // cout << "got word " << HEX32_STRING(word) << endl;
    char c[3];
    c[0] = (word>>16)&0xff;
    c[1] = (word>>8 )&0xff;
    c[2] = (word>>0 )&0xff;
    splitRpc->addData(c, 3);

    // std::string load("ABCD");
    // splitRpc->addData(load.c_str(), load.size());

    // unsigned char nullc = (unsigned char)0;

    // splitRpc->addData(&nullc, 1);
    // splitRpc->addData(&nullc, 1);
}

void EventDsp::gotCompleteMessageFromMock(const std::string& s) {
    cout << s << endl;
    dispatchRpc->dispatch(s);
}


// math for initial setup with 64 sc
// FIXME why is this here, how can we make this in json?
uint32_t EventDsp::subcarrierForBufferIndex(const size_t i) const {
    uint32_t expected_subcarrier = 945 + i*2;
    return expected_subcarrier;
}



