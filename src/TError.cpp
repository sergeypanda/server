#include "../include/TError.h"

TError::TError()
{
	fatal = true;
	code = "";
	message = L"";
	data = L"";

}




TError * TError::Clone()
{

        TError * no = new TError();
		no->object_id = object_id;
		no->object_name = object_name;
		no->class_name = class_name;
		no->tag = tag;
		no->id = id;
		no->owner_id = owner_id;
		no->owner = owner;
		no->owner_type = owner_type;
		no->fatal = fatal;
		no->code = code;
		no->message = message;
		no->data = data;

        return no;
}
TClassAttribute * TError::_GetAttributes()
{
        int count = 1;
        stringstream ss;
        TClassAttribute * root = new TClassAttribute();
        TClassAttribute * a;
        root->id = 1;
        root->name = "";
        root->type = "TError";
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
        a->name = "object_name";
        ss << object_name;
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
        a->serialize = true;
        

        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        

        a = new TClassAttribute();
        a->id = count;
        a->name = "fatal";
        
        if (fatal == true)
                ss << "1";
        else
                ss << "0";
                        
        a->template_name = "";
        a->type = "bool";
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
        a->name = "code";
        ss << code;
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
        ss << TUtility::UnicodeToUtf8(message);
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
        a->name = "data";
        ss << TUtility::UnicodeToUtf8(data);
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
        a->name = "params";
        
        a->template_name = "";
        a->type = "map";
        a->value = ss.str();
        a->is_array = false;
        a->is_map = true;
        a->is_pointer = false;
        a->size = "0";
        a->is_class = false;
        a->serialize = true;
        

        map<string, string>::iterator i;

		for (i = params.begin(); i != params.end(); ++i)
		{

 			TClassAttribute * at = new TClassAttribute();
	        at->id = count;
	        ss << i->first;
	        at->name = ss.str();
	        ss.str("");
   			ss.clear();
   			ss << i->second;
	        at->type = "string";
	        at->value = ss.str();
	        at->is_array = false;
	        at->is_map = false;
	        at->is_pointer = false;
	        at->size = "0";
	        at->is_class = false;
	        at->serialize = true;

	        ss.str("");
   			ss.clear();
   			a->nodes.push_back(at);
  			    count++;
		}
                
        ss.str("");
        ss.clear();
        root->nodes.push_back(a);
        count++;
        
        return root;
}
void TError::FromJson_(const Value& v)
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

                if(name_ == "owner_type")
                {
                        owner_type = value_.get_str();
   
                }

                if(name_ == "fatal" && type_ == bool_type)
                {
                        fatal = value_.get_bool();
                }
				else if(name_ == "fatal" && type_ == str_type)
                {
                        fatal = TUtility::StringToBool(value_.get_str());
                }

                if(name_ == "code")
                {
                        code = value_.get_str();
   
                }

                if(name_ == "message")
                {
                        message = TUtility::Utf8ToUnicode(value_.get_str());
                }

                if(name_ == "data")
                {
                        data = TUtility::Utf8ToUnicode(value_.get_str());
                }

		}
        return;
		
}