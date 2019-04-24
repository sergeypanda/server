#ifndef TLANGUAGE_H_HEADER_INCLUDED_B48C915B
#define TLANGUAGE_H_HEADER_INCLUDED_B48C915B

#include "TBaseClass.h"

//serializable 
// [not_serialize] object_type, class_name, owner_id, owner, owner_type
class TLanguage : public TBaseClass
{
  public:
    TLanguage();

    wstring name;
    string short_name;
	string charset;
	bool is_default;
	bool active;

	TClassAttribute * _GetAttributes();
	void FromJson_(const Value& v);
	TLanguage * Clone();

	
	
  };



#endif /* TLANGUAGE_H_HEADER_INCLUDED_B48C915B */
