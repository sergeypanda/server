#include "../include/TClassAttribute.h"
#include "../include/TBaseClass.h"

TClassAttribute::TClassAttribute()
{
	is_array = false;
	is_class = false;
	is_pointer = false;
	serialize = true;
	id = 0;
	obj_ptr = NULL;
}

TClassAttribute::~TClassAttribute()
{
	for(int i = 0; i < (int) nodes.size(); ++i)
	{
		delete nodes.at(i);
	}
	nodes.clear(); 
}