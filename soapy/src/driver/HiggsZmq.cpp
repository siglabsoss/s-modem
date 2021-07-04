#include "HiggsSoapyDriver.hpp"
#include "HiggsDriverSharedInclude.hpp"
#include "feedback_bus.h"
#include "common/convert.hpp"
#include "driver/EventDsp.hpp"

#include <ctime>
#include <chrono>

using namespace std;

// void HiggsDriver::isZmq() {
//     cout << "isZmq() yes" << endl;
// }


void HiggsDriver::patchDiscoveryTableForNat(void) {
    const std::string role = GET_RADIO_ROLE();


    for(uint32_t i = 0; i < discovery_nodes.size(); i++) {
        auto &node = discovery_nodes[i];

        // cout << node.id << "\n";
        // cout << node.ip << "\n";
        // cout << node.name << "\n";
        // cout << "\n";

        if( role == "arb" ) {
            if(node.id == 1) {
                node.ip = "192.168.2.45";
            }
            if(node.id == 2) {
                node.ip = "192.168.2.52";
            }
        }

        if( role == "tx" ) {
            if(node.id == 1) {
                node.ip = "192.168.2.45";
            }
            if(node.id == 2) {
                node.ip = "192.168.2.52";
            }
            if(node.id == 18) {
                node.ip = "192.168.2.48";
            }
        }

    }
}


///
///  When Soapy fires up the stream, one of the things we do is start the zmq thread
///  the zmq thread does very little computation.  it:
///    1. connects to discovery server and sends receives messages (every 5 seconds)
///    2. connects to every other radio in the simulation
///    3. sends / receives messages sent by any peer
///


void HiggsDriver::zmqUpdateConnections(void) {
    size_t our_node_hash = this_node.hash();

    if( GET_PATCH_DISCOVERY_FOR_NAT() ) {
        patchDiscoveryTableForNat();
    }

    // cout << "us:" << this_node.stringify() << endl;
    // cout << "table has " << discovery_nodes.size() << " rows" << endl;
    for(uint32_t i = 0; i < discovery_nodes.size(); i++) {
        size_t hash = discovery_nodes[i].hash();

        // cout << "node: " << i << endl;
        // cout << "them:" << discovery_nodes[i].stringify();

        bool already_connected = false;
        bool node_was_us = false;
        for(uint32_t j = 0; j < active_zmq.size(); j++ ) {
            if(active_zmq[j] == hash) {
                already_connected = true;
                // cout << "already_connected" << endl;
                break;
            }
            
        }

        if(hash == our_node_hash) {
            node_was_us = true;
        }

        if(!already_connected && !node_was_us) {
            string conn = discovery_nodes[i].connect_str(0);
            const auto _id = discovery_nodes[i].id;
            const auto _name = discovery_nodes[i].name;

            const auto _port1 = discovery_nodes[i].port1;

            if( _port1 != 0 ) {
                // peer's config files are setup such that the peer is requesting that
                // we connect to it.  however the firewall is actually on our side
                // so technically it should be OUR config files that specify this
                // this backwards logic is ok for now
                cout << "peer " << _id << " requesting that we connect to it " << "\n";
                cout << discovery_nodes[i].connect_str(1) << "\n";

                zmq_sock_sub.connect(discovery_nodes[i].connect_str(1));

                cout << "will connect to " << conn << "  (peer id: " << _id  << ") (" << _name << ")" << endl;
                zmq_sock_pub.connect(discovery_nodes[i].connect_str(0));
            } else {
                const bool we_are_inbound = GET_ZMQ_PUBLISH_INBOUND();
                if( we_are_inbound ) {
                    cout << "heard from "<< conn << "  (peer id: " << _id  << ") (" << _name << ")" << endl;
                } else {
                    cout << "will connect to " << conn << "  (peer id: " << _id  << ") (" << _name << ")" << endl;
                    zmq_sock_pub.connect(discovery_nodes[i].connect_str(0));
                }
            }

            active_zmq.push_back(hash);
        }

    }
}

/// 
/// Accept or consume a message over zmq of type feedback bus
/// only call this if you have already checked the mask bits and it's for us
///
void HiggsDriver::zmqAcceptRemoteFeedbackBus(const std::string& tag, const unsigned char* const data, const size_t length) {
    const auto header = reinterpret_cast<const feedback_frame_vector_t* const>(data);

    if( header->destination1 & FEEDBACK_DST_PC ) {
                // cout << "tag here 2" << endl;
                // cout << "FIXME: Don't know how to consume a pc->pc message yet" << endl;
                zmqSendFeedbackBusForPcBevOffThread(data, length);
            } else {

                cout << "sending to our higgs with d0 " 
                << HEX32_STRING(header->destination0) << " type " 
                << header->type << ", " 
                << ((const feedback_frame_vector_t*)header)->vtype << " size " << length << endl;

                const auto vheader = reinterpret_cast<const feedback_frame_vector_t* const>(header);
                (void)vheader;

                if( ((const feedback_frame_vector_t*)header)->vtype == FEEDBACK_VEC_INVOKE_USER_DATA ) {
                    cout << "zmqAcceptRemoteFeedbackBus() does not support this" << endl;

                }  // user data hack
                else {

                    if(accept_remote_feedback_bus_higgs) {
                        sendToHiggsLowPriority(data, length);
                    } else {
                        cout << "zmqAcceptRemoteFeedbackBus() DROPPING due to accept_remote_feedback_bus flag" << endl;
                    }
                } // send to higgs


            } // was for pc
}

///
///  called for all zmq tagged messages
///  the idea was that multiple tags could lead here
///  ZMQ_TAG_ALL is the main feeder of this. See:
///  - HiggsDriver::zmqHandleMessage
///  - HiggsDriver::zmqAcceptRemoteFeedbackBus
///  - HiggsDriver::zmqSendFeedbackBusForPcBevOffThread
///  - EventDsp::handleParsedFrameForPc
///  - EventDsp::handlePcPcVectorType
void HiggsDriver::zmqHandleRemoteFeedbackBus(const std::string& tag, const unsigned char* const data, const size_t length) {

        ///
        /// this reinterpret_cast<> is the same as doing
        ///
        ///    feedback_frame_vector_t* header = (feedback_frame_vector_t*)data;
        ///
        const auto header = reinterpret_cast<const feedback_frame_vector_t* const>(data);


        bool should_print_message = true;
        if(header->type == FEEDBACK_TYPE_VECTOR &&
                ((const feedback_frame_vector_t*)header)->vtype == FEEDBACK_VEC_TRANSPORT_RB ) {
                should_print_message = false;
        }

        // if false, force to false, it true, leave alone
        if( !zmq_should_print_received ) {
            should_print_message = false;
        }
            
        if( should_print_message ) {
            cout << "got message of type " 
            << header->type << ", " 
            << ((const feedback_frame_vector_t*)header)->vtype 
            << " length " << length << endl;
        }

        // for(uint32_t i = 0; i < (length/4); i++) {
        //     cout << HEX32_STRING( (int) ((uint32_t*)data)[i] ) << " ";
        // }

        // cout << endl;

        uint32_t our_mask = feedback_peer_to_mask(this_node.id);

        // cout << "our_mask " << our_mask << end;

        // this is a feedback bus message


        if( header->destination0 & our_mask ) {

            if( should_print_message) {
                cout << "Accepting message type " << header->type << ", " << ((const feedback_frame_vector_t*)header)->vtype << "\n";
            }

                // the message matched our mask
                // lets accept / consume it
                zmqAcceptRemoteFeedbackBus(tag, data, length);
            // can we remove peer 8??

            

        } else {
            //FIXME: we will refine our zmq and not broadcast messages to all nodes using pub/sub filters

            if( should_print_message) {
                cout << "Ignoring message type " << header->type << ", " << ((const feedback_frame_vector_t*)header)->vtype << " which is not for us\n";
            }
        }

}


// void HiggsDriver::zmqDispatchTagRing();

// void HiggsDriver::zmqDispatchTagZmqAll();


///
/// handle a message that we got via the sub socket (runs on zmq thread)
/// tag : this is the first zmq "packet", we use this to filter what it is
/// data : pointer to byte data in second "packet", this is the payload,
/// length : how many bytes of information are pointed to by data
///
void HiggsDriver::zmqHandleMessage(const std::string tag, const unsigned char* const data, const size_t length) {
// sendToHiggs()

    ///
    ///  tag matches a broadcast
    ///
    if( tag[0] == ZMQ_TAG_RING[0] ) {
        const raw_ringbus_t* const raw_ringbus = (const raw_ringbus_t* const)data;

        // send first for latency
        rb->send(raw_ringbus->ttl, raw_ringbus->data);


        bool do_print = false;

        if( (raw_ringbus->data & TYPE_MASK) == TX_CFO_LOWER_CMD && raw_ringbus->ttl == 2) {
            do_print = false;
        }

        if( (raw_ringbus->data & TYPE_MASK) == TX_CFO_UPPER_CMD && raw_ringbus->ttl == 2) {
            do_print = false;
        }

        if(do_print) {
            cout << "got ringbus zmq message ttl " << raw_ringbus->ttl << " data " << HEX32_STRING(raw_ringbus->data) << endl;
        }
        
        return;
    }

    // FIXME, OR == our own channel
    if( tag == ZMQ_TAG_ALL || tag == zmqGetPeerTag(this_node.id).c_str()) {
        ///
        /// the idea that if we have our own channels the would all still call this function
        /// zmq would decide which tags to filter to us, and then this fn can filter down more
        zmqHandleRemoteFeedbackBus(tag, data, length);
    } // was for us

}


/// we are rx side.
/// we got a zmq feedback message from a tx side which holds the peer id and the actual
/// ringbus messages
/// Messages put here are picked up in EventDsp::handleParsedFrameForPc
void HiggsDriver::zmqSendFeedbackBusForPcBevOffThread(const unsigned char* const data, const size_t length) {
    // length in bytes

    // cout << "zmqSendFeedbackBusForPcBevOffThread() len chars " << length << endl;

    uint32_t byte_count = (uint32_t)length;
    // write 4 byte length first
    // oh lord if we miss this
    // other end of bev reads first 4 bytes before doing anything
    bufferevent_write(dspEvents->fb_pc_bev->in, &byte_count, sizeof(uint32_t));
    bufferevent_write(dspEvents->fb_pc_bev->in, data, length);
}


// void HiggsDriver::zmqDebugPub(void) {
//     using clock = std::chrono::steady_clock;
//     static clock::time_point print_period = clock::now();
//     // static clock::time_point update_period = clock::now();
//     static clock::time_point now;

//     constexpr uint32_t ms_print_period = 2*1000;
//     // constexpr uint32_t ms_broadcast_period = 2*1000;

//     if( active_zmq.size() != 0 ) {

//         now = clock::now();
//         std::chrono::duration<double> print_elapsed_time = now-print_period;
//         if( print_elapsed_time > std::chrono::milliseconds(ms_print_period) )
//         {
//             // cout << "////////////////////////////////////////////////" << endl;

//             zmqPublish("I", "hello from " + this_node.name);

//             print_period = now;
//         }
//     }
// }

std::string HiggsDriver::zmqRbPeerTag(const size_t peer) {
    const char c = ZMQ_TAG_PEER_START + peer;
    std::string result(ZMQ_TAG_RING);
    result += c;
    return result;
}

void HiggsDriver::zmqSubcribes(void) {
    
    /// we need to subscribe to a character
    /// we calculate this by starting with 'A' and then use addition to get to another character
    const char node_char = ZMQ_TAG_PEER_START + this_node.id;


    //////////////
    //
    // ringbus that gets delivered over rb port (not a fb bus message)
    //
    // zmq_sock_sub.setsockopt( ZMQ_SUBSCRIBE, ZMQ_TAG_RING, strlen(ZMQ_TAG_RING));

    std::string rb_tag = ZMQ_TAG_RING;
    rb_tag += node_char;

    std::string peer_tag = ZMQ_TAG_ALL;
    peer_tag += this_node.id;

    //std::cout << "###############peerx tag: " << peer_tag << std::endl;
    //std::cout << "###############node id: " << this_node.id << std::endl;

    zmq_sock_sub.setsockopt( ZMQ_SUBSCRIBE, rb_tag.c_str(), rb_tag.length());

    zmq_sock_sub.setsockopt( ZMQ_SUBSCRIBE, zmqGetPeerTag(this_node.id).c_str(), peer_tag.length());
    //////////////
    //
    //  Feedback Bus
    //
    //   every radio subscribes to group 0
    zmq_sock_sub.setsockopt( ZMQ_SUBSCRIBE, ZMQ_TAG_ALL, strlen(ZMQ_TAG_ALL));

    // subscribe to individual channel
    zmq_sock_sub.setsockopt( ZMQ_SUBSCRIBE, &node_char, 1);
}

///
/// This thread is run as a member of the main HiggsDriver object
/// this thread mostly receives zmq messages.  Messages are sent off-thread
/// via buffer events which must be marked as thread safe
///
void HiggsDriver::zmqRxThread(void)
{
    using clock = std::chrono::steady_clock;

    discovery_server_down_message = zmq_thread_start = clock::now();

    cout << "zmqRxThread()" << endl;

    uint32_t zmq_listen_port = 0;
    zmq_listen_port = GET_ZMQ_LISTEN_PORT(); // member on class, could be removed

    const std::string bind_address = "tcp://*:" + to_string(zmq_listen_port);

  //  Initialize poll set
   
    // rb->send(3, raw_ringbus->data);

    static clock::time_point drop_period = clock::now();
    static clock::time_point now;

    constexpr uint32_t ms_drop_period = 200;

    const bool pub_inbound = GET_ZMQ_PUBLISH_INBOUND();

    uint32_t pub_bind_port = 0;
    if( pub_inbound ) {
        pub_bind_port = GET_ZMQ_PUBLISH_PORT();
    }

    zmqSetupDiscovery(zmq_listen_port, pub_bind_port);


    ///
    ///  This specifies how much rx/tx sockets will hold
    ///  this code compiles, but is untested.  we can just use defaults
    ///  we can change this later
    ///
    /// int high_water_mark = 4096;
    /// set outbound watermark for pub, and inbound for sub
    /// zmq_setsockopt(zmq_sock_pub, ZMQ_SNDHWM, &high_water_mark, sizeof(high_water_mark) );
    /// zmq_setsockopt(zmq_sock_sub, ZMQ_RCVHWM, &high_water_mark, sizeof(high_water_mark) );
    ///
    /// https://stackoverflow.com/questions/36512643/how-to-understand-the-zmq-rcvhwm-option-of-zeromq-correctly


    if(zmq_should_listen) {
        cout << "ZMQ Binding: " << bind_address << endl;
        zmq_sock_sub.bind(bind_address.c_str());
        zmqSubcribes();
    

        //         int val;
        // val = 0; // set linger to 0
        // this->socket->setsockopt(ZMQ_LINGER, &val, sizeof(val));
    }

    if( pub_inbound ) {
        const std::string pub_bind_address = "tcp://*:" + to_string(pub_bind_port);
        zmq_sock_pub.bind(pub_bind_address.c_str());
    }


     zmq::pollitem_t items [1] = {
        { zmq_sock_sub, 0, ZMQ_POLLIN, 0 }
    };

    // string connect_ip = "127.0.0.1";
    // string connect_port = "10005";
    // string connect_address = "tcp://" + connect_ip + ":" + connect_port;

    // if(zmq_should_connect) {
    //     cout << "ZMQ Connecting: " << connect_address << endl;
    //     zmq_sock_pub.connect(connect_address.c_str());
    //     // pusher = new zmq::socket_t(context, ZMQ_PUSH);
    // }

    zmq_should_print_received = GET_PRINT_RECEIVED_ZMQ();

    // zmqSetupDiscovery();
    bool new_peers = false;

    // copy data to this std atomic so we can read from other threads
    // lazy way of doing a lock
    connected_node_count = 0;

    // this is the priority on this thread, so we can spend
    // a long time waiting for a packet here
    // zmqPetDiscovery() also runs on this thread and is low priority
    uint32_t zmq_poll_interval = 100;//300; // in ms // FIXME what about 10

    zmq::message_t part0,part1;
    int r0,r1;
    cout << "above loop" << endl;

    _zmq_thread_ready = true;

    while(higgsRunZmqThread) {
        if( dspThreadReady() ) {
            break;
        }
        usleep(1000);
        cout << "zmq wait for dsp\n";    
    }

    while(higgsRunZmqThread) {
        // cout << "zmq loop " << discovery_server_message_count << "\n";

        if(discovery_server_message_count == 0) {
            now = clock::now();
            size_t elapsed_since_message = std::chrono::duration_cast<std::chrono::microseconds>( 
                now - discovery_server_down_message
            ).count();

            size_t elapsed_since_start = std::chrono::duration_cast<std::chrono::microseconds>( 
                now - zmq_thread_start
            ).count();

            // print if it's been more than 5.5 seconds since start and more than 1 second since last print
            if( (elapsed_since_start > 5.5E6) && (elapsed_since_message > 1E6) ) {
                cout << "=========================================================\n";
                cout << "=\n";
                cout << "=  Seems like " + GET_DISCOVERY_IP() + " is down\n";
                cout << "=\n";
                cout << "=========================================================\n\n";
                discovery_server_down_message = now;
            }
        }

        // before and after this function call discovery_nodes()
        // may change
        new_peers = zmqPetDiscovery();

        if(new_peers) {
            zmqUpdateConnections();
            connected_node_count = discovery_nodes.size() - 1; // copy to atomic, assume we are in the table
        }

        // zmqDebugPub();

        // debugFeedbackBus();

        // storage for internal messages
        std::string internal_tag;
        std::vector<char> internal_chars;
        bool have_internal = false;

        if(zmq_should_listen) {
            zmq::poll(&items[0], 1, zmq_poll_interval);
            
            if (items[0].revents & ZMQ_POLLIN) {
                // cout << "got message ";
                r0 = zmq_sock_sub.recv(&part0);
                r1 = zmq_sock_sub.recv(&part1, ZMQ_RCVMORE);

                if( r1 != 1 ) {
                    cout << "ERROR received a single part ZMQ message, dropping" << endl;
                }
                if( r0 != 1 ) {
                    cout << "ERROR ZMQ poller is weird" << endl;
                }



                now = clock::now();
                std::chrono::duration<double> drop_elapsed = now-drop_period;
                if( drop_elapsed <= std::chrono::milliseconds(ms_drop_period) )
                {
                    static int zmq_print_count = 0;
                    // don't even check for r1,r0, just drop it no matter what
                    // do nothing
                    if( zmq_print_count < 5 ) {
                        cout << "zmqRxThread() has just booted, dropping packet" << endl;
                    }
                    zmq_print_count++;
                } else {
                    if(r1 == 1 && r0 == 1) {
                        // cout << "r " << r0 << endl;
                        std::string p0_string(static_cast<char*>(part0.data()), part0.size());
                        auto p1_char = static_cast<unsigned char*>(part1.data());

                        // cout << "got ZMQ tag: " << p0_string << endl;
                        // cout << "got ZMQ    : " << p1_char << endl;
                        zmqHandleMessage(p0_string, p1_char, part1.size());
                    } else {
                        cout << "Unknown values for r0,r1: " << r0 << "," << r1 << endl;
                    }
                }
            } // if events

            { // lock context
                std::lock_guard<std::mutex> lock(internal_zmq_mutex);
                if( internal_zmq.size() ) {
                    // if there is an element, we copy it to the variables outside this scope
                    // then we set this flag, and erase element [0]
                    std::tie(internal_tag,internal_chars) = internal_zmq[0];
                    internal_zmq.erase(internal_zmq.begin());
                    have_internal = true;
                } else {
                    have_internal = false;
                }
            }

            if( have_internal ) {
                zmqHandleMessage(internal_tag, (const unsigned char*)internal_chars.data(), internal_chars.size());
            }

        } // if should_listen
    } // while

    cout << "zmqRxThread() closing" << endl;
}

///
///
///

std::string HiggsDriver::zmqGetPeerTag(const size_t peer) {
    std::string tag = "";
    tag += ZMQ_TAG_PEER_START + peer;
    return tag;
}

void HiggsDriver::zmqPublishOrInternal(const size_t peer, const std::vector<uint32_t>& vec) {
    if( peer == this_node.id) {
        zmqPublishInternal(ZMQ_TAG_ALL, vec);
    } else {
        
        zmqPublish(zmqGetPeerTag(peer).c_str(), vec);
    }
}

// void HiggsDriver::zmqPublishInternal(const std::string& header, const char* const message, const size_t length) {
//     std::lock_guard<std::mutex> lock(internal_zmq_mutex);
// }
void HiggsDriver::zmqPublishInternal(const std::string& header, const std::vector<uint32_t>& vec) {
    const char* data = (const char* const) vec.data();
    const size_t length = vec.size()*4;

    std::vector<char> aschar;
    for(size_t i = 0; i < length; i++) {
        aschar.push_back(data[i]);
    }

    // cout << "zmqPublishInternal for " << header << " :\n";

    // for(const auto w : vec) {
    //     cout << HEX32_STRING(w) << "\n";
    // }

    std::lock_guard<std::mutex> lock(internal_zmq_mutex);

    internal_zmq.push_back((internal_zmq_t){header, aschar});
}

/// thread safe
/// this version accepts a char* allowing us to send null data
/// Do not call this, instead call zmqPublishOrSelf() which supports
/// publishing to an internal queue so we can pick up our own zmq messages without
/// touching the socket
void HiggsDriver::zmqPublish(const std::string& header, const char* const message, const size_t length) {
    // cout << "eating zmqPublish()" << endl;
    // return;
    zmq::message_t z_header(header.size());
    memcpy(z_header.data(), header.c_str(), header.size());

    zmq::message_t z_foobar(length);
    memcpy(z_foobar.data(), message, length);
    
    bool ret;

    {
        std::lock_guard<std::mutex> lock(zmq_pub_mutex);
        ret = zmq_sock_pub.send(z_header, ZMQ_SNDMORE);
        // cout << "ret: " << ret << endl;
        ret = zmq_sock_pub.send(z_foobar);
        (void)ret;
    }
}

/// thread safe BY PROXY that it calles another thread save function
/// no lock is taken at this level, that is handled by sub function
void HiggsDriver::zmqPublish(const std::string& header, const std::vector<uint32_t>& vec) {
    // cout << "eating zmqPublish()" << endl;
    // return;
    const char* data = (const char* const) vec.data();
    const size_t length = vec.size()*4;

    zmqPublish(header, data, length);
}

// message
///
void HiggsDriver::zmqRingbusPeer(const size_t peer, const char* const message, const size_t length) {
    const std::string tag = zmqRbPeerTag(peer);

    // raw_ringbus_t* payload = (raw_ringbus_t*)message;
    // cout << "Sending Peer ttl " << payload->ttl << ", data " << HEX32_STRING(payload->data) << "\n";

    if( peer == this_node.id ) {
        // I believe we already catch ringbus for our own peer outside of this function so warn if we see this
        // that's why we do not call zmqPublishOrInternal()
        cout << "zmqRingbusPeer() will not deliver to own ringbus\n";
    }

    zmqPublish(tag, message, length);
}

// weird to put on zmq thread, however many messages bound for higgs
// will eventually come from here
// void HiggsDriver::debugFeedbackBus() {
//     using clock = std::chrono::steady_clock;
//     static clock::time_point print_period = clock::now();
//     clock::time_point now;
//     uint32_t ms_print_period = 100;


//     static uint32_t num_sent = 0;


//     now = clock::now();
//     std::chrono::duration<double> print_elapsed_time = now-print_period;
//     if( print_elapsed_time > std::chrono::milliseconds(ms_print_period) )
//     {
//         cout << "Sent" << endl;
//         // print_table(discovery_nodes);
//         print_period = now;
//         // cs00TurnstileAdvance(10);
//         uint32_t type_ringbus_state = 42;

//         std::vector<uint32_t> p = feedback_ringbus_packet(
//                   RING_ENUM_CS10,
//                   EDGE_EDGE_IN | type_ringbus_state,
//                   RING_ENUM_CS10,
//                   EDGE_EDGE_IN | (type_ringbus_state+1),
//                   RING_ENUM_CS10,
//                   EDGE_EDGE_IN | (type_ringbus_state+2) );

//         constexpr bool force_split = false;
//         if(force_split) {
//             std::size_t const split_point = rand() % 14;
//             std::vector<uint32_t> split_lo(p.begin(), p.begin() + split_point);
//             std::vector<uint32_t> split_hi(p.begin() + split_point, p.end());

//             sendToHiggs(split_lo);
//             sendToHiggs(split_hi);
//         } else {
//             sendToHiggs(p);
//         }

//         num_sent++;

//         if(num_sent == 100) {
//             usleep(1000*1000);
//             // this should dump all error counters
//             rb->send(RING_ADDR_CS20, EDGE_EDGE_OUT | 0 );
//         }
//     }
// }

bool HiggsDriver::zmqThreadReady() const {
    return _zmq_thread_ready;
}