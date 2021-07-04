#include "EventDsp.hpp"

#include "HiggsDriverSharedInclude.hpp"

#include "vector_helpers.hpp"

#include "schedule.h"

using namespace std;

#include "driver/FsmMacros.hpp"

#include "EventDspFsmStates.hpp"
#include "CustomEventTypes.hpp"

#include <signal.h> 
#include <sstream>
#include "common/convert.hpp"
#include "driver/RadioEstimate.hpp"
#include "driver/AttachedEstimate.hpp"
#include "driver/VerifyHash.hpp"
#include "driver/RetransmitBuffer.hpp"
#include "driver/FlushHiggs.hpp"
#include "driver/TxSchedule.hpp"
#include "duplex_schedule.h"



void EventDsp::handleCustomEventTx(const custom_event_t* const e) {
    cout << "EventDsp::handleCustomEventTx()" << endl;

    switch(e->d0) {
        case FINISHED_TDMA_EV:
        case RADIO_ENTERED_BG_EV:
        case REQUEST_MAPMOV_DEBUG:
        case FSM_PING_EV:
        case FSM_PONG_EV:
        case EXIT_DATA_MODE_EV:
        case DUPLEX_FINE_SYNC:
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

void EventDsp::createStatusTx() {
    cout << "EventDsp::createStatusTx()" << endl;
    // path, timeout in second, automatically print when stale
    status->create("tx.mapmov_reply", 0.0, true);
    status->set   ("tx.mapmov_reply", STATUS_OK);
}

void EventDsp::tickFsmTx() {

    tx_dsp_state = tx_dsp_state_pending;
    // tickle(&dsp_state);
    auto __dsp_preserve = tx_dsp_state;
    struct timeval tv = DEFAULT_NEXT_STATE_SLEEP;
    size_t __timer = __UNSET_NEXT__;
    size_t __imed = __UNSET_NEXT__;
    size_t next = __UNSET_NEXT__;

    switch(tx_dsp_state) {
        case PRE_BOOT:
            cout << "tickFsmTx PRE_BOOT\n";
            if( false ) {
                // cout << endl << endl << "EventDsp::tickFSM detected non TX mode, shutting down" << endl << endl;
                // next = GO_NOW(TX_STALL);
            } else {
                // our role is tx
                cout << endl << endl;
                cout << "node.csv encodes our id as                " << soapy->this_node.id << endl;

                // 2 for tx
                // ringbusPeerLater(attached->peer_id, (raw_ringbus_t){RING_ADDR_CS20, SMODEM_BOOT_CMD|0x2}, 0);

                next = GO_AFTER(WAIT_FOR_SELF_SYNC, _1_MS_SLEEP);
            }
        break;

        case WAIT_FOR_SELF_SYNC: {
            if( GET_RUNTIME_SELF_SYNC_DONE() ) {
                next = GO_AFTER(AFTER_PRE_BOOT, _1_MS_SLEEP);
            } else {
                // static int self_sync_wait_tx = 0;
                // cout << "TX Waiting for self sync " << self_sync_wait_tx++ << "\n";
                next = GO_AFTER(WAIT_FOR_SELF_SYNC, _300_MS_SLEEP);
            }
            break;
        }

        case AFTER_PRE_BOOT: {
                ///
                /// Inits
                ///
                /// We can put general inits here assuming they are first need after this point

                // prevent crash due to flushHiggs not being up
                // only found in "tx/rx" mode
                while(!soapy->flushHiggs) {
                    usleep(100);
                }

                // next = GO_AFTER(RESET_ATTACHED_FB_BUS_0, _1_MS_SLEEP);
                next = GO_AFTER(JS_HANDLE_FB_BUS_0, _1_MS_SLEEP);
                soapy->flushHiggs->allow_zmq_higgs = false;
                // next = GO_AFTER(CHECK_ATTACHED_FB_BUS_0, _1_MS_SLEEP);
            }
            break;

        case SET_RANDOM_ROTATION: {

            if(this->role == "tx") {

                const int32_t tnum = soapy->getTxNumber();

                switch(tnum) {
                    case -1:
                        cout << "Error: We are rx\n";
                        break;
                    case -2:
                        cout << "Error init.json is not consistent regarding tx numbers\n";
                        break;
                }

                uint32_t role_enum = soapy->duplexEnumForRole(this->role, tnum);



                op(RING_ADDR_TX_PARSE    , "set", 10, role_enum); usleep(1000);
                op(RING_ADDR_TX_FFT      , "set", 10, role_enum); usleep(1000);

                op(RING_ADDR_RX_FFT      , "set", 10, role_enum); usleep(1000);
                op(RING_ADDR_RX_FINE_SYNC, "set", 10, role_enum); usleep(1000);
                op(RING_ADDR_RX_EQ,        "set", 10, role_enum); usleep(1000);
                op(RING_ADDR_RX_MOVER,     "set", 10, role_enum); usleep(1000);

                soapy->cs31CFOTurnoff(0); usleep(1000);
                soapy->cs32SFOCorrection(0); usleep(1000);

                
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS31, COOKED_DATA_TYPE_CMD, 2000);
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS32, COOKED_DATA_TYPE_CMD, 3000);
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS22, COOKED_DATA_TYPE_CMD, 4000);
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS21, COOKED_DATA_TYPE_CMD, 5000);
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS11, COOKED_DATA_TYPE_CMD, 6000);
                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS12, COOKED_DATA_TYPE_CMD, 7000);

                ringbusPeerLater(soapy->this_node.id, RING_ADDR_CS32, EQ_DATA_RX_CMD | 1  , 8000);

                // usleep(7000);

                attached->should_mask_data_tone_tx_eq = false;
                attached->initialMask();
                usleep(50000);
                attached->setRandomRotationEq();

            }


            if(GET_SFO_RESET_ZERO()) {
                cout << "Attached Reset SFO\n";
                attached->sfo_estimated = 0;
                attached->updatePartnerSfo();
            }
            if(GET_CFO_RESET_ZERO()) {
                cout << "Attached Reset CFO\n";
                attached->cfo_estimated = 0;
                attached->updatePartnerCfo();
            }


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
            attached->sfo_estimated = GET_SFO_INIT_ATTACHED();
            attached->updatePartnerSfo();
            
            cout << "Attached Update SFO based on prior information\n";

            if( GET_CFO_APPLY_INITIAL() ) {
                next = GO_AFTER(SET_CFO_INIT_0, _50_MS_SLEEP);
            } else {
                next = GO_AFTER(DID_BOOT, _50_MS_SLEEP);
            }

            break;

        case SET_CFO_INIT_0:
            attached->cfo_estimated = GET_CFO_INIT_ATTACHED();
            attached->updatePartnerCfo();
            
            cout<<"Attached Update CFO based on prior information!!!!!!!\n";

            next = GO_AFTER(DID_BOOT, _50_MS_SLEEP);

            break;

        case DID_BOOT:
            next = GO_NOW(DECIDE_HOW_TO_BEGIN);
            break;

        case DECIDE_HOW_TO_BEGIN: {
                if( GET_DEBUG_TX_FILL_LEVEL() ) {
                    cout << "GET_DEBUG_TX_FILL_LEVEL() triggering TxSchedule FSM" << endl;
                    soapy->txSchedule->doWarmup = true;
                    // next = GO_AFTER(DEBUG_TX_MAPMOV_0, _1_SECOND_SLEEP);
                    next = GO_AFTER(WAIT_EVENTS, TINY_SLEEP); // take THIS fsm to wait events
                } else if( GET_DEBUG_RESET_FB_ONLY() ) {
                    next = GO_AFTER(RESET_ATTACHED_FB_BUS_0, _1_SECOND_SLEEP);
                } else {
                    // normal
                    cout << "EventDspFsm going to WAIT_EVENTS" << endl;

                    // new to allow this here
                    soapy->flushHiggs->allow_zmq_higgs = true;
                    next = GO_AFTER(WAIT_EVENTS, TINY_SLEEP);
                }
            }


            break;


        case CHECK_ATTACHED_FB_BUS_0:
            cout << "entering CHECK_ATTACHED_FB_BUS_0" << endl;
            times_tried_attached_feedback_bus = 0;
            next = GO_NOW(CHECK_ATTACHED_FB_BUS_1);
            break;

        case CHECK_ATTACHED_FB_BUS_1: {

            std::vector<uint32_t> dumb;
            dumb.resize(16);
            auto packet = feedback_vector_packet(
                FEEDBACK_VEC_STATUS_REPLY,
                dumb,
                0,
                0xdeadbeef);

            for(int i = 0; i < 32; i++) {
                packet.insert(packet.begin(),0);
            }

            // for(auto w : packet) { 
            //     cout << HEX32_STRING(w) << endl;
            // }

            // for(int i = 0; i < 1024; i++) {
            //     packet.push_back(0xff000000 | i);
            // }

            soapy->sendToHiggs(packet);


            // exit(0);

            times_tried_attached_feedback_bus++;
            
            next = GO_AFTER(CHECK_ATTACHED_FB_BUS_2, _500_MS_SLEEP);
            }
            break;

        case CHECK_ATTACHED_FB_BUS_2:
            // cout << "GET_SKIP_CHECK_ATTACHED_FB() " << GET_SKIP_CHECK_ATTACHED_FB() << endl;
            if( attached->feedback_alive_count == 0 && !GET_SKIP_CHECK_ATTACHED_FB() ) {


                if(times_tried_attached_feedback_bus > 3 ) {
                    cout << endl;
                    cout << "-------------------------------------------" << endl;
                    cout << "Feedback bus is down for our attached TX Higgs" << endl;
                    cout << "try re-programming tx-bitfiles"              << endl;
                    cout << "-------------------------------------------" << endl;
                    if(GET_INTEGRATION_TEST_ENABLED()) {
                        cout << "TEST FAILED" << endl;
                        exit(1);
                    }
                }

                if( GET_RETRY_RESET_FB_BUS_AFTER_DOWN() ) {
                    next = GO_AFTER(CHECK_ATTACHED_FB_BUS_1, _1_SECOND_SLEEP);
                } else {
                    next = GO_AFTER(DEBUG_STALL, _1_SECOND_SLEEP);
                }


                // if( times_tried_attached_feedback_bus > 6 ) {
                //     next = GO_AFTER(RESET_ATTACHED_FB_BUS_0, _1_SECOND_SLEEP);
                // }

            } else {
                cout << endl;
                cout << "-------------------------------------------" << endl;
                cout << "Attached Feedback Bus is Alive" << endl;
                cout << "-------------------------------------------" << endl;
                // fixme, can be instant, longer just for feels
                next = GO_AFTER(DECIDE_HOW_TO_BEGIN, _5_SECOND_SLEEP);
            }

            // if not ready
            break;

        case JS_HANDLE_FB_BUS_0: {
            cout << "JS_HANDLE_FB_BUS_0\n";
            soapy->settings->vectorSet(true, (std::vector<std::string>){"runtime","js","recover_fb","request"});
            next = GO_AFTER(JS_HANDLE_FB_BUS_1, _50_MS_SLEEP);
        }
        break;

        case JS_HANDLE_FB_BUS_1: {
            cout << "JS_HANDLE_FB_BUS_1\n";
            bool done = settings->getDefault<bool>(false, "runtime", "js", "recover_fb", "result");

            if( !done ) {
                next = GO_AFTER(JS_HANDLE_FB_BUS_1, _1_SECOND_SLEEP);
            } else {
                soapy->settings->vectorSet(false, (std::vector<std::string>){"runtime","js","recover_fb","result"});
                next = GO_AFTER(SET_RANDOM_ROTATION, _3_SECOND_SLEEP);
            }
        }
        break;

        case RESET_ATTACHED_FB_BUS_0: {
                    cout << endl;
                    cout << "attempting to fix feedback bus" << endl;
                    raw_ringbus_t rb0 = {RING_ADDR_ETH, MAPMOV_RESET_CMD};
                    ringbusPeerLater(attached->peer_id, &rb0, 0);

                    raw_ringbus_t rb1 = {RING_ADDR_TX_PARSE, MAPMOV_RESET_CMD};
                    ringbusPeerLater(attached->peer_id, &rb1, 0);

                    next = GO_AFTER(RESET_ATTACHED_FB_BUS_1, _100_MS_SLEEP);
                    reset_zeros_counter = 0;
                }
            break;

        case RESET_ATTACHED_FB_BUS_1: {
                // zeros for now
                // auto p = feedback_flush_packet( 16 );
                // soapy->sendToHiggs(p);

                reset_zeros_counter++;
                if(reset_zeros_counter < 10) {
                    // repeat 10 times
                    next = GO_AFTER(RESET_ATTACHED_FB_BUS_1, _50_MS_SLEEP);
                } else {
                    // on to the next one
                    next = GO_AFTER(RESET_ATTACHED_FB_BUS_2, _100_MS_SLEEP);
                }

            }
            break;

        case RESET_ATTACHED_FB_BUS_2: {

                // auto p = feedback_unjam_packet();
                // vector<uint32_t> dumb = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
                // auto p = feedback_vector_packet(
                //         FEEDBACK_VEC_FLUSH,
                //         dumb,
                //         0,
                //         FEEDBACK_DST_HIGGS);

                // for(unsigned int i = 0; i < 4; i++) {
                //     // unsigned char* uchar_data = reinterpret_cast<unsigned char *>(p.data());
                //     // auto len = p.size()*4;
                //     soapy->sendToHiggs(p);
                //     // usleep(10*1000);
                // }

                times_tried_attached_feedback_bus = 0;

                // could be much shorter

                if( GET_DEBUG_RESET_FB_ONLY() ) {
                    next = GO_AFTER(RESET_ATTACHED_FB_BUS_0, _1_SECOND_SLEEP);
                } else {
                    next = GO_AFTER(CHECK_ATTACHED_FB_BUS_0, _50_MS_SLEEP);
                }
            }
            break;

        case WAIT_EVENTS: {
            bool triggered = false;
                if(  most_recent_event.d0 == REQUEST_MAPMOV_DEBUG
                  && most_recent_event.d1 == 1 ) {
                    cout << "TX side got REQUEST_MAPMOV_DEBUG" << endl;
                    clearRecentEvent(); // clear
                    
                    // next = GO_NOW(DEBUG_TX_MAPMOV_0);

                    soapy->txSchedule->doWarmup = true;

                    // triggered = true;
                }

                if( settings->getDefault<bool>(false, "runtime", "tx", "trigger_warmup") ) {
                    soapy->txSchedule->doWarmup = true;
                }
                

                if(most_recent_event.d0 == FSM_PING_EV) {
                    cout << "TX side got FSM_PING_EV" << endl;

                    auto our_peer = soapy->this_node.id;
                    // send a pong back
                    auto reply_to = most_recent_event.d1;

                    sendCustomEventToPartner(
                        reply_to,
                        MAKE_EVENT(FSM_PONG_EV, our_peer)
                        );


                    clearRecentEvent(); // clear
                    // next = GO_NOW(DEBUG_TX_MAPMOV_0);
                    // triggered = true;
                    soapy->txSchedule->doWarmup = true;
                }

                if(most_recent_event.d0 == EXIT_DATA_MODE_EV) {
                    cout << "Got event EXIT_DATA_MODE_EV\n";
                    soapy->txSchedule->exitDataMode = true;
                    next = GO_NOW(WAIT_TX_SCHEDULE_EXIT);
                    clearRecentEvent(); // clear
                    triggered = true;
                }

                if( most_recent_event.d0 == DUPLEX_FINE_SYNC) {
                    cout << "Got event DUPLEX_FINE_SYNC going to DUPLEX_FINE_0\n";
                    next = GO_NOW(DUPLEX_FINE_0);
                    clearRecentEvent();
                    triggered = true;
                }

                if(_thread_should_terminate) {
                    next = GO_NOW(TX_PREPARE_FSM_SHUTDOWN);
                }


                // soapy->flushHiggs->sendZmqHiggsNow();

                if( !triggered ) {
                    next = GO_NOW(WAIT_EVENTS);
                }
            }
            break;


        case DUPLEX_FINE_0:
            _rx_should_insert_sfo_cfo = true;


            // always true for narrow mode
            if( true ) {
                std::string eval1 = "\
                sjs.bs('tx', 0, 75, true); \
                sjs.ringbus(hc.RING_ADDR_CS11, hc.TX_TONE_2_VALUE | 0x2000);\
                ";
                jsEval(eval1);

                // std::string eval = "\
                // sjs.attached.setAllEqMask(sjs.masks.narrow_1); \
                // sjs.attached.updatePartnerEq(false, false); \
                // ";
                // jsEval(eval);


                // send this once
                // used for EQ_1 (only used in duplex mode)
                // attached->sendEqOne();


            }
            

            // next = GO_NOW(WAIT_EVENTS);
            next = GO_AFTER(DUPLEX_FINE_1, _16_SECOND_SLEEP );
            break;
        case DUPLEX_FINE_1:
            sendEvent(MAKE_EVENT(REQUEST_FINE_SYNC_EV,0));
            next = GO_NOW(WAIT_EVENTS);
            break;
        case DUPLEX_FINE_2:
            break;

        case WAIT_TX_SCHEDULE_EXIT:
            if( soapy->txSchedule->dsp_state == WAIT_EVENTS ) {
                cout << "TxSchedule went to sleep\n";
                soapy->flushHiggs->allow_zmq_higgs = true;
                next = GO_NOW(WAIT_EVENTS);
            }
            break;













        case TX_PREPARE_FSM_SHUTDOWN:
            soapy->flushHiggs->allow_zmq_higgs = false;
            next = GO_AFTER(TX_PREPARE_FSM_SHUTDOWN_STAGE_0, _1_SECOND_SLEEP);
            break;

        case TX_PREPARE_FSM_SHUTDOWN_STAGE_0: {
                auto tv2 = _1_SECOND_SLEEP;
                event_base_loopexit(evbase, &tv2);
            }
            break;











































        case DEBUG_TX_LOCAL_EPOC_0:
            break;

        case WAIT_FOR_PEERS:
            break;

        case DEBUG_PEER_PERFORMANCE_0:
            break;

        case DEBUG_STALL:
        case TX_STALL:
            next = GO_AFTER(TX_STALL, _1_SECOND_SLEEP);
            break;

        break;
        default:
            cout << "Bad state (" << tx_dsp_state << ") in eventdsp::tickFsmTx" << endl;
            break;
    }


    if( __dsp_preserve != tx_dsp_state ) {
        cout << "\nERROR dsp_state changed during operation.  Expected: " 
        << __dsp_preserve
        << " Got: " << tx_dsp_state
        << endl;
        usleep(100);
        assert(0);
    }

    if(__timer != __UNSET_NEXT__) {
        // cout << "timer mode" << endl;
        tx_dsp_state_pending = __timer;
        evtimer_add(tx_fsm_next_timer, &tv);
    } else if(__imed != __UNSET_NEXT__) {
        tx_dsp_state_pending = __imed;
        auto timm = SMALLEST_SLEEP;
        evtimer_add(tx_fsm_next_timer, &timm);
    } else if(next != 0 && next != __UNSET_NEXT__) {
        cout << endl << "ERROR You cannot use:  ";
        cout << endl << "   next = " << endl << endl << "without GO_NOW() or GO_AFTER()      (inside EventDsp::tickFsm)" << endl << endl << endl;
        usleep(100);
        assert(0);
    } else {
        // default
        tx_dsp_state_pending = tx_dsp_state;
        evtimer_add(tx_fsm_next_timer, &tv);
        // cout << "using default next in tiskFsm" << endl;
    }
}