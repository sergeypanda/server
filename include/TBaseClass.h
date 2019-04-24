#ifndef BASE_CLASS_H
#define BASE_CLASS_H

#include "TMacros.h"
#include "TClassAttribute.h"
#include <json_spirit/json_spirit.h>
#include <json_spirit/json_spirit_reader_template.h>

class TUtility;

using namespace std;
using namespace json_spirit;
//serializable

class TBaseClass
{
public:
	int object_id;
	string object_name;
	string object_type;
	string class_name;
	string tag;

	int id;
    int owner_id;
	string owner;
	string owner_type;

	TBaseClass();
	virtual ~TBaseClass();

	virtual TBaseClass* Clone();

	
	void Init();

	virtual TClassAttribute * _GetAttributes();

	string GetStructureToJson(TClassAttribute * attr,int main_tags, string name);
	string ToJson(TClassAttribute * attr,int main_tags, string name, bool encode_string = true);
	char* ToJsonC(TClassAttribute * attr, int main_tags, string name);
	string ToJsonString(TClassAttribute * attr,int main_tags, string name);
	string ToXml(TClassAttribute * attr, int main_tags, string name);
	string Serialize(int main_tags, string name, string response_type, bool encode_string = true);

	void FromJson(string json);
	virtual void FromJson_(const Value& v);

};

#include "TUtility.h"

#endif
