#include "HiggsSoapyDriver.hpp"
#include "HiggsDriverSharedInclude.hpp"
#include "driver/EventDsp.hpp"
#include "driver/RadioEstimate.hpp"
#include "common/convert.hpp"
#include <math.h>
#include "driver/AirPacket.hpp"
#include "random.h"
#include "common/GenericOperator.hpp"

//////
///
///  Most, if not all, of these functions are 
///  called on the libevent2 evented thread
///

using namespace std;


///
/// Warning as of now, all feedback bus messages
/// must be length 1 or longer (17 or longer with header included)
///






///
/// Send a fb bus packet to the cs20 on the rx side
/// if it is awake, it will reply with a rb
///
void EventDsp::pingPartnerFbBus(const size_t peer) {
    // cout << "ping parner " << peer << endl;
    // there is a bug, sending size 0 does not work
    // size 1 maybe?
    std::vector<uint32_t> dumb;
    dumb.resize(16);
    auto packet = feedback_vector_packet(
                FEEDBACK_VEC_STATUS_REPLY,
                dumb,
                feedback_peer_to_mask(peer),
                FEEDBACK_DST_HIGGS);

    // for(auto w : packet) {
    //     cout << "0x" << HEX32_STRING(w) << ", ";
    // }
    // cout << endl << endl;

    soapy->zmqPublishOrInternal(peer, packet);
}

void EventDsp::resetPartnerEq(const size_t peer) {
    raw_ringbus_t rb0 = {RING_ADDR_TX_EQ, EQUALIZER_CMD | 0};
    zmqRingbusPeerLater(peer, &rb0, 0);
    // soapy->zmqRingbusPeer(peer, (char*)&rb0, sizeof(rb0));
    // usleep(10);
}

///
/// Reset barrel shift for partner (or self)
/// pass empty string to do tx and rx, otherwise pass an explicit "tx" or "rx"
void EventDsp::resetPartnerBs(const size_t peer, const std::string& which) {

    std::vector<std::string> work;
    work.reserve(2);

    if(which == "") {
        work.push_back("tx");
        work.push_back("rx");
    } else {
        work.push_back(which);
    }

    __suseconds_t future = 0; // I think this is a long int

    for(const auto w : work) {
        if( w == "tx") {
            const raw_ringbus_t rb0 = {RING_ADDR_TX_EQ, DEFAULT_APP_BARREL_SHIFT_CMD | 0};
            zmqRingbusPeerLater(peer, &rb0, future);
            future+=500;

            const raw_ringbus_t rb1 = {RING_ADDR_TX_FFT, DEFAULT_APP_BARREL_SHIFT_CMD | 0};
            zmqRingbusPeerLater(peer, &rb1, future);
            future+=500;
        }

        if( w == "rx") {
            const raw_ringbus_t rb2 = {RING_ADDR_RX_FFT, DEFAULT_APP_BARREL_SHIFT_CMD | 0};
            zmqRingbusPeerLater(peer, &rb2, future);
            future+=500;

            const raw_ringbus_t rb3 = {RING_ADDR_RX_FINE_SYNC, DEFAULT_APP_BARREL_SHIFT_CMD | 0};
            zmqRingbusPeerLater(peer, &rb3, future);
            future+=500;
        }
    }

    if( future == 0 ) {
        cout << "warning resetPartnerBs() didn't send anything\n";
    }
}

void EventDsp::setPartnerTDMASubcarrier(const size_t peer, const uint32_t sc) {
    uint32_t masked = sc & 0x3ff;

    cout << "//// Setting TX Subcarrier to " << masked << " to peer " << peer << "\n";

    raw_ringbus_t rb0 = {RING_ADDR_TX_PARSE, SET_TDMA_SC_CMD | masked};
    ringbusPeerLater(peer, rb0, 0);
}

void EventDsp::setPartnerTDMA(const RadioEstimate* peer, const uint32_t dmode, const uint32_t value) {
    setPartnerTDMA(peer->peer_id, dmode, value);
}

void EventDsp::setPartnerTDMA(const size_t peer, const uint32_t dmode, const uint32_t value) {
    cout << "//// Sending TDMA to peer " << peer
    << " mode " << dmode <<  " arg " << value << " /////////////////////////////////////////////////////////////////////////"
    << "\n";

    uint32_t lower = ((dmode&0xff)<<16) | (value&0xffff);
    raw_ringbus_t rb0 = {RING_ADDR_TX_PARSE, TDMA_CMD | lower};
    zmqRingbusPeerLater(peer, &rb0, 0);
    // soapy->zmqRingbusPeer(peer, (char*)&rb0, sizeof(rb0));
    
}

void EventDsp::setPartnerCfo(const size_t peer, const double cfo_estimated, bool alsoRxChain) {
    const bool sign = (cfo_estimated<0.0);
    const uint32_t cfo_abs_tx = (uint32_t)abs(cfo_estimated*131.072);

    // const uint32_t cfo_abs_rx = (uint32_t)abs(cfo_estimated*131.072*1280); // this version causes rounding error
    const uint32_t cfo_abs_rx = (uint32_t)abs( (int32_t)(cfo_abs_tx)*1280); // this version works way better

    const uint32_t lowerTx  = (cfo_abs_tx&0xffff);
    uint32_t upperTx       = ((cfo_abs_tx>>16)&0xffff);

	const uint32_t lowerRx  = (cfo_abs_rx&0xffff);
    uint32_t upperRx       = ((cfo_abs_rx>>16)&0xffff);

    if(sign) {
      upperRx      |= 0x010000;
    } else {
      upperTx      |= 0x010000;
      // upperRx      |= 0x010000;
    }
    
    const uint32_t our_id = soapy->this_node.id;

    // true for transmit
    // false for receive
    bool sendToPartnerRadio = (peer!=our_id);

    if(sendToPartnerRadio) {
        const raw_ringbus_t rb0 = {RING_ADDR_TX_TIME_DOMAIN, TX_CFO_LOWER_CMD | lowerTx};
        const raw_ringbus_t rb1 = {RING_ADDR_TX_TIME_DOMAIN, TX_CFO_UPPER_CMD | upperTx};

        zmqRingbusPeerLater(peer, &rb0, 0);
        zmqRingbusPeerLater(peer, &rb1, 100);

        if( alsoRxChain ) {
            const raw_ringbus_t rb2 = {RING_ADDR_RX_FFT, TX_CFO_LOWER_CMD | lowerRx};
            const raw_ringbus_t rb3 = {RING_ADDR_RX_FFT, TX_CFO_UPPER_CMD | upperRx};

            zmqRingbusPeerLater(peer, &rb2, 200);
            zmqRingbusPeerLater(peer, &rb3, 300);

            cout << "Sending cfo Partner Radio Rx Chain " << HEX32_STRING(lowerTx) << endl;
            cout << "Sending cfo Partner Radio Rx Chain " << HEX32_STRING(upperTx) << endl;
        }


        cout << "Sending cfo Partner Radio Tx Chain " << HEX32_STRING(lowerTx) << endl;
        cout << "Sending cfo Partner Radio Tx Chain " << HEX32_STRING(upperTx) << endl;

    } else {
        const raw_ringbus_t rb0 = {RING_ADDR_TX_TIME_DOMAIN, TX_CFO_LOWER_CMD | lowerTx};
        const raw_ringbus_t rb1 = {RING_ADDR_TX_TIME_DOMAIN, TX_CFO_UPPER_CMD | upperTx};

        zmqRingbusPeerLater(peer, &rb0, 0);
        zmqRingbusPeerLater(peer, &rb1, 100);
        
        if( alsoRxChain ) {
            soapy->cs31CFOCorrection(TX_CFO_LOWER_CMD | lowerRx);
            usleep(500);
            soapy->cs31CFOCorrection(TX_CFO_UPPER_CMD | upperRx);

            cout << "Sending cfo Self Radio Rx Chain " << HEX32_STRING(lowerRx) << endl;
            cout << "Sending cfo Self Radio Rx Chain " << HEX32_STRING(upperRx) << endl;
        }



    }
}

void EventDsp::zeroPartnerEq(const size_t peer) {
    std::vector<uint32_t> zrs;
    zrs.resize(1024);
    for(size_t i = 0; i < 1024; i++) {
        zrs[i] = 0;
    }
    auto packet = feedback_vector_packet(
                FEEDBACK_VEC_TX_EQ,
                zrs,
                feedback_peer_to_mask(peer),
                FEEDBACK_DST_HIGGS);

    soapy->zmqPublishOrInternal(peer, packet);
}

