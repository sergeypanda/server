#ifndef CLASS_ATTRIBUTE_H
#define CLASS_ATTRIBUTE_H

#include <vector>
#include <string>

class TBaseClass;
using namespace std;

class TClassAttribute
{
public:
	int id;
	string name;
	string type;
	string value;
	string template_name;
	bool is_array;
	bool is_class;
	bool is_pointer;
	bool is_map;
	bool serialize;
	string size;
	TBaseClass * obj_ptr;

	vector<TClassAttribute *> nodes;

	TClassAttribute();
	~TClassAttribute();

};
#endif