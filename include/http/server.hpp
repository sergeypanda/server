//
// server.hpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_SERVER_HPP
#define HTTP_SERVER3_SERVER_HPP

#include <boost/asio.hpp>
#include <vector>
#include <boost/thread/mutex.hpp>

#include "request.hpp"

#include "connection.hpp"
#include <boost/thread.hpp>

class request_handler;
class TSocketManager;
class connection;
class TQueue;

/// The top-level class of the HTTP server.
class server
  : private boost::noncopyable
{
public:
  /// Construct the server to listen on the specified TCP address and port, and
  /// serve up files from the given directory.
  explicit server(const std::string& address, const std::string& port,
      const std::string& doc_root, std::size_t thread_pool_size, int max_cont_lenght, unsigned int min_upload_speed, TQueue * queue_);
    ~server();

	///https://theboostcpplibraries.com/boost.asio-scalability explanation of using multiple threads on io_service run
   std::vector<std::shared_ptr<boost::thread> > threads; // changed from boost::shared_ptr

   std::map<int, bool> free_ports;

 //  TSocketManager * owner;
//   TQueue * queue;

   request_handler * request_handler_;

  int max_cont_lenght;

  unsigned int min_upload_speed;

// boost::asio::deadline_timer timer_;

  /// Run the server's io_service loop.
  void run();

  /// Stop the server.
  void stop();

  void acceptor_close();

  void io_service_run();

  /// Handle completion of an asynchronous accept operation.
  void handle_accept(const boost::system::error_code& e);
	void start_accept(const boost::system::error_code& e);

	
private:
	boost::mutex ios_mutex;
	bool ios_reset;


  /// The number of threads that will call io_service::run().
  std::size_t thread_pool_size_;

  /// The io_service used to perform asynchronous operations.
  boost::asio::io_service io_service_;

  /// Acceptor used to listen for incoming connections. acceptor is a passive socket impl
  boost::asio::ip::tcp::acceptor acceptor_;

  /// The next connection to be accepted.
  connection_ptr new_connection_;

  /// The handler for all incoming requests.

	///signal to close acceptor and all incoming connvetions and shutdown the server 
  boost::asio::signal_set m_signal;


};

 // namespace server3
 // namespace http


#endif // HTTP_SERVER3_SERVER_HPP