void EventDsp::setPartnerEq(const RadioEstimate& peer, const std::vector<uint32_t>& s) {
    setPartnerEqCore(peer, s, FEEDBACK_VEC_TX_EQ);
}
void EventDsp::setPartnerEq(const size_t peer, const std::vector<uint32_t>& s) {
    setPartnerEqCore(peer, s, FEEDBACK_VEC_TX_EQ);
}
void EventDsp::setPartnerEqOne(const RadioEstimate& peer, const std::vector<uint32_t>& s) {
    setPartnerEqCore(peer, s, FEEDBACK_VEC_TX_EQ_1);
}
void EventDsp::setPartnerEqOne(const size_t peer, const std::vector<uint32_t>& s) {
    setPartnerEqCore(peer, s, FEEDBACK_VEC_TX_EQ_1);
}



void EventDsp::setPartnerEqCore(const RadioEstimate& peer, const std::vector<uint32_t>& s, const uint32_t type) {
    setPartnerEqCore(peer.peer_id, s, type);
}

void EventDsp::setPartnerEqCore(const size_t peer, const std::vector<uint32_t>& s, const uint32_t type) {

    cout << "//// Sending EQ to peer " << peer << " size " << s.size() << " /////////////////////////////////////////////////////////////////////////" << endl;

    auto packet = feedback_vector_packet(
        type, 
        s, 
        feedback_peer_to_mask(peer),
        FEEDBACK_DST_HIGGS);


    if( GET_PRINT_OUTGOING_ZMQ() ) {
        cout << endl;
        cout << endl;
        for( auto w : packet ) {
            cout << HEX32_STRING(w) << "\n";
        }
        cout << endl;
        cout << endl;
    }



    soapy->zmqPublishOrInternal(peer, packet);
}



// from the tx side, going back to rx side
void EventDsp::transportRingToPartner(const size_t peer, const std::vector<uint32_t>& s) {

    auto packet = feedback_vector_packet(
        FEEDBACK_VEC_TRANSPORT_RB, 
        s, 
        feedback_peer_to_mask(peer),
        FEEDBACK_DST_PC);

    if( peer == soapy->this_node.id ) {
        // doesn't make sense I think, that's why we do not call zmqPublishOrInternal()
        cout << "transportRingToPartner() will not deliver to own ringbus\n";
    }

    soapy->zmqPublish(ZMQ_TAG_ALL, packet);
}


void EventDsp::sendCustomEventToPartner(const size_t peer, const uint32_t d0, const uint32_t d1) {
    custom_event_t e;
    e.d0 = d0;
    e.d1 = d1;
    sendCustomEventToPartner(peer, e);
}

void EventDsp::sendCustomEventToPartner(const size_t peer, const custom_event_t e) {

    cout << "//// Sending CustomEvent " << "to peer " << peer << " /////////////////////////////////////////////////////////////////////////" << endl;

    std::vector<uint32_t> s = custom_event_to_vector(e);

    auto packet = feedback_vector_packet(
        FEEDBACK_VEC_DELIVER_FSM_EVENT, 
        s, 
        feedback_peer_to_mask(peer),
        FEEDBACK_DST_PC);

    soapy->zmqPublishOrInternal(peer, packet);
}


// same as sendHashesToPartner, but we pass sequence as it's own param
// in other code we use sendHashesToPartner() but we put the seq on hashes before we call it
void EventDsp::sendHashesToPartnerSeq(const size_t peer, const std::string& us, const std::vector<uint32_t>& hashes, const uint32_t tx_seq) {
    // cout << "//// Sending " << hashes.size() << " hashes to peer " << peer << " ////" << endl;
    
    // copy and modify the copy
    auto copy = hashes;
    copy.insert(copy.begin(), (uint32_t)tx_seq);


    auto packet = feedback_vector_packet(
                FEEDBACK_VEC_PACKET_HASH,
                copy,
                feedback_peer_to_mask(peer),
                FEEDBACK_DST_PC);

    soapy->zmqPublishOrInternal(peer, packet);


    if( GET_PRINT_VERIFY_HASH() ) {
        cout << "EventDsp::sendHashesToPartner got " << copy.size() << " hashes" << endl;
        for(auto x : copy) {
            cout << "   " << HEX32_STRING(x) << endl;
        }
    }
}


void EventDsp::sendHashesToPartner(const size_t peer, const std::string& us, const std::vector<uint32_t>& hashes) {
    // cout << "//// Sending " << hashes.size() << " hashes to peer " << peer << " ////" << endl;
    auto packet = feedback_vector_packet(
                FEEDBACK_VEC_PACKET_HASH,
                hashes,
                feedback_peer_to_mask(peer),
                FEEDBACK_DST_PC);

    soapy->zmqPublishOrInternal(peer, packet);


    if( GET_PRINT_VERIFY_HASH() ) {
        cout << "EventDsp::sendHashesToPartner got " << hashes.size() << " hashes" << endl;
        for(auto x : hashes) {
            cout << "   " << HEX32_STRING(x) << endl;
        }
    }
}

// we are rx, and we are telling tx we missed some packets
void EventDsp::sendRetransmitRequestToPartner(const size_t peer, const uint8_t seq, const std::vector<uint32_t>& indices) {
    cout << "//// Sending " << indices.size() << " packets for retransmit for seq " << (int)seq << " idxs ";

    for( auto x : indices ) {
        cout << " (" << x << ")";
    }

    cout << " hashes to peer " << peer << " ////" << endl;
    
    std::vector<uint32_t> p;
    p = indices;
    p.insert(p.begin(), (uint32_t)seq);

    for( auto x : p ) {
        cout << " - " << HEX32_STRING(x) << endl;
    }

    auto packet = feedback_vector_packet(
                FEEDBACK_VEC_REQ_RETRANSMIT,
                p,
                feedback_peer_to_mask(peer),
                FEEDBACK_DST_PC);

    // cout << "pre pub\n";
    soapy->zmqPublishOrInternal(peer, packet);
    // cout << "post pub\n";
}

void EventDsp::sendVectorTypeToPartnerPC(const size_t peer, const size_t vtype, const std::vector<uint32_t>& args) {
    auto packet = feedback_vector_packet(
                vtype,
                args,
                feedback_peer_to_mask(peer),
                FEEDBACK_DST_PC);

    soapy->zmqPublishOrInternal(peer, packet);
}


void EventDsp::sendVectorTypeToMultiPC(const std::vector<size_t> peers, const size_t vtype, const std::vector<uint32_t>& args) {
    uint32_t bitfield = 0;

    bool for_us = false;

    for(const auto peer : peers) {
        bitfield |= feedback_peer_to_mask(peer);
        if( peer == soapy->this_node.id ) {
            for_us = true;
        }
    }
    auto packet = feedback_vector_packet(
                vtype,
                args,
                bitfield,
                FEEDBACK_DST_PC);

    std::string tag = ZMQ_TAG_ALL;

    if(peers.size() == 1) {
        //std::cout << "SEND TO TAG: " << soapy->zmqGetPeerTag(peers[0]).c_str() << std::endl;
        //std::cout << "Peer: " << peers[0] << std::endl;  
        soapy->zmqPublish(soapy->zmqGetPeerTag(peers[0]).c_str(), packet);
    } else {
        soapy->zmqPublish(ZMQ_TAG_ALL, packet);
    }
    //tag = "@x";
    //std::cout << "SEND TO TAG: " << tag << std::endl;
    //std::cout << "Peer: " << peers[0] << std::endl;

    

    if( for_us ) {
        soapy->zmqPublishInternal(ZMQ_TAG_ALL, packet);
    }
}



void EventDsp::setPartnerPhase(const size_t peer, const double rads) {

    // cout << "rads " << rads << endl;
    const double inrange = fmod(rads+2*M_PI, 2*M_PI);

    // cout << "inrange " << inrange << endl;
    const uint32_t phase_counts = (uint32_t)((inrange/(2*M_PI))*0xffffffff);

    // cout << "phase_counts " << phase_counts << endl;

    const uint32_t lower =   (phase_counts&0xffff);
    const uint32_t upper = (((phase_counts>>16)&0xffff) | 0x020000);

    const raw_ringbus_t rb0 = {RING_ADDR_TX_TIME_DOMAIN, TX_CFO_LOWER_CMD | lower};
    const raw_ringbus_t rb1 = {RING_ADDR_TX_TIME_DOMAIN, TX_CFO_UPPER_CMD | upper};

    zmqRingbusPeerLater(peer, &rb0, 0);
    zmqRingbusPeerLater(peer, &rb1, 100);

    // cout << "Sending Phase " << HEX32_STRING(lower) << endl;
    // cout << "Sending Phase " << HEX32_STRING(upper) << endl;
}

void EventDsp::setPartnerSfoAdvance(const RadioEstimate* peer, const uint32_t amount, const uint32_t direction, const bool alsoRxChain) {
    setPartnerSfoAdvance(peer->peer_id,  amount,  direction);
}

void EventDsp::setPartnerSfoAdvance(const RadioEstimate& peer, const uint32_t amount, const uint32_t direction, const bool alsoRxChain) {
    setPartnerSfoAdvance(peer.peer_id,  amount,  direction);
}

// warning this will make multuple rb calls to protect mode 4
void EventDsp::setPartnerSfoAdvance(const size_t peer, const uint32_t _amount, const uint32_t _direction, const bool alsoRxChain) {
    // mask to 24 bits before oring

    uint32_t amount = _amount & 0xffffff;
    const uint32_t direction = _direction & 0xffffff;

    constexpr uint32_t cap = 128;

    // STO 
    if(direction == 4 || direction == 3) {
        // chunked operation
        cout << "chunked setPartnerSfoAdvance() for " << peer <<endl;

        uint32_t amount_chunk;
        uint32_t us_timer = 0;
        while(amount)
        {

            amount_chunk = std::min((uint32_t)cap, amount);
            cout << "amount_chunk " << amount_chunk << endl;

            const raw_ringbus_t rb0 = {RING_ADDR_TX_TIME_DOMAIN, SFO_PERIODIC_ADJ_CMD | amount_chunk};
            const raw_ringbus_t rb1 = {RING_ADDR_TX_TIME_DOMAIN, SFO_PERIODIC_SIGN_CMD | direction };

            zmqRingbusPeerLater(peer, &rb0, us_timer);
            us_timer += 5000;
            zmqRingbusPeerLater(peer, &rb1, us_timer);
            us_timer += 5000;

            if( alsoRxChain ) {
                const raw_ringbus_t rb2 = {RING_ADDR_RX_FFT, SFO_PERIODIC_ADJ_CMD | amount_chunk};
                const raw_ringbus_t rb3 = {RING_ADDR_RX_FFT, SFO_PERIODIC_SIGN_CMD | direction };

                zmqRingbusPeerLater(peer, &rb2, us_timer);
                us_timer += 5000;
                zmqRingbusPeerLater(peer, &rb3, us_timer);
                us_timer += 5000;
            }

            
            amount -= amount_chunk;

        }


    } else {
        // SFO

        // direction 0, 1, 2

        cout << "normal setPartnerSfoAdvance()" << endl;
        // normal operation

        const raw_ringbus_t rb0 = {RING_ADDR_TX_TIME_DOMAIN, SFO_PERIODIC_ADJ_CMD | amount};
        const raw_ringbus_t rb1 = {RING_ADDR_TX_TIME_DOMAIN, SFO_PERIODIC_SIGN_CMD | direction };

        // soapy->zmqRingbusPeer(peer, (char*)&rb0, sizeof(rb0));
        zmqRingbusPeerLater(peer, &rb0, 0);
        zmqRingbusPeerLater(peer, &rb1, 1);


        if( alsoRxChain ) {

            const raw_ringbus_t rb2 = {RING_ADDR_RX_FFT, SFO_PERIODIC_ADJ_CMD | amount};
            const raw_ringbus_t rb3 = {RING_ADDR_RX_FFT, SFO_PERIODIC_SIGN_CMD | direction };

            zmqRingbusPeerLater(peer, &rb2, 0);
            zmqRingbusPeerLater(peer, &rb3, 1);
        }
    }
}




void EventDsp::sendPartnerNoop(const size_t peer) {

    // 0 rb is ignored by eth
    raw_ringbus_t rb0 = {0, 0};

    soapy->zmqRingbusPeer(peer, (char*)&rb0, sizeof(rb0));
}


// always positive
// void EventDsp::advancePartnerSchedule(const size_t peer, const uint32_t _amount) {
//     uint32_t amount = _amount & 0xffffff;

    // raw_ringbus_t rb0 = {RING_ADDR_CS11, SCHEDULE_CMD | amount};

//     soapy->zmqRingbusPeer(peer, (char*)&rb0, sizeof(rb0));
// }



// void EventDsp::resetPartnerScheduleAdvance(const RadioEstimate& peer) {
//     resetPartnerScheduleAdvance(peer.peer_id);
// }
// // weird name, but tries to set the timing to within the timing
// // of the network
// void EventDsp::resetPartnerScheduleAdvance(const size_t peer) {
//     raw_ringbus_t rb0 = {RING_ADDR_CS20, SCHEDULE_RESET_CMD | 0};
//     soapy->zmqRingbusPeer(peer, (char*)&rb0, sizeof(rb0));
// }

//
// value is shifed and passed as middle 8 bits
// low_value is 16 bits, is the lower, unshifted part
// also, low_value is not used right now
//
// See test_tx_6/cs20/main.c
//

    // switch(dmode) {
    //     case 1:
    //         align_canned_to_progress = 0;
    //         break;
    //     case 2:
    //         align_canned_to_progress = 1;
    //         break;
    //     case 3:
    //         // OV_COUNTER_SC is data (normal) (default)
    //         inserted_data_is_pilot = 0;
    //         break;
    //     case 4:
    //         // OV_COUNTER_SC is pilot (normal) (default)
    //         inserted_data_is_pilot = 1;
    //         break;



void EventDsp::sendJointAck(
                const size_t peer
                ,const uint32_t packet_epoc
                ,const uint32_t packet_ts
                ,const uint8_t seq
                ,const uint32_t rx_epoc
                ,const uint32_t rx_ts
                ,const bool dropped
                ) {

    uint32_t our_id = soapy->this_node.id;

    std::vector<uint32_t> vec = {
        our_id
        ,packet_epoc
        ,packet_ts
        ,seq
        ,rx_epoc
        ,rx_ts
        ,dropped?1u:0u
    };

    sendVectorTypeToPartnerPC(peer, FEEDBACK_VEC_JOINT_ACK, vec);
}

void EventDsp::sendJointAck(
                const size_t peer
                ,const uint32_t packet_lifetime
                ,const uint8_t seq
                ,const uint32_t now_lifetime
                ,const bool dropped
                ) {

    uint32_t our_id = soapy->this_node.id;

    std::vector<uint32_t> vec = {
        our_id
        ,0
        ,packet_lifetime
        ,seq
        ,0
        ,now_lifetime
        ,dropped?1u:0u
    };

    sendVectorTypeToPartnerPC(peer, FEEDBACK_VEC_JOINT_ACK, vec);
}

// void EventDsp::sendTxDrop(
//                 const size_t peer
//                 ,const uint32_t packet_epoc
//                 ,const uint32_t packet_ts
//                 ,const uint8_t seq
//     )

void EventDsp::jsEval(const std::string& in) {

    auto data = stringToWords(in);

    auto packet = feedback_vector_packet(
            FEEDBACK_VEC_JS_INTERNAL,
            data,
            feedback_peer_to_mask(soapy->this_node.id),
            FEEDBACK_DST_PC);
    // if( for_us ) {
    soapy->zmqPublishInternal(ZMQ_TAG_ALL, packet);
    // }

}

void EventDsp::partnerOp(const size_t peer, const uint32_t fpga, const std::string s, const uint32_t _sel, const uint32_t _val) {
    auto pack = siglabs::rb::op(s, _sel, _val, GENERIC_OPERATOR_CMD);

    raw_ringbus_t rb0 = {fpga, pack[0] };
    raw_ringbus_t rb1 = {fpga, pack[1] };
    zmqRingbusPeerLater(peer, &rb0, 0);
    zmqRingbusPeerLater(peer, &rb1, 1);
}

void EventDsp::op(const uint32_t fpga, const std::string s, const uint32_t _sel, const uint32_t _val) {
    const size_t our_id = soapy->this_node.id;
    partnerOp(our_id, fpga, s, _sel, _val);
}



void EventDsp::forceZeros(const size_t peer, const bool force_zeros) {

    const uint32_t rb_word = (ZERO_TX_CMD) | (force_zeros?1:0);

    const raw_ringbus_t rb0 = {RING_ADDR_TX_TIME_DOMAIN, rb_word };
    zmqRingbusPeerLater(peer, &rb0, 0);
}


// check attached feedback bus
void EventDsp::test_fb(void) {
            std::vector<uint32_t> dumb;
            dumb.resize(16);
            auto packet = feedback_vector_packet(
                FEEDBACK_VEC_STATUS_REPLY,
                dumb,
                0,
                0xdeadbeef);

            soapy->sendToHiggs(packet);
}
