#include "../../include/http/TSocketManager.h"
//#include "TQueue.h"
#include "../../include/TConfig.h"
#include "../../include/http/TClient.h"
#include "../../include/http/server.hpp"
#include "../../include/http/ssl_server.hpp"

TSocketManager::TSocketManager(string host, string port, string ssl_port, string server_root, int num_threads, int max_content_lenght, unsigned int min_upload_speed)
{
//	queue = NULL;
	config = NULL;
	http_server = NULL;
	ssl_http_server = NULL;
	client = NULL;
	on_close = false;

	Init(host, port, ssl_port, server_root, num_threads, max_content_lenght, min_upload_speed);
}
TSocketManager::~TSocketManager()
{
//	SAFEDELETE(queue)
	SAFEDELETE(config)
	SAFEDELETE(http_server)
	SAFEDELETE(ssl_http_server)
	SAFEDELETE(client)

}
void TSocketManager::Init(string host, string port, string ssl_port, string server_root, int num_threads, int max_content_lenght, unsigned int min_upload_speed)
{
	this->host = host;
	this->port = port;
	this->ssl_port = ssl_port;
	this->server_root = server_root;
	this->num_threads = num_threads;
	this->max_content_lenght = max_content_lenght;
	this->min_upload_speed = min_upload_speed;

//	queue = new TQueue();


	logger->Add("GUARDIAN START");

}

void TSocketManager::StartServer(TQueue * queue)
{
//	boost::thread main_thread(boost::bind(&TSocketManager::StartSSLServer, this));

	for(;;)
	{
        if(on_close)
            break;

		if(http_server != NULL)
		{
			cout<< "Error! Server deleted! Run new instance!\r\n";
			delete http_server;
		}

		http_server = new server(host, port, server_root, num_threads, max_content_lenght, min_upload_speed, queue);

	// Run the server until stopped.
		http_server->run();


	}
}
void TSocketManager::StartSSLServer()
{
	for(; ;)
	{
	    if(on_close)
            break;

		if(ssl_http_server != NULL)
		{
			cout<< "Error! Server deleted! Run new instance!\r\n";
			delete ssl_http_server;
		}

 		ssl_http_server = new ssl_server(host, ssl_port, server_root, num_threads, max_content_lenght, min_upload_speed, this);

		// Run the server until stopped.
		ssl_http_server->run();
	}
}
void TSocketManager::StopServer()
{
	if (!http_server)
		return;

	on_close = true;
	http_server->stop();

	if(ssl_http_server)
		ssl_http_server->stop();
}


string TSocketManager::SendHttpRequest(string host, string port, vector<unsigned char> req, int errors_count, bool wait_for_reply)
{
	string response;
	try
	{
		if (port == "" || host == "")
			throw 0;

		boost::asio::io_service io_service;


		string str(req.begin(), req.end());
		TClient client(io_service, host, port, str, wait_for_reply);
		io_service.run();
		response = client.get_response();
		cout << response;

	}
	catch (...)
	{
		stringstream ss;
		ss << "\r\nhost :" << host << "\r\n port: " << port << "\r\n ";
		std::cout << "Exception: " << "\r\nhost :" << host << "\r\n port: " << port << "\r\n ";
		logger->Add(ss.str());
	}

	return response;
}
string TSocketManager::SendHttpRequest(string host, string port, string req, int errors_count, bool wait_for_reply)
{
	string response = "";

	try
	{
		vector<unsigned char> reqv(req.begin(), req.end());
		response = SendHttpRequest(host, port, reqv, errors_count, wait_for_reply);
	}

	catch (...)
	{
		cout << "Caught exception.  " << endl;
		logger->Add("RequestProcessor() line: SendHttpRequest");
	}

	return response;
}

string TSocketManager::GetFreePort(string host)
{
	stringstream str;
	string tmp;

	for(int i = PORTS_NUM_BEGIN, k = PORTS_NUM_END; i < k; ++i)
	{
		if(http_server->free_ports[i] == true)
		{
			http_server->free_ports[i] = false;
			str << i;
			str >> tmp;
			str.clear();

			if(CheckPort(host, tmp))
				break;
			else
				continue;
		}
		else
			continue;
	}

	return tmp;
}
void TSocketManager::CheckFreePorts(string host)
{
#ifndef _WIN32
	stringstream str;
	string tmp;

	for(int i = PORTS_NUM_BEGIN, k = PORTS_NUM_END; i < k; ++i)
	{
		if(http_server->free_ports[i] == false)
		{
			str << i;
			str >> tmp;
			str.clear();

			if(CheckPort(host, tmp))
				http_server->free_ports[i] = true;
		}
		else
			continue;
	}
#endif
}
bool TSocketManager::CheckPort(string host, string port)
{
	try
	{
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::acceptor acceptor_ (io_service);
		boost::asio::ip::tcp::resolver resolver(io_service);
		boost::asio::ip::tcp::resolver::query query(host, port);
		boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

		acceptor_.open(endpoint.protocol());
		acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(false));
		acceptor_.bind(endpoint);
		acceptor_.close();

	}
	catch (std::exception& e)
	{
		setlocale (LC_CTYPE,"Russian");
		std::cout << "Exception: " << e.what() << "\n";
		return false;
	}
	catch (...)
	{

		std::cout << "Exception: Port is busy! " << "\n";
		return false;
	}
	return true;

}
void TSocketManager::FreePort(string port)
{
    if(http_server)
    {
	for(int i = PORTS_NUM_BEGIN, k = PORTS_NUM_END; i < k; ++i)
	{
		if(i == atoi(port.c_str()))
		{
			http_server->free_ports[i] = true;
			break;
		}

		else
			continue;
	}

    }
}
