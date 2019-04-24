#ifndef TUTILITY_H
#define	TUTILITY_H

#include "TMacros.h"
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#include <boost/algorithm/string.hpp>
#include <vector>
#include <stdlib.h>
#include <stdarg.h>

#include "TLogger.h"

class TError;
class TBaseClass;

extern TLogger * logger;
using namespace std;

class TUtility
{
private:
	static int gmt;

public:

	TUtility();
	static void SetGMT(int _gmt);
	static int GetGMT();
	static int _time();
	static int GetDateInterval(int& date, wstring& interval, bool to_highest);
	static void UtilitySleep(long milliSeconds);
	static unsigned long GetTimeMilliseconds();
	static string RemoveSpaces(string& str);
	static wstring RemoveSpaces(wstring& wstr);
	static string AddSpaces(string str);
	static string cchar_to_str(const char * str);
	static char * str_to_cstr(const string& s);
	static wstring AnsiToUnicode(string &ansistr);
	static wstring Utf8ToUnicode(const string& utf8str);
	static string UnicodeToUtf8(const wstring& unicodestr);
	static string UnicodeToAnsi(wstring &unicodestr);
	static string BoolToString(bool val, bool num = true);
	static bool StringToBool(string str);
	static bool StringToBool(wstring str);
	static string GetTime(int year);
	static int GetServerTime();
	static vector<char> StringToVector(string str, vector<char> vec);
	static vector<char> VectorMerge(vector<char> vec1, vector<char> vec2, int pos);
	static string& str_replace(const string &search, const string &replace, string &subject);
	static wstring str_replace(const wstring &search, const wstring &replace, wstring &subject);
	static wstring replace_html_entity(wstring& str);
	static string replace_html_entity(string& str);

	static string UrlDecode(string &s);
	static bool is_numeric(const std::string& s);
	static bool is_numeric(const std::wstring& s);
	static int get_fractional(double val);

	static double get_rounded(double val, int dig, bool to_highest);
	static wstring get_rounded(double val, int dig, bool to_highest, int precision);
	static void get_rounded(wstring& val, int precision);
	static void  get_rounded(string& val, int precision);

	static string ToXml(int val, string name);
	static string ToXml(string val, string name);
	static string ToXml(vector<int> vec, string name);
	static string ToXml(map<string, string> params);

	static string ToJson(int val, string name);
	static string ToJson(string val, string name);
	static string ToJson(vector<int> vec, string name);
	static string ToJson(vector<string> vec, string name);
	static string ToJson(vector< map <wstring, wstring> > vec, string name);
	static string ToJson(map<string, string> params);
	static string ToJson(map<wstring, wstring> params);

	static string Serialize(map<string, string> params, string response_type);

	static int wstoi(wstring str);
	static int wstoi(wchar_t h);
	static double wstof(wstring str);
	static wstring itows(int val);
	static string itoa(int val);
	static wstring ftows(double val);
	static string ftoa(double val);
	static string base64_decode(string const& encoded_string);
	static string base64_encode(char const* bytes_to_encode, unsigned int in_len);
	static bool is_base64(unsigned char c);
	static wstring itowords(int numb);
	static string AddQuotes(string str);
	static string UriDecode(const std::string & sSrc);
	static string UriEncode(const std::string & sSrc);

	static bool FileExists(string path);
	static bool CheckZeroValue(int el_num, bool throw_error, int val ...);
	static bool CheckZeroValue(int el_num, bool throw_error, TBaseClass * obj ...);
//	static void CheckZeroValue(int el_num, TCollection<TBaseClass> * col ...);

	static int GetProgramPid();
	static void TimeElapsed(long double & start_time, string func_name);

	static void RandomNum(std::vector<int> & vec);
	static bool doWork(std::function<bool(int)> filter, int maxVal = 1'000'000);
};


#endif
