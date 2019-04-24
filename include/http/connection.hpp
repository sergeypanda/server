//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_CONNECTION_HPP
#define HTTP_SERVER3_CONNECTION_HPP
#define FUCKINGBITCH


#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/deadline_timer.hpp>
#include "request.hpp"
#include "reply.hpp"
#include "request_parser.hpp"

class request_handler;


/// Represents a single connection from a client.
class connection
  : public boost::enable_shared_from_this<connection>,
    private boost::noncopyable
{
public:



  /// Construct a connection with the given io_service. should always be explicit to avoid implicit conversion which is very bad
	/// idea
  explicit connection(boost::asio::io_service& io_service,
      request_handler& handler, int max_content_lenght, unsigned int min_upload_speed);
  
  /// Start the first asynchronous operation for the connection.
  void start();

  void close(const boost::system::error_code& error);

  void CheckSpeed(const boost::system::error_code& error);

  /// Get the socket associated with the connection.
  boost::asio::ip::tcp::socket& socket()
  {
	  return socket_;
  };

  ~connection() = default;


private:
  /// Handle completion of a read operation.
  void handle_read(const boost::system::error_code& e,
      std::size_t bytes_transferred);

  /// Handle completion of a write operation.
  void handle_write(const boost::system::error_code& e);

  /// Strand to ensure the connection's handlers are not called concurrently.
  boost::asio::io_service::strand strand_;

  /// Socket for the connection.
  boost::asio::ip::tcp::socket socket_;

  /// The handler used to process the incoming request.
  request_handler& request_handler_;

  /// Buffer for incoming data.
  boost::array<unsigned char, 8192> buffer_;

  /// The incoming request.
  request request_;

  /// The parser for the incoming request.
  request_parser request_parser_;

  /// The reply to be sent back to the client.
  reply reply_;


  int max_content_lenght_;
  unsigned int min_upload_speed;
  unsigned int total_bytes_transferred;
  time_t start_time;
  unsigned int speed;
  boost::asio::deadline_timer timer_;
};

typedef boost::shared_ptr<connection> connection_ptr;

 // namespace server3
 // namespace http

#endif // HTTP_SERVER3_CONNECTION_HPP
