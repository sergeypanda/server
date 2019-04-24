//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
#include "../../include/http/TSocketManager.h"

#include <boost/thread/mutex.hpp>
#include "../../include/http/request_handler.hpp"
#include <vector>
#include <fstream>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include "../../include/http/mime_types.hpp"
#include "../../include/http/reply.hpp"
#include "../../include/http/request.hpp"
#include "../../include/http/TQueue.h"
#include <thread>

boost::mutex mutex;
int count=0;

request_handler::request_handler(const std::string& doc_root, TQueue * queue_)
  : doc_root_(doc_root), queue(queue_)
{
}

void request_handler::handle_request(request * req, reply& rep)
{


	std::string request_path = "";

	if(req->method == "GET ")
	{
	  // Decode url to path.
//		std::cout << "  if (!url_decode(req->uri, request_path))\n";
	  if (!url_decode(req->uri, request_path))
	  {
		rep = reply::stock_reply(reply::bad_request);
		return;
	  }

	  // Request path must be absolute and not contain "..".
	  if (request_path.empty() || request_path[0] != '/'
		  || request_path.find("..") != std::string::npos)
	  {
		rep = reply::stock_reply(reply::bad_request);
		return;
	  }
//	  std::cout << "   if (request_path[request_path.size() - 1] == '/')))\n";
	  // If path ends in slash (i.e. is a directory) then add "index.html".
	  if (request_path[request_path.size() - 1] == '/')
	  {
		request_path += "index.html";
	  }
	}

//   string str (req->source.begin(), req->source.end());
//   cout<< str;
	

  if(req->method == "POST ")
  {
//	  std::cout << "\n 1 \n\n";
//	  rep.headers.resize(0);
	  rep.content = queue->GetReply(req, rep);
	  if (rep.content.substr(0, 5) != "HTTP/")
	  {
		  rep.content.insert(0, "HTTP/1.0 200 OK\r\n");
	  }
  }

  else
  {
//	  std::cout << "\n 2 \n\n";
	  // Determine the file extension.
	  std::size_t last_slash_pos = request_path.find_last_of("/");
	  std::size_t last_dot_pos = request_path.find_last_of(".");
	  std::string extension;
	  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
	  {

		extension = request_path.substr(last_dot_pos + 1);
	  }

	  int found = 0;
	  if(request_path.find("/getimage/", 0) != -1)
	  {
		found = 1; // getimage found;

		if (request_path.find("/getimage/thumb/", 0) != -1)
		{
			found = 2; // thumb found;
		}
	  }
	  else if(request_path.find("/getfile/", 0) != -1)
	  {
		  found = 3;
	  }
//	  std::cout << "\n 3 \n\n";

	  if (found == 1 || found == 2)
	  {
		string id;

		TRequestInfo * req_info = new TRequestInfo();
		req_info->command = "GetImage";
		req_info->language = 1;

		if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
		{
			id = request_path.substr(last_slash_pos + 1, last_dot_pos - last_slash_pos - 1);
		}

		map<string, string> param;
		param["id"] = id;
		param["select_raw"] = TUtility::itoa(1);

		if (found == 2)
		{
			param["thumb"] = TUtility::itoa(1);
		}
		else
		{
			param["thumb"] = TUtility::itoa(0);
		}
		std::cout << "\n 4 \n\n";
		req_info->parameters = TUtility::UriEncode(TUtility::ToJson(param));
		req->post_parms["json"] = req_info->ToJson(NULL, 1, "", false);
		rep.content = queue->GetReply(req, rep);
		SAFEDELETE(req_info)

		string content = TUtility::UriDecode(rep.content);

		string succesfull = "";
		std::size_t file_pos = rep.content.find("\"succesfull\"");
		file_pos = (int)rep.content.find_first_of(":", file_pos) + 1;
		std::size_t file_end_pos = rep.content.find_first_of(",", file_pos);

		if (file_end_pos != std::string::npos && file_end_pos > file_pos)
		{
			succesfull = rep.content.substr(file_pos, file_end_pos - file_pos);
		}
		if (succesfull == "1")
		{
			string file_raw;
			if (found == 2)
			{
				file_pos = content.find("\"thumb\"", 0)+7;
				file_pos = content.find("\"TImageThumb\"", file_pos)+13;
				file_pos = content.find("\"file_raw\":", file_pos)+11;
				file_pos = content.find_first_of("\"", file_pos)+1;
				file_end_pos = content.find_first_of("\"", file_pos);
			}
			else
			{
				file_pos = content.find("\"file_raw\":", 0)+11;
				file_pos = content.find_first_of("\"", file_pos)+1;
				file_end_pos = content.find_first_of("\"", file_pos);
			}

			if (file_end_pos != std::string::npos && file_end_pos > file_pos)
			{
				file_raw = content.substr(file_pos, file_end_pos - file_pos);
				file_raw = TUtility::base64_decode(file_raw);
				if (file_raw != "")
				{
					rep.content = file_raw;
					rep.status = reply::ok;
					rep.headers.resize(2);
					rep.headers[0].name = "Content-Length";
					rep.headers[0].value = boost::lexical_cast<std::string>(file_raw.size());
					rep.headers[1].name = "Content-Type";
					rep.headers[1].value = extension_to_type(extension);
					return;
				}

			}
		}
		rep.content = "";
		request_path = "/no_image.png";
		extension = "png";
		std::cout << "\n 5 \n\n";
	  }
	  else if(found == 3)
	  {
		  string id;
		  std::cout << "\n 6 \n\n";
		  TRequestInfo * req_info = new TRequestInfo();
		  req_info->command = "GetFile";
		  req_info->language = 1;

		  if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
		  {
			  id = request_path.substr(last_slash_pos + 1, last_dot_pos - last_slash_pos - 1);
		  }

		  map<string, string> param;
		  param["id"] = id;
		  param["select_raw"] = TUtility::itoa(1);

		  req_info->parameters = TUtility::UriEncode(TUtility::ToJson(param));
		  req->post_parms["json"] = req_info->ToJson(NULL, 1, "", false);






		  //reply path to shared memory
		  rep.content = queue->GetReply(req, rep);






		  std::cout << "\n 7 \n\n";

		  SAFEDELETE(req_info)

		  string content = TUtility::UriDecode(rep.content);

		  string succesfull = "";
		  std::size_t file_pos = rep.content.find("\"succesfull\"");
		  file_pos = (int)rep.content.find_first_of(":", file_pos) + 1;
		  std::size_t file_end_pos = rep.content.find_first_of(",", file_pos);

		  if (file_end_pos != std::string::npos && file_end_pos > file_pos)
		  {
			  succesfull = rep.content.substr(file_pos, file_end_pos - file_pos);
		  }
		  if (succesfull == "1")
		  {
			  string file_raw;
			  file_pos = content.find("\"file_raw\":", 0)+11;
			  file_pos = content.find_first_of("\"", file_pos)+1;
			  file_end_pos = content.find_first_of("\"", file_pos);

			  if (file_end_pos != std::string::npos && file_end_pos > file_pos)
			  {
				  file_raw = content.substr(file_pos, file_end_pos - file_pos);
				  file_raw = TUtility::UriDecode(file_raw);
				  file_raw = TUtility::str_replace("\\\"", "\"", file_raw);
				  if (file_raw != "")
				  {
					  rep.content = file_raw;
					  rep.status = reply::ok;
					  rep.headers.resize(2);
					  rep.headers[0].name = "Content-Length";
					  rep.headers[0].value = boost::lexical_cast<std::string>(file_raw.size());
					  rep.headers[1].name = "Content-Type";
					  rep.headers[1].value = extension_to_type(extension);
					  return;
				  }

			  }
		  }
	}
//	std::cout << "\n 8 \n\n";
	   // Open the file to send back.
	   int queue_item_id = rep.queue_item_id;
 	   std::string full_path = doc_root_ + request_path;
	   
 	   char buffer[1024];
	   std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
	   if (!is.is_open()) //was == null
	   {
		 rep = reply::stock_reply(reply::not_found);
		 rep.queue_item_id = queue_item_id;
		 return;
	   }
	   while (is.read(buffer, sizeof(buffer)).gcount() > 0)
	   {
		 rep.content.append(buffer, is.gcount());
	   }
		rep.status = reply::ok;
//		std::cout << "\n 9 \n\n";
		rep.headers.resize(2);
		rep.headers[0].name = "Content-Length";
		rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
		rep.headers[1].name = "Content-Type";
		rep.headers[1].value = extension_to_type(extension);

  }

}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value = 0;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}

 // namespace server3
 // namespace http
