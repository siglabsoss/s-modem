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
using namespace std::literals::chrono_literals;


// how many ms to keep entries around for
uint32_t table_prune_ms = 6*1000;


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

// https://stackoverflow.com/questions/15777073/how-do-you-print-a-c11-time-point
void print_table(std::vector<discovery_t> &nodes) {
    cout << endl;
    cout << "------------------------------------------" << endl;

    // std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    for(uint32_t i = 0; i < nodes.size(); i++) {
        std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + (nodes[i].last_seen - std::chrono::steady_clock::now()));
        // std::chrono::duration<double> et = now-nodes[i].last_seen;
        cout << nodes[i].stringify() << " :: " << now_c << endl;

    }
    cout << endl;
}

// prune table
void prune_table(std::vector<discovery_t> &nodes) {
    // cout << endl;
    // cout << "------------------------------------------" << endl;

    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

    for(uint32_t i = 0; i < nodes.size(); i++) {
        // std::time_t now_c = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + (nodes[i].last_seen - std::chrono::steady_clock::now()));
        std::chrono::duration<double> old = now-nodes[i].last_seen;

        if( old > std::chrono::milliseconds(table_prune_ms) ) {
            nodes.erase(nodes.begin() + i);
            break;
        }

        // print_elapsed_time > std::chrono::milliseconds(ms_print_period)
        
        // cout << nodes[i].stringify() << " :: " << now_c << " (" << et << ")" << endl;


    }
    // cout << endl;
}

void table_insert(std::vector<discovery_t> &nodes, string msg) {
    discovery_t proposed(msg);

    bool found = false;
    for(uint32_t i = 0; i < nodes.size(); i++) {
        if( nodes[i].id == proposed.id ) {
            cout << "found at " << i << endl;
            nodes[i].last_seen = std::chrono::steady_clock::now();
            found = true;
            break;
        }
    }

    if(!found) {
        // std::chrono::steady_clock::time_point now = ;
        
        // proposed.last_seen = std::chrono::steady_clock::time_point(0);
        proposed.last_seen = std::chrono::steady_clock::now();

        
        nodes.push_back(proposed);
    }
}

// we send N table entries as individual zmq messages
// the final message is an empty string which signifies to clients, that server 
// is done sending the array
void broadcast_table(zmq::socket_t &zsock, std::vector<discovery_t> &nodes) {
    for(uint32_t i = 0; i < nodes.size(); i++) {
        string init_msg = nodes[i].stringify();

        // std::string init_msg = us.stringify();
        zmq::message_t z_head(init_msg.size());
        memcpy(z_head.data(), (init_msg.c_str()), (init_msg.size()));

        zsock.send(z_head);
    }



    // std::string init_msg = us.stringify();
    string null_msg = "";
    zmq::message_t z_null(null_msg.size());
    memcpy(z_null.data(), (null_msg.c_str()), (null_msg.size()));

    zsock.send(z_null);


}

int main(void) {
    cout << "Test zmq" << endl;
    uint32_t discovery_port0 = 10006;
    uint32_t discovery_port1 = 10007;
    string bind0 = "tcp://*:" + to_string(discovery_port0);
    string bind1 = "tcp://*:" + to_string(discovery_port1);

    zmq::context_t       zmq_context(1);
    zmq::socket_t  zmq_sock_pull(zmq_context, ZMQ_PULL);
    zmq::socket_t  zmq_sock_pub(zmq_context, ZMQ_PUB);
    

    zmq_sock_pull.bind(bind0);
    zmq_sock_pub.bind(bind1);


    // zmq_sock_pub.connect(connect_address.c_str());
    cout << "post bind " << endl;

     zmq::pollitem_t items [] = {
        { zmq_sock_pull, 0, ZMQ_POLLIN, 0 }
        // { zmq_sock_pub, 0, ZMQ_POLLIN, 0 }
    };

    int r0;

    std::vector<discovery_t> nodes;

    // FIXME: move this to jsonrpc
    using clock = std::chrono::steady_clock;
    clock::time_point print_period = clock::now();
    clock::time_point update_period = clock::now(); // how often do we send
    clock::time_point now;
    
    uint32_t ms_print_period = 3*1000;

    uint32_t ms_broadcast_period = 5*1000;

    // zmq::poll will wait for this period in ms
    // I believe the entire thread is sleeping during this
    // smaller numbers means higher cpu consumption, higher values
    // will block above timer values from evaluating exactly on time 
    uint32_t zmq_poll_period = 50;

    zmq::message_t part0;
    while(1) {
        zmq::poll(&items[0], 1, zmq_poll_period);
            
        now = clock::now();

        if (items[0].revents & ZMQ_POLLIN) {
            r0 = zmq_sock_pull.recv(&part0);
            cout << "sock 0 has something (" << part0.size() << ")" << endl;

            if( part0.size() != 0 ) {
                cout << "r " << r0 << endl;
                std::string sock0_string(static_cast<char*>(part0.data()), part0.size());
                cout << "sock0_string " << sock0_string << endl;

                table_insert(nodes, sock0_string);

                // these 2 lines set the next broadcast time 200ms in the future
                constexpr auto broadcast_backoff = 4800ms;
                update_period = now - broadcast_backoff;
            } else {
                cout << "sock 0 got message length zero" << endl;
            }
        }


        std::chrono::duration<double> print_elapsed_time = now-print_period;
        if( print_elapsed_time > std::chrono::milliseconds(ms_print_period) )
        {
            prune_table(nodes);
            print_table(nodes);
            print_period = now;
            // cs00TurnstileAdvance(10);
        }

        std::chrono::duration<double> update_elapsed_time = now-update_period;
        if( update_elapsed_time > std::chrono::milliseconds(ms_broadcast_period) )
        {
            prune_table(nodes);
            broadcast_table(zmq_sock_pub, nodes);
            update_period = now;

            // if( force_broadcast ) {
            //     force_broadcast = false;
            // }
        }


        // if (items[1].revents & ZMQ_POLLIN) {
        //     cout << "sock 1 has something" << endl;
        //     r0 = zmq_sock_pub.recv(&part0);

        //     cout << "r " << r0 << endl;
        //     std::string sock1_string(static_cast<char*>(part0.data()), part0.size());
        //     cout << "sock1_string " << sock1_string << endl;
        // }

    }


    // cout << "sent" << endl;
}
