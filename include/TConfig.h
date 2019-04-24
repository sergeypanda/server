#ifndef TCONFIG_H
#define TCONFIG_H

#include "TUtility.h"

#include <map>
#include <fstream>


using namespace std;

class TConfig
{
	public:

		fstream file;
		std::string file_path;

		map<std::string, std::string> content;

		TConfig();

		void ChangeParameterValue(map<string, string> vals);
		void FileOpen(string file_name);
		void FileClose(void);
		void Load(string file_name);

};

#endif
