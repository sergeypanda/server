#include "../../include/http/ssl_server.hpp"
#include "../../include/http/request_handler.hpp"

ssl_server::ssl_server(const std::string& address, const std::string& port,
			   const std::string& doc_root, std::size_t thread_pool_size, int max_cont_lenght, unsigned int min_upload_speed, TSocketManager * owner_)

			   : thread_pool_size_(thread_pool_size),
//			   timer_(io_service_),
				context_(io_service_, boost::asio::ssl::context::sslv23),
			   acceptor_(io_service_,
			   boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), atoi(port.c_str()))),
//			   new_connection_(new ssl_connection(io_service_, context_, *request_handler_, max_cont_lenght, min_upload_speed)),
			   request_handler_(new request_handler(doc_root, NULL)),
			   max_cont_lenght(max_cont_lenght),
			   min_upload_speed(min_upload_speed),
			   owner(owner_)

			   
{
//	boost::asio::ip::tcp::resolver resolver(io_service_);
	for(int i = 20000, k = 30000; i < k; ++i)
	{
		free_ports[i] = true;
	}
	try
	{
// 		boost::asio::ip::tcp::resolver::query query(address, port);
// 		boost::asio::ip::tcp::endpoint endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), atoi(port.c_str())); //*resolver.resolve(query);
// 	
// 
// 		acceptor_.open(endpoint.protocol());
// 		acceptor_.bind(endpoint);

		context_.set_options(
			boost::asio::ssl::context::default_workarounds
			| boost::asio::ssl::context::no_sslv2
			| boost::asio::ssl::context::single_dh_use);
		context_.set_password_callback(boost::bind(&ssl_server::get_password, this));
		context_.use_certificate_chain_file("server.pem");
		context_.use_private_key_file("server.pem", boost::asio::ssl::context::pem);
		context_.use_tmp_dh_file("dh512.pem");

		boost::shared_ptr<ssl_connection> new_connection_ (new ssl_connection(io_service_, context_, *request_handler_, max_cont_lenght, min_upload_speed));
		
		acceptor_.listen();	
		acceptor_.async_accept(new_connection_->socket(),
			boost::bind(&ssl_server::handle_accept, this, new_connection_,
			boost::asio::placeholders::error));

//		request_handler_->owner = this->owner;
	}
	catch (std::exception& e)
	{
		setlocale (LC_CTYPE,"Russian");
		std::cout << "Exception: " << e.what() << "\n";
		cin.get();
	}





	//--------------------------------------------------------------------------------
}

string ssl_server::get_password() const
{
	return "test";
}


void ssl_server::io_service_run()
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

void ssl_server::run()
{
	// Create a pool of threads to run all of the io_services.

	for (std::size_t i = 0; i < thread_pool_size_; ++i)
	{
//     boost::shared_ptr<boost::thread> thread(new boost::thread(
//           boost::bind(&boost::asio::io_service::run, &io_service_)));
		boost::shared_ptr<boost::thread> thread(new boost::thread(
			boost::bind(&ssl_server::io_service_run, this)));
		threads.push_back(thread);

		//---------------------------------------------------------------
	}

	// Wait for all threads in the pool to exit.
	for (std::size_t i = 0; i < threads.size(); ++i)
	{
		threads[i]->join();
	}
}


void ssl_server::stop()
{
	cout << "stop";
	io_service_.stop();
}

void ssl_server::handle_accept(boost::shared_ptr<ssl_connection> new_connection_, const boost::system::error_code& e)
{
	if (!e)
	{
		new_connection_->start();
		boost::shared_ptr<ssl_connection> new_connection_ (new ssl_connection(io_service_, context_, *request_handler_, max_cont_lenght, min_upload_speed));
//		new_connection_.reset(new ssl_connection(io_service_, context_, *request_handler_, max_cont_lenght, min_upload_speed));
		acceptor_.async_accept(new_connection_->socket(),
			boost::bind(&ssl_server::handle_accept, this, new_connection_,
			boost::asio::placeholders::error));

// 		 	timer_.expires_from_now(boost::posix_time::seconds(5));
// 			timer_.async_wait(boost::bind(&ssl_server::acceptor_close, this));

	}
	else
	{
		new_connection_.reset();
	}
}

// namespace server3
// namespace http
void ssl_server::acceptor_close()
{
	acceptor_.close();
}
