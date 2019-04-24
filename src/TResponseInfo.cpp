#include "../include/TResponseInfo.h"


TResponseInfo::TResponseInfo()
{
	Init();
}

TResponseInfo::~TResponseInfo()
{
}

void TResponseInfo::ParseResponseString(string response)
{
	int i = response.find("\r\n\r\n", 0)+4;
	string res = "";
	if(i != -1)
	{
		res = response.substr(i, response.length() - i);
	}
	if(res != "")
	{
		FromJson(res);
	}
}

void TResponseInfo::Init()
{
	server_version = _VERSION;
	succesfull = 0;
	command = "";
	data = "";
	client_id = 0;
	server_time = time(0);	
	request_id = "";
	error = "";
}

string TResponseInfo::ToXml(int mode) // 0 -complete xml, 1 only header, 2 only footer
{
	stringstream ss;
	
	if(mode == 0 || mode == 1)
	{
		ss << "<TResponseInfo type=\"xml\" request_id=\"" << request_id << "\" client_id=\"" << client_id << "\" server_time=\"" << server_time << "\" exec_time=\"" << exec_time << "\"> " ;
		ss << "<command>" << command << "</command>";
		ss << "<parameters>" << parameters << "</parameters>";
		ss << "<succesfull>" << succesfull << "</succesfull>";
		ss << "<message_code>" << message_code << "</message_code>";
		ss << "<message>" << message << "</message>";
		ss << "<lang_id>" << lang_id << "</lang_id>";
		ss << "<data>";
	}

	if(mode == 0)
		ss << data;

	if(mode == 0 || mode == 2)
	{
		ss << "</data>";
		ss << "<error>" << error << "</error>";
		ss << "</TResponseInfo>";
	}



	string s(ss.str());
	ss.clear(); 
	return s;
}

string TResponseInfo::ToJson(int mode) // 0 -complete xml, 1 only header, 2 only footer
{
	stringstream ss;

	if(mode == 0 || mode == 1)
	{
		ss << "{\"class_name\":\"TResponseInfo\"," << " \"version\":\"" << server_version << "\", \"request_id\":\"" << request_id << "\", \"client_id\":" << client_id << ", \"server_time\":" << server_time << ", \"exec_time\":" << exec_time << ", " ;
//		ss << "\"license_number\": \"" << TLicense::GetInstance()->number << "\", ";
//		ss << "\"license_expire\": \"" << TLicense::GetInstance()->expire << "\", ";
		ss << "\"command\": \"" << command << "\", ";
		ss << "\"parameters\":\"" << parameters << "\", ";
		ss << "\"succesfull\":" << succesfull << ", ";
		ss << "\"message_code\":\"" << message_code << "\",";
		ss << "\"message\":\"" << message << "\",";
		ss << "\"lang_id\":" << lang_id << ", ";
		ss << "\"data\":\"";
	}
	if(mode == 0)
		ss << data;

	if(mode == 0 || mode == 2)
	{
		ss << "\", ";
		ss << "\"error\":\"" << error << "\" ";
	}

	ss << "}";

	string s(ss.str());
	ss.clear(); 
	return s;
}

void TResponseInfo::AddData(string data)
{
	data = TUtility::UriEncode(data);
}
TClassAttribute * TResponseInfo::_GetAttributes()
{
        int count = 1;
        stringstream ss;
        TClassAttribute * root = new TClassAttribute();
        TClassAttribute * a;
        root->id = 1;
        root->name = "";
        root->type = "TResponseInfo";
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
        a->serialize = false;
        

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
        a->serialize = false;
        

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
        a->serialize = false;
        

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
        a->serialize = false;
        

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
        a->serialize = false;
        

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
        a->serialize = false;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "client_id";
        ss << client_id;
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
        a->name = "request_id";
        ss << request_id;
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
        a->name = "server";
        ss << server;
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
        a->name = "server_version";
        ss << server_version;
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
        a->name = "server_pid";
        ss << server_pid;
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
        a->name = "server_port";
        ss << server_port;
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
        a->name = "command";
        ss << command;
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
        a->name = "data";
        ss << data;
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
        a->name = "error";
        ss << error;
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
        a->name = "parameters";
        ss << parameters;
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
        a->name = "succesfull";
        ss << succesfull;
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
        a->name = "message_code";
        ss << message_code;
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
        a->name = "message";
        ss << message;
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
        a->name = "lang_id";
        ss << lang_id;
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
        a->name = "server_time";
        ss << server_time;
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
        a->name = "exec_time";
        ss << exec_time;
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
void TResponseInfo::FromJson_(const Value& v)
{

        Object obj = v.get_obj();
		for( Object::size_type i = 0; i != obj.size(); ++i )
		{
			const Pair& pair = obj[i];

			const string& name_  = pair.name_;
			const Value&  value_ = pair.value_;
			Value_type type_ = value_.type(); 

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
   
                if(name_ == "client_id" && type_ == int_type)
                {
                        client_id = value_.get_int();
                }
				else if(name_ == "client_id" && type_ == str_type)
                {
                        client_id = atoi(value_.get_str().c_str());
                }

                if(name_ == "request_id")
                {
                        request_id = value_.get_str();
   
                }

                if(name_ == "server")
                {
                        server = value_.get_str();
   
                }

                if(name_ == "server_version")
                {
                        server_version = value_.get_str();
   
                }

                if(name_ == "server_pid")
                {
                        server_pid = value_.get_str();
   
                }

                if(name_ == "server_port")
                {
                        server_port = value_.get_str();
   
                }

                if(name_ == "command")
                {
                        command = value_.get_str();
   
                }

                if(name_ == "data")
                {
                        data = value_.get_str();
   
                }

                if(name_ == "error")
                {
                        error = value_.get_str();
   
                }

                if(name_ == "parameters")
                {
                        parameters = value_.get_str();
   
                }
   
                if(name_ == "succesfull" && type_ == int_type)
                {
                        succesfull = value_.get_int();
                }
				else if(name_ == "succesfull" && type_ == str_type)
                {
                        succesfull = atoi(value_.get_str().c_str());
                }

                if(name_ == "message_code")
                {
                        message_code = value_.get_str();
   
                }

                if(name_ == "message")
                {
                        message = value_.get_str();
   
                }
   
                if(name_ == "lang_id" && type_ == int_type)
                {
                        lang_id = value_.get_int();
                }
				else if(name_ == "lang_id" && type_ == str_type)
                {
                        lang_id = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "server_time" && type_ == int_type)
                {
                        server_time = value_.get_int();
                }
				else if(name_ == "server_time" && type_ == str_type)
                {
                        server_time = atoi(value_.get_str().c_str());
                }
   
                if(name_ == "exec_time" && type_ == int_type)
                {
                        exec_time = value_.get_int();
                }
				else if(name_ == "exec_time" && type_ == str_type)
                {
                        exec_time = atoi(value_.get_str().c_str());
                }

		}
        return;
		
}
TResponseInfo * TResponseInfo::Clone()
{

        TResponseInfo * no = new TResponseInfo();
		no->object_id = object_id;
//		no->object_type = object_type;
		no->class_name = class_name;
		no->tag = tag;
		no->id = id;
		no->owner_id = owner_id;
		no->owner = owner;
		no->owner_type = owner_type;
		no->client_id = client_id;
		no->request_id = request_id;
		no->server = server;
		no->server_version = server_version;
		no->server_pid = server_pid;
		no->server_port = server_port;
		no->command = command;
		no->data = data;
		no->error = error;
		no->parameters = parameters;
		no->succesfull = succesfull;
		no->message_code = message_code;
		no->message = message;
		no->lang_id = lang_id;
		no->server_time = server_time;
		no->exec_time = exec_time;

        return no;
}