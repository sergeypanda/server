#ifndef TSOCKETMANAGER_H
#define TSOCKETMANAGER_H

#include <boost/asio.hpp>
#include <boost/function.hpp>



class TClient;
class TConfig;
class TQueue;
class ssl_server;
class server;

using boost::asio::ip::tcp;

using namespace std;

class TSocketManager
{
public:
	int id;
	string host;
	string port;
	string ssl_port;
	string server_root;
	int num_threads;
	int max_content_lenght;
	int timeout_request;
	unsigned int min_upload_speed;
	TClient * client;

	TConfig * config;
//	TQueue * queue;
	server * http_server;
	ssl_server * ssl_http_server;
    bool on_close;

	TSocketManager(string host, string port, string ssl_port, string server_root, int num_threads, int max_content_lenght, unsigned int min_upload_speed);
	~TSocketManager();

	void Init(string host, string port, string ssl_port, string server_root, int num_threads, int max_content_lenght, unsigned int min_upload_speed);
	void StartServer(TQueue * queue);
	void StartSSLServer();
	void StopServer();

	string SendHttpRequest(string host, string port, vector<unsigned char> req, int errors_count = 0, bool wait_for_reply = true);
	string SendHttpRequest(string host, string port, string req, int errors_count = 0, bool wait_for_reply = true);

	string GetFreePort(string host);
	void FreePort(string port);
	bool CheckPort(string host, string port);
    void CheckFreePorts(string host);
};


#endif
