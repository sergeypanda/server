//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "../../include/http/request_parser.hpp"
#include "../../include/http/request.hpp"
#include "../../include/TConfig.h"


request_parser::request_parser()
  : state_(method_start)
{
}

void request_parser::reset()
{
  state_ = method_start;
}


bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (c >= 0 && c <= 31) || (c == 127);
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

//fixing the issue with class template function needed to be placed in the header file
template boost::tuple<boost::tribool, unsigned char*> request_parser::parse(request& req, unsigned char* begin, unsigned char* end);

template <typename InputIterator>
boost::tuple<boost::tribool, InputIterator> request_parser::parse(request& req,
	InputIterator begin, InputIterator end)
{
	boost::tribool result;
	while (begin != end)
	{
		//while(begin != s.find("\r\n\r\n"))
		if (req.method == "")
			req.header.push_back(*begin++);
		else
			req.message.push_back(*begin++);

	}

	if (req.method == "")
	{
		string s(req.header.begin(), req.header.end());
		string str_message;
		string str_header;
		size_t offset = s.find("\r\n\r\n");
		if (offset != string::npos)
		{
			str_message = s.substr(offset + 4);
			/* Fix the case where only "GET /xxxxx HTTP1.0\r\n\r\n" is sent (no other headers)*/
			if (offset == s.find("\r\n"))
				str_header = s.substr(0, offset + 2);
			else
				str_header = s.substr(0, offset);

			req.header.clear();
			for (unsigned int i = 0; i < str_header.length(); ++i)
				req.header.push_back(str_header.c_str()[i]);

			for (unsigned int i = 0; i < str_message.length(); ++i)
				req.message.push_back(str_message.c_str()[i]);

			if (!parse_header(req))
				return result = false;

		}

		else
		{
			result = boost::indeterminate;
			return result;
		}

	}

	if (req.method == "GET ")
	{
		req.message.clear();
		return result = true;
	}

	if (req.method == "POST ")
	{
		if (req.cont_len > req.max_content_lenght_size)
			return result = false;

		if (req.cont_len > req.message.size())
		{
			return result = boost::indeterminate;
		}

		if (req.cont_len < req.message.size())
			req.message.resize(req.cont_len);

		if (!parse_message(req))
			return result = false;

	}

	req.source.insert(req.source.end(), req.header.begin(), req.header.end());

	addHeaderDelimiter(req.source);

	req.source.insert(req.source.end(), req.message.begin(), req.message.end());

	return result = true;

}
//template void request_parser::addHeaderDelimiter(vector<unsigned char>& source);
template<typename T> 
void request_parser::addHeaderDelimiter(T& source)
{
//	puts(__FUNCSIG__);
	source.push_back('\r');
	source.push_back('\n');
	source.push_back('\r');
	source.push_back('\n');
}

bool request_parser::parse_header(request & request_)
{
	string str (request_.header.begin(), request_.header.end());
//	str.append("\r\n");
	size_t pos=0;


	pos = str.find("POST ", pos);
	if(pos != str.npos)
		request_.method = "POST ";

	if(request_.method == "")
	{
		pos = 0;
		pos = str.find("GET ", pos);
		if(pos != str.npos)
			request_.method = "GET ";

		else
			return false;
	}


	GetUrl(request_, str);

	GetCookie(request_, str);

	size_t begin = 0, end = 0, diff = 0;
	string par, val;
	
	while(begin < str.length())
	{
		begin = str.find("\r\n", begin);
		end = str.find(": ", begin+4);
		begin += 2;
		diff = end - begin;

		par.append(str, begin, diff);	
		
		if(par == "Cookie")
		{
			par.clear();
			begin = str.find("Cookie", begin);
//			begin += 2;

			end = str.find(": ", begin);
			diff = end - begin;
			par.append(str, begin, diff);	

		}
		begin = end;
		diff = end - begin;

		end = str.find("\r\n", begin);
		begin += 2;
		diff = end - begin;
		
		val.append(str, begin, diff);
		begin = end;

		if(par == "Content-Length")
		{
			request_.cont_len = atoi(val.c_str());
			if(request_.cont_len > request_.max_content_lenght_size)
				return false;
		}

		request_.request_header[par] = val;
		
		par.clear();
		val.clear();
		

	}

	return true;
}


bool request_parser::GetUrl(request & request_, string str)
{

	size_t pos=0;
	string str_uri, sub_string_url;
	size_t url_end_pos = 0, uri_end_pos = 0;

	if (request_.method != "")
	{
		
		url_end_pos = str.find(" HTTP/", pos);
	//	url_end_pos = str.find(" HTTP/1.0\r\n", pos);
		if(request_.method == "GET ")
			sub_string_url = str.substr(pos+4, url_end_pos-4);
		else
			sub_string_url = str.substr(pos+5, url_end_pos-4);

		size_t begin = sub_string_url.find_first_of("?", pos);

		if(begin > 0 )
		{	
			str_uri.append(sub_string_url, pos, begin);
			request_.uri = str_uri;

			string param, val;
			size_t end, diff;

			while (begin < sub_string_url.npos)
			{
				begin += 1;
				end = sub_string_url.find("=", begin);
				diff = end - begin;

				param.append(sub_string_url, begin, diff);
				begin = end;
				begin += 1;


				end = sub_string_url.find("&", begin);
				diff = end - begin;

				val.append(sub_string_url, begin, diff);
				begin = end;

				request_.url_parms[param] = val;
				param.clear();
				val.clear();

			}  // while

		} // if
		else
			request_.uri = str_uri;

	} // if

	return true;
}


bool request_parser::GetCookie(request & request_, string str)
{

	str = TUtility::RemoveSpaces(str);
	size_t begin = 0, end = 0, diff = 0, rn = 0;
	string str_cookie, name, value;
	str_cookie = "Cookie:";
	begin = str.find(str_cookie, begin);
	
	if(begin != str.npos)
	{
		begin+=str_cookie.length();
		size_t offset = str.find("\r\n", begin);
		while(begin < offset)
		{
			end = str.find("=", begin);
			diff = end - begin;

			name.append(str, begin, diff);
			begin = end;
			begin += 1;

			end = str.find(";", begin);
			if(end > offset)
				end = offset;
			diff = end - begin;

			value.append(str, begin, diff);
			begin = end;
			if(begin != offset)
				begin += 1;

			request_.cookie[name] = value;
			name.clear();
			value.clear();
		} // while


		return true;
	} //if



	return true;
}


bool request_parser::parse_message(request & request)
{
	string str (request.message.begin(), request.message.end());


	size_t begin = 0, end = 0, diff = 0, tmp = 0;

	string name, value;
	
	if (parse_form(request))
	{
		return true;
	}

	if(begin != str.npos)
	{
		size_t offset = str.length();
		while(begin < offset)
		{
			end = str.find("=", begin);
			if (end > offset)
				break;

			diff = end - begin;

			name.append(str, begin, diff);
			begin = end;
			begin += 1;

			end = str.find("&", begin);
			if(end > offset)
				end = offset;
			diff = end - begin;

			value.append(str, begin, diff);
			begin = end;
			begin += 1;

			request.post_parms[name] = value;
			name.clear();
			value.clear();
		} // while
	
	} // if

	return true;
}


bool request_parser::parse_form(request & request)
{
	size_t begin = 0, end = 0, diff = 0, tmp = 0;
	int mp_find = 0;
	string mp = request.request_header["Content-Type"];
	string boundary = "";

	mp_find = mp.find("multipart/form-data;");

	if (mp_find == mp.npos)
	{
		return false;
	}

	boundary.append(mp, mp.find("boundary=")+9, mp.size());

	string str (request.message.begin(), request.message.end());
	
	size_t bl = boundary.length() + 2;		// 2 = /r/n

	begin = str.find(boundary, 0) + bl;	
	string section = "";
	string name = "", value = "", cont_type = "";
	int i = 1;
	int bg = 0, nd = 0;
	if(begin != str.npos)
	{
		size_t offset = str.length() - bl;
		while(begin < offset)
		{
			end = str.find(boundary, begin)-2;
			diff = end - begin;

			section.append(str, begin, diff);

			bg = section.find("name=", 0)+ 6;
			nd = section.find("\"", bg);
			diff = nd - bg;
			name.append(section, bg, diff);
			bg = nd;

			bg = section.find("filename=\"", bg);
			if (bg != -1)
			{
				bg += 10;
				nd = section.find("\"", bg);
				diff = nd - bg;
				value.append(section, bg, diff);
				bg = section.find("Content-Type: ", bg) + 14;
				nd = section.find_first_of("\r\n", bg);
				diff = nd - bg;
				cont_type.append(section, bg, diff);
				request.file.name = name;
				request.file.filename = value;
				std::size_t last_dot_pos = value.find_last_of(".");
				if (last_dot_pos != std::string::npos)
				{

					request.file.file_type = value.substr(last_dot_pos + 1);
				}
				request.file.content_type = cont_type;

				bg = section.find_first_of("\r\n", bg) + 4;
				nd = section.find_last_of("\r\n", section.length());
				diff = nd - bg;
				request.file.file_raw.append(section, bg, diff-1);	//??

			}
			else
			{	
				bg = nd + 5;
				nd = section.find_last_of("\r\n", section.length());
				diff = nd - bg;
				value.append(section, bg, diff-1);
			}

			begin = end;
			begin += bl;
			request.post_parms[name] = value ;
			section.clear();
			name.clear();
			value.clear();
			i++;
		} // while

	}

	return true;
}