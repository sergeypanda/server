#ifndef TRESPONSE_INFO_H
#define TRESPONSE_INFO_H

#include "TMacros.h"

#include "TBaseClass.h"

//#include "TLicense.h"

// [not_serialize] object_type, class_name, owner_id, owner, owner_type, object_id

class TResponseInfo : public TBaseClass
{

public:

	int client_id;
	string request_id;
	string server;
	string server_version;
	string server_pid;
	string server_port;

	string command;
	string data;
	string error;
	string parameters;
	int succesfull;
	string message_code;
	string message;
	int lang_id;
	int server_time;
	unsigned long exec_time;

	TResponseInfo();
	~TResponseInfo();

	void Init();
	void ParseResponseString(string response);
	void AddData(string data);
	string ToXml(int mode);
	string ToJson(int mode);

	void FromJson_(const Value& v);
	TClassAttribute * _GetAttributes();
	TResponseInfo * Clone();
};
#endif
