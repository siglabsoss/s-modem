#include "HiggsSoapyDriver.hpp"
#include "HiggsDriverSharedInclude.hpp"
#include "driver/EventDsp.hpp"
#include "driver/RadioEstimate.hpp"
#include "common/convert.hpp"
#include <math.h>
#include "driver/AirPacket.hpp"
#include "driver/RetransmitBuffer.hpp"
#include "random.h"


using namespace std;


static unsigned tunFirstLength(HiggsDriver* const soapy) {
    if( soapy->txTun2EventDsp.size() ) {
        auto peek = soapy->txTun2EventDsp.peek();
        return peek.size();
    }

    cout << "warning tunFirstLength() was called when no packets were ready" << endl;
    return 0;
}

// expensive
size_t EventDsp::wordsInTun(void) {
    return soapy->txTun2EventDsp.expensiveSizeAll();
}


void EventDsp::debugJoint(const std::vector<size_t>& peers, const uint8_t seq, const size_t size, const uint32_t epoc, const uint32_t ts) {
    // const uint8_t seq = debug_joint_seq;
    // debug_joint_seq++;

    size_t total = 0;
    constexpr size_t pack = 350;


    std::vector<uint32_t> packet;
    unsigned int state = 0xdead0000;
    for(size_t i = 0; i < pack; i++ ) {
        state = xorshift32(state, &state, 1);
        packet.push_back(state);
    }

    std::vector<std::vector<uint32_t>> all;

    uint32_t j = 0;
    while(total+pack <= size) {

        packet[0] = j;
        packet[1] = j;
        packet[2] = j;
        packet[3] = j;

        all.emplace_back(packet);
        total += packet.size();
        j++;
    }

    if( total < size ) {
        auto copy = packet;
        copy.resize(size-total);
        all.emplace_back(copy);
    }

    auto payload = AirPacket::packJointZmq(soapy->this_node.id, epoc, ts, seq, all);
    sendVectorTypeToMultiPC(peers, FEEDBACK_VEC_JOINT_TRANSMIT, payload);
}


void EventDsp::setupRepeat() {
    repeatBuffer.resize(5);
    repeatHistory = 0;
    enableRepeat = false;
}


// This is called by the javascript thread
//
// We call the functional code below
// The code below takes 1 function for a few different tasks
// packets_ready_t how many packets
// next_packet_t vector of words for next packet (destructive)
// first packet len - what is the size of the next packet, lambda's must be consistent
// hash_for_words_t - should be a pure function that takes words and returns a hash.
// retransmit_t   - passes information to the retransmit
std::pair<size_t,size_t> EventDsp::scheduleJointFromTunData(const std::vector<size_t>& peers, const uint8_t seq, const size_t max_words, const uint32_t epoc, const uint32_t ts) {

    // placeholders
    packets_ready_t    a;
    next_packet_t      b;
    first_packet_len_t c;
    hash_for_words_t   d;
    retransmit_t       e;

    repeatHistory  = (repeatHistory+1)%5;
    const unsigned historyLoad = (repeatHistory+4)%5;

    // how many packets are there
    packets_ready_t fn_packets = [&](void) {
        return repeatBuffer[repeatHistory].size() + retransmitArb->packetsReady() + soapy->txTun2EventDsp.size();
    };

    // get the next packet
    // must return data in the same order as tunFirstLength() (packets_ready_t,first_packet_len_t)
    next_packet_t fn_next = [&](void) {

        if( repeatBuffer[repeatHistory].size() ) {
            auto copy = repeatBuffer[repeatHistory][0];
            repeatBuffer[repeatHistory].erase(repeatBuffer[repeatHistory].begin());
            return copy;
        }

        if( retransmitArb->packetsReady() ) {
            return retransmitArb->getPacket();
        }
        if( soapy->txTun2EventDsp.size() ) {
            auto copy = soapy->txTun2EventDsp.dequeue();
            if( enableRepeat ) {
                repeatBuffer[historyLoad].push_back(copy);
            }
            //cout << "Copied packet into history " << historyLoad << "\n";
            return copy;
        }

        cout << "warning tunGetPacket() was called when no packets were ready" << endl;
        return (std::vector<uint32_t>){};
    };

    // get th elength of the next packet
    // must return data in the same order as tunFirstLength() (packets_ready_t,first_packet_len_t)
    first_packet_len_t fn_first_len = [&](void) {
        if( repeatBuffer[repeatHistory].size() ) {
            return repeatBuffer[repeatHistory][0].size();
        }

        if( retransmitArb->packetsReady() ) {
            return (size_t)retransmitArb->firstLength();
        }

        if( soapy->txTun2EventDsp.size() ) {
            auto peek = soapy->txTun2EventDsp.peek();
            return (size_t)peek.size();
        }

        cout << "warning tunFirstLength() (lambda) was called when no packets were ready" << endl;
        return (size_t)0;
    };

    hash_for_words_t fn_hash = [&](const std::vector<uint32_t>& words) {
        auto packet_hash = verifyHashArb->getHashForWords(words);
        return packet_hash;
    };

    retransmit_t fn_retransmit = [&](const std::vector<uint32_t>&hashes, const std::vector<std::vector<uint32_t>>&for_retransmit, const uint32_t tx_seq) {

        // cout << "retransmit ";
        // prepend the hashes with the sequence number of this packet
        // hashes.insert(hashes.begin(), (uint32_t)tx_seq);

        // save the packet we just sent, for later
        retransmitArb->sentOverAir(tx_seq, for_retransmit);

        // cout << "Hashes: ";
        // for(const auto w : hashes ) {
        //     cout << HEX32_STRING(w) << ", ";
        // }
        // cout << "\n";


        // arb sends hashes to rx (This is why it feels backwards)
        size_t send_hash_to = GET_PEER_RX();
        this->sendHashesToPartnerSeq(send_hash_to, "", hashes, (uint32_t)tx_seq);
    };


    bool runRetransmitCode = GET_DATA_HASH_VALIDATE_ALL();

    // std::vector<uint32_t> hashes;

    return scheduleJointFromTunDataFunctional(
         peers
        ,seq
        ,max_words
        ,epoc
        ,ts
        ,runRetransmitCode
        // ,hashes
        ,fn_packets
        ,fn_next
        ,fn_first_len
        ,fn_hash
        ,fn_retransmit
        );
}



/// caller specifies how many words to pack. this function pulls packets from std::functions that are
/// passed as arguments.
/// builds / sends a zmq message which is the joint transmit
/// these are packets that were pulled from the tun and built together into a
/// siglabs packet. does not have sync word or coding at this point
/// @param peers - who to send to
/// @param seq - seq to pass to air packet, the coarse sequence number of this packet
/// @param max_words max allowed pull
/// @param epoc  the epoc to send at
/// @param ts    the timeslot to send at
/// @param getPacketsReady  - call this function to find out how many packets there are
/// @param getNextPacket  - call this function return and dequeue the next packet
/// @param getNextPacketLenght  - call this function to peek at the next packet. the peek is only the length. packet is not touched


std::pair<size_t,size_t> EventDsp::scheduleJointFromTunDataFunctional(
    const std::vector<size_t>& peers,
    const uint8_t seq,
    const size_t max_words,
    const uint32_t epoc,
    const uint32_t ts,
    const bool doRetransmit,
    // std::vector<uint32_t>& hashes,
    packets_ready_t&    getPacketsReady,
    next_packet_t&      getNextPacket,
    first_packet_len_t& getNextLength,
    hash_for_words_t&   getHash,
    retransmit_t&       updateRetransmit
    ) {
    // size_t ret;

    auto packetsReady = getPacketsReady(); //soapy->txTun2EventDsp.size();

    if( packetsReady == 0 ) {
        cout << "scheduleJointFromTunData() did not send a ZMQ, there were 0 packets ready\n";
        return {0,0};
    }

    std::vector<uint32_t> hashes;
    // hashes.resize(0);
    std::vector<std::vector<uint32_t>> for_retransmit; // for now, this is identical to 'all'

    // cout << "scheduleJointFromTunData() " << epoc << ", " << ts << "\n";

    bool more = true;
    unsigned total_packets = 0;
    size_t total_words = 0;

    // all is a vector of vector
    // we pull vectors out of the buffer via the std::function 
    // we take the hash but we do not attach the hash here
    // we convert a function call into a vector of vector of packets as words
    //
    // then below we convert this to a single vector with a transformation
    // AirPacket::packJointZmq
    std::vector<std::vector<uint32_t>> all;

    while(more) {
        total_packets++;
        const auto fromTun = getNextPacket(); //soapy->txTun2EventDsp.dequeue();
        
        uint32_t packet_hash = 0;

        // if true, we are copying all original packets and hashes and using
        // the retransmit code
        // note that we do NOT put the hash into the packet here
        // see loadAllPackets
        if(doRetransmit) {
            packet_hash = getHash(fromTun);

            // cout << "  saved hash " << HEX32_STRING(packet_hash) << endl;
            hashes.emplace_back(packet_hash);
            for_retransmit.push_back(fromTun);
        }


        total_words += fromTun.size();

        all.emplace_back(fromTun);

        more = false;
        

        // was txTun2EventDsp.size()
        if( getPacketsReady() > 0 ) {
            const auto next_size = getNextLength(); //tunFirstLength(soapy);

            if( (next_size + total_words) <= max_words) {
                more = true;
            }
        }
    }

    // we're good to go
    auto payload = AirPacket::packJointZmq(soapy->this_node.id, epoc, ts, seq, all);
    sendVectorTypeToMultiPC(peers, FEEDBACK_VEC_JOINT_TRANSMIT, payload);


    // do this after we send zmq to save on timing
    if(doRetransmit) {
        updateRetransmit(hashes, for_retransmit, (uint32_t)seq);
    }


    return {total_words,total_packets};
}


unsigned EventDsp::dumpTunBuffer(void) {
    unsigned packetsReady = soapy->txTun2EventDsp.size();

    bool more = packetsReady > 0;

    unsigned burned = 0;

    while(more) {
        soapy->txTun2EventDsp.dequeue(); // burn
        packetsReady = soapy->txTun2EventDsp.size();
        more = packetsReady > 0;
        burned++;
    };

    // cout << "dumpTunData dropped " << burned << " packets\n";
    return burned;
}
