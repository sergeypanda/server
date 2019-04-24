
#include "../include/TBaseClass.h"


TBaseClass::TBaseClass()
{
	Init();
}

void TBaseClass::Init()
{
	object_id = 0;
	id = 0;
	owner_id = 0;
}

TBaseClass::~TBaseClass()
{

}

string TBaseClass::ToXml(TClassAttribute * attr,int main_tags, string name)
{
	bool nl = false;
	stringstream xml;
	if(attr == NULL)
	{
		nl = true;
		attr = _GetAttributes();
	}
	if(name.length() == 0)
	{
		name = attr->type;
	}

	if(main_tags)
	{
		xml << "<" << name << " type=\"" << attr->type << "\">"  << "\r\n";
	}
	if(name.length() && attr->value != "" && !attr->nodes.size())
	{
		xml << attr->value;
	}

	TClassAttribute * tt;

	for(int j = 0; j < (int)attr->nodes.size(); ++j)
	{
		tt = attr->nodes.at(j);

		if(tt->serialize == false)
			continue;

		if(tt->is_array)
		{
			xml << "\t<" << tt->name << " is_array = \"true\" " << ">\n ";
			xml << "<array_items>\r\n";
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				xml << "\t\t" << ToXml(tt->nodes[i], 1, "array_item");
			}
			xml << "</" << tt->name << ">" << "</array_items>\r\n";
		}
		else if(tt->is_class)
		{
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				xml << ToXml(tt->nodes[i], 1  , tt->name);
			}
		} 
		else if(tt->is_map)
		{
			xml << "\t<" << tt->name <<">\n ";
			xml << "<array_items>\r\n";
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				xml << "\t\t" << ToXml(tt->nodes[i], 1, "array_item");
			}
			xml << "</" << tt->name << ">" << "</array_items>\r\n";
		}
		else 
		{
			xml << "\t<" << tt->name << " type=\"" << tt->type << "\">";

			if (tt->type == "string" || tt->type == "wstring")
			{
				xml << TUtility::replace_html_entity(tt->value);
			}
			else
				xml << tt->value;

			xml << "</" << tt->name << ">" << "\r\n";
		}

	}

	if(main_tags)
	{
		xml << "</" << name << ">\r\n";
	}

	string str(xml.str());
	xml.str("");
	xml.clear(); 

	if(nl)
	{
		delete attr;
	}
	
	return str;
}
string TBaseClass::Serialize(int main_tags, string name, string response_type, bool encode_string)
{
	string str;

	if (response_type == "xml")
	{
		str = ToXml(NULL, main_tags, name);	
	}
	else
		str = ToJson(NULL, main_tags, name, encode_string);
		

	return str;
}
string TBaseClass::ToJson(TClassAttribute * attr,int main_tags, string name, bool encode_string) //shade - ekranit' ili net
{
	bool nl = false;
	stringstream json;
	if(attr == NULL)
	{
		nl = true;
		attr = _GetAttributes();
	}
	if (name != "")
		json << "\"" << name << "\":";

	if(main_tags)
	{
		json << "{\"type\":\"" << attr->type << "\","  << "";
	}
	if(name.length() && attr->value != "" && !attr->nodes.size())
	{
		if(encode_string)
			TUtility::str_replace("\"", "\\\"", attr->value);
		json << attr->value;
	}

	TClassAttribute * tt;

	for(int j = 0; j < (int)attr->nodes.size(); ++j)
	{
		tt = attr->nodes.at(j);

		if(tt->serialize == false)
			continue;

		if(tt->is_array)
		{
			json << "\"" << tt->name << "\":[" << "";
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				json << "" << ToJson(tt->nodes[i], 1, "", encode_string);

				if (i+1 != tt->nodes.size())
					json << ",";
			}
			json << "]" << "";	

		}
		else if(tt->is_class)
		{
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				json << ToJson(tt->nodes[i], 1  , tt->name, encode_string);
				if (i+1 != tt->nodes.size())
					json << ",";
			}
		} 
		else if (tt->is_map)
		{
			json << "\"" << tt->name << "\" :{ \"type\" : \"map\", \"items\" : { ";

			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				string v = tt->nodes[i]->value;
				if(encode_string)
					 v = TUtility::UriEncode(v);

				json << "\"" << tt->nodes[i]->name <<"\" : \"" << v <<"\"";
				if (i+1 != tt->nodes.size())
					json << ", ";
			}
			json << "}}" << "";	
		}
		else 
		{
			json << "\"" << tt->name << "\": ";

			if (tt->type == "string" || tt->type == "wstring")
			{
				json << "\"";
				if(encode_string)
				{
					tt->value = TUtility::UriEncode(tt->value);
				}
				
				json << tt->value;
				json << "\"";
			}
			else
				json << tt->value;

		}

		if (j+1 != attr->nodes.size())
			json << ",";
		json << "";
	}

	if(main_tags)
	{
		json << "}";
	}

	string str(json.str());
	json.str("");
	json.clear(); 

	if(nl)
	{
		delete attr;
	}

	return str;
}

char * TBaseClass::ToJsonC(TClassAttribute * attr,int main_tags, string name)
{
	bool nl = false;
	char *xml = "";
	
	if(attr == NULL)
	{
		nl = true;
		attr = _GetAttributes();
	}
	if (name != "")
	{
		const char * n = name.c_str();
		int len = strlen(n)+8;


		char* ch = (char *)malloc(len* sizeof(char));
		
		strcpy(ch, "\""); 
		strcat(ch, n);
		strcat(ch, "\":");
		xml = (char*)malloc(len * sizeof(char));
		strcpy(xml, ch);
	}

	if(main_tags)
	{
		int len = strlen(attr->type.c_str())+18;

		char* c = (char *)malloc((len)* sizeof(char));

		strcpy(c, "{\"type\":\"");
		strcat(c, attr->type.c_str());
		strcat(c, "\",");
		xml = (char*)malloc(len* sizeof(char));
		strcpy(xml, c);
	}
	if(name.length() && attr->value != "" && !attr->nodes.size())
	{
		char * tmp = xml;
		xml = (char*)malloc((strlen(tmp)+ strlen(attr->value.c_str())+1)* sizeof(char));
		strcpy(xml, tmp);
	}

	TClassAttribute * tt;

	for(int j = 0; j < (int)attr->nodes.size(); ++j)
	{
		tt = attr->nodes.at(j);

		if(tt->serialize == false)
			continue;

		if(tt->is_array)
		{
			char * tmp = xml;
			const char *n = tt->name.c_str();
			int len = strlen(tmp)+ strlen(n)+6+3;

			xml = (char*)malloc(len* sizeof(char));
			strcpy(xml, tmp);
			strcat(xml, "\"");
			strcat(xml, n);
			strcat(xml, "\":[");

			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				char * tmp = xml;
				char * c = ToJsonC(tt->nodes[i], 1, "");
				len = strlen(tmp)+ strlen(c) + tt->nodes.size()+1;
				xml = (char*)malloc(len* sizeof(char));
				strcpy(xml, tmp);
				strcat(xml, c);
				if (i+1 != tt->nodes.size())
					strcat(xml, ",");
			}
			strcat(xml, "]");	

		}
		else if(tt->is_class)
		{
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				char * tmp = xml;
				char * c = ToJsonC(tt->nodes[i], 1, tt->name);
				int len = strlen(tmp)+ strlen(c) + tt->nodes.size()+1;
				xml = (char*)malloc(len* sizeof(char));
				strcpy(xml, tmp);
				strcat(xml, c);
				if (i+1 != tt->nodes.size())
					strcat(xml, ",");
			}
		} 
		else if (tt->is_map)
		{
			strcat(xml, "\"");
			strcat(xml, tt->name.c_str());
			strcat(xml, "\" :{ \"type\" : \"map\", \"items\" : { ");

			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				strcat(xml, "\"");
				strcat(xml, tt->nodes[i]->name.c_str());
				strcat(xml, "\" : \"");
				strcat(xml, tt->nodes[i]->value.c_str());
				strcat(xml, "\"");
				if (i+1 != tt->nodes.size())
					strcat(xml, ", ");
			}
			strcat(xml, "}}");	
		}
		else 
		{
			char * tmp = xml;
			const char * n = tt->name.c_str();
			const char * v = tt->value.c_str();
			int len = strlen(tmp)+ strlen(n)+1 + strlen(v)+1+attr->nodes.size()+9;
			xml = (char*)malloc(len* sizeof(char));
			strcpy(xml, tmp);

			strcat(xml, "\"");
			strcat(xml, n);
			strcat(xml, "\":\"");
			strcat(xml, v);
			strcat(xml, "\"");
		}
		
		if (j+1 != attr->nodes.size())
		{
			char * tmp = xml;
			int len = strlen(xml)+2;
			xml = (char*)malloc(len* sizeof(char));
			strcpy(xml, tmp);
			strcat(xml, ",");
		}
	}

	if(main_tags)
	{
		char * tmp = xml;
		int len = strlen(xml)+2;
		xml = (char*)malloc(len* sizeof(char));
		strcpy(xml, tmp);
		strcat(xml, "}");
	}


	if(nl)
	{
		delete attr;
	}

	return xml;
}
string TBaseClass::ToJsonString(TClassAttribute * attr,int main_tags, string name)
{
	bool nl = false;
	string xml;
	if(attr == NULL)
	{
		nl = true;
		attr = _GetAttributes();
	}
	if (name != "")
	{
		xml += "\"";
		xml += name;
		xml += "\":";
	}

	if(main_tags)
	{
		xml += "{\"type\":\""; 
		xml += attr->type;
		xml += "\",";
	}
	if(name.length() && attr->value != "" && !attr->nodes.size())
	{
		xml += attr->value;
	}

	TClassAttribute * tt;

	for(int j = 0; j < (int)attr->nodes.size(); ++j)
	{
		tt = attr->nodes.at(j);

		if(tt->serialize == false)
			continue;

		if(tt->is_array)
		{
			xml += "\"";
			xml += tt->name;
			xml += "\":[";

			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				xml += ToJsonString(tt->nodes[i], 1, "");
				if (i+1 != tt->nodes.size())
					xml += ",";
			}
			xml += "]";	

		}
		else if(tt->is_class)
		{
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				xml += ToJsonString(tt->nodes[i], 1  , tt->name);
				if (i+1 != tt->nodes.size())
					xml += ",";
			}
		} 
		else if (tt->is_map)
		{
			xml += "\"";
			xml += tt->name;
			xml += "\" :{ \"type\" : \"map\", \"items\" : { ";

			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				xml += "\"";
				xml += tt->nodes[i]->name;
				xml += "\" : \"";
				xml += tt->nodes[i]->value;
				xml += "\"";
				if (i+1 != tt->nodes.size())
					xml += ", ";
			}
			xml += "}}";	
		}
		else 
		{
			xml += "\"";
			xml += tt->name;
			xml += "\": ";
			if (tt->type == "string" || tt->type == "wstring")
				xml += "\"";

			xml += tt->value;

			if (tt->type == "string" || tt->type == "wstring")
				xml += "\"";
		}

		if (j+1 != attr->nodes.size())
			xml += ",";
	}

	if(main_tags)
	{
		xml += "}";
	}

	if(nl)
	{
		delete attr;
	}

	return xml;
}
string TBaseClass::GetStructureToJson(TClassAttribute * attr,int main_tags, string name)
{
	bool nl = false;
	stringstream xml;
	if(attr == NULL)
	{
		nl = true;
		attr = _GetAttributes();
	}
	if (name != "")
		xml << "\"" << name << "\" :";

	if(main_tags)
	{
		xml << "{ \"type\" : \"" << attr->type << "\","  << "\r\n";
	}

	TClassAttribute * tt;

	for(int j = 0; j < (int)attr->nodes.size(); ++j)
	{
		tt = attr->nodes.at(j);

		if(tt->serialize == false)
			continue;

		if(tt->is_array)
		{
			xml << "\t \"" << tt->name << "\" :[" << "\r\n";
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				xml << "\t\t" << ToJson(tt->nodes[i], 1, "");
				if (i+1 != tt->nodes.size())
					xml << ", ";
			}
			xml << "]" << "\r\n";	

		}
		else if(tt->is_class)
		{
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				xml << ToJson(tt->nodes[i], 1  , tt->name);
				if (i+1 != tt->nodes.size())
					xml << ", ";
			}
		} 
		else if (tt->is_map)
		{
			xml << "\t \"" << tt->name << "\" :[" << "\r\n";
			for ( int i = 0; i < (int)tt->nodes.size(); i++)
			{
				xml << "\t\t" << ToJson(tt->nodes[i], 1, "");
				if (i+1 != tt->nodes.size())
					xml << ", ";
			}
			xml << "]" << "\r\n";	
		}
		else 
		{
			xml << "\t \"" << tt->name << "\" : \"" << "";
			xml << "\" ";
		}

		if (j+1 != attr->nodes.size())
			xml << ", ";
		xml << "\r\n";
	}

	if(main_tags)
	{
		xml << " }";
	}

	string str(xml.str());
	xml.str("");
	xml.clear(); 

	if(nl)
	{
		delete attr;
	}

	return str;
}
void TBaseClass::FromJson(string json)
{
	if(json == "")
	{
		return ;
	}

	json = TUtility::UriDecode(json);

	Value v;

	try{

		read_string_or_throw( json, v );
		FromJson_(v);

	}
	catch(json_spirit::Error_position e )
	{
		cout << e.reason_.c_str();
		cout << L"json_spirit::Error_position exception raised.";
		throw e;
	}
	catch(std::runtime_error e )
	{
		cout << L"std::runtime_error exception raised.";
		throw e;
	}
	catch(...)
	{
		cout << L"json_spirit::read_string exception raised.";
		throw 0;
	}

}
























































































TBaseClass * TBaseClass::Clone()
{

        TBaseClass * no = new TBaseClass();
		no->object_id = object_id;
		no->object_name = object_name;
		no->class_name = class_name;
		no->tag = tag;
		no->id = id;
		no->owner_id = owner_id;
		no->owner = owner;
		no->owner_type = owner_type;

        return no;
}
TClassAttribute * TBaseClass::_GetAttributes()
{
        int count = 1;
        stringstream ss;
        TClassAttribute * root = new TClassAttribute();
        TClassAttribute * a;
        root->id = 1;
        root->name = "";
        root->type = "TBaseClass";
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
        
        return root;
}
void TBaseClass::FromJson_(const Value& v)
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

                if(name_ == "object_name")
                {
                        object_name = value_.get_str();
   
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

		}
        return;
		
}