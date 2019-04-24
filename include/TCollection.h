#ifndef TCOLLECTION_H
#define TCOLLECTION_H

#include "TBaseClass.h"
#include <boost/thread/mutex.hpp>
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_reader_template.h>
#include <json_spirit/json_spirit_writer_template.h>
#include <vector>
#include <string>
//#include "TSerializationJsonCollect.h"
#include "TMacros.h"
#include <map>

using namespace std;
using namespace json_spirit;

template <class T>
class TCollection : public TBaseClass
{

public:

	vector<T*> items;
	vector<int> ids;
	
	string item_type;
	bool is_pointer;

	int count;
	int from;
	int num;
	int total;

	string order_by;
	string search;

	map<string, string> summary;
	boost::mutex tmutex;

	TCollection(string it)
	{
		if (it == "")
			delete this; // 

//		SetSerializator(new TSerializationJsonCollect());

		item_type = it; 
		count = 0;

		object_id = 0;
		id = 0;
		owner_id = 0;

		from = 0;
		num = 0;
		total = 0;

		order_by = "";
		search = "";
	}

	~TCollection()
	{
		for (int i = 0; i < (int) items.size(); ++i)
		{
			try
			{
				SAFEDELETE(items.at(i));
			}
			catch (...)
			{
			}
		}
	}

	void Add(T* item)
	{
		tmutex.lock();
		
		if(item)
			items.push_back(item);
		
		tmutex.unlock();
	}
	void Add(TCollection<T> * new_items)
	{
		tmutex.lock();

		if(new_items)
		{
			items.insert(items.end(), new_items->items.begin(), new_items->items.end());
			new_items->items.erase (new_items->items.begin(), new_items->items.end());
		}

		tmutex.unlock();
	}
	void Insert(T * new_item)
	{
		tmutex.lock();
		
		if(new_item)
			items.insert(items.begin(), new_item);
		
		tmutex.unlock();
	}
	void Insert(TCollection<T> * new_items)
	{
		tmutex.lock();
		if(new_items)
		{
			items.insert(items.begin(), new_items->items.begin(), new_items->items.end());
			new_items->items.erase (new_items->items.begin(), new_items->items.end());
		}
		tmutex.unlock();
	}
	T* Get(int id, bool clone = true)
	{
		tmutex.lock();
		for(int i = 0; i < (int)items.size(); ++i)
		{
			T * tmp = items.at(i);
			if(tmp->id == id)
			{
				tmutex.unlock();
				if(clone)
					return tmp->Clone();
				else
					return tmp;
			}
		}
		tmutex.unlock();
		return NULL;
	}
	T* GetByIndex(int index, bool clone = false)
	{
		if(clone)
			return items.at(index)->Clone();
		else
			return items.at(index);
			
		return NULL;
	}
	TCollection <T> * Get()
	{
		tmutex.lock();

		TCollection <T> * col = this->Clone();

		tmutex.unlock();

		return col;
	}
	TCollection <T> * GetByIds(vector<int> ids)
	{
		TCollection <T> * col = new TCollection <T>(item_type);
		tmutex.lock();
		for(int i = 0; i < (int)ids.size(); ++i)
		{
			for(int j = 0; j < (int)items.size(); ++j)
			{
				if(ids.at(i) == items.at(j)->id)
				{
					col->items.push_back(items.at(j)->Clone());
				}
			}

		}
		tmutex.unlock();

		return col;
	}
	void Remove(int id)
	{
		tmutex.lock();
		for(int i = 0; i < (int)items.size(); ++i)
		{
			if(items.at(i)->id == id)
			{
				delete items.at(i);
				items.erase(remove(items.begin(), items.end(), items.at(i)), items.end());
				break;
			}
		}
		tmutex.unlock();
	}
	void RemoveIdByIndex(int index)
	{
		tmutex.lock();

		if(index < ids.size())
		{
			ids.erase(remove(ids.begin(), ids.end(), ids.at(index)), ids.end());
		}

		tmutex.unlock();
	}

	void RemoveByIndex(int index)
	{
		tmutex.lock();

		if(index < items.size())
		{
			delete items.at(index);
			items.erase(remove(items.begin(), items.end(), items.at(index)), items.end());
		}

		tmutex.unlock();
	}

	void RemoveAll()
	{
		tmutex.lock();
		for(int i = 0; i < (int)items.size(); ++i)
		{
			if (items.at(i))
			{
				delete items.at(i);
				items.erase(remove(items.begin(), items.end(), items.at(i)), items.end());
			}
		}
		tmutex.unlock();
	}

	void Drop(T * item)
	{
		tmutex.lock();
		for(int i = 0; i < (int)items.size(); ++i)
		{
			if(items.at(i) == item)
			{
				items.erase(remove(items.begin(), items.end(), items.at(i)), items.end());
				break;
			}
		}
		tmutex.unlock();
	}
	void Clear()
	{
		tmutex.lock();

		items.erase(items.begin(), items.end());

		tmutex.unlock();
	}

	void Update(T* item)
	{

		tmutex.lock();
		for(int i = 0; i < (int)items.size(); ++i)
		{
			if(items.at(i)->id == item->id)
			{
				delete items.at(i);
				items.at(i) = item;
				break;
			}
		}
		tmutex.unlock();
	}

	int GetSize()
	{
		return items.size();
	}

	int GetIdsSize()
	{
		return ids.size();
	}
	
	string GetStructureToJson(int main_tags, string name)
	{
		stringstream ss;

		if (name != "")
			ss << " \"" << name << "\" :";		

		ss << "{ \"type\" : \"TCollection\", \r\n";
		ss << "\"item_type\" : \"" << item_type << "\", \r\n";
		ss << "\"items\" :[ \r\n"; 

		if (items.size() != 0)
		{
			if (item_type[0] == 'T')
			{
				for(int i = 0; i < (int)items.size(); ++i)
				{ 
					ss << items.at(i)->ToJson(NULL, 1, "");
					if (i+1 != items.size())
						ss << ",";

					ss << " \r\n";
				}
			}
		}	

		ss << " ], \r\n";

		map<string, string>::iterator i;

		ss << "\"summary\" :{";

		i = summary.begin();
		for (int k = 0; k != summary.size(); ++k)
		{

			ss << "\"" << i->first << "\" : \"" << ""  << "\" ";
			if (k+1 != summary.size())
			{
				ss << ",";
			}
			i++;
		}

		ss << "} \r\n";


		ss << " } ";

		string str(ss.str());
		ss.str("");
		ss.clear(); 

		return str;

	}
	TCollection <T>* Clone()
	{
		TCollection <T> * no = new TCollection(item_type);
		no->object_id = object_id;
		no->id = id;
		no->owner_id = owner_id;
		no->item_type = item_type;
		no->count = count;
		no->is_pointer = is_pointer;
		no->from = from;
		no->num = num;
		no->total = total;
		no->order_by = order_by;
		no->search = search;

		for (int i = 0; i < (int)items.size(); ++i)
		{
			T * o =  items.at(i)->Clone();
			no->items.push_back(o);
		}

		return no;
	}
	void FromJson(string json)
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
	void FromJson_(const Value& v)
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

			if(name_ == "id" && type_ == int_type)
			{
				id = value_.get_int();
			}
			else if(name_ == "id" && type_ == str_type)
			{
				id = atoi(value_.get_str().c_str());
			}

			if(name_ == "item_type")
			{
				item_type = value_.get_str();
			}

			if(name_ == "is_pointer")
			{
				is_pointer = value_.get_bool();
			}

			if(name_ == "from" && type_ == int_type)
			{
				from  = value_.get_int();
			}
			else if(name_ == "from" && type_ == str_type)
			{
				from = atoi(value_.get_str().c_str());
			}

			if(name_ == "num" && type_ == int_type)
			{
				num  = value_.get_int();
			}
			else if(name_ == "num" && type_ == str_type)
			{
				num = atoi(value_.get_str().c_str());
			}

			if(name_ == "total" && type_ == int_type)
			{
				total  = value_.get_int();
			}
			else if(name_ == "total" && type_ == str_type)
			{
				total = atoi(value_.get_str().c_str());
			}

			if(name_ == "order_by")
			{
				order_by = value_.get_str();
			}

			if(name_ == "search")
			{
				search = value_.get_str();
			}

			if(name_ == "items")
			{
				const Array arr = value_.get_array();
				for (Array::size_type j = 0; j != arr.size(); ++j)
				{	
					T * q = new T();
					q->FromJson_(arr[j]);
					items.push_back(q);
				}
			}

			if(name_ == "ids")
			{
				const Array arr = value_.get_array();
				for (Array::size_type j = 0; j != arr.size(); ++j)
				{	
					const Value& v =  arr.at(j);
					if(v.type() == int_type)
					{
						ids.push_back(v.get_int());
					}
					else if(v.type() == str_type)
					{
						ids.push_back(atoi(v.get_str().c_str()));
					}
				}
			}


		}	
		return;
	}
	TClassAttribute * _GetAttributes()
	{
		int _count = 1;
		stringstream ss;
		TClassAttribute * root = new TClassAttribute();
		TClassAttribute * a;
		root->id = 1;
		root->name = "";
		root->type = "TCollection";
		root->value = "";
		root->is_array = false;
		root->is_class = true;
		root->obj_ptr = this;
		_count++;

		a = new TClassAttribute();
		a->id = _count;
		a->name = "object_id";
		ss << object_id;
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
		_count++;

		a = new TClassAttribute();
		a->id = _count;
		a->name = "id";
		ss << id;
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
		_count++;


		a = new TClassAttribute();
		a->id = _count;
		a->name = "owner_id";
		ss << owner_id;
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
		_count++;


		a = new TClassAttribute();
		a->id = _count;
		a->name = "total";
		ss << total;
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
		_count++;

		a = new TClassAttribute();
		a->id = _count;
		a->name = "from";
		ss << from;
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
		_count++;

		a = new TClassAttribute();
		a->id = _count;
		a->name = "num";
		ss << num;
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
		_count++;

		a = new TClassAttribute();
		a->id = _count;
		a->name = "count";
		ss << items.size();
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
		_count++;


		a = new TClassAttribute();
		a->id = _count;
		a->name = "items";

		a->type = item_type;
		a->value = ss.str();
		a->is_array = true;
		a->is_map = false;
		a->is_pointer = false;
		a->size = "0";
		a->is_class = true;
		a->serialize = true;

		for(int i = 0; i < (int)items.size(); i++)
		{
			a->nodes.push_back(items[i]->_GetAttributes());
		}

		ss.str("");
		ss.clear();
		root->nodes.push_back(a);
		_count++;


		return root;
	}
};

#endif
