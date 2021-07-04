#include "BoostTcpClient.hpp"
#include "driver/EventDsp.hpp"

using namespace std;


void BoostTcpClient::DoResolveConnect() {
  // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&request_);
    request_stream << '\0';
    request_stream << '\0';
    request_stream << "[{ \"u\" : 1000001},{} ]";
    request_stream << '\0';
    // request_stream << "GET " << path << " HTTP/1.0\r\n";
    // request_stream << "Host: " << server << "\r\n";
    // request_stream << "Accept: */*\r\n";
    // request_stream << "Connection: close\r\n\r\n";

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.
    // tcp::resolver::query query(server, "http");
    tcp::resolver::query query(server_, port_);
    // tcp::resolver::query query(server, "3000");

    if( should_connect_ ) {

        resolver_.async_resolve(query,
            boost::bind(&BoostTcpClient::handle_resolve, this,
              boost::asio::placeholders::error,
              boost::asio::placeholders::iterator));
    }
}

BoostTcpClient::BoostTcpClient(boost::asio::io_service& io_service,
      const std::string& server, const std::string& port, bool connect, HiggsDriver *s)
    : resolver_(io_service),
      socket_(io_service),
      server_(server),
      port_(port),
      should_connect_(connect),
      soapy(s)
  {
    
        DoResolveConnect();
    
  }

  void BoostTcpClient::write(const std::string& msg) {

        if( !should_connect_ || !can_write ) {
            // cout << "write bail" << endl;
            return;
        }

        std::ostream request_stream(&request_);
        // request_stream << "{ \"key2 \" : \"value2 \" }";
        request_stream << msg; //"{ \"key2 \" : \"value2 \" }";
        request_stream << '\0';

        boost::asio::async_write(socket_, request_,
          boost::bind(&BoostTcpClient::handle_write_request, this,
            boost::asio::placeholders::error));

  }


void BoostTcpClient::handle_resolve(const boost::system::error_code& err,
      tcp::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      boost::asio::async_connect(socket_, endpoint_iterator,
          boost::bind(&BoostTcpClient::handle_connect, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "BoostTcpClient Error: " << err.message() << "\n";
    }
  }

  void BoostTcpClient::handle_connect(const boost::system::error_code& err)
  {
    cout << "handle_connect()" << endl;
    if (!err)
    {
        can_write = true;
      // The connection was successful. Send the request.
      boost::asio::async_write(socket_, request_,
          boost::bind(&BoostTcpClient::handle_write_request, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "BoostTcpClient Error: " << err.message() << "\n";
    }
  }

  void BoostTcpClient::handle_write_request(const boost::system::error_code& err)
  {
    // cout << "handle_write_request()" << endl;
    if (!err)
    {
      // Read the response status line. The response_ streambuf will
      // automatically grow to accommodate the entire line. The growth may be
      // limited by passing a maximum size to the streambuf constructor.


      // boost::asio::async_read(socket_, response_,
      //     boost::bind(&BoostTcpClient::handle_read_status_line, this,
      //       boost::asio::placeholders::error));

        if(!did_fire) {
        boost::asio::async_read(socket_, 
        boost::asio::buffer(data, 1),
        boost::bind(&BoostTcpClient::handle_read_status_line, this,
        boost::asio::placeholders::error));
        did_fire = true;
        }
    }
    else
    {
      std::cout << "BoostTcpClient Error: " << err.message() << "\n";
    }
  }

  void BoostTcpClient::handle_read_status_line(const boost::system::error_code& err)
  {
    // cout << "handle_read_status_line()" << endl;
    if (!err)
    {
        // cout << data[0] << endl;

        if(data[0] != (char)0) {
            oss << data[0];
        } else {
            std::string str = oss.str();
            if( str.size() != 0 ) {
                // cout << "handle_read_status_line Got back (" << str.size() << ") " << str << endl;
                gotJsonPayload(str);
                oss.str(std::string()); // clear out the stringstream
            }
        }

        boost::asio::async_read(socket_, 
        boost::asio::buffer(data, 1),
        boost::bind(&BoostTcpClient::handle_read_status_line, this,
        boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "BoostTcpClient Error: " << err << "\n";

      // FIXME: halp
      std::cout << "Error with Metoer socket, maybe meteor server restarted?" << endl;
      std::cout << "For now you must restart s-modem" << endl;

      // DoResolveConnect();
    }
  }

    // void BoostTcpClient::handleDashboardRequest_1(const nlohmann::json &j) {
    //     if( !j["$set"].is_null() ) {
    //         auto unpack = j["$set"];
    //         if( !unpack["k"].is_null() && !unpack["d"].is_null() ) {
    //             if( unpack["d"] == "u") {
    //                 std::string ss = unpack["k"];
    //                 char c = ss.at(0);
    //                 switch(c) {
    //                     case 'q':
    //                         cout << "Advance 100" << endl;
    //                         soapy->cs00TurnstileAdvance(100);
    //                         break;
    //                     case 'w':
    //                         cout << "Advance 10" << endl;
    //                         soapy->cs00TurnstileAdvance(10);
    //                         break;
    //                     case 'e':
    //                         cout << "Advance 1" << endl;
    //                         soapy->cs00TurnstileAdvance(1);
    //                         break;

    //                     case 'a':
    //                         cout << "Backwards 100" << endl;
    //                         soapy->cs00TurnstileAdvance(1280-100);
    //                         break;
    //                     case 's':
    //                         cout << "Backwards 10" << endl;
    //                         soapy->cs00TurnstileAdvance(1280-10);
    //                         break;
    //                     case 'd':
    //                         cout << "Backwards 1" << endl;
    //                         soapy->cs00TurnstileAdvance(1280-1);
    //                         break;


    //                     // eq options
    //                     case 'y':
    //                         cout << "Send Channel EQ" << endl;
    //                         // client.sendChannelEq();
    //                         soapy->manualSendChannelEq();
    //                         break;
    //                     case 'h':
    //                         cout << "Reset Tx EQ" << endl;
    //                         // client.resetChannelEq();
    //                         soapy->manualResetChannelEq();
    //                         break;
    //                     case 'n':
    //                         cout << "Zero Tx EQ" << endl;
    //                         // client.zeroChannelEq();
    //                         soapy->manualZeroChannelEq();
    //                         break;


    //                     // these are RADIO INDEX and not radio id
    //                     case 'c':
    //                         cout << "Radio 0, data mode" << endl;
    //                         cout << "JsonServer::setScheduleMode( " << 0 << " " << 3 << ")" << endl;
    //                         soapy->dspEvents->setPartnerScheduleModeIndex(0, 3, 0);
    //                         // client.setScheduleMode(0, 3);
    //                         break;
    //                     case 'v':
    //                         cout << "Radio 0, pilot mode mode" << endl;
    //                         cout << "JsonServer::setScheduleMode( " << 0 << " " << 4 << ")" << endl;
    //                         soapy->dspEvents->setPartnerScheduleModeIndex(0, 4, 0);
    //                         // client.setScheduleMode(0, 4);
    //                         break;
    //                     case 'u':
    //                         cout << "Radio 1, data mode" << endl;
    //                         cout << "JsonServer::setScheduleMode( " << 1 << " " << 3 << ")" << endl;
    //                         soapy->dspEvents->setPartnerScheduleModeIndex(1, 3, 0);
    //                         // client.setScheduleMode(1, 3);
    //                         break;
    //                     case 'i':
    //                         cout << "Radio 1, pilot mode mode" << endl;
    //                         cout << "JsonServer::setScheduleMode( " << 1 << " " << 4 << ")" << endl;
    //                         soapy->dspEvents->setPartnerScheduleModeIndex(1, 4, 0);
    //                         // client.setScheduleMode(1, 4);
    //                         break;

    //                     default:
    //                         break;
    //                 }
    //                 cout << "key: " << unpack["k"] << " " << unpack["d"] << endl;
    //                 // switch()
    //             }
    //         }
    //     }
    // }

    void BoostTcpClient::handleDashboardRequest_1000s(const int u_val, const nlohmann::json &j) {
        cout << "handleDashboardRequest_1000s()" << endl;


        if( j["k"].is_null() || j["v"].is_null() || j["t"].is_null() ) {
            cout << "Some part of the message was missing, dropping" << endl;
            return;
        } else {
            const auto key =  j["k"];
            const auto val =  j["v"];
            const auto type = j["t"];

            std::vector<std::string> path_vec;
            for (const auto& x : j["k"].items()) {
                path_vec.push_back(x.value());
            }

            bool type_found = false;

            if(type == "bool") {
                soapy->settings->vectorSet(
                    j["v"].get<bool>(),
                    path_vec );

                type_found = true;
            }

            if(type == "int") {
                soapy->settings->vectorSet(
                    j["v"].get<int>(),
                    path_vec );
                
                type_found = true;
            }

            if(type == "double") {
                soapy->settings->vectorSet(
                    j["v"].get<double>(),
                    path_vec );
                
                type_found = true;
            }

            if(type == "string") {
                soapy->settings->vectorSet(
                    j["v"].get<std::string>(),
                    path_vec );
                
                type_found = true;
            }

            if( !type_found ) {
                cout << "Type: " << type << " was not found, dropping" << endl;
            }

            // for(auto x : path_vec) {
            //     cout << "fx: " << x << endl;
            // }

            cout << "made it inside " << key << " " << val << " " << type << endl;
            cout << j.dump() << endl;
        }
    }


    void BoostTcpClient::handleDashboardRequest(const int u_val, const nlohmann::json &j) {
        switch(u_val) {
            // case 1:
            //     handleDashboardRequest_1(j);
            //     break;
            case 1000:
                handleDashboardRequest_1000s(u_val, j);
                break;
            case 0: // default value / unhandled
            default:
                break;
        }
    }


    void BoostTcpClient::gotJsonPayload(const std::string str) {
        nlohmann::json j;

        try {
            // in >> j;
            j = nlohmann::json::parse(str);
        } catch(nlohmann::json::parse_error &e) {
            // print error message;
            return;
        }

        // try {
        //     // a = JSON.parse(response);
        // } catch(Exception e) {
        //     // alert(e); // error in the above string (in this case, yes)!
        // }

        

        int top_count = 0;
        for (auto& x : j.items()){(void)x;top_count++;}
        // cout << "top_count " << top_count << endl;

        if( top_count != 2 ) {
            return;
        }

        
        int u_val = 0;
         // example for an array
        int i = 0;
        for (auto& x : j.items())
        {
            // std::cout << "key: " << x.key() << ", value: " << x.value() << '\n';
            // for( auto& y : x.items() ) {
            //    cout << "x" << endl;
            // }

            if( i == 0 ) {
                if( x.value()["u"].is_null() ) {
                    // cout << "WAS NULL" << endl;
                    return;
                }
                u_val = x.value()["u"];
                // cout << x.value()["u"] << endl;
            } else {
                handleDashboardRequest(u_val, x.value());
            }

            i++;
        }
    }

  void BoostTcpClient::handle_read_headers(const boost::system::error_code& err)
  {
    cout << "handle_read_headers()" << endl;
    if (!err)
    {
      // Process the response headers.
      std::istream response_stream(&response_);
      std::string header;
      while (std::getline(response_stream, header) && header != "\r")
        std::cout << header << "\n";
      std::cout << "\n";

      // Write whatever content we already have to output.
      if (response_.size() > 0)
        std::cout << &response_;

      // Start reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&BoostTcpClient::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "BoostTcpClient Error: " << err << "\n";
    }
  }

  void BoostTcpClient::handle_read_content(const boost::system::error_code& err)
  {
    cout << "handle_read_content()" << endl;
    if (!err)
    {
      // Write all of the data that has been read so far.
      std::cout << &response_;

      // Continue reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&BoostTcpClient::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else if (err != boost::asio::error::eof)
    {
      std::cout << "BoostTcpClient Error: " << err << "\n";
    }
  }
