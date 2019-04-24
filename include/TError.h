#ifndef TERROR_H_
#define TERROR_H_

#include "TBaseClass.h"
//serializable
// [not_serialize] object_name, class_name, owner

using namespace std;
class TError : public TBaseClass
{
public:
	bool fatal;

	string code;
	wstring message;
	wstring data;
	map<string, string> params;

	TError();

	TClassAttribute * _GetAttributes();
	void FromJson_(const Value& v);
	TError * Clone();

  };



#endif /* TERROR_H_ */
