#ifndef TSESSION_H
#define TSESSION_H

#include "TBaseClass.h"

//serializable

class TSession : public TBaseClass
{
public:
						// id from Data Base
	string ip;			     		
	string cookie_sid;				// from Cookie
	int pandora_pid;
	int user_id;
	wstring name;
	wstring login;
	wstring password;
	wstring company;
	string host;
	string port;
	int db_id;
	int errors_count;

	int start_time;			// login time
	int last_request_time;	
	int request_count;		//number of requests sent from start time
	int bytes_sent;
	int bytes_received;		
	int threads_count;		// кол-во потоков кот. обрабатывают запросы клиента из очереди

	int pandora_handler;//handler на запущенную пандору

	int status;				// for future use

	int status_id;
	string notes;
	int external_key;

	TSession ();
	TSession(int id_, int user_id_, string ip_, wstring login_, string cookie_, string host_, string port_, int pandora_pid_);

	TClassAttribute * _GetAttributes();
	void FromJson_(const Value& v);
	TSession * Clone();
  };
#endif