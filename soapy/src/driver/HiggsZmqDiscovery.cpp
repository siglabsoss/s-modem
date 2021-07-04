#include "HiggsSoapyDriver.hpp"
#include "HiggsDriverSharedInclude.hpp"
#include <ctime>
#include <chrono>
#include <fstream> 

using namespace std;

// static functions, not in the class
bool discoveryUpdateTable(std::vector<discovery_t> &nodes, string msg);
void discoveryPrintTable(std::vector<discovery_t> &nodes);
void discoveryNotifyServer(zmq::socket_t &zsock, discovery_t us);




// runs once on main thread
void HiggsDriver::discoveryCheckIdentity() {

    // load to class from init.json.  this replaces node.csv
    node_on_disk = discovery_t(GET_DISCOVERY_PC_IP(), 0, 0, GET_OUR_ID(), GET_ZMQ_NAME());

    // cout << "body:" << endl;
    // cout << node_on_disk.stringify() << endl;
}




void HiggsDriver::zmqSetupDiscovery(const uint32_t p0, const uint32_t p1) {
    const std::string discovery_ip = GET_DISCOVERY_IP();
    const string conn0 = "tcp://" + discovery_ip + ":" + to_string(GET_DISCOVERY_PORT0());
    const string conn1 = "tcp://" + discovery_ip + ":" + to_string(GET_DISCOVERY_PORT1());

    cout << endl << "Using DISCOVERY server: " << endl << conn0 << endl << endl;

    zmq_sock_discovery_sub.setsockopt( ZMQ_SUBSCRIBE, "", 0);

    zmq_sock_discovery_push.connect(conn0);
    zmq_sock_discovery_sub.connect(conn1);

    this_node.ip = node_on_disk.ip;
    this_node.name = node_on_disk.name;
    this_node.id = node_on_disk.id;

    this_node.port0 = p0;
    this_node.port1 = p1;

    cout << "zmq pt " << p0 << endl;
    cout << "This Node:" << endl;
    cout << this_node.stringify() << endl;

    _this_node_ready = true;
    
    discoveryNotifyServer(zmq_sock_discovery_push, this_node);
}



bool HiggsDriver::thisNodeReady() const {
    return _this_node_ready;
}





/**
 * Handles all discovery tasks.  call this from the zmq thread
 * \returns true if discovery_nodes was updated
 */
bool HiggsDriver::zmqPetDiscovery() {

    bool table_changed = false;

    static zmq::pollitem_t items [] = {
        { zmq_sock_discovery_sub, 0, ZMQ_POLLIN, 0 }
    };

    // this pet discovery is very low priority
    // since there are other things on this thread, we don't spend
    // any time waiting for a packet
    static uint32_t zmq_poll_period = 0;

    int r0;

    using clock = std::chrono::steady_clock;
    static clock::time_point print_period = clock::now();
    static clock::time_point update_period = clock::now();
    static clock::time_point now;

    constexpr uint32_t ms_print_period = 2*1000;
    constexpr uint32_t ms_broadcast_period = 2*1000;

    zmq::message_t part0;
    zmq::poll(&items[0], 1, zmq_poll_period);
            
    if (items[0].revents & ZMQ_POLLIN) {
        // cout << "sock 0 has something" << endl;
        discovery_server_message_count++;

        r0 = zmq_sock_discovery_sub.recv(&part0);
        (void)r0;
        // cout << "r " << r0 << endl;
        const std::string sock0_string(static_cast<char*>(part0.data()), part0.size());
        // cout << "sock0_string (" << sock0_string.size() << ") " << sock0_string << endl;

        table_changed = discoveryUpdateTable(discovery_nodes, sock0_string);

        // cout << "table_changed " << table_changed << endl;

    }

    now = clock::now();
    std::chrono::duration<double> print_elapsed_time = now-print_period;
    if( print_elapsed_time > std::chrono::milliseconds(ms_print_period) )
    {
        discoveryPrintTable(discovery_nodes);
        print_period = now;
    }

    std::chrono::duration<double> update_elapsed_time = now-update_period;
    if( update_elapsed_time > std::chrono::milliseconds(ms_broadcast_period) )
    {


        // discovery_t us("127.0.0.1", 10005, 10006, 4, "ben");
        // discovery_t our_meteor("192.168.1.63",  10008, 0, 21, "Meteor.js");
        // discoveryNotifyServer(zmq_sock_discovery_push, our_meteor);




        discoveryNotifyServer(zmq_sock_discovery_push, this_node);
        update_period = now;
    }

    return table_changed;
}













/**
 * This function acceps a string and a reference to the vector of nodes
 * 
 * If msg is an empty string, we update the referenced nodes
 * \returns true if nodes was updated
 */
bool discoveryUpdateTable(std::vector<discovery_t> &nodes, string msg) {
    static std::vector<discovery_t> cached;

    if(msg.size() != 0) {
        discovery_t proposed(msg);

        cached.push_back(proposed);
        return false;
    } else {
        nodes.erase(nodes.begin(), nodes.end());
        nodes = cached;
        cached.erase(cached.begin(), cached.end());
        return true;
    }
}

void discoveryPrintTable(std::vector<discovery_t> &nodes) {
    // cout << endl;
    // cout << "------------------------------------------" << endl;
    // for(uint32_t i = 0; i < nodes.size(); i++) {
    //     cout << nodes[i].stringify() << endl;
    // }
    // cout << endl;
}


void discoveryNotifyServer(zmq::socket_t &zsock, discovery_t us) {
    int ret;
    (void)ret;
    std::string init_msg = us.stringify();
    zmq::message_t z_head(init_msg.size());
    memcpy(z_head.data(), (init_msg.c_str()), (init_msg.size()));
    ret = zsock.send(z_head);
    // cout << "Updating server with our IP, ret: " << ret << endl;
}
