#include "../include/TLogger.h"


TLogger::TLogger(std::string fp)
{
	Init(fp);
}
TLogger::~TLogger()
{
}

void TLogger::FileOpen(void)
{
	mutex.lock();
	file = fopen(file_path.c_str(), "a");
}

/********************************************************/

void TLogger::FileClose(void)
{
	fclose(file);
	mutex.unlock();
}

/********************************************************/

void TLogger::Add(std::string s)
{
	FileOpen();

	str.str("");
	time_t seconds = time(NULL);
	struct tm t;
	LOCALTIME_S;

//	localtime_s(&t, &seconds);
	str << 1900+t.tm_year << "-" << t.tm_mon << "-" << t.tm_mday << " " << t.tm_hour << ":"<< t.tm_min << ":" << t.tm_sec << "\t" ;
	str << s << "\r\n";

	fputs(str.str().c_str(), file);
	str.str("");
	str.str().erase();
	FileClose();
}

void TLogger::Add(std::wstring ws)
{
	FileOpen();

	wstr.str(L"");
	time_t seconds = time(NULL);
	struct tm t;
	LOCALTIME_S;

//	localtime_s(&t, &seconds);

	wstr << 1900+t.tm_year << L"-" << t.tm_mon << L"-" << t.tm_mday << L" " << t.tm_hour << L":"<< t.tm_min << L":" << t.tm_sec << L"\t" ;
	wstr << ws << L"\r\n";

	fputws(wstr.str().c_str(), file);
	wstr.str(L"");
	wstr.str().erase();
	FileClose();
}
void TLogger::Add(int val)
{
	FileOpen();

	wstr.str(L"");
	time_t seconds = time(NULL);
	struct tm t;
	LOCALTIME_S;

//	localtime_s(&t, &seconds);

	wstr << 1900+t.tm_year << L"-" << t.tm_mon << L"-" << t.tm_mday << L" " << t.tm_hour << L":"<< t.tm_min << L":" << t.tm_sec << L"\t" ;
	wstr << val << L"\r\n";

	fputws(wstr.str().c_str(), file);
	wstr.str(L"");
	wstr.str().erase();
	FileClose();
}

void TLogger::Add(char * ch)
{
	FileOpen();

	str.str("");
	time_t seconds;
	time(&seconds);
	struct tm t;
	LOCALTIME_S;

//	localtime_s(&t, &seconds);
	str << 1900+t.tm_year << "-" << t.tm_mon << "-" << t.tm_mday << " " << t.tm_hour << ":"<< t.tm_min << ":" << t.tm_sec << "\t" ;
	str << ch << "\r\n";

	fputs(str.str().c_str(), file);
	str.str("");
	str.str().erase();
	FileClose();
}


/********************************************************/
void TLogger::Init(std::string fp)
{
	if(fp == "")
		file_path = "log.txt";

	else
		file_path = fp;

}
