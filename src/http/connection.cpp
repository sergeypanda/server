//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "../../include/http/connection.hpp"
#include <boost/bind.hpp>

#include "../../include/http/reply.hpp"
#include "../../include/http/request_handler.hpp"
#include "../../include/http/TSocketManager.h"
#include "../../include/http/TQueue.h"
#include "../../include/http/server.hpp"


connection::connection(boost::asio::io_service& io_service,
    request_handler& handler, int max_content_lenght, unsigned int min_upload_speed)
  : strand_(io_service),
    timer_(io_service),
    socket_(io_service),
    request_handler_(handler),
	max_content_lenght_(max_content_lenght),
	min_upload_speed(min_upload_speed)
{
	total_bytes_transferred = 0;
}



void connection::start()
{
	start_time = time(0);
//	std::cout << "start connection\n";
	request_.start_time = start_time;
	socket_.async_read_some(boost::asio::buffer(buffer_),
	         
        boost::bind(&connection::handle_read, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));

/*	boost::asio::async_read(socket(), boost::asio::buffer(buffer_),
		boost::asio::transfer_at_least(1),
		strand_.wrap(boost::bind(&connection::handle_read, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)));*/

  timer_.expires_from_now(boost::posix_time::seconds(10));
 // timer_.async_wait(boost::bind(&connection::CheckSpeed, shared_from_this(), boost::asio::placeholders::error));

}


void connection::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{
	total_bytes_transferred += bytes_transferred;
//	std::cout << "handle_read\n";
  if (!e)
  {
    boost::tribool result;

	
	if(request_.ip == "")
		request_.ip = socket_.remote_endpoint().address().to_string();

	request_.max_content_lenght_size = this->max_content_lenght_;

    boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
   	request_, buffer_.data(), buffer_.data() + bytes_transferred);

	auto data = buffer_.data();
//	std::cout << typeid(data).name();

    if (result)
    {
	  time_t total_t = time(0) - start_time;
	  if(total_t != 0)
	 	  request_.connection_speed = total_bytes_transferred / (unsigned int)total_t;

	  request_.last_request_time = time(0);
//	  std::cout << " else if request_handler_.handle_request(&request_, reply_);\n";
      request_handler_.handle_request(&request_, reply_);

//	  std::cout << "boost::asio::async_write(socket_, reply_.to_buffers(),\n";
	  boost::asio::async_write(socket_, reply_.to_buffers(),
	                           strand_.wrap(
		  boost::bind(&connection::handle_write, shared_from_this(),
		  boost::asio::placeholders::error)));

	  
    }
    else if (!result)
	{
//		std::cout << " else if (!result)  reply_ = reply::stock_reply(reply::bad_request);\n";
      reply_ = reply::stock_reply(reply::bad_request);
	  boost::asio::async_write(socket_, reply_.to_buffers(),
	                           strand_.wrap(
		 boost::bind(&connection::handle_write, shared_from_this(),
		  boost::asio::placeholders::error)));
    }
    else
    {
//		std::cout << "else socket_.async_read_some(boost::asio::buffer(buffer_),\n";
		boost::asio::async_read(socket(), boost::asio::buffer(buffer_),
			boost::asio::transfer_at_least(1),
		                        strand_.wrap(boost::bind(&connection::handle_read, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred)));

/*      socket_.async_read_some(boost::asio::buffer(buffer_),
          strand_.wrap(
            boost::bind(&connection::handle_read, shared_from_this(),
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred))); */
    }
  }
//  std::cout << " handle_read done\n\n";

  // If an error occurs then no new asynchronous operations are started. This
  // means that all shared_ptr references to the connection object will
  // disappear and the object will be destroyed automatically after this
  // handler returns. The connection class's destructor closes the socket.
 // socket_.close();
}



void connection::handle_write(const boost::system::error_code& e)
{
 boost::system::error_code ignored_ec;
  if (!e)
  {
    // Initiate graceful connection closure.
    
//	std::cout << " socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);	\n";
    socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);	

	timer_.cancel(ignored_ec);

  }
 // std::cout << " request_handler_.queue->DeleteQueueItem(reply_.queue_item_id);\n";
  request_handler_.queue->DeleteQueueItem(reply_.queue_item_id);
	
  if(socket_.is_open());
	socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);

//  reply_.print();
//  std::cout << " deleting done;\n";
  // No new asynchronous operations are started. This means that all shared_ptr
  // references to the connection object will disappear and the object will be
  // destroyed automatically after this handler returns. The connection class's
  // destructor closes the socket.

}

 // namespace server3
 // namespace http

void connection::close(const boost::system::error_code& error)
{
	if (error == boost::asio::error::operation_aborted)
	{
		cout << "\r\n Timer! operation aborted \r\n";

		boost::system::error_code ignored_ec;

		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);	

		timer_.cancel(ignored_ec);
	}
}

void connection::CheckSpeed(const boost::system::error_code& error)
{

	if (!error)
	{
		time_t total_time;
		total_time = time(0) - start_time;
		if (total_time == 0)
		{
			timer_.expires_from_now(boost::posix_time::seconds(1));
			timer_.async_wait(boost::bind(&connection::CheckSpeed, shared_from_this(), boost::asio::placeholders::error));

			return;
		}

		if ((total_bytes_transferred / (unsigned int)total_time) < this->min_upload_speed)
		{
			cout << "\r\nSpeed connection is low. Shutdown Socket\r\n";
			boost::system::error_code ignored_ec;
			timer_.cancel();
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		}
		else
		{
			timer_.expires_from_now(boost::posix_time::seconds(1));
			timer_.async_wait(boost::bind(&connection::CheckSpeed, shared_from_this(), boost::asio::placeholders::error));
		}

	}
}