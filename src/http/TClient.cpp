#include "../../include/http/TClient.h"
#include <boost/bind/bind.hpp>
#include <boost/asio/buffer.hpp>


TClient::~TClient()
{
}

TClient::TClient(boost::asio::io_service& io_service,
      const string& host, const string& port, const string& req, bool wait_for_reply_) :
	  resolver_(io_service),
		  timer_(io_service),
		socket_(io_service),
		wait_for_reply(wait_for_reply_)
  {
    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    std::ostream request_stream(&request_);
    request_stream << req;

    // Start an asynchronous resolve to translate the server and service names
    // into a list of endpoints.

    tcp::resolver::query query(host, port);
	resolver_.async_resolve(query, [this](auto& error, auto& iter)
	{
		this->handle_resolve(error, iter);
	});

/*    resolver_.async_resolve(query,
        boost::bind(&TClient::handle_resolve, this,
          boost::asio::placeholders::error,
          boost::asio::placeholders::iterator));*/

  }
	  void TClient::close_socket(boost::system::error_code& error)
	  {
		  try
		  {
			  timer_.cancel(error);
			  if (error)
				throw error;

			  socket_.close(error);
			  if (error)
				  throw error;
		  }
		  catch (boost::system::error_code& error)
		  {
			  logger->Add(error.message());
			  cout << error.message();
		  }

	  }


  void TClient::handle_resolve(const boost::system::error_code& err,
      tcp::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      // Attempt a connection to each endpoint in the list until we
      // successfully establish a connection.
		const boost::system::error_code error;

		boost::asio::async_connect(socket_, endpoint_iterator, [this](auto& error, auto& iter)
		{
			this->handle_connect(error);
		});

/*		boost::asio::async_connect(socket_, endpoint_iterator,
          boost::bind(&TClient::handle_connect, this,
            boost::asio::placeholders::error));*/

		timer_.expires_from_now(boost::posix_time::seconds(100));
		timer_.async_wait([this](auto error)
		{
			this->close_socket(error);
		});

//    	timer_.async_wait(boost::bind(&TClient::close_socket, this, error));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
	  logger->Add(err.message());
    }
  }

  void TClient::handle_connect(const boost::system::error_code& err)
  {
    if (!err)
    {
      // The connection was successful. Send the request.

  		boost::asio::async_write(socket_, request_,[this](auto& error, auto& bytes_transfered)
		{
			this->handle_write_request(error);
		});

    /*  boost::asio::async_write(socket_, request_,
          boost::bind(&TClient::handle_write_request, this,
            boost::asio::placeholders::error));*/
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
	  logger->Add(err.message());
    }
  }


  void TClient::handle_write_request(const boost::system::error_code& err)
  {
    if(!wait_for_reply)
	{
        close_socket(const_cast<boost::system::error_code&>(err));
		return;
	}

    if (!err)
    {
      // Read the response status line. The response_ streambuf will
      // automatically grow to accommodate the entire line. The growth may be
      // limited by passing a maximum size to the streambuf constructor.
      boost::asio::async_read_until(socket_, response_, "\r\n",
          boost::bind(&TClient::handle_read_status_line, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err.message() << "\n";
	  logger->Add(err.message());
    }
  }

  void TClient::handle_read_status_line(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Check that response is OK.
      std::istream response_stream(&response_);
      std::string http_version;
      response_stream >> http_version;
      unsigned int status_code;
      response_stream >> status_code;
      std::string status_message;
      std::getline(response_stream, status_message);
      if (!response_stream || http_version.substr(0, 5) != "HTTP/")
      {
		logger->Add("Invalid response");
        std::cout << "Invalid response\n";
        return;
      }
      if (status_code != 200)
      {
		logger->Add("Response returned with status code");
        std::cout << "Response returned with status code ";
        std::cout << status_code << "\n";
        return;
      }

      // Read the response headers, which are terminated by a blank line.
      boost::asio::async_read_until(socket_, response_, "\r\n\r\n",
          boost::bind(&TClient::handle_read_headers, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
	  logger->Add(err.message());
    }
  }

  void TClient::handle_read_headers(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Start reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&TClient::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else
    {
      std::cout << "Error: " << err << "\n";
	  logger->Add(err.message());
    }
  }

  void TClient::handle_read_content(const boost::system::error_code& err)
  {
    if (!err)
    {
      // Continue reading remaining data until EOF.
      boost::asio::async_read(socket_, response_,
          boost::asio::transfer_at_least(1),
          boost::bind(&TClient::handle_read_content, this,
            boost::asio::placeholders::error));
    }
    else if (err != boost::asio::error::eof)
    {
      std::cout << "Error: " << err << "\n";
	  logger->Add(err.message());
    }
	else
	{
		close_socket(const_cast<boost::system::error_code&>(err));
	}

  }

  string TClient::get_response()
  {
	  vector<unsigned char> buffer;
	  while (response_.sgetc()!=EOF)
	  {
		  buffer.push_back(response_.sbumpc());
	  }
	  string str(buffer.begin(), buffer.end());
	  return str;
  }
