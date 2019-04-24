#include "../include/TRequestInfo.h"

TRequestInfo::TRequestInfo()
{
	Init();
}
TRequestInfo::TRequestInfo(string command_, string data_, string session_, string key_, string parameters_, 
									  string response_type_)
{
	language = 1;
	command = command_;
	data = data_;
	tag = "1308324036965";
	session = session_;
	key = key_;
	client_id = 1;
	parameters = parameters_;
	response_type = response_type_;
}

TRequestInfo::~TRequestInfo()
{
	command.erase();
	data.erase();
	ip.erase();
	client_type.erase();
}

void TRequestInfo::Init()
{
	command = "";
	data = "";
	client_id = 0;
	db_id = 0;
	language = 0;
	id = 0;
	ip = "";
	client_type = "";
}

TClassAttribute * TRequestInfo::_GetAttributes()
{
        int count = 1;
        stringstream ss;
        TClassAttribute * root = new TClassAttribute();
        TClassAttribute * a;
        root->id = 1;
        root->name = "";
        root->type = "TRequestInfo";
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
 //       ss << //object_type;
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
		a->name = "key";
		ss << key;
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
        a->name = "client_type";
        ss << client_type;
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
        a->name = "client_version";
        ss << client_version;
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
        a->name = "session";
        ss << session;
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
        a->name = "language";
        ss << language;
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
        a->name = "response_type";
        ss << response_type;
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
        a->name = "db_name";
        ss << db_name;
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
        a->name = "db_alias";
        ss << db_alias;
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
		a->name = "script";
		ss << script;
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
        
        return root;
}
void TRequestInfo::FromJson_(const Value& v)
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

                if(name_ == "client_type")
                {
                        client_type = value_.get_str();
   
                }

                if(name_ == "client_version")
                {
                        client_version = value_.get_str();
   
                }

                if(name_ == "ip")
                {
                        ip = value_.get_str();
   
                }

                if(name_ == "session")
                {
                        session = value_.get_str();
   
                }

                if(name_ == "command")
                {
                        command = value_.get_str();
   
                }

                if(name_ == "data")
                {
                        data = value_.get_str();
   
                }

                if(name_ == "parameters")
                {
                        parameters = value_.get_str();
   
                }
   
                if(name_ == "language" && type_ == int_type)
                {
                        language = value_.get_int();
                }
				else if(name_ == "language" && type_ == str_type)
                {
                        language = atoi(value_.get_str().c_str());
                }

                if(name_ == "response_type")
                {
                        response_type = value_.get_str();
   
                }
				if(name_ == "key")
				{
					key = value_.get_str();

				}

                if(name_ == "db_id" && type_ == int_type)
                {
                        db_id = value_.get_int();
                }
				else if(name_ == "db_id" && type_ == str_type)
                {
                        db_id = atoi(value_.get_str().c_str());
                }

                if(name_ == "db_name")
                {
                        db_name = value_.get_str();
   
                }

                if(name_ == "db_alias")
                {
                        db_alias = value_.get_str();
                }

				if(name_ == "script")
				{
					script = value_.get_str();
				}

		}
        return;
		
}
TRequestInfo * TRequestInfo::Clone()
{

        TRequestInfo * no = new TRequestInfo();
		no->object_id = object_id;
//		no->object_type = object_type;
		no->class_name = class_name;
		no->tag = tag;
		no->id = id;
		no->owner_id = owner_id;
		no->owner = owner;
		no->owner_type = owner_type;
		no->client_id = client_id;
		no->client_type = client_type;
		no->client_version = client_version;
		no->ip = ip;
		no->session = session;
		no->key = key;
		no->command = command;
		no->data = data;
		no->parameters = parameters;
		no->language = language;
		no->response_type = response_type;
		no->db_id = db_id;
		no->db_name = db_name;
		no->db_alias = db_alias;
		no->script = script;

        return no;
}