#pragma once
#include <iostream>
#include <istream>
#include <ostream>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "3rd/json.hpp"
#include "HiggsSoapyDriver.hpp"

using boost::asio::ip::tcp;

class HiggsDriver;

class BoostTcpClient
{
public:
  BoostTcpClient(boost::asio::io_service& io_service,
      const std::string& server, const std::string& path,bool connect=true, HiggsDriver *s=0);


 

  void write(const std::string& msg);

private:
  void handle_resolve(const boost::system::error_code& err,tcp::resolver::iterator endpoint_iterator);
  void handle_connect(const boost::system::error_code& err);
  void handle_write_request(const boost::system::error_code& err);
  void handle_read_status_line(const boost::system::error_code& err);
  void handle_read_headers(const boost::system::error_code& err);
  void handle_read_content(const boost::system::error_code& err);
  void gotJsonPayload(const std::string str);
  void handleDashboardRequest(const int u_val, const nlohmann::json &j);
  // void handleDashboardRequest_1(const nlohmann::json &j);
  void handleDashboardRequest_1000s(const int u_val, const nlohmann::json &j);

   // called automatically
  void DoResolveConnect();
  
  tcp::resolver resolver_;
  tcp::socket socket_;
  boost::asio::streambuf request_;
  boost::asio::streambuf response_;
  std::string server_;
  std::string port_;
  bool should_connect_;
  char data[1024];
  std::ostringstream oss;
  bool did_fire = false;
  HiggsDriver* soapy;
  bool can_write = false;
};
