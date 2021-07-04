
#include "HiggsSoapyDriver.hpp"

#include "HiggsDriverSharedInclude.hpp"

#include <iostream>

// #include <boost/program_options/options_description.hpp>
// #include <boost/program_options/parsers.hpp>
// #include <boost/program_options/variables_map.hpp>


using namespace std;

#include <boost/asio/io_service.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;

#include "BoostTcpClient.hpp"


// class tcp_connection
//   : public boost::enable_shared_from_this<tcp_connection>
// {
// public:
//   typedef boost::shared_ptr<tcp_connection> pointer;

//   static pointer create(boost::asio::io_service& io_service)
//   {
//     return pointer(new tcp_connection(io_service));
//   }

//   tcp::socket& socket()
//   {
//     return socket_;
//   }

// why is this a local SHARED GLOBAL?
static BoostTcpClient* client;


void HiggsDriver::updateDashboard(const std::string& msg) {
    // cout << "write " << msg << "\n";
    std::unique_lock<std::mutex> lock(_dash_sock_mutex);
    client->write(msg);
}

using boost::asio::ip::tcp;



void HiggsDriver::boostIOThread(void)
{
    cout << "entering boostIOThread()" << endl;
    // while(higgsRunBoostIO) {
    //     cout << "foo" << endl;
    // }

    // moved from higgs
    boost::asio::io_service boost_io_service;


    // boost::asio::io_service io;

   
    // auto url = "foo";

    // boost::asio::io_service io_service;

    // tcp::resolver resolver(io_service);
    // tcp::resolver::query query(udp::v4(), argv[1], "daytime");
    // udp::endpoint receiver_endpoint = *resolver.resolve(query);

    // udp::socket socket(io_service);
    // socket.open(udp::v4());


    // tcp::resolver Resolver(boost_io_service);

    // string port = "10008";
    // tcp::resolver::query Query("192.168.1.63", port);

    // tcp::resolver::iterator EndPointIterator = Resolver.resolve(Query);

    // TCPClient Client(boost_io_service, EndPointIterator);

    // cout << "Client is started!" << endl;

    // auto url1 = "localhost";

    auto meteor_ip_address = GET_DASHBOARD_IP();
    auto meteor_port = GET_DASHBOARD_PORT();
    bool do_connect = GET_DASHBOARD_DO_CONNECT();

    bool do_connect_final;

    if( meteor_ip_address.length() > 0 && 
        meteor_port.length() > 0 &&
        do_connect == true ) {
            do_connect_final = true;
            cout << endl << "Connecting to DASHBOARD at " << meteor_ip_address << ":" << meteor_port << endl << endl;
        } else {
            do_connect_final = false;
            cout << "Will NOT connect to Dashboard TCP socket" << endl;
        }

    client = new BoostTcpClient(boost_io_service, meteor_ip_address, meteor_port, do_connect_final, this);


    // seems like this always crashes when we do not connect. why??


    cout << "io.run()" << endl;
    _boost_thread_ready = true;
    bool normal_exit = boost_io_service.run();

    if(normal_exit) {
        cout << "boostIOThread() had nothing to do, exiting " << endl;
    } else {
        cout << "boostIOThread() had a crash, exiting" << endl;
    }
}


// void HiggsDriver::rxAsyncThread(void)
// _boost_io_thread


// void boostIO(void);
//     std::atomic<bool> higgsRunBoostIO;
//     std::thread _boost_io_thread;

bool HiggsDriver::boostThreadReady() const {
    return _boost_thread_ready;
}