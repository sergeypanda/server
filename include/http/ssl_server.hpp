#ifndef HTTP_SERVER3_SSLSERVER_HPP
#define HTTP_SERVER3_SSLSERVER_HPP

#include <boost/asio.hpp>

#include <vector>

#include <boost/thread/mutex.hpp>
#include "ssl_connection.hpp"

#include <boost/asio/ssl.hpp>
#include <boost/thread.hpp>

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
typedef boost::shared_ptr<ssl_connection> ssl_connection_ptr;

class request_handler;
class TSocketManager;
class ssl_connection;

/// The top-level class of the HTTP server.
class ssl_server
	: private boost::noncopyable
{
public:
	/// Construct the server to listen on the specified TCP address and port, and
	/// serve up files from the given directory.
	explicit ssl_server(const std::string& address, const std::string& port,
		const std::string& doc_root, std::size_t thread_pool_size, int max_cont_lenght, unsigned int min_upload_speed, TSocketManager * owner_);

	std::vector<boost::shared_ptr<boost::thread> > threads;

	std::map<int, bool> free_ports;

	TSocketManager * owner;

	request_handler * request_handler_;

	int max_cont_lenght;

	unsigned int min_upload_speed;

//	boost::asio::deadline_timer timer_;

	/// Run the server's io_service loop.
	void run();

	/// Stop the server.
	void stop();

	void acceptor_close();

	string get_password() const;

	void io_service_run();
	boost::mutex ios_mutex;
	bool ios_reset;

	/// Handle completion of an asynchronous accept operation.
	void handle_accept(boost::shared_ptr<ssl_connection> new_connection_, const boost::system::error_code& e);

	/// The number of threads that will call io_service::run().
	std::size_t thread_pool_size_;

	/// The io_service used to perform asynchronous operations.
	boost::asio::io_service io_service_;

	/// Acceptor used to listen for incoming connections.
	boost::asio::ip::tcp::acceptor acceptor_;

	/// The next connection to be accepted.
	ssl_connection_ptr new_connection_;

	/// contex for ssl 
	boost::asio::ssl::context context_;

};

// namespace server3
// namespace http


#endif