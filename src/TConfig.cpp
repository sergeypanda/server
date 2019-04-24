#include "../include/TConfig.h"

TConfig::TConfig()
{

}

void TConfig::FileOpen(string file_name)
{

	file.open(file_name.c_str(), ios::in);

	if(!file.is_open())
	{
		cout<< "Cannot open config file\r\n";
		system("pause");
	}
	file_path = file_name;
}
void TConfig::FileClose(void)
{
	file.close();
}

void TConfig::Load(string file_name)
{
	FileOpen(file_name);

 	file.seekg (0, ios::end);
 	int length = file.tellg();
 	file.seekg (0, ios::beg);

	char buffer[1024];

	while(!file.eof())
	{

		file.getline(buffer, sizeof(buffer));

		string str_buf(buffer);


		if(str_buf == "" || str_buf == "\r") continue;

		size_t begin = 0, end = 0, diff = 0;
		string name, value;

		end = str_buf.find("=", begin);
		if(end == -1)
			continue;
		diff = end - begin;

		name.append(str_buf, begin, diff);
		name = TUtility::RemoveSpaces(name);
		begin = end;
		begin += 1;

		begin = str_buf.find_first_not_of(" ", begin);
		if(begin == -1)
			continue;

		end = str_buf.find(" \r\n", begin);
		if(end == -1)
		{
			end = str_buf.length();
		}
		diff = end - begin;

		value.append(str_buf, begin, diff);
		begin = end;

        TUtility::str_replace("\r", "", value);
        TUtility::str_replace("\n", "", value);

		content[name] = value;
		name.clear();
		value.clear();


	}

	FileClose();
}

void TConfig::ChangeParameterValue(map<string, string> vals)
{

	FileOpen(file_path);

	char buffer[1024];
	string tmp_file, old_val;

	while (file.read(buffer, sizeof(buffer)).gcount() > 0)
	{
		tmp_file.append(buffer, file.gcount());
	}
	map<string, string>::iterator it;

	for(it = vals.begin(); it != vals.end(); it++)
	{
		old_val = content[it->first];

		logger->Add(old_val);
		TUtility::str_replace(old_val, it->second, tmp_file);
	}

	FileClose();

	ofstream new_file ("ok-server.conf");
	new_file << tmp_file;
	new_file.close();

}


