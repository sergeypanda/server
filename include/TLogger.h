#ifndef TLOGGER_H
#define TLOGGER_H

#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <boost/thread/mutex.hpp>

#ifdef _WIN32
#define LOCALTIME_S localtime_s (&t, &seconds );
#else
#include <stdio.h>
#define LOCALTIME_S localtime_r (&seconds, &t);
#endif


using namespace std;
class TLogger
{
public:
	boost::mutex mutex;
	stringstream str;
	wstringstream wstr;
	FILE *file;
	string file_path;

	TLogger(string fp);
	~TLogger();

	void FileOpen(void);
	void FileClose(void);
	void Init(string file);

    void Add(int val);
	void Add(string str);
	void Add(wstring ws);
	void Add(char * ch);


};

#endif
