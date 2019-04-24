#ifndef TREQUEST_INFO_H
#define TREQUEST_INFO_H

#include "TBaseClass.h"

// [not_serialize] object_type, class_name, owner_id, owner, owner_type, object_id


class TRequestInfo : public TBaseClass
{

public:
	int client_id;
	string client_type;
	string client_version;
	string ip;
	string session;
	string key;

	string command;
	string data;
	string parameters;
	int language;
	string response_type;
	string script;

	int db_id;
	string db_name;
	string db_alias;

	TRequestInfo();
	TRequestInfo(string command_, string data_, string session_, string key_, string parameters_, string response_type_);
	~TRequestInfo();

	void Init();
	void FromJson_(const Value& v);
	TClassAttribute * _GetAttributes();
	TRequestInfo * Clone();


};
#endif