#include <iostream>
#include <stdint.h>
#include <time.h>
#include <string>
#include <zmq.hpp>
#include <unistd.h> //usleep
#include <ctime>
#include <chrono>

#include "common/utilities.hpp"
#include "common/discovery.hpp"

using namespace std;



void send_msg(zmq::socket_t &z, string header, string message) {
    // std::string header = "A";
    zmq::message_t z_header(header.size());
    memcpy(z_header.data(), (header.c_str()), (header.size()));

    // std::string message = "foo";
    zmq::message_t z_foobar(message.size());
    memcpy(z_foobar.data(), (message.c_str()), (message.size()));
    
    bool ret;

    ret = z.send(z_header, ZMQ_SNDMORE);
    cout << "ret: " << ret << endl;
    ret = z.send(z_foobar);
}

// void d() {
//     cout << "d()" << endl;

//     discovery_t o1;
//     o1.port0 = 100;
//     o1.port1 = 101;
//     o1.id = 23;
//     o1.ip = "192.14.5.1";
//     o1.name = "ben";

//     cout << o1.stringify() << endl;

//     discovery_t o2(o1.stringify());

//     cout << o2.stringify() << endl;

//     exit(0);
// }

void update_table(std::vector<discovery_t> &nodes, string msg) {
    static std::vector<discovery_t> cached;

    if(msg.size() != 0) {
        discovery_t proposed(msg);

        cached.push_back(proposed);
    } else {
        nodes.erase(nodes.begin(), nodes.end());
        nodes = cached;
        cached.erase(cached.begin(), cached.end());
    }
}

void print_table(std::vector<discovery_t> &nodes) {
    cout << endl;
    cout << "------------------------------------------" << endl;
    for(uint32_t i = 0; i < nodes.size(); i++) {
        cout << nodes[i].stringify() << endl;
    }
    cout << endl;
}


void update_server(zmq::socket_t &zsock, discovery_t us) {
    int ret;
    std::string init_msg = us.stringify();
    zmq::message_t z_head(init_msg.size());
    memcpy(z_head.data(), (init_msg.c_str()), (init_msg.size()));
    ret = zsock.send(z_head);
    cout << "Updating server with our IP, ret: " << ret << endl;
}


int main(void) {
    cout << "Test zmq" << endl;
    std::string discovery_ip = "net4.siglabs";
    uint32_t discovery_port0 = 10006;
    uint32_t discovery_port1 = 10007;
    string conn0 = "tcp://" + discovery_ip + ":" + to_string(discovery_port0);
    string conn1 = "tcp://" + discovery_ip + ":" + to_string(discovery_port1);

    zmq::context_t       zmq_context(1);
    zmq::socket_t  zmq_sock_discovery_push(zmq_context, ZMQ_PUSH);
    zmq::socket_t  zmq_sock_discovery_sub(zmq_context, ZMQ_SUB);

    zmq_sock_discovery_sub.setsockopt( ZMQ_SUBSCRIBE, "", 0);

    zmq_sock_discovery_push.connect(conn0);
    zmq_sock_discovery_sub.connect(conn1);


    // zmq_sock_discovery_sub.connect(connect_address.c_str());
    cout << "post bind " << endl;

     zmq::pollitem_t items [] = {
        { zmq_sock_discovery_sub, 0, ZMQ_POLLIN, 0 }
    };




    // discovery_t us("127.0.0.1", 10005, 10006, 4, "ben");
    discovery_t us("127.0.0.1",  10005, 10006, 5, "janson");


    update_server(zmq_sock_discovery_push, us);


    uint32_t zmq_poll_period = 50;

    int r0;


    std::vector<discovery_t> discovery_nodes;

    using clock = std::chrono::steady_clock;
    clock::time_point print_period = clock::now();
    clock::time_point update_period = clock::now();
    clock::time_point now;

    uint32_t ms_print_period = 2*1000;
    uint32_t ms_broadcast_period = 2*1000;

    zmq::message_t part0;
    while(1) {
        zmq::poll(&items[0], 1, zmq_poll_period);
            
        if (items[0].revents & ZMQ_POLLIN) {
            // cout << "sock 0 has something" << endl;

            r0 = zmq_sock_discovery_sub.recv(&part0);
            (void)r0;
            // cout << "r " << r0 << endl;
            std::string sock0_string(static_cast<char*>(part0.data()), part0.size());
            // cout << "sock0_string (" << sock0_string.size() << ") " << sock0_string << endl;

            update_table(discovery_nodes, sock0_string);

        }

        now = clock::now();
        std::chrono::duration<double> print_elapsed_time = now-print_period;
        if( print_elapsed_time > std::chrono::milliseconds(ms_print_period) )
        {
            print_table(discovery_nodes);
            print_period = now;
            // cs00TurnstileAdvance(10);
        }

        std::chrono::duration<double> update_elapsed_time = now-update_period;
        if( update_elapsed_time > std::chrono::milliseconds(ms_broadcast_period) )
        {
        //     prune_table(discovery_nodes);
        //     broadcast_table(zmq_sock_pub, discovery_nodes);

            update_server(zmq_sock_discovery_push, us);
            update_period = now;
        }

        // if (items[1].revents & ZMQ_POLLIN) {
        //     cout << "sock 1 has something" << endl;
        // }

    }

    
}
