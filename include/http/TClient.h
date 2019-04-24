#ifndef TCLIENT_H
#define TCLIENT_H


#include <boost/asio.hpp>
#include "../TLogger.h"

using namespace std;
using boost::asio::ip::tcp;
extern TLogger * logger;

class TClient
{
public:
  TClient(boost::asio::io_service& io_service,
      const string& server, const string& path, const string& req, bool wait_for_reply = true);

  ~TClient();

  string get_response();

private:
  void handle_resolve(const boost::system::error_code& err,
      tcp::resolver::iterator endpoint_iterator);

  void handle_connect(const boost::system::error_code& err);

  void handle_write_request(const boost::system::error_code& err);

  void handle_read_status_line(const boost::system::error_code& err);

  void handle_read_headers(const boost::system::error_code& err);

  void handle_read_content(const boost::system::error_code& err);

  void close_socket(boost::system::error_code& error);

  boost::asio::deadline_timer timer_;
  tcp::resolver resolver_;
  tcp::socket socket_;
  boost::asio::streambuf request_;
  boost::asio::streambuf response_;
  bool wait_for_reply;
  std::array<char, 4096> bytes;
};

#endif
