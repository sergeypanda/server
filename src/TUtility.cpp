

#include <boost/lexical_cast.hpp>
#include <random>
#include <thread>
#include "../include/TUtility.h"
#include "../include/TBaseClass.h"
#include "../include/TError.h"

int TUtility::gmt = 0;


TUtility::TUtility()
{

}
void TUtility::SetGMT(int _gmt)
{
	gmt = _gmt;
}

int TUtility::_time()
{
	return time(0) + GetGMT();
}
int TUtility::GetGMT()
{
	return gmt;
}
int TUtility::GetDateInterval(int& date, wstring& interval, bool to_highest)
{
	int result = 0;
	int days_in_year = 365;

	int days_in_month[][2] = 
	{
		{0, 31},
		{1, 29},
		{2, 31},
		{3, 30},
		{4, 31},
		{5, 30},
		{6, 31},
		{7, 31},
		{8, 30},
		{9, 31},
		{10, 30},
		{11, 31}
	};

	time_t tt = date;
	tm* lt = localtime(&tt);
	if(!lt)
		return 0;

	bool leapsum =(!(lt->tm_year+1900%4)&&(lt->tm_year+1900%100))||!(lt->tm_year+1900%400);

	if(leapsum)
	{
		days_in_year = 366;
		days_in_month[1][1] = 28;
	}

	if(!(interval == L"day" || interval == L"week" || interval == L"month" || interval == L"year"))
		interval = L"day";

	if(interval == L"day")
	{
		result = 86400; 
	}
	else if(interval == L"week")
	{
		if(to_highest)
			lt->tm_mday += 7 - (lt->tm_wday -1);
		else
			lt->tm_mday -= lt->tm_wday -1;

		result = 86400 * 7;
	}
	else if(interval == L"month")
	{
		if(to_highest)
		{
			lt->tm_mon += 1;
			lt->tm_mday = 1;
		}
		else
			lt->tm_mday = 1;

		result = 86400 * days_in_month[lt->tm_mon][1];
	}
	else if(interval == L"year")
	{
		if(to_highest)
		{
			lt->tm_year += 1;
			lt->tm_mon = 0;
			lt->tm_mday = 1;
		}
		else
		{
			lt->tm_mon = 0;
			lt->tm_mday = 1;
		}

		result = 86400 * days_in_year;
	}
	if(interval == L"day" && to_highest)
	{
		lt->tm_hour = 23;
		lt->tm_min = 59;
		lt->tm_sec = 59;
	}
	else
	{
		lt->tm_hour = 0;
		lt->tm_min = 0;
		lt->tm_sec = 0;
	}

	date = mktime(lt);

	return result;
}
void TUtility::UtilitySleep(long milliSeconds)
{
	const int NANOSECONDS_PER_MILLISECOND = 1000000;


	if (0 == milliSeconds)
	{
		boost::thread::yield(); // just do a yield
		return;
	}
	boost::xtime time;


	int sec = 0;
	// if larger than 1 sec, do some voodoo for the boost::xtime struct
	if (milliSeconds >= 1000)
	{ // convert ms > 1000 into secs + remaining ms
		int secs = milliSeconds /1000;
		milliSeconds = milliSeconds - secs*1000;
		sec += secs;
	}
	milliSeconds*=NANOSECONDS_PER_MILLISECOND;
	// normal boost time sleep stuff
	// get current time
	boost::xtime_get(&time,boost::TIME_UTC_);
	// add # of desired secs and ms
	time.nsec+=milliSeconds;
	time.sec+= sec;
	// sleep until that time

	boost::thread::sleep(time);
}

unsigned long TUtility::GetTimeMilliseconds()
{
    boost::posix_time::ptime time = boost::posix_time::microsec_clock::local_time();
    boost::posix_time::time_duration duration( time.time_of_day() );
    return duration.total_milliseconds();

}

string TUtility::cchar_to_str(const char * str)
{
	std::string tmp_str(str);
	return tmp_str;
}


char * TUtility::str_to_cstr(const std::string& s)
{
	char * result = new char[s.size() + 1];
	strcpy(result, s.c_str());
	return result;
}

string TUtility::RemoveSpaces(string& str)
{
	return str_replace(" ", "", str);
}
wstring TUtility::RemoveSpaces(wstring& wstr)
{
	return str_replace(L" ", L"", wstr);

}

string TUtility::AddSpaces(string str)
{
	string res;
	res = str;
	for (int i = 1;  i <= str.length(); ++i)
	{
		str.insert(i, "  ");
		i+=2;

	}

	return str;

}

wstring TUtility::Utf8ToUnicode(const string& utf8str)
{
	setlocale(LC_CTYPE, "");
	size_t strLength = utf8str.length() + 1;
    wchar_t * wch = new wchar_t [strLength*4];
	mbstowcs(wch, utf8str.c_str(), strLength*4);
	std::wstring wstr(wch);
	delete[] wch;

	return wstr;
}

//�������������� �� utf16 � utf8
string TUtility::UnicodeToUtf8(const wstring& unicodestr)
{
	setlocale(LC_CTYPE, "");
	size_t strLength = unicodestr.length() + 1;
	char * ch = new char [strLength*4];
	wcstombs(ch, unicodestr.c_str(), strLength*4);
	std::string utf8str(ch);
	delete[] ch;

	return utf8str;
}

string TUtility::BoolToString(bool val, bool num)
{
	if (!num)
	{
		if(val)
			return "true";
		else
			return "false";
	}
	else
	{
		if(val)
			return "1";
		else
			return "0";
	}

}

string TUtility::GetTime(int year)
{
	time_t rawtime;
	struct tm timeinfo;
	time ( &rawtime );
	LOCALTIME;

//	localtime_s ( &timeinfo, &rawtime );

	if(year)
		timeinfo.tm_year += 1;

	char buffer [80];
	strftime (buffer, 80, "%a, %m %b %Y %X GMT", &timeinfo);
	return buffer;
}
int TUtility::GetServerTime()
{
	return time(0);
}
/********************************************************/

vector<char> TUtility::StringToVector(string str, vector<char> vec)
{
	for(int i = 0; i < (int) str.size(); i++)
	{
		vec.push_back(str[i]);
	}

	return vec;
}

/**********************************************************/

vector<char> TUtility::VectorMerge(vector<char> vec1, vector<char> vec2, int pos)
{
	vector<char>::iterator it = vec1.begin() + pos;
	vec1.insert(it, vec2.begin(), vec2.end());
	return vec1;
}



string& TUtility::str_replace(const string &search, const string &replace, string &subject)
{
	string buffer;

	int sealeng = search.length();
	int strleng = subject.length();

	if (sealeng==0)
		return subject;//no change

	for(int i=0, j=0; i<strleng; j=0 )
	{
		while (i+j<strleng && j<sealeng && subject[i+j]==search[j])
			j++;
		if (j==sealeng)//found 'search'
		{
			buffer.append(replace);
			i+=sealeng;
		}
		else
		{
			buffer.append( &subject[i++], 1);
		}
	}
	subject = buffer;
	return subject;
}

wstring TUtility::str_replace(const wstring &search, const wstring &replace, wstring &subject)
{
	wstring buffer;
	wstring tmp;

	int sealeng = search.length();
	int strleng = subject.length();

	if (sealeng==0)
		return subject;//no change

	for(int i=0, j=0; i<strleng; j=0 )
	{
		while (i+j<strleng && j<sealeng && subject[i+j]==search[j])
			j++;
		if (j==sealeng)//found 'search'
		{
			buffer.append(replace);
			i+=sealeng;
		}
		else
		{
			buffer.append( &subject[i++], 1);
		}
	}
	tmp = buffer;
	return tmp;
}

wstring TUtility::replace_html_entity(wstring& str)
{
	str_replace(L"<", L"&lt;", str);
	str_replace(L">", L"&gt;", str);

	return str;
}
string TUtility::replace_html_entity(string& str)
{
	str_replace("<", "&lt;", str);
	str_replace(">", "&gt;", str);

	return str;
}

string TUtility::UrlDecode(string &s)
{
	string buffer = "";
	int len = s.length();

	for (int i = 0; i < len; i++) {
		int j = i ;
		char ch = s.at(j);
		if (ch == '%'){
			char tmpstr[] = "0x0__";
			int chnum;
			tmpstr[3] = s.at(j+1);
			tmpstr[4] = s.at(j+2);
			chnum = strtol(tmpstr, NULL, 16);
			buffer += chnum;
			i += 2;
		} else {
			buffer += ch;
		}
	}

	return buffer;
}

bool TUtility::StringToBool(string str)
{
	if(str == "1" || str == "true")
		return true;
	else
		return false;
}

bool TUtility::StringToBool(wstring str)
{
	if(str == L"1" || str == L"true")
		return true;
	else
		return false;
}


bool TUtility::is_numeric(const std::string& s)
{
	for (int i = 0; i < (int) s.length(); i++)
	{
		if (!std::isdigit(s[i]))
			return false;
	}

	return true;
}

bool TUtility::is_numeric(const std::wstring& s)
{
	for (int i = 0; i < (int) s.length(); i++)
	{
		if (!std::isdigit(s[i]))
			return false;
	}

	return true;
}

string TUtility::ToXml(int val, string name)
{
	stringstream ss;
	string s;

	ss << "<" << name << " type = \"int\">" << val <<"</" << name << ">";

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}

string TUtility::ToXml(vector<int> vec, string name)
{
	stringstream ss;
	string s;

	ss << "<" << name << " is_array = \"true\">";
	for (int i = 0; i < (int) vec.size(); ++i)
	{
		ss << "<array_item>" << vec.at(i) << "</array_item>";
	}
	ss << "</" << name << ">";

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}

string TUtility::ToXml(string val, string name)
{
	stringstream ss;
	string s;

	ss << "<" << name << " type = \"string\">" << val <<"</" << name << ">";

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}

string TUtility::ToJson(int val, string name)
{
	stringstream ss;
	string s;

	ss << "{\"" << name << "\" : \""<< val << "\" }";

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}
string TUtility::ToJson(string val, string name)
{
	stringstream ss;
	string s;

	ss << "{\"" << name << "\" : \""<< val << "\" }";

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}

string TUtility::ToJson(vector<int> vec, string name)
{
	stringstream ss;
	string s;

	ss << "{\"" << name << "\" :[";
	for (int i = 0; i < (int) vec.size(); ++i)
	{
		ss << "\"" << vec.at(i) << "\"";
		if (i+1 != (int)vec.size())
			ss << ", ";

	}
	ss << "]}";

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}
string TUtility::ToJson(vector<string> vec, string name)
{
	stringstream ss;
	string s;

	ss << "{\"" << name << "\" :[";
	for (int i = 0; i < (int) vec.size(); ++i)
	{
		ss << /*"\"" <<*/ vec.at(i) /*<< "\""*/;
		if (i+1 != (int)vec.size())
			ss << ", ";

	}
	ss << "]}";

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}
string TUtility::ToJson(vector< map <wstring, wstring> > vec, string name)
{
	stringstream ss;
	string s;

	ss << "{\"" << name << "\" :[";
	for (int i = 0; i < (int) vec.size(); ++i)
	{
		ss << TUtility::ToJson(vec.at(i));
		if (i+1 != (int)vec.size())
			ss << ", ";

	}
	ss << "]}";

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}
string TUtility::ToJson(map<string, string> params)
{
	stringstream ss;
	string s;

	map<string, string>::iterator i;

	ss << "{";

	i = params.begin();
	for (int k = 0; k != params.size(); ++k)
	{

		ss << "\"" << i->first << "\":\"" << i->second  << "\"";
		if (k+1 != params.size())
		{
			ss << ",";
		}
		i++;
	}

	ss << "}";

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}
string TUtility::ToJson(map<wstring, wstring> params)
{
	wstringstream ss;
	string s;

	map<wstring, wstring>::iterator i;

	ss << L"{";

	i = params.begin();
	for (int k = 0; k != params.size(); ++k)
	{

		ss << L"\"" << i->first << L"\":\"" << i->second  << L"\"";
		if (k+1 != params.size())
		{
			ss << L",";
		}
		i++;
	}

	ss << L"}";

	s = TUtility::UnicodeToUtf8(ss.str());

	ss.str(L"");
	ss.clear();

	return s;
}
string TUtility::ToXml(map<string, string> params)
{
	stringstream ss;
	string s;

	map<string, string>::iterator i;

	i = params.begin();
	for (int k = 0; k != params.size(); ++k)
	{

		ss << "<" << i->first << ">" << i->second  << "</" << i->first << ">" ;
		i++;
	}

	s = ss.str();

	ss.str("");
	ss.clear();

	return s;
}



string TUtility::Serialize(map<string, string> params, string response_type)
{
	string str;
	if (response_type == "json")
	{
		str = ToJson(params);
	}
	else
		str = ToXml(params);

	return str;
}



int TUtility::wstoi(wstring str)
{
	try
	{
		return atoi(UnicodeToUtf8(str).c_str());
	}

	catch (...)
	{
		return 0;
	}

}
int TUtility::wstoi(wchar_t h)
{
	wstringstream ws;
	int num;
	try
	{

		ws << h;

		ws >> num;

		ws.str(L"");
		ws.clear();



	}

	catch (...)
	{
		return 0;
	}
	return num;
}
double TUtility::wstof(wstring str)
{
	try
	{
		return atof(UnicodeToUtf8(str).c_str());
	}

	catch (...)
	{
		return 0.0;
	}
	return 0.0;
}
wstring TUtility::itows(int val)
{
	wstringstream ws;
	wstring res;
	ws << val;

	res = ws.str();

	ws.str(L"");
	ws.clear();

	return res;

}
string TUtility::itoa(int val)
{
	stringstream s;
	string res;
	s << val;

	res = s.str();

	s.str("");
	s.clear();

	return res;

}
wstring TUtility::ftows(double val)
{
	char ch[100];

	int i = sprintf(ch, "%f", val);
	string res(ch);

	return TUtility::Utf8ToUnicode(res);


}
string TUtility::ftoa(double val)
{
	char ch[100];

	int i = sprintf(ch, "%f", val);
	string res(ch);

	return res;

}


bool TUtility::is_base64(unsigned char c) {
	return (isalnum(c) || (c == '+') || (c == '/'));
}

string TUtility::base64_encode(char const* bytes_to_encode, unsigned int in_len) {

	const string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (in_len--) {
		char_array_3[i++] = *(bytes_to_encode++);
		if (i == 3) {
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; (i <4) ; i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while((i++ < 3))
			ret += '=';

	}

	return ret;

}

string TUtility::base64_decode(string const& encoded_string) {
	const string base64_chars =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/";

	for (int i = 0; i < (int)encoded_string.size(); ++i)
	{
		if(!is_base64(encoded_string[i]) && encoded_string[i] != '=')
			return "";
	}

	int in_len = encoded_string.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];
	std::string ret;

	while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
		char_array_4[i++] = encoded_string[in_]; in_++;
		if (i ==4) {
			for (i = 0; i <4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i) {
		for (j = i; j <4; j++)
			char_array_4[j] = 0;

		for (j = 0; j <4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
	}

	return ret;
}

int TUtility::get_fractional(double val)
{
	wstring str;
	int t = 0;
	str = boost::lexical_cast<wstring>(val);
	t = str.find(L".", 0)+1;

	if (t != -1)
	{
		str = str.substr(t, 2);
	}
	else
		str = L"00";

	if (str.length() == 1)
	{
		str += L"0";
	}
	return TUtility::wstoi(str);

}
double TUtility::get_rounded(double val, int dig, bool to_highest)
{
	double num = to_highest ? 0.5 : -0.5;
	return floor((val * pow(10.0, dig)) + num) / pow(10.0, dig);
}
wstring TUtility::get_rounded(double val, int dig, bool to_highest, int precision)
{
	double num = TUtility::get_rounded(val, dig, to_highest);
	wstring str = TUtility::ftows(num);
	TUtility::get_rounded(str, precision);
	return str;
}
void TUtility::get_rounded(wstring& val, int precision)
{
	int t = 0;
	t = val.find(L".", 0)+1;

	if(t > 4)
		val.insert(t-4, L",");
	
	if (!(t <= 0))
		val = val.substr(0, t +precision);

}
void TUtility::get_rounded(string& val, int precision)
{
	int t = 0;
	t = val.find(".", 0)+1;

	if (!(t <= 0))
		val = val.substr(0, t +precision);

}
wstring TUtility::itowords(int numb)
{
	//массив, для определения разряда, в котором находимся(сотни,десятки,еденицы...)


	int osn[11]={0,1,10,100,1000,10000,100000,1000000,10000000,100000000,1000000000};

	//массив основsdfаний

	char *basis[11][10]=
	{
		{"","","","","","","","","",""},
		{"","один","два","три","четыре","пять","шесть","семь","восемь","девять"},
		{"","десять","двадцать","тридцать","сорок","пятьдесят","шестьдесят","семьдесят","восемьдесят","девяносто"},
		{"","сто","двести","триста","четыреста","пятьсот","шестьсот","семьсот","восемьсот","девятьсот"},
		{"","одна","две","три","четыре","пять","шесть","семь","восемь","девять"},
		{"","десять","двадцать","тридцать","сорок","пятьдесят","шестьдесят","семьдесят","восемьдесят","девяносто"},
		{"","сто","двести","триста","четыреста","пятьсот","шестьсот","семьсот","восемьсот","девятьсот"},
		{"","один","два","три","четыре","пять","шесть","семь","восемь","девять"},
		{"","десять","двадцать","тридцать","сорок","пятьдесят","шестьдесят","семьдесят","восемьдесят","девяносто"},
		{"","сто","двести","триста","четыреста","пятьсот","шестьсот","семьсот","восемьсот","девятьсот"},
		{"","один","два","три ","четыре","пять","шесть","семь","восемь","девять"}
	};
	/*
	char *basis[11][10]=
		{
			{"","","","","","","","","",""},
			{"","one","two","three","four","five","six","seven","eight","nine"},
			{"","ten","twelve","thirty","forty","fifty","sixty","seventy","eighty","ninety"},
			{"","one hundred","two hundred","three hundred","four hundred","five hundred","six hundred","seven hundred","eight hundred","nine hundred"},
			{"","one","two","three","four","five","six","seven","eight","nine"},
			{"","ten","twelve","thirty","forty","fifty","sixty","seventy","eighty","ninety"},
			{"","one hundred","two hundred","three hundred","four hundred","five hundred","six hundred","seven hundred","eight hundred","nine hundred"},
			{"","one","two","three","four","five","six","seven","eight","nine"},
			{"","ten","twelve","thirty","forty","fifty","sixty","seventy","eighty","ninety"},
			{"","one hundred","two hundred","three hundred","four hundred","five hundred","six hundred","seven hundred","eight hundred","nine hundred"},
			{"","one","two","three","four","five","six","seven","eight","nine"}
		};

*/
/*		//массив окончаний
		char *termination[10][10]=
		{
			{"","","","thousand","","","million","","","milliard"},
			{"","","","thousand","","","million","","","milliard"},
			{"","","","thousand","","","million","","","milliard"},
			{"","","","thousand","","","million","","","milliard"},
			{"","","","thousand","","","million","","","milliard"},
			{"","","","thousand","","","million","","","milliard"},
			{"","","","thousand","","","million","","","milliard"},
			{"","","","thousand","","","million","","","milliard"},
			{"","","","thousand","","","million","","","milliard"},
			{"","","","thousand","","","million","","","milliard"}
		};
*/
		char *termination[10][10]=
		{
			{"","","","тысяч","","","миллионов","","","миллиардов"},
			{"","","","тысяча","","","миллион","","","миллиард"},
			{"","","","тысячи","","","миллиона","","","миллиарда"},
			{"","","","тысячи","","","миллиона","","","миллиарда"},
			{"","","","тысячи","","","миллиона","","","миллиарда"},
			{"","","","тысяч","","","миллионов","","","миллиардов"},
			{"","","","тысяч","","","миллионов","","","миллиардов"},
			{"","","","тысяч","","","миллионов","","","миллиардов"},
			{"","","","тысяч","","","миллионов","","","миллиардов"},
			{"","","","тысяч","","","миллионов","","","миллиардов"}
		};

			char *dec[10]={"десять","одиннадцать","двенадцать","тринадцать","четырнадцать","пятнадцать","шестнадцать","семнадцать","восемнадцать","девятнадцать"};
//			char *dec[10]={"ten","eleven","twelve","thirteen","fourteen","fifteen","sixteen","seventeen","eighteen","nineteen"};

			int n1=numb;
			int cnt=0;

			//подсчет количесива цифр в числе
			while (numb){
				numb/=10;
				++cnt;
			}

			char result[100]={};
			int celoe=0;

			stringstream str;
			while (n1){
				if (!((cnt+1) % 3))
				{
					if ((n1/osn[cnt])==1)
					{
						n1%=osn[cnt];
						--cnt;
						celoe=n1/osn[cnt];
						n1%=osn[cnt];

						str << dec[celoe] << " ";
						--cnt;
						if (!(cnt % 3))
						{
							str << termination[0][cnt] << " ";
						}
					}
					if (!cnt) break;
				}
				celoe=n1/osn[cnt];
				n1%=osn[cnt];

				str << basis[cnt][celoe] << " ";
				--cnt;

				if (!(cnt % 3))
				{
					str << termination[celoe][cnt] << " ";
				}
			}
			wstring res = TUtility::Utf8ToUnicode(str.str());

			return res;
}

string TUtility::AddQuotes(string str)
{
	int begin = 0;
	int end = 0;
	if (str == "")
	 return str;

	string rep = "&quot;";

 	while(begin < str.length())
 	{
 		begin = str.find("\"", begin);
		if (begin == -1)
			return str;

		str.replace(begin, 1, rep);
		begin+=2;
 	}

	return str;

}

bool TUtility::CheckZeroValue(int el_num, bool throw_error, int val ...)
{
	va_list ap;
	va_start(ap,val);   // начало параметров

	try
	{

		if (val <= 0)
		{
			TError error;
			error.message = L"Bad Data";
			throw error;
		}

		for (int i = 0; i < el_num-1; i++) {
			int p = va_arg(ap, int);
			if (p <= 0)
			{
				TError error;
				error.message = L"Bad Data";
				throw error;
			}
		}

	}

	catch (TError& error)
	{
		va_end(ap);
		if (throw_error)
			throw error;	
		else
			return false;
	}

	va_end(ap);
	return true;
}
bool TUtility::CheckZeroValue(int el_num, bool throw_error, TBaseClass * obj ...)
{
	va_list ap;
	va_start(ap, obj);   // начало параметров

	try
	{
		if (obj == NULL)
		{
			TError error;
			error.message = L"Bad Data";
			throw error;
		}

		for (int i = 0; i < el_num-1; i++) {
			TBaseClass * p = va_arg(ap, TBaseClass*);
			if (p == NULL)
			{
				TError error;
				error.message = L"Bad Data";
				throw error;
			}
		}

	}

	catch (TError& error)
	{
		va_end(ap);
		if (throw_error)
			throw error;	
		else
			return false;
	}

	va_end(ap);
	return true;
}

/*void TUtility::CheckZeroValue(int el_num, TCollection<TBaseClass> * col ...)
{
	va_list ap;
	va_start(ap, col);   // начало параметров

	try
	{
		if (col == NULL)
		{
			TError error;
			error.message = L"Bad Data";
			throw error;
		}

		for (int i = 0; i < el_num-1; i++) {

			TCollection<TBaseClass> * p = va_arg(ap, TCollection<TBaseClass>*);
			if (p == NULL)
			{
				TError error;
				error.message = L"Bad Data";
				throw error;
			}
		}

	}

	catch (TError& error)
	{
		va_end(ap);
		throw error;
	}

	va_end(ap);
}
*/

int TUtility::GetProgramPid()
{
	#ifdef _WIN32
	return _getpid();
	#else
	return (int) getpid();
	#endif
}

void TUtility::TimeElapsed(long double & start_time, string func_name)
{
	#ifdef _WIN32
	cout << "function: " << func_name<< endl;
	cout << "time elapsed: " << clock() - start_time<< endl;
	cout << "*************************************\r\n";
	#endif
}

void TUtility::RandomNum(std::vector<int> & vec)
{

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(-100, 100);
	std::generate_n(std::inserter(vec, vec.end()), 100, [&dist, &gen]() { return dist(gen); });


/*	DeadLock Example
 *	because we used_deferred and not calling get we will end up with the deadlock
 *	std::vector<int> random_numbers;
	//	TUtility::RandomNum(random_numbers);
	auto fut_result = std::async(launch::deferred, [&random_numbers]()
	{
		std::random_device rd;
		std::mt19937 gen(rd());
		std::normal_distribution<> dist(-100, 100);
		std::generate_n(std::back_inserter(random_numbers), 100, [&gen, &dist]()
		{
			return dist(gen);
		});
	});

	while (fut_result.wait_for(100ms) != std::future_status::ready)
	{

	}*/
}

bool TUtility::doWork(std::function<bool(int)> filter, int maxVal)
{
	//this code will crash, because thread is created and is joinable 
	//but when program exits without t.join the destructor of the thread is called and
	//program terminates. joinable threads needs to be joined before its destructor is called
	// detach will result in the same result. because the destructor would be called when thread
	// is still working on the  task. need to wait till it finished the work

	std::vector<int> goodVals;
	std::thread t([&filter, maxVal, &goodVals]
	{                            
		for (auto i = 0; i <= maxVal; ++i)           
		{ if (filter(i)) goodVals.push_back(i); }
	});
	auto nh = t.native_handle();

	t.detach();
	std::this_thread::sleep_for(10s);

	if (false) {
		t.join();                               
		 return true;                          
	}     

	return false;
}