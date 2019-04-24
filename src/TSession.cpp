#include "../include/TSession.h"

TSession::TSession()
{
	id = 0;					// id from Data Base
	ip = "";
	cookie_sid = "";		// from Cookie
	name = L"";
	login = L""; 
	port = "";
	pandora_pid = 0;
	db_id = 0;
	errors_count = 0;
		

	start_time = 0;			// login time
	last_request_time = 0;	
	request_count = 0;		//number of requests sent from start time
	bytes_sent = 0;
	bytes_received = 0;		
	threads_count = 0;		// кол-во потоков кот. обрабатывают запросы клиента из очереди


	//handler на запущенную пандору

	status = -1;				// for future use
}

TSession::TSession(int id_, int user_id_, string ip_, wstring login_, string cookie_, string host_, string port_, int pandora_pid_)
{
	id = id_;
	cookie_sid = cookie_;
	user_id = user_id_;
	start_time = time(0);
	login = login_;
	ip = ip_;
	pandora_pid = pandora_pid_;
	port = port_;
	host = host_;
	last_request_time = start_time;	

	name = L"";
	db_id = 0;
	errors_count = 0;
		
	request_count = 0;		
	bytes_sent = 0;
	bytes_received = 0;		
	threads_count = 0;		

	status = -1;			
}
TSession * TSession::Clone()
{

        TSession * no = new TSession();
		no->object_id = object_id;
//		no->object_type = object_type;
		no->class_name = class_name;
		no->tag = tag;
		no->id = id;
		no->owner_id = owner_id;
		no->owner = owner;
		no->owner_type = owner_type;
		no->ip = ip;
		no->cookie_sid = cookie_sid;
		no->pandora_pid = pandora_pid;
		no->user_id = user_id;
		no->name = name;
		no->login = login;
		no->password = password;
		no->company = company;
		no->host = host;
		no->port = port;
		no->db_id = db_id;
		no->errors_count = errors_count;
		no->start_time = start_time;
		no->last_request_time = last_request_time;
		no->request_count = request_count;
		no->bytes_sent = bytes_sent;
		no->bytes_received = bytes_received;
		no->threads_count = threads_count;
		no->pandora_handler = pandora_handler;
		no->status = status;
		no->status_id = status_id;
		no->notes = notes;
		no->external_key = external_key;

        return no;
}
TClassAttribute * TSession::_GetAttributes()
{
        int count = 1;
        stringstream ss;
        TClassAttribute * root = new TClassAttribute();
        TClassAttribute * a;
        root->id = 1;
        root->name = "";
        root->type = "TSession";
        root->value = "";
        root->is_array = false;
        root->is_class = true;
        count++;

        a = new TClassAttribute();
        a->id = count;
        a->name = "object_id";
        ss << object_id;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "object_type";
//        ss << object_type;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "class_name";
        ss << class_name;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "tag";
        ss << tag;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "id";
        ss << id;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "owner_id";
        ss << owner_id;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "owner";
        ss << owner;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "owner_type";
        ss << owner_type;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "ip";
        ss << ip;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "cookie_sid";
        ss << cookie_sid;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "pandora_pid";
        ss << pandora_pid;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "user_id";
        ss << user_id;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "name";
        ss << TUtility::UnicodeToUtf8(name);
        a->template_name = "";
        a->type = "wstring";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "login";
        ss << TUtility::UnicodeToUtf8(login);
        a->template_name = "";
        a->type = "wstring";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "password";
        ss << TUtility::UnicodeToUtf8(password);
        a->template_name = "";
        a->type = "wstring";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "company";
        ss << TUtility::UnicodeToUtf8(company);
        a->template_name = "";
        a->type = "wstring";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "host";
        ss << host;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "port";
        ss << port;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "db_id";
        ss << db_id;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "errors_count";
        ss << errors_count;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "start_time";
        ss << start_time;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "last_request_time";
        ss << last_request_time;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "request_count";
        ss << request_count;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "bytes_sent";
        ss << bytes_sent;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "bytes_received";
        ss << bytes_received;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "threads_count";
        ss << threads_count;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "pandora_handler";
        ss << pandora_handler;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "status";
        ss << status;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "status_id";
        ss << status_id;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "notes";
        ss << notes;
        a->template_name = "";
        a->type = "string";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "external_key";
        ss << external_key;
        a->template_name = "";
        a->type = "int";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = false;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        
        return root;
}
void TSession::FromJson_(const Value& v)
{

        Object obj = v.get_obj();
		for( Object::size_type i = 0; i != obj.size(); ++i )
		{
			const Pair& pair = obj[i];

			const string& name_  = pair.name_;
			const Value&  value_ = pair.value_;
			Value_type type_ = value_.type(); 
   
                if(name_ == "object_id" && type_ == int_type)
                {
                        object_id = value_.get_int();
                }
				else if(name_ == "object_id" && type_ == str_type)
                {
                        object_id = atoi(value_.get_str().c_str());
                }

                if(name_ == "object_type")
                {
//                        object_type = value_.get_str();
   
                }

                if(name_ == "class_name")
                {
                        class_name = value_.get_str();
   
                }

                if(name_ == "tag")
                {
                        tag = value_.get_str();
   
                }
   
                if(name_ == "id" && type_ == int_type)
                {
                        id = value_.get_int();
                }
				else if(name_ == "id" && type_ == str_type)
                {
                        id = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "owner_id" && type_ == int_type)
                {
                        owner_id = value_.get_int();
                }
				else if(name_ == "owner_id" && type_ == str_type)
                {
                        owner_id = atoi(value_.get_str().c_str());
                }

                if(name_ == "owner")
                {
                        owner = value_.get_str();
   
                }

                if(name_ == "owner_type")
                {
                        owner_type = value_.get_str();
   
                }

                if(name_ == "ip")
                {
                        ip = value_.get_str();
   
                }

                if(name_ == "cookie_sid")
                {
                        cookie_sid = value_.get_str();
   
                }
   
                if(name_ == "pandora_pid" && type_ == int_type)
                {
                        pandora_pid = value_.get_int();
                }
				else if(name_ == "pandora_pid" && type_ == str_type)
                {
                        pandora_pid = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "user_id" && type_ == int_type)
                {
                        user_id = value_.get_int();
                }
				else if(name_ == "user_id" && type_ == str_type)
                {
                        user_id = atoi(value_.get_str().c_str());
                }

                if(name_ == "name")
                {
                        name = TUtility::Utf8ToUnicode(value_.get_str());
                }

                if(name_ == "login")
                {
                        login = TUtility::Utf8ToUnicode(value_.get_str());
                }

                if(name_ == "password")
                {
                        password = TUtility::Utf8ToUnicode(value_.get_str());
                }

                if(name_ == "company")
                {
                        company = TUtility::Utf8ToUnicode(value_.get_str());
                }

                if(name_ == "host")
                {
                        host = value_.get_str();
   
                }

                if(name_ == "port")
                {
                        port = value_.get_str();
   
                }
   
                if(name_ == "db_id" && type_ == int_type)
                {
                        db_id = value_.get_int();
                }
				else if(name_ == "db_id" && type_ == str_type)
                {
                        db_id = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "errors_count" && type_ == int_type)
                {
                        errors_count = value_.get_int();
                }
				else if(name_ == "errors_count" && type_ == str_type)
                {
                        errors_count = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "start_time" && type_ == int_type)
                {
                        start_time = value_.get_int();
                }
				else if(name_ == "start_time" && type_ == str_type)
                {
                        start_time = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "last_request_time" && type_ == int_type)
                {
                        last_request_time = value_.get_int();
                }
				else if(name_ == "last_request_time" && type_ == str_type)
                {
                        last_request_time = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "request_count" && type_ == int_type)
                {
                        request_count = value_.get_int();
                }
				else if(name_ == "request_count" && type_ == str_type)
                {
                        request_count = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "bytes_sent" && type_ == int_type)
                {
                        bytes_sent = value_.get_int();
                }
				else if(name_ == "bytes_sent" && type_ == str_type)
                {
                        bytes_sent = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "bytes_received" && type_ == int_type)
                {
                        bytes_received = value_.get_int();
                }
				else if(name_ == "bytes_received" && type_ == str_type)
                {
                        bytes_received = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "threads_count" && type_ == int_type)
                {
                        threads_count = value_.get_int();
                }
				else if(name_ == "threads_count" && type_ == str_type)
                {
                        threads_count = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "pandora_handler" && type_ == int_type)
                {
                        pandora_handler = value_.get_int();
                }
				else if(name_ == "pandora_handler" && type_ == str_type)
                {
                        pandora_handler = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "status" && type_ == int_type)
                {
                        status = value_.get_int();
                }
				else if(name_ == "status" && type_ == str_type)
                {
                        status = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "status_id" && type_ == int_type)
                {
                        status_id = value_.get_int();
                }
				else if(name_ == "status_id" && type_ == str_type)
                {
                        status_id = atoi(value_.get_str().c_str());
                }

                if(name_ == "notes")
                {
                        notes = value_.get_str();
   
                }
   
                if(name_ == "external_key" && type_ == int_type)
                {
                        external_key = value_.get_int();
                }
				else if(name_ == "external_key" && type_ == str_type)
                {
                        external_key = atoi(value_.get_str().c_str());
                }

		}
        return;
		
}