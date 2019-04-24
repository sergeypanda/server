//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2010 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER3_REQUEST_HPP
#define HTTP_SERVER3_REQUEST_HPP

#include <string>
#include <vector>
#include "header.hpp"
#include <boost/thread/thread.hpp>
#include "../TRequestInfo.h"

using namespace std;

struct uploaded_file
{
	string name;
	string filename;
	string content_type;
	string file_raw;
	string file_type;

};

/// A request received from a client.
struct request
{
	string ip;
	int user_id;

	string method;  // ++
	uploaded_file file;

	string uri;  // ++
	int http_version_major;
	int http_version_minor;
	vector<unsigned char> source;

	vector<unsigned char>header;
	vector<unsigned char>message;

	map <string, string> url_parms; // ++
	map <string, string> post_parms; // a, 1, b, 2, c, 3
	map <string, string> request_header;// content_type, host, user agent  i td;
	map <string, string> cookie;// ++
	unsigned int cont_len;  // content-length
	unsigned int max_content_lenght_size;
	unsigned int connection_speed;

	int start_time;
	vector <unsigned char> buffer;
	unsigned long enter_time;
	int last_request_time;

	TRequestInfo * info;

	int thread_index;
	bool cron;

	request(): info(NULL), user_id(0), enter_time(0), last_request_time(0), thread_index(0),
				start_time(0), cont_len(0), max_content_lenght_size(0), connection_speed(0), http_version_major(0),
				http_version_minor(0), cron(false){};
	~request()
	{
		if(info != NULL)
			delete info;
	}

	request * Clone()
	{
		request * req = new request();

		req->ip = ip;
		req->user_id = user_id;

		req->method = method;
		req->file = file;

		req->uri = uri;
		req->http_version_major = http_version_major;
		req->http_version_minor = http_version_minor;
		req->source = source;

		req->header = header;
		req->message = header;

		req->url_parms = url_parms;
		req->post_parms = post_parms;
		req->request_header = request_header;
		req->cookie = cookie;
		req->cont_len = cont_len;
		req->max_content_lenght_size = max_content_lenght_size;
		req->connection_speed = connection_speed;

		req->start_time = start_time;

		req->buffer = buffer;

		req->enter_time = enter_time;
		req->last_request_time = last_request_time;

		if (info != NULL)
		{
			req->info = info->Clone();
		}
		else
			req->info = NULL;

		req->thread_index = thread_index;
		req->cron = cron;
		return req;
	}

};

 // namespace server3
 // namespace http

#endif // HTTP_SERVER3_REQUEST_HPP
