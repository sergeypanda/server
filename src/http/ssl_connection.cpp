//
// connection.cpp
// ~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "../../include/http/ssl_connection.hpp"
#include <vector>
#include <boost/bind.hpp>
#include "../../include/http/request_handler.hpp"
#include "../../include/http/TSocketManager.h"
#include "../../include/http/TQueue.h"

ssl_connection::ssl_connection(boost::asio::io_service& io_service, boost::asio::ssl::context& context,
    request_handler& handler, int max_content_lenght, unsigned int min_upload_speed)
  : strand_(io_service),
    timer_(io_service),
    socket_(io_service, context),
    request_handler_(handler),
	max_content_lenght_(max_content_lenght),
	min_upload_speed(min_upload_speed)
{
	total_bytes_transferred = 0;
}

ssl_connection::~ssl_connection()
{

}

/// Get the socket associated with the SSL connection.

ssl_socket::lowest_layer_type&  ssl_connection::socket()
{
	return socket_.lowest_layer();
}

void  ssl_connection::start()
{
	socket_.async_handshake(boost::asio::ssl::stream_base::server,
		strand_.wrap(
		boost::bind(&ssl_connection::handle_handshake, shared_from_this(),
		boost::asio::placeholders::error)));
}

void  ssl_connection::handle_handshake(const boost::system::error_code& error)
{
	if (!error)
	{
		start_time = time(0);
		request_.start_time = start_time;
		socket_.async_read_some(boost::asio::buffer(buffer_),
			strand_.wrap(
			boost::bind(&ssl_connection::handle_read, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred)));

		timer_.expires_from_now(boost::posix_time::seconds(10));
//		timer_.async_wait(boost::bind(&ssl_connection::CheckSpeed, shared_from_this(), boost::asio::placeholders::error)); 
	}
	else
	{
		char error_string[256];
		ERR_error_string_n(error.value(), error_string,sizeof(error_string));
		cout << "HandShake Error. ";
		logger->Add(error_string);
		logger->Add("try adding https://");
		shared_from_this().reset();
	}
}

void ssl_connection::CheckSpeed(const boost::system::error_code& error)
{

	if(!error)
	{
		time_t total_time;
		total_time = time(0) - start_time;
		if (total_time == 0)
		{
			timer_.expires_from_now(boost::posix_time::seconds(1));
			timer_.async_wait(boost::bind(&ssl_connection::CheckSpeed, this, boost::asio::placeholders::error));
			
			return;
		}
		
		if((total_bytes_transferred / (unsigned int) total_time) < this->min_upload_speed)
		{
			cout<< "\r\nSpeed connection is low. Shutdown Socket\r\n";
			boost::system::error_code ignored_ec;
			timer_.cancel();
			socket_.shutdown(ignored_ec);	
		}
		else
		{
			timer_.expires_from_now(boost::posix_time::seconds(1));
			timer_.async_wait(boost::bind(&ssl_connection::CheckSpeed, this, boost::asio::placeholders::error));
		}

	}
}

void ssl_connection::handle_read(const boost::system::error_code& e,
    std::size_t bytes_transferred)
{
	total_bytes_transferred += bytes_transferred;

  if (!e)
  {
    boost::tribool result;

	
 	if(request_.ip == "")
		request_.ip = socket_.next_layer().remote_endpoint().address().to_string();	


	request_.max_content_lenght_size = this->max_content_lenght_;

    boost::tie(result, boost::tuples::ignore) = request_parser_.parse(
   	request_, buffer_.data(), buffer_.data() + bytes_transferred);



    if (result)
    {
	  time_t total_t = time(0) - start_time;
	  if(total_t != 0)
	 	  request_.connection_speed = total_bytes_transferred / (unsigned int)total_t;

	  request_.last_request_time = time(0);
      request_handler_.handle_request(&request_, reply_);

	
		//reply_.content += "\0";
		string buff = reply_.to_string();
		std::vector<boost::asio::const_buffer> bs;
		bs = reply_.to_buffers();
		boost::asio::async_write(socket_, bs,
		strand_.wrap(
 		  boost::bind(&ssl_connection::handle_write, shared_from_this(),
 		  boost::asio::placeholders::error)));
  
    }
    else if (!result)
    {
      reply_ = reply::stock_reply(reply::bad_request);
	  boost::asio::async_write(socket_, reply_.to_buffers(),
		strand_.wrap(
		 boost::bind(&ssl_connection::handle_write, shared_from_this(),
		  boost::asio::placeholders::error)));
    }
    else
    {

      socket_.async_read_some(boost::asio::buffer(buffer_),
		  strand_.wrap(
               boost::bind(&ssl_connection::handle_read, shared_from_this(),
              boost::asio::placeholders::error,
              boost::asio::placeholders::bytes_transferred))); 
    }
  }

  // If an error occurs then no new asynchronous operations are started. This
  // means that all shared_ptr references to the connection object will
  // disappear and the object will be destroyed automatically after this
  // handler returns. The connection class's destructor closes the socket.
}

void ssl_connection::handle_write(const boost::system::error_code& e)
{
  if (!e)
  {
    // Initiate graceful connection closure.
    boost::system::error_code ignored_ec;
	socket_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);

	timer_.cancel(ignored_ec);

  }

	request_handler_.queue->DeleteQueueItem(reply_.queue_item_id);

  // No new asynchronous operations are started. This means that all shared_ptr
  // references to the connection object will disappear and the object will be
  // destroyed automatically after this handler returns. The connection class's
  // destructor closes the socket.
}

 // namespace server3
 // namespace http
void ssl_connection::close(boost::system::error_code& error)
{
	if (error == boost::asio::error::operation_aborted)
	{
		cout << "\r\n Timer! operation aborted \r\n";

		boost::system::error_code ignored_ec;

		socket_.next_layer().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);

		timer_.cancel(error);
	}
}

