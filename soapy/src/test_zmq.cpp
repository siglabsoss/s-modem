#include <iostream>
#include <stdint.h>
#include <time.h>
#include <string>
#include <zmq.hpp>
#include <unistd.h> //usleep


#include "common/utilities.hpp"

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

int main(void) {
    cout << "Test zmq" << endl;

    zmq::context_t       zmq_context(1);
    zmq::socket_t  zmq_sock_pub(zmq_context, ZMQ_PUB);



    string connect_ip = "127.0.0.1";
    string connect_port = "10005";
    string connect_address = "tcp://" + connect_ip + ":" + connect_port;

    cout << "connecting to " << connect_address << endl;

    zmq_sock_pub.connect(connect_address.c_str());
    cout << "post con " << endl;

    // int val = 0;
    // zmq_sock_pub->setsockopt(ZMQ_LINGER, &val, sizeof(val));

    usleep(100E3);

    send_msg(zmq_sock_pub, "AB", "message A");
    // send_msg(zmq_sock_pub, "B", "Message B");

    // std::string header = "A";
    // zmq::message_t z_header(header.size());
    // memcpy(z_header.data(), (header.c_str()), (header.size()));

    // std::string message = "Afoo";
    // zmq::message_t z_foobar(message.size());
    // memcpy(z_foobar.data(), (message.c_str()), (message.size()));
    
    // bool ret;

    // ret = zmq_sock_pub->send(z_header, ZMQ_SNDMORE);
    // cout << "ret: " << ret;
    // ret = zmq_sock_pub->send(z_foobar);
    // cout << "ret: " << ret;

    // ret = zmq_sock_pub->send(z_header, ZMQ_SNDMORE);
    // cout << "ret: " << ret;
    // ret = zmq_sock_pub.send(z_foobar);
    // cout << "ret: " << ret << endl;




    cout << "sent" << endl;
}

int main2(void) {
    cout << "Test zmq" << endl;

      //  Prepare our context and socket
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);

    std::cout << "Connecting to hello world server…" << std::endl;
    socket.connect ("tcp://localhost:5555");

    //  Do 10 requests, waiting each time for a response
    for (int request_nbr = 0; request_nbr != 10; request_nbr++) {
        zmq::message_t request (5);
        memcpy (request.data (), "Hello", 5);
        std::cout << "Sending Hello " << request_nbr << "…" << std::endl;
        socket.send (request);

        //  Get the reply.
        zmq::message_t reply;
        socket.recv (&reply);
        std::cout << "Received World " << request_nbr << std::endl;
    }
    return 0;
}