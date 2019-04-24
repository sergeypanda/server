//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "../../include/http/server.hpp"
#include <boost/thread.hpp>
#include <vector>
#include "../../include/http/request_handler.hpp"
#include "../../include/http/TQueue.h"



server::server(const std::string& address, const std::string& port,
    const std::string& doc_root, std::size_t thread_pool_size, int max_cont_lenght, unsigned int min_upload_speed, TQueue * queue_)
  : thread_pool_size_(thread_pool_size),
    acceptor_(io_service_),
	request_handler_(new request_handler(doc_root, queue_)),
	max_cont_lenght(max_cont_lenght),
	min_upload_speed(min_upload_speed),
	m_signal(io_service_, SIGINT, SIGTERM)

{
	
	m_signal.async_wait([this](boost::system::error_code error,  int signal)
	{
		std::cout << "Server received termination signal : " << signal << " !\n" ;
		this->acceptor_.cancel();
	});


  for(int i = 20000, k = 30000; i < k; ++i)
  {
		free_ports[i] = true;
  }

  try
  {
	  ///using query class and resolver to resolve the host and create and endpoint object - resolver returns iterator becuase host name can have 
	  /// a lot of different network interfaces with different ip addresses. take first one or iterate thru it.
	  ///If you want to connect to a hostname (not an IP address), here's what you do:
	  boost::asio::ip::tcp::resolver resolver(io_service_); // DNS name resolution  functionality
	  boost::asio::ip::tcp::resolver::query query(address, port); // DNS name resolution query. 
	  ///construct an endpoint object after the DNS name resolution
	  boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query); // resolve returns ip::tcp::resolver::iterator 

//	  boost::system::error_code err;
//	  boost::asio::ip::address addr(boost::asio::ip::address::from_string(address, err));
//	  boost::asio::ip::tcp::endpoint endpoint2(addr, atoi(port.c_str()));

	  // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR).
	  ///Call the acceptor socket's open() method, passing the object representing the protocol.  Creating and opening an acceptor socket.
	  acceptor_.open(endpoint.protocol()); // endpoint's  protocol() method returns an object of the asio::ip::tcp class representing  the corresponding protocols. 
	  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	  acceptor_.bind(endpoint); //  Bind the acceptor socket to the server endpoint created .

	  acceptor_.listen(); // overload listen(int backlog) backlog is size of the queue containing the pending connection  // requests. 
	  ///When processing a pending connection request, the acceptor socket allocates a new active socket!!!
	  ///When ready to process a connection request, call the acceptor socket's accept() method passing an active socket object created 
/*	  acceptor_.async_accept(new_connection_->socket(),
		  [this](boost::system::error_code error)
	  {
		  this->handle_accept(error);
	  });*/

	  boost::system::error_code error;
	  handle_accept(error);
  }
  catch (std::exception& e)
  {
	  setlocale (LC_CTYPE,"Russian");
	  std::cout << "Exception: " << e.what() << "\n";
	  cin.get();
  }

}

void server::handle_accept(const boost::system::error_code& e)
{

	std::cout << "accepting request\n";
	if (!e)
	{
//		new_connection_->start();
		new_connection_.reset(new connection(io_service_, *request_handler_, max_cont_lenght, min_upload_speed));
		acceptor_.async_accept(new_connection_->socket(),
			boost::bind(&server::start_accept, this,
				boost::asio::placeholders::error));
/*		acceptor_.async_accept(new_connection_->socket(), [this](boost::system::error_code error)
		{
			this->start_accept(error);

		});*/


		// 	timer_.expires_from_now(boost::posix_time::seconds(5));
		//	timer_.async_wait(boost::bind(&server::acceptor_close, this));

	}
}

void server::start_accept(const boost::system::error_code& e)
{
	if (!e)
	{
		new_connection_->start();
	}

	handle_accept(e);
}

void server::io_service_run()
{

		try
 		{
			io_service_.run();
			cout<< " run() exited normally!\r\n";

		}
		catch (std::exception& e)
		{
			setlocale (LC_CTYPE,"Russian");
			std::cout << "Exception: " << e.what() << "\n";
			cin.get();
		}
}

void server::run()
{
  // Create a pool of threads to run all of the io_services.

  for (std::size_t i = 0; i < thread_pool_size_; ++i)
  {

	  std::shared_ptr<boost::thread> thread(new boost::thread(
		 [this](){
		  this->io_service_run();
	  }
	  ));

    threads.push_back(thread);

	//---------------------------------------------------------------
  }

  // Wait for all threads in the pool to exit.
  for (std::size_t i = 0; i < threads.size(); ++i)
  {
    threads[i]->join();
  }
}


void server::stop()
{
	cout << "stop";
	io_service_.stop();
}
void server::acceptor_close()
{
	acceptor_.close();
}
server::~server()
{
	SAFEDELETE(request_handler_)
}