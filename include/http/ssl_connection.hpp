//
// connection.hpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_SSLCONNECTION_HPP
#define HTTP_SERVER3_SSLCONNECTION_HPP
#define SSLFUCKINGBITCH

#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "reply.hpp"
#include "request.hpp"

#include "request_parser.hpp"

class request_handler;
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;

/// Represents a single connection from a client.
class ssl_connection
  : public boost::enable_shared_from_this<ssl_connection>,
    private boost::noncopyable

{
	/// Socket for the SSL connection.
	ssl_socket socket_;

public:

	int max_content_lenght_;

	unsigned int min_upload_speed;
  /// Construct a connection with the given io_service.
  explicit ssl_connection(boost::asio::io_service& io_service, boost::asio::ssl::context& context,
      request_handler& handler, int max_content_lenght, unsigned int min_upload_speed);


  unsigned int total_bytes_transferred;

  time_t start_time;

  unsigned int speed;

  /// Start the first asynchronous operation for the connection.
  void close(boost::system::error_code& error);
  void close2(const boost::system::error_code& error);
  void CheckSpeed(const boost::system::error_code& error);

  boost::asio::deadline_timer timer_;
  ~ssl_connection();

  /// Handle completion of a read operation.
  void handle_read(const boost::system::error_code& e,
      std::size_t bytes_transferred);

  /// Handle completion of a write operation.
  void handle_write(const boost::system::error_code& e);

  /// Strand to ensure the connection's handlers are not called concurrently.
  boost::asio::io_service::strand strand_;

  /// Socket for the connection.
 // boost::asio::ip::tcp::socket socket_;

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


  //some data
  enum { max_length = 1024 };
  char data_[max_length];

    /// Get the socket associated with the SSL connection.
  ssl_socket::lowest_layer_type& socket();
  void start();
  void handle_handshake(const boost::system::error_code& error);


};


 // namespace server3
 // namespace http

#endif // HTTP_SERVER3_CONNECTION_HPP
