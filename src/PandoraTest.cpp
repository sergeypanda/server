#include "../include/PandoraTest.h"

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/regex/v4/regex.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include "../include/TError.h"
#include "../include/http/TQueue.h"
#include <future>
#include <random>
#include "../demangle/demangle.h"
#include "../include/TLanguage.h"
#include "../include/http/TClient.h"

PandoraTest::PandoraTest( string port, string db_name, string conf_path, bool auto_conf)
{
	first_run = true;
	is_client_window_open = true;
	stoping_server		= false;
	stop_server		= false;
	cron_new_req = true;
	stop_cron		= false;
//	db			= NULL;
	manager			= NULL;

	Init(port, db_name, conf_path, auto_conf);
}
PandoraTest::~PandoraTest()
{
	StopCron();
	for (int i = 0; i < (int) sessions.size(); ++i)
	{
		SAFEDELETE(sessions.at(i))
	}
//	SAFEDELETE(db)
	SAFEDELETE(manager)
}

#ifdef _WIN32
string slash = "\\";
#else
string slash = "/";
#endif

void PandoraTest::Init(string port, string db_name, string conf_path, bool auto_conf)
{
	db_name += ".FDB";
	string host, server_root, path;
	boost::filesystem::path full_path(boost::filesystem::current_path());
	int cron_sleep = 0;

//	std::function<bool(int)> lambda = [](int) { return true; };
//	TUtility::doWork(lambda);

	try
	{

		path = full_path.string();
		
		if (conf_path == "")
		{
            conf_path = path;

            std::cout << full_path << std::endl;

            //create config;
            conf_path += slash;

            conf_path += "ok-server.conf";
		}

		cout << "\r\n " << conf_path << "\r\n";
		config->content["server_path"] = path;

		if(auto_conf)
		{
			config->Load(conf_path);
			map<string, string> conf_params;
			stringstream val;
			val << path  << slash << "client";
			conf_params["server_root"] = val.str();
			val.str("");
			val << path  << slash << "db" << slash;
			conf_params["users_db_path"] = val.str();
			val.str("");
			val << path  << slash << "db" << slash;
			conf_params["reports_users_db_path"] = val.str();
			val.str("");
			config->ChangeParameterValue(conf_params);
		}
		config->Load(conf_path);
		config->content["port"] = port;

		int client_window_timeout = atoi(config->content["client_window_timeout"].c_str());
		client_window_timeout *= 1000;
		bool static_server = atoi(config->content["static_server"].c_str());
		int t = atoi(config->content["cron_sleep"].c_str());
		
		if(t <= 10)
			cron_sleep = 1000;
		else	
			cron_sleep = t;

		request_processing_threads = atoi(config->content["request_processing_threads"].c_str());
		if(!request_processing_threads)
			request_processing_threads = 1;

		request_processing_threads = 20;

		int	num_threads = atoi(config->content["num_threads"].c_str());
		if(!num_threads)
			num_threads = 1;

		host = config->content["host"];
		server_root = config->content["server_root"];

		pandora_port = port;
		pandora_host = host;

		int max_content_lenght = atoi(config->content["max_content_lenght_size"].c_str());

		string db_host, db_path, db_user_name, db_password, ssl_port;
		ssl_port = config->content["ssl_port"];
		db_host = config->content["db_host"];
		db_path = config->content["users_db_path"];
		db_user_name = config->content["db_user_name"];
		db_password = config->content["db_password"];

		unsigned int min_upload_speed = atoi(config->content["min_upload_speed"].c_str());

		db_path += db_name;

		queue = new TQueue();
		manager = new TSocketManager(pandora_host, pandora_port, ssl_port, server_root, num_threads, max_content_lenght, min_upload_speed);
		
//		manager->SendHttpRequest( "https://theboostcpplibraries.com", "443" , string("hello") );
//		db = new TDatabase(db_host, db_path, db_user_name, db_password);

		for (int i = 0; i < request_processing_threads ; ++i)  // threads for "Get..." commands
		{
			boost::shared_ptr<boost::thread> thread(new boost::thread(boost::bind(&PandoraTest::RequestProcessor, this)));

			threads.push_back(thread);
		}

		boost::thread main_thread(boost::bind(&PandoraTest::RequestProcessor, this)); //thread for insert, update, etc..
		main_thread_id = main_thread.get_id();

//		boost::thread cron_thread(boost::bind(&PandoraTest::StartCron, this, cron_sleep));
//		boost::thread test_thread(boost::bind(&PandoraTest::CheckClientWindow, this, false, true));
		manager->StartServer(queue);
	}

	catch (TError& error)
	{
		ProcessError(NULL, error.code, TUtility::UnicodeToUtf8(error.message), "", true);
	}


}
void PandoraTest::CheckClientWindow(int sleep_time, bool static_server)
{

	for (;static_server ? false : true;)
	{
		if (!is_client_window_open)
		{
			string guard_host = config->content["guardian_host"];
			string guard_port = config->content["guardian_port"];
			string params = "", str_req = "";
			logger->Add("Closing server");

			params = TUtility::ToJson(TUtility::GetProgramPid(), "pid");
			TRequestInfo * reqv = new TRequestInfo("ServerClosed", "", "", config->content["key"], params, "json");
			str_req = PrepareHTTPRequest(true, "json", "", "", reqv->ToJson(NULL, 1, "", false));
			manager->SendHttpRequest(guard_host, guard_port, str_req, 3, false);
			logger->Add("Closing server 2");
			CloseServer();
			logger->Add("Closed server");
			break;
		}
		is_client_window_open = false;
		TUtility::UtilitySleep(sleep_time);
	}
	TUtility::UtilitySleep(sleep_time);

}

/************************************************************************/
/*                                                                      */
/************************************************************************/

void PandoraTest::CloseServer()
{

	queue->StopQueue();
	stoping_server = true;
	while(queue->GetQueueSize() != 0)
	{
        logger->Add(TUtility::itoa(queue->GetQueueSize()));
		logger->Add("Closing Server");
		TUtility::UtilitySleep(100);
	}
//	db->RemoveSessions();
	logger->Add("Stoping Server");
	manager->StopServer();
	logger->Add("Server Stoped");
	stop_server = true;
}
void PandoraTest::StartCron(int sleep_time)
{
#ifdef blah	
	reply rep;
	string response = "";
	TCollection<TCronTask> * tasks = NULL;
	TSession * session = NULL;
	TResponseInfo * resp = NULL;
	request * req = NULL;
	map<wstring, wstring> param;

	for (;;)
	{

		try
		{
			if (stop_cron)
				break;

			if (!cron_new_req)
			{
				TUtility::UtilitySleep(2000);
				continue;
			}

			CronPause();
			tasks = db->GetCronTasks(time(0));
			
			if (tasks)
			{
				for (int i = 0; i < (int)tasks->items.size(); ++i)
				{
					TCronTask * cr = tasks->items.at(i);
					try
					{
						int sid  = db->GetObjectID();
						string cookie = GenCookie(sleep_time);

						session = new TSession(sid, cr->user_id, "", L"cron", cookie, pandora_host, pandora_port, TUtility::GetProgramPid());

						bool res = db->AddSession(session);
						if (!res)
							throw 0;

						req = PrepareRequest(cr->command, cr->data, cookie, TUtility::UriEncode(cr->parameters), "json");
						req->cron = true;

						response = queue->GetReply(req, rep);

						resp = new TResponseInfo();
						response = response.substr(response.find_first_of("{", 0), response.find_last_of("}", 0));
						response = TUtility::UriEncode(response);
                        resp->FromJson(response);
						if (!resp->succesfull)
						{
							if(resp->error != "")
							{
								TError error;
								error.FromJson(resp->error);
								throw error;
							}
						
							cr->status_id = CRON_TASK_ERROR;
							cr->error = resp->error;
							db->SaveCronTask(cr);
						}
						else
						{
							cr->status_id = CRON_TASK_DONE;
							db->SaveCronTask(cr);
						}					
                    }
					catch(TError& error)
					{
						cr->status_id = CRON_TASK_ERROR;
						cr->error = TUtility::UnicodeToUtf8(error.message);
						db->SaveCronTask(cr);
					}
                    catch(...)
                    {
						cr->status_id = CRON_TASK_ERROR;
						cr->error = "Cron exception caught!";
						db->SaveCronTask(cr);
                    }

					queue->DeleteQueueItem(rep.queue_item_id);
					SAFEDELETE(session)
					SAFEDELETE(resp)
					SAFEDELETE(req)
				}
				
			}

		}
		catch (...)
		{
			
		}

		SAFEDELETE(tasks)
		TUtility::UtilitySleep(sleep_time);
		SAFEDELETE(resp)
		SAFEDELETE(req)
		SAFEDELETE(session)
	}
#endif
}

void PandoraTest::StopCron()
{
	stop_cron = true;
}
void PandoraTest::CronPause()
{
	cmutex.lock();
	cron_new_req = false;
	cmutex.unlock();
}
void PandoraTest::CronResume()
{
	cmutex.lock();
	cron_new_req = true;
	cmutex.unlock();
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
void PandoraTest::RequestProcessor()
{
	TResponseInfo * resp = NULL;
	map<string, string>::iterator it;
	string req;
	string r_mes;
	string req_type = "";
	TQueueItem * Qit = NULL;
	while(1)
	{

		bool accept = false;
		TUtility::UtilitySleep(1);
		if (stop_server)
			break;

		TQueueItem * Qit = queue->GetRequest();

		if(Qit == NULL)
			continue;

		it = Qit->request_->post_parms.find("json");
		if (it == Qit->request_->post_parms.end())
		{
			it = Qit->request_->post_parms.find("xml");
		}

		if(it != Qit->request_->post_parms.end())
		{
			req_type = it->first;
			req = TUtility::base64_decode(Qit->request_->post_parms[req_type]);
			if (req == "")
			{
				req = Qit->request_->post_parms[req_type];
			}

			if(Qit->request_->info == NULL)
				Qit->request_->info = ParseRequestInfo(req);

			if(Qit->request_->info == NULL)
			{
				TResponseInfo * resp = new TResponseInfo();
				resp->succesfull = 0;
				resp->message_code = "P_PARSE_REQUEST_INFO";

				if (req_type == "json")
					r_mes = resp->ToJson(0);
				else
					r_mes = resp->ToXml(0);

				r_mes = PrepareHTTPResponce(r_mes);
				queue->AddResponse(Qit->id, r_mes);
				delete resp;
				continue;
			}

			if(boost::this_thread::get_id() != main_thread_id)
			{
				if(Qit->request_->info->command.find("Get", 0) == string::npos || Qit->request_->info->command.find("Get", 0) != 0)
				{
					queue->RefuseRequest(Qit);
					continue;
				}
			}
			int begin = TUtility::GetTimeMilliseconds();

			resp = ExecCommand(Qit->request_);
			resp->data = TUtility::UriEncode(resp->data);

			if (Qit->status == 3)
			{
				delete resp;
				SAFEDELETE(Qit)
				continue;
			}

			resp->exec_time = TUtility::GetTimeMilliseconds() - begin;

			if (req_type == "json")
				r_mes = resp->ToJson(0);
			else
				r_mes = resp->ToXml(0);

			r_mes = PrepareHTTPResponce(r_mes);

			queue->AddResponse(Qit->id, r_mes);

			delete resp;
			continue;
		}
		else
		{
			TResponseInfo * resp = new TResponseInfo();
			resp->succesfull = 0;
			resp->message = "Pandora: Unknown directive";
			resp->message_code = "P_UNKNOWN_DIRECTIVE";

			if (req_type == "xml")
				r_mes = resp->ToXml(0);
			else
				r_mes = resp->ToJson(0);

			r_mes = PrepareHTTPResponce(r_mes);

			delete resp;
			resp = NULL;
			queue->AddResponse(Qit->id, r_mes);
			continue;

		}
	}
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
request * PandoraTest::PrepareRequest(string command, string data, string session, string parameters, string response_type)
{
	TRequestInfo * reqv = new TRequestInfo(command, data, session, "", parameters, response_type);
	string str = reqv->ToJson(NULL, 1, "", false);
	request * req = new request();
	req->post_parms[response_type] = str;
	req->cookie["psession"] = session;

	return req;
}
string PandoraTest::PrepareHTTPRequest(bool post_req, string post_req_type, string http_url, string host, string request_data)
{
	stringstream request_stream;
	string s = "", req_message_type = "", req_type = "";

	if (post_req_type == "gjson")
		req_message_type = "gjson=";
	else if(post_req_type == "json")
		req_message_type = "json=";

	if(post_req)
	{
		req_type = "POST ";
	}
	else
		req_type = "GET ";

	http_url += " HTTP/1.0";

    request_stream << req_type << http_url << "\r\n"; //
    request_stream << "Host: " << host <<"\r\n";
    request_stream << "Accept: */*\r\n";
	request_stream << "Content-Length: "<< request_data.length()+ req_message_type.length() <<"\r\n";
	request_stream << "Cookie: \r\n";
	request_stream << "Content-Type: application/x-www-form-urlencoded\r\n";
	request_stream << "Connection: close\r\n\r\n";


	if(post_req)
		request_stream << req_message_type << request_data;

	s = request_stream.str();
	return s;
}

TRequestInfo * PandoraTest::ParseRequestInfo(string &req)
{

	TRequestInfo * info = NULL;
	try
	{
		info = ParseRequestInfoJson(req);
	}

	catch (...)
	{
	    ProcessError(NULL, "", "ParseRequest  error" , "");
		SAFEDELETE(info)
	}

	return info;
}
TRequestInfo * PandoraTest::ParseRequestInfoJson(string &req)
{
	TRequestInfo* info = NULL;
	try
	{
		info = new TRequestInfo();

		int datab = (int)req.find("\"data\"", 0) + 6;
		datab = (int)req.find(":", datab) + 1;
		datab = (int)req.find_first_of("\"", datab) + 1;
		int datae = (int)req.find("\"", datab);

		string data = req.substr(datab,datae - datab);
		req = req.erase(datab,datae - datab);

		int paramb = (int)req.find("\"parameters\"") + 12;
		paramb = (int)req.find_first_of(":", paramb) + 1;
		paramb = (int)req.find_first_of("\"", paramb) + 1;
		int parame = (int)req.find("\"", paramb);

		string param = req.substr(paramb,parame - paramb);
		req = req.erase(paramb,parame - paramb);

		info->FromJson(req);
		info->data = data;
		info->parameters = param;

	}

	catch (...)
	{
		SAFEDELETE(info)
		throw 0;
	}

	return info;
}

map <wstring, wstring> PandoraTest::ParseParameters(string params)
{
	map <wstring, wstring> tmp;

	if(params == "")
	{
		return tmp;
	}

	params = TUtility::UriDecode(params);


	try{

		tmp = ParseParametersJson(params);

	}

	catch(...)
	{
	    ProcessError(NULL, "", "Parse Parameters Error" , "");
	}


	return tmp;
}

map <wstring, wstring> PandoraTest::ParseParametersJson(string params)
{
	map <wstring, wstring> tmp;
	wstring name;
	Value v;

	try{

		read_string_or_throw( params, v );
		Object& obj = v.get_obj();
		for (Object::size_type i = 0; i < obj.size(); ++i)
		{
			const Pair& pair = obj[i];

			const string& name_  = pair.name_;
			const Value&  value_ = pair.value_;
			name = TUtility::Utf8ToUnicode(name_);
			Value_type type = value_.type();

			if (type == str_type)
			{
				tmp[name] = TUtility::Utf8ToUnicode(value_.get_str());
			}
			else if(type == int_type)
			{
				tmp[name] = TUtility::itows(value_.get_int());
			}
			else if (type == real_type)
			{
				tmp[name] = TUtility::ftows(value_.get_real());
			}
			else if(type == bool_type)
			{
				tmp[name] = TUtility::itows((int)value_.get_bool());
			}
		}

	}
	catch(json_spirit::Error_position e )
	{
	    ProcessError(NULL, e.reason_.c_str(), "json_spirit::Error_position exception raised." , "");
		throw e;
	}
	catch(std::runtime_error e )
	{
	    ProcessError(NULL, "", "std::runtime_error exception raised." , "");
		throw e;
	}
	catch(...)
	{
	    ProcessError(NULL, "", "json_spirit::read_string exception raised." , "");
		throw 0;
	}


	return tmp;
}

vector<int> PandoraTest::ParseVectorIds(string params)
{
	vector<int> tmp;

	if(params == "")
	{
		return tmp;
	}

	params = TUtility::UriDecode(params);

	try{

		tmp = ParseVectorIdsJson(params);


	}

	catch(...)
	{
	    ProcessError(NULL, "", "Parse Parameters Error!" , "");
	}


	return tmp;
}
vector<int> PandoraTest::ParseVectorIdsJson(string params)
{
	vector<int> tmp;
	wstring name;
	Value v;

	try{

		read_string_or_throw( params, v );

		Object& obj = v.get_obj();
		for (Array::size_type i = 0; i < obj.size(); ++i)
		{
			const Pair& pair = obj[i];

			const string& name_  = pair.name_;
			const Value&  value_ = pair.value_;

			Value_type type = value_.type();

			if (type == array_type)
			{
				Array arr = value_.get_array();
				for (Array::size_type j = 0; j < arr.size(); ++j)
				{
					const Value& vl = arr[j];
					Value_type tp = vl.type();
					if (tp == int_type)
					{
						tmp.push_back((vl.get_int()));
					}
					else if (tp == str_type)
					{
						tmp.push_back(atoi(vl.get_str().c_str()));
					}


				}
			}

		}

	}
	catch(json_spirit::Error_position e )
	{
	    ProcessError(NULL, e.reason_.c_str(), "json_spirit::Error_position exception raised." , "");
		throw e;
	}
	catch(std::runtime_error e )
	{
	    ProcessError(NULL, "", "std::runtime_error exception raised." , "");
		throw e;
	}
	catch(...)
	{
	    ProcessError(NULL, "", "json_spirit::read_string exception raised." , "");
		throw 0;
	}


	return tmp;
}
vector<wstring> PandoraTest::ParseVectorString(string params)
{
	vector<wstring> tmp;

	if(params == "")
	{
		return tmp;
	}

	params = TUtility::UriDecode(params);

	try{

		tmp = ParseVectorStringJson(params);


	}

	catch(...)
	{
		ProcessError(NULL, "", "Parse Parameters Error" , "");
	}


	return tmp;
}
vector<wstring> PandoraTest::ParseVectorStringJson(string params)
{
	vector<wstring> tmp;
	wstring name;
	Value v;

	try{

		read_string_or_throw( params, v );

		Object& obj = v.get_obj();
		for (Array::size_type i = 0; i < obj.size(); ++i)
		{
			const Pair& pair = obj[i];

			const string& name_  = pair.name_;
			const Value&  value_ = pair.value_;

			Value_type type = value_.type();

			if (type == array_type)
			{
				Array arr = value_.get_array();
				for (Array::size_type j = 0; j < arr.size(); ++j)
				{
					const Value& vl = arr[j];
					Value_type tp = vl.type();

					if (tp == str_type)
					{
						tmp.push_back(TUtility::Utf8ToUnicode((vl.get_str())));
					}


				}
			}

		}

	}
	catch(json_spirit::Error_position e )
	{
	    ProcessError(NULL, e.reason_.c_str(), "json_spirit::Error_position exception raised." , "");
		throw e;
	}
	catch(std::runtime_error e )
	{
	    ProcessError(NULL, "", "std::runtime_error exception raised." , "");
		throw e;
	}
	catch(...)
	{
	    ProcessError(NULL, "", "json_spirit::read_string exception raised." , "");
		throw 0;
	}


	return tmp;
}

/************************************************************************/
/* SESSIONS                                                             */
/************************************************************************/

TResponseInfo * PandoraTest::LoginRequest(request * req)
{

	wstring login = L"", passw = L"";
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int user_id = 0;
	TSession * session = NULL;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->data);
		login =  params[L"login"];
		passw = params[L"password"];

//		user_id = db->CheckUserLogin(login, passw);

		if(user_id != 0)
		{
			if(first_run)
			{
				stringstream url;
				url << "/install.php?" << "op=server&version=" << _VERSION << "&key=" << config->content["install_key"];
				manager->SendHttpRequest("www.monely.com", "80", PrepareHTTPRequest(false, "", url.str(), "www.monely.com", ""), 3, false);
				first_run = false;
			}


			int sid = 0;//db->GetObjectID();
			string cookie = GenCookie(sid);

			session = new TSession(sid, user_id, req->ip, login, cookie, pandora_host, pandora_port, TUtility::GetProgramPid());

			bool res = true;//db->AddSession(session);

			if (res)
			{
				resp->succesfull = 1;
				resp->message = "Pandora Login Success!";
				resp->data = TUtility::ToJson(cookie, string("session"));
			}
			else
			{
				resp->succesfull = 0;
				resp->message = "Cannot Add Session";
				resp->message_code = "P_CANNOT_ADD_SESSION";
			}

		}

		else
		{
			resp->succesfull = 0;
			resp->message = "Wrong user name or password";
			resp->message_code = "P_WRONG_USER_NAME_OR_PASSWORD";
		}

	}

	catch (...)
	{
		ProcessError(resp, "", "Login  error" , "");

	}

	SAFEDELETE(session)
		return resp;
}
TResponseInfo * PandoraTest::LogoutRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();

	try
	{
		if(1)
//		if(db->RemoveSession(req->cookie["psession"]))
		{
			resp->succesfull = 1;
			resp->message = "Logout succesfull";

			return resp;
		}
		else
		{
			resp->succesfull = 0;
			resp->message = "Logout Error";
			resp->message_code = "P_LOGOUT_ERROR";
			return resp;
		}

	}

	catch (...)
	{
		ProcessError(resp, "", "Login  error" , "");

	}

	return resp;

}
TResponseInfo * PandoraTest::CheckSessionRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();
	int user_id = 0;
	bool result = false;

	try
	{
		TUtility::CheckZeroValue(1, true, req->info->language);
//		user_id = db->CheckSession(req->cookie["psession"], req->ip);

		if(user_id == 0)
		{
			TError error;
			error.code = "session_timeout";
			error.message = L"Session is expired";
			throw error;
		}
		else
		{
			req->user_id = user_id;
			resp->succesfull = 1;
		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, TUtility::UnicodeToUtf8(error.message), TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)), false);

	}
	catch (...)
	{
		ProcessError(resp, "", "CheckSession  error" , "", false);

	}

	return resp;
}

TResponseInfo * PandoraTest::GetSessionsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TSession> * sessions = NULL;

	try
	{

//		sessions = db->GetSessions();

		if(sessions != NULL)
		{
			resp->data = sessions->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetSessions operation succesfull!";
		}
		else
		{
			resp->succesfull = 0;
			resp->message = "GetSessions operation error!";
			resp->message_code = "G_GET_SESSIONS_ERROR";
		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetSessions  error" , "");

	}

	SAFEDELETE(sessions)
		return resp;
}

TResponseInfo * PandoraTest::ValidateParameters(request * req)
{
	TResponseInfo * resp = new TResponseInfo();


	try
	{
		TUtility::CheckZeroValue(1, true, req->info->language);
		resp->succesfull = 1;

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, TUtility::UnicodeToUtf8(error.message), TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)), false);

	}
	catch (...)
	{
		ProcessError(resp, "", "CheckSession  error" , "", false);

	}

	return resp;
}
/************************************************************************/
/* EXEC COMMAND                                                         */
/************************************************************************/
void PandoraTest::UpdateSessionData(TResponseInfo * resp, request * req)
{
//	db->SaveSession(req->cookie["psession"], req->last_request_time, 1 ,req->source.size(), 0, 0);

	if (!resp->succesfull)
	{
//		db->SaveSession(req->cookie["psession"], 0,0,0,0,1);
	}
}

TResponseInfo * PandoraTest::ExecCommand(request * req)
{

	TRequestInfo * info = req->info;
	TResponseInfo * resp = NULL;
/*
	try
	{
		if(info->command == "Login")
		{
			resp = LoginRequest(req);
			return resp;
		}

		if(info->command == "Logout")
		{
			resp = LogoutRequest(req);
			return resp;
		}
		*/

		if(info->command == "GetLanguages")
		{
			resp = GetLanguagesRequest(req);
			return resp;
		}
		/*
		if(info->command == "GetCheckSession")
		{
			resp = CheckSessionRequest(req);
			return resp;
		}


		if (info->key == config->content["key"])
		{
			req->user_id = ADMIN_USER;
		}
		else
		{
			resp = CheckSessionRequest(req);
			if(!resp->succesfull)
			{
				return resp;
			}
			SAFEDELETE(resp)
		}

		resp = ValidateParameters(req);
		if(!resp->succesfull)
			return resp;
		else
			SAFEDELETE(resp)

		if(info->command == "AddLanguage")
		{
			resp = AddLanguageRequest(req);
			return resp;
		}

		if(info->command == "SaveLanguage")
		{
			resp = SaveLanguageRequest(req);
			return resp;
		}

		if(info->command == "RemoveLanguage")
		{
			resp = RemoveLanguageRequest(req);
			return resp;
		}

		if(info->command == "GetLanguage")
		{
			resp = GetLanguageRequest(req);
			return resp;
		}


		if(info->command == "SaveDefaultLanguage")
		{
			resp = SaveDefaultLanguageRequest(req);
			return resp;
		}

		if(info->command == "ActivateLanguages")
		{
			resp = ActivateLanguagesRequest(req);
			return resp;
		}

		//Statuses

		if(info->command == "AddStatus")
		{
			resp = AddStatusRequest(req);
			return resp;
		}

		if(info->command == "SaveStatus")
		{
			resp = SaveStatusRequest(req);
			return resp;
		}

		if(info->command == "RemoveStatus")
		{
			resp = RemoveStatusRequest(req);
			return resp;
		}

		if(info->command == "GetStatus")
		{
			resp = GetStatusRequest(req);
			return resp;
		}

		if(info->command == "GetStatuses")
		{
			resp = GetStatusesRequest(req);
			return resp;
		}

		//Measurements

		if(info->command == "AddMeasurement")
		{
			resp = AddMeasurementRequest(req);
			return resp;
		}

		if(info->command == "SaveMeasurement")
		{
			resp = SaveMeasurementRequest(req);
			return resp;
		}

		if(info->command == "RemoveMeasurement")
		{
			resp = RemoveMeasurementRequest(req);
			return resp;
		}

		if(info->command == "GetMeasurement")
		{
			resp = GetMeasurementRequest(req);
			return resp;
		}

		if(info->command == "GetMeasurements")
		{
			resp = GetMeasurementsRequest(req);
			return resp;
		}

		if(info->command == "SaveDefaultCurrency")
		{
			resp = SaveDefaultCurrencyRequest(req);
			return resp;
		}
		//Rates

		if(info->command == "AddRate")
		{
			resp = AddRateRequest(req);
			return resp;
		}

		if(info->command == "SaveRate")
		{
			resp = SaveRateRequest(req);
			return resp;
		}

		if(info->command == "RemoveRate")
		{
			resp = RemoveRateRequest(req);
			return resp;
		}

		if(info->command == "GetRateById")
		{
			resp = GetRateByIdRequest(req);
			return resp;
		}

		if(info->command == "GetRate")
		{
			resp = GetRateRequest(req);
			return resp;
		}

		if(info->command == "GetRateView")
		{
			resp = GetRateViewRequest(req);
			return resp;
		}

		if(info->command == "GetRates")
		{
			resp = GetRatesRequest(req);
			return resp;
		}

		if(info->command == "GetRatesView")
		{
			resp = GetRatesViewRequest(req);
			return resp;
		}
		//Categories

		if(info->command == "GetCategories")
		{
			resp = GetCategoriesRequest(req);
			return resp;
		}

		if(info->command == "AddCategory")
		{
			resp = AddCategoryRequest(req);
			return resp;
		}

		if(info->command == "SaveCategory")
		{
			resp = SaveCategoryRequest(req);
			return resp;
		}

		if(info->command == "RemoveCategory")
		{
			resp = RemoveCategoryRequest(req);
			return resp;
		}

		if(info->command == "GetCategory")
		{
			resp = GetCategoryRequest(req);
			return resp;
		}

		//Persons

		if(info->command == "AddPerson")
		{
			resp = AddPersonRequest(req);
			return resp;
		}

		if(info->command == "SavePerson")
		{
			resp = SavePersonRequest(req);
			return resp;
		}

		if(info->command == "RemovePerson")
		{
			resp = RemovePersonRequest(req);
			return resp;
		}

		if(info->command == "GetPerson")
		{
			resp = GetPersonRequest(req);
			return resp;
		}

		//Companies

		if(info->command == "AddCompany")
		{
			resp = AddCompanyRequest(req);
			return resp;
		}

		if(info->command == "SaveCompany")
		{
			resp = SaveCompanyRequest(req);
			return resp;
		}

		if(info->command == "RemoveCompany")
		{
			resp = RemoveCompanyRequest(req);
			return resp;
		}

		if(info->command == "GetCompany")
		{
			resp = GetCompanyRequest(req);
			return resp;
		}


		//Partners

		if(info->command == "AddPartner")
		{
			resp = AddPartnerRequest(req);
			return resp;
		}

		if(info->command == "SavePartner")
		{
			resp = SavePartnerRequest(req);
			return resp;
		}

		if(info->command == "RemovePartner")
		{
			resp = RemovePartnerRequest(req);
			return resp;
		}

		if(info->command == "GetPartner")
		{
			resp = GetPartnerRequest(req);
			return resp;
		}

		if(info->command == "GetPartners")
		{
			resp = GetPartnersRequest(req);
			return resp;
		}

		if(info->command == "GetPartnersSearchView")
		{
			resp = GetPartnersSearchViewRequest(req);
			return resp;
		}
		//Employees

		if(info->command == "AddEmployee")
		{
			resp = AddEmployeeRequest(req);
			return resp;
		}

		if(info->command == "SaveEmployee")
		{
			resp = SaveEmployeeRequest(req);
			return resp;
		}

		if(info->command == "RemoveEmployee")
		{
			resp = RemoveEmployeeRequest(req);
			return resp;
		}

		if(info->command == "GetEmployee")
		{
			resp = GetEmployeeRequest(req);
			return resp;
		}

		if(info->command == "GetEmployees")
		{
			resp = GetEmployeesRequest(req);
			return resp;
		}

		if(info->command == "GetEmployeesView")
		{
			resp = GetEmployeesViewRequest(req);
			return resp;
		}

		//Employees

		if(info->command == "AddContactPerson")
		{
			resp = AddContactPersonRequest(req);
			return resp;
		}

		if(info->command == "SaveContactPerson")
		{
			resp = SaveContactPersonRequest(req);
			return resp;
		}

		if(info->command == "RemoveContactPerson")
		{
			resp = RemoveContactPersonRequest(req);
			return resp;
		}

		if(info->command == "GetContactPerson")
		{
			resp = GetContactPersonRequest(req);
			return resp;
		}

		if(info->command == "GetContactPersonsView")
		{
			resp = GetContactPersonsViewRequest(req);
			return resp;
		}
		//PartnerView

		if(info->command == "GetPartnerView")
		{
			resp = GetPartnerViewRequest(req);
			return resp;
		}

		if(info->command == "GetPartnersView")
		{
			resp = GetPartnersViewRequest(req);
			return resp;
		}

		if(info->command == "SavePartnerName")
		{
			resp = SavePartnerNameRequest(req);
			return resp;
		}


		// Partner Balances
		if(info->command == "GetPartnerBalanceView")
		{
			resp = GetPartnerBalanceViewRequest(req);
			return resp;
		}

		if(info->command == "GetPartnerBalancesView")
		{
			resp = GetPartnerBalancesViewRequest(req);
			return resp;
		}


		//Settings

		if(info->command == "GetSettings")
		{
			resp = GetSettingsRequest(req);
			return resp;
		}

		if(info->command == "SetSettings")
		{
			resp = SetSettingsRequest(req);
			return resp;
		}

		//CategoriesObjects

		if(info->command == "SaveCategoriesByObject")
		{
			resp = SaveCategoriesByObjectRequest(req);
			return resp;
		}

		if(info->command == "GetCategoriesByObject")
		{
			resp = GetCategoriesByObjectRequest(req);
			return resp;
		}

		//Employees

		if(info->command == "AddProduct")
		{
			resp = AddProductRequest(req);
			return resp;
		}

		if(info->command == "GetProduct")
		{
			resp = GetProductRequest(req);
			return resp;
		}

		if(info->command == "RemoveProduct")
		{
			resp = RemoveProductRequest(req);
			return resp;
		}

		if(info->command == "AddTangibleProduct")
		{
			resp = AddTangibleProductRequest(req);
			return resp;
		}

		if(info->command == "SaveTangibleProduct")
		{
			resp = SaveTangibleProductRequest(req);
			return resp;
		}

		if(info->command == "RemoveTangibleProduct")
		{
			resp = RemoveTangibleProductRequest(req);
			return resp;
		}

		if(info->command == "GetTangibleProduct")
		{
			resp = GetTangibleProductRequest(req);
			return resp;
		}

		if(info->command == "GetTangibleProducts")
		{
			resp = GetTangibleProductsRequest(req);
			return resp;
		}



		if(info->command == "GetTangibleProductView")
		{
			resp = GetTangibleProductViewRequest(req);
			return resp;
		}

		if(info->command == "GetProductView")
		{
			resp = GetProductViewRequest(req);
			return resp;
		}

		if(info->command == "GetProductsView")
		{
			resp = GetProductsViewRequest(req);
			return resp;
		}
		if(info->command == "GetProductsSearchView")
		{
			resp = GetProductsSearchViewRequest(req);
			return resp;
		}

		if(info->command == "SaveProductMinStockLevels")
		{
			resp = SaveProductMinStockLevelsRequest(req);
			return resp;
		}
		if(info->command == "GetProductMinStockLevelsView")
		{
			resp = GetProductMinStockLevelsViewRequest(req);
			return resp;
		}

		//Product Substitute

		if(info->command == "AddProductSub")
		{
			resp = AddProductSubRequest(req);
			return resp;
		}

		if(info->command == "SaveProductSub")
		{
			resp = SaveProductSubRequest(req);
			return resp;
		}

		if(info->command == "RemoveProductSub")
		{
			resp = RemoveProductSubRequest(req);
			return resp;
		}

		if(info->command == "GetProductSub")
		{
			resp = GetProductSubRequest(req);
			return resp;
		}

		if(info->command == "GetProductSubs")
		{
			resp = GetProductSubsRequest(req);
			return resp;
		}

		if(info->command == "GetProductSubsView")
		{
			resp = GetProductSubsViewRequest(req);
			return resp;
		}

		if(info->command == "GetProductSubView")
		{
			resp = GetProductSubViewRequest(req);
			return resp;
		}

		// BUNDLES

		if(info->command == "AddBundle")
		{
			resp = AddBundleRequest(req);
			return resp;
		}

		if(info->command == "SaveBundle")
		{
			resp = SaveBundleRequest(req);
			return resp;
		}

		if(info->command == "RemoveBundle")
		{
			resp = RemoveBundleRequest(req);
			return resp;
		}

		if(info->command == "GetBundle")
		{
			resp = GetBundleRequest(req);
			return resp;
		}


		if(info->command == "GetBundleView")
		{
			resp = GetBundleViewRequest(req);
			return resp;
		}


		//IMAGES

		if(info->command == "AddImage")
		{
			resp = AddImageRequest(req);
			return resp;
		}

		if(info->command == "SaveImage")
		{
			resp = SaveImageRequest(req);
			return resp;
		}

		if(info->command == "RemoveImage")
		{
			resp = RemoveImageRequest(req);
			return resp;
		}

		if(info->command == "GetImage")
		{
			resp = GetImageRequest(req);
			return resp;
		}

		if(info->command == "GetImages")
		{
			resp = GetImagesRequest(req);
			return resp;
		}

		//Product Kits

		if(info->command == "AddProductKit")
		{
			resp = AddProductKitRequest(req);
			return resp;
		}

		if(info->command == "SaveProductKit")
		{
			resp = SaveProductKitRequest(req);
			return resp;
		}

		if(info->command == "RemoveProductKit")
		{
			resp = RemoveProductKitRequest(req);
			return resp;
		}

		if(info->command == "GetProductKit")
		{
			resp = GetProductKitRequest(req);
			return resp;
		}

		if(info->command == "GetProductKits")
		{
			resp = GetProductKitsRequest(req);
			return resp;
		}

		if(info->command == "GetProductKitsView")
		{
			resp = GetProductKitsViewRequest(req);
			return resp;
		}

		if(info->command == "GetProductKitView")
		{
			resp = GetProductKitViewRequest(req);
			return resp;
		}

		//SERVICE

		if(info->command == "AddService")
		{
			resp = AddServiceRequest(req);
			return resp;
		}

		if(info->command == "SaveService")
		{
			resp = SaveServiceRequest(req);
			return resp;
		}

		if(info->command == "RemoveService")
		{
			resp = RemoveServiceRequest(req);
			return resp;
		}

		if(info->command == "GetService")
		{
			resp = GetServiceRequest(req);
			return resp;
		}

		if(info->command == "GetServiceView")
		{
			resp = GetServiceViewRequest(req);
			return resp;
		}

		//Software

		if(info->command == "AddSoftware")
		{
			resp = AddSoftwareRequest(req);
			return resp;
		}

		if(info->command == "SaveSoftware")
		{
			resp = SaveSoftwareRequest(req);
			return resp;
		}

		if(info->command == "RemoveSoftware")
		{
			resp = RemoveSoftwareRequest(req);
			return resp;
		}

		if(info->command == "GetSoftware")
		{
			resp = GetSoftwareRequest(req);
			return resp;
		}

		if(info->command == "GetSoftwareView")
		{
			resp = GetSoftwareViewRequest(req);
			return resp;
		}

		//Warehouse

		if(info->command == "AddWarehouse")
		{
			resp = AddWarehouseRequest(req);
			return resp;
		}

		if(info->command == "SaveWarehouse")
		{
			resp = SaveWarehouseRequest(req);
			return resp;
		}

		if(info->command == "RemoveWarehouse")
		{
			resp = RemoveWarehouseRequest(req);
			return resp;
		}

		if(info->command == "GetWarehouse")
		{
			resp = GetWarehouseRequest(req);
			return resp;
		}
		if(info->command == "GetWarehouses")
		{
			resp = GetWarehousesRequest(req);
			return resp;
		}

		//Users

		if(info->command == "AddUser")
		{
			resp = AddUserRequest(req);
			return resp;
		}

		if(info->command == "SaveUser")
		{
			resp = SaveUserRequest(req);
			return resp;
		}

		if(info->command == "RemoveUser")
		{
			resp = RemoveUserRequest(req);
			return resp;
		}

		if(info->command == "GetUser")
		{
			resp = GetUserRequest(req);
			return resp;
		}
		if(info->command == "GetUsers")
		{
			resp = GetUsersRequest(req);
			return resp;
		}
		if(info->command == "GetUsersView")
		{
			resp = GetUsersViewRequest(req);
			return resp;
		}
		if(info->command == "GetUserView")
		{
			resp = GetUserViewRequest(req);
			return resp;
		}
		if(info->command == "SaveUserPassword")
		{
			resp = SaveUserPasswordRequest(req);
			return resp;
		}
		//Users Online

		if(info->command == "GetUsersOnline")
		{
			resp = GetSessionsRequest(req);
			return resp;
		}

		//Warehouse

		if(info->command == "AddWarehouse")
		{
			resp = AddWarehouseRequest(req);
			return resp;
		}
		if(info->command == "SaveWarehouse")
		{
			resp = SaveWarehouseRequest(req);
			return resp;
		}
		if(info->command == "RemoveWarehouse")
		{
			resp = RemoveWarehouseRequest(req);
			return resp;
		}
		if(info->command == "GetWarehouse")
		{
			resp = GetWarehouseRequest(req);
			return resp;
		}
		if(info->command == "GetWarehouses")
		{
			resp = GetWarehousesRequest(req);
			return resp;
		}

		//PutInDocuments

		if(info->command == "AddPutInDocument")
		{
			resp = AddPutInDocumentRequest(req);
			return resp;
		}

		if(info->command == "SavePutInDocument")
		{
			resp = SavePutInDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemovePutInDocument")
		{
			resp = RemovePutInDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetPutInDocument")
		{
			resp = GetPutInDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetPutInDocumentView")
		{
			resp = GetPutInDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetPutInDocuments")
		{
			resp = GetPutInDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetPutInDocumentsView")
		{
			resp = GetPutInDocumentsViewRequest(req);
			return resp;
		}

		if(info->command == "PostPutInDocument")
		{
			resp = PostPutInDocumentRequest(req);
			return resp;
		}
		if(info->command == "UnpostPutInDocument")
		{
			resp = UnpostPutInDocumentRequest(req);
			return resp;
		}

		//DOCUMENTS

		if(info->command == "GetDocuments")
		{
			resp = GetDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetDocumentsView")
		{
			resp = GetDocumentsViewRequest(req);
			return resp;
		}
		if(info->command == "GetDocumentTypeId")
		{
			resp = GetDocumentTypeIdRequest(req);
			return resp;
		}
		if(info->command == "GetDocumentView")
		{
			resp = GetDocumentViewRequest(req);
			return resp;
		}

		//Doc Items
		if(info->command == "AddDocItem")
		{
			resp = AddDocItemRequest(req);
			return resp;
		}

		if(info->command == "AddDocItemPutIn")
		{
			resp = AddPutInDocItemRequest(req);
			return resp;
		}

		if(info->command == "SaveDocItemPutIn")
		{
			resp = SavePutInDocItemRequest(req);
			return resp;
		}
		if(info->command == "SaveDocItemsPutIn")
		{
			resp = SavePutInDocItemsRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemPutIn")
		{
			resp = RemovePutInDocItemRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemPutIn")

		{
			resp = GetPutInDocItemRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemsPutInView")
		{
			resp = GetPutInDocItemsViewRequest(req);
			return resp;
		}

		//TAXES

		if(info->command == "AddTax")
		{
			resp = AddTaxRequest(req);
			return resp;
		}

		if(info->command == "SaveTax")
		{
			resp = SaveTaxRequest(req);
			return resp;
		}

		if(info->command == "RemoveTax")
		{
			resp = RemoveTaxRequest(req);
			return resp;
		}

		if(info->command == "GetTax")
		{
			resp = GetTaxRequest(req);
			return resp;
		}
		if(info->command == "GetTaxes")
		{
			resp = GetTaxesRequest(req);
			return resp;
		}
		if(info->command == "GetTaxesView")
		{
			resp = GetTaxesViewRequest(req);
			return resp;
		}
		if(info->command == "AddCustomTaxes")
		{
			resp = AddCustomTaxesRequest(req);
			return resp;
		}

		// Discounts

		if(info->command == "GetDiscountsView")
		{
			resp = GetDiscountsViewRequest(req);
			return resp;
		}
		if(info->command == "AddCustomDiscounts")
		{
			resp = AddCustomDiscountsRequest(req);
			return resp;
		}


		if(info->command == "GetLogsView")
		{
			resp = GetLogsViewRequest(req);
			return resp;
		}


		//Write off documents

		if(info->command == "AddWriteOffDocument")
		{
			resp = AddWriteOffDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveWriteOffDocument")
		{
			resp = SaveWriteOffDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveWriteOffDocument")
		{
			resp = RemoveWriteOffDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetWriteOffDocument")
		{
			resp = GetWriteOffDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetWriteOffDocumentView")
		{
			resp = GetWriteOffDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetWriteOffDocuments")
		{
			resp = GetWriteOffDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetWriteOffDocumentsView")
		{
			resp = GetWriteOffDocumentsViewRequest(req);
			return resp;
		}
		if(info->command == "PostWriteOffDocument")
		{
			resp = PostWriteOffDocumentRequest(req);
			return resp;
		}
		if(info->command == "UnpostWriteOffDocument")
		{
			resp = UnpostWriteOffDocumentRequest(req);
			return resp;
		}


		if(info->command == "AddDocItemWriteOff")
		{
			resp = AddDocItemWriteOffRequest(req);
			return resp;
		}

		if(info->command == "SaveDocItemWriteOff")
		{
			resp = SaveDocItemWriteOffRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemWriteOff")
		{
			resp = RemoveDocItemWriteOffRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemWriteOff")
		{
			resp = GetDocItemWriteOffRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemsWriteOffView")
		{
			resp = GetDocItemsWriteOffViewRequest(req);
			return resp;
		}
		if(info->command == "ReserveDocItemWriteOff")
		{
			resp = ReserveDocItemWriteOffRequest(req);
			return resp;
		}
		if(info->command == "UnReserveDocItemWriteOff")
		{
			resp = UnReserveDocItemWriteOffRequest(req);
			return resp;
		}


		if(info->command == "GetLotsView")
		{
			resp = GetLotsViewRequest(req);
			return resp;
		}
		if(info->command == "GetLotView")
		{
			resp = GetLotViewRequest(req);
			return resp;
		}
		if(info->command == "GetDocItemLotsView")
		{
			resp = GetDocItemLotsViewRequest(req);
			return resp;
		}
		if(info->command == "GetDocItemLotView")
		{
			resp = GetDocItemLotViewRequest(req);
			return resp;
		}


		if(info->command == "AddBankAccount")
		{
			resp = AddBankAccountRequest(req);
			return resp;
		}
		if(info->command == "SaveBankAccount")
		{
			resp = SaveBankAccountRequest(req);
			return resp;
		}

		if(info->command == "RemoveBankAccount")
		{
			resp = RemoveBankAccountRequest(req);
			return resp;
		}

		if(info->command == "GetBankAccount")
		{
			resp = GetBankAccountRequest(req);
			return resp;
		}
		if(info->command == "GetBankAccountView")
		{
			resp = GetBankAccountViewRequest(req);
			return resp;
		}
		if(info->command == "GetBankAccounts")
		{
			resp = GetBankAccountsRequest(req);
			return resp;
		}
		if(info->command == "GetBankAccountsView")
		{
			resp = GetBankAccountsViewRequest(req);
			return resp;
		}


		if(info->command == "GetDocPaymentSum")
		{
			resp = GetDocPaymentSumRequest(req);
			return resp;
		}


		if(info->command == "AddInPaymentDocument")
		{
			resp = AddInPaymentDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveInPaymentDocument")
		{
			resp = SaveInPaymentDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveInPaymentDocument")
		{
			resp = RemoveInPaymentDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetInPaymentDocument")
		{
			resp = GetInPaymentDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetInPaymentDocumentView")
		{
			resp = GetInPaymentDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetInPaymentDocuments")
		{
			resp = GetInPaymentDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetInPaymentDocumentsView")
		{
			resp = GetInPaymentDocumentsViewRequest(req);
			return resp;
		}

		if(info->command == "PostInPaymentDocument")
		{
			resp = PostInPaymentDocumentRequest(req);
			return resp;
		}

		if(info->command == "UnpostInPaymentDocument")
		{
			resp = UnpostInPaymentDocumentRequest(req);
			return resp;
		}

		//OUT PAYMENT

		if(info->command == "AddOutPaymentDocument")
		{
			resp = AddOutPaymentDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveOutPaymentDocument")
		{
			resp = SaveOutPaymentDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveOutPaymentDocument")
		{
			resp = RemoveOutPaymentDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetOutPaymentDocument")
		{
			resp = GetOutPaymentDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetOutPaymentDocumentView")
		{
			resp = GetOutPaymentDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetOutPaymentDocuments")
		{
			resp = GetOutPaymentDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetOutPaymentDocumentsView")
		{
			resp = GetOutPaymentDocumentsViewRequest(req);
			return resp;
		}

		if(info->command == "PostOutPaymentDocument")
		{
			resp = PostOutPaymentDocumentRequest(req);
			return resp;
		}

		if(info->command == "UnpostOutPaymentDocument")
		{
			resp = UnpostOutPaymentDocumentRequest(req);
			return resp;
		}

		//ADD COSTS

		if(info->command == "AddAddCostsDocument")
		{
			resp = AddAddCostsDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveAddCostsDocument")
		{
			resp = SaveAddCostsDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveAddCostsDocument")
		{
			resp = RemoveAddCostsDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetAddCostsDocument")
		{
			resp = GetAddCostsDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetAddCostsDocumentView")
		{
			resp = GetAddCostsDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetAddCostsDocuments")
		{
			resp = GetAddCostsDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetAddCostsDocumentsView")
		{
			resp = GetAddCostsDocumentsViewRequest(req);
			return resp;
		}

		if(info->command == "PostAddCostsDocument")
		{
			resp = PostAddCostsDocumentRequest(req);
			return resp;
		}

		if(info->command == "UnpostAddCostsDocument")
		{
			resp = UnpostAddCostsDocumentRequest(req);
			return resp;
		}
		// Product Summary

		if(info->command == "CalculateProductSummary")
		{
			resp = CalculateProductSummaryRequest(req);
			return resp;
		}

		//Account Summary

		if(info->command == "GetAccountSummary")
		{
			resp = GetAccountSummaryRequest(req);
			return resp;
		}

		if(info->command == "GetAccountSummariesView")
		{
			resp = GetAccountSummariesViewRequest(req);
			return resp;
		}

		//Relation documents


		if(info->command == "GetRelationDocuments")
		{
			resp = GetRelationDocumentsRequest(req);
			return resp;
		}


		// Product In Docs
		if(info->command == "AddProductInDocument")
		{
			resp = AddProductInDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveProductInDocument")
		{
			resp = SaveProductInDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveProductInDocument")
		{
			resp = RemoveProductInDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetProductInDocument")
		{
			resp = GetProductInDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetProductInDocumentView")
		{
			resp = GetProductInDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetProductInDocuments")
		{
			resp = GetProductInDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetProductInDocumentsView")
		{
			resp = GetProductInDocumentsViewRequest(req);
			return resp;
		}

		if(info->command == "PostProductInDocument")
		{
			resp = PostProductInDocumentRequest(req);
			return resp;
		}
		if(info->command == "UnpostProductInDocument")
		{
			resp = UnpostProductInDocumentRequest(req);
			return resp;
		}

		//Doc items Product In

		if(info->command == "SaveDocItemProductIn")
		{
			resp = SaveDocItemProductInRequest(req);
			return resp;
		}

		if(info->command == "SaveDocItemsProductIn")
		{
			resp = SaveDocItemsProductInRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemProductIn")
		{
			resp = RemoveDocItemProductInRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemProductIn")
		{
			resp = GetDocItemProductInRequest(req);
			return resp;
		}
		if(info->command == "GetDocItemsProductInView")
		{
			resp = GetDocItemsProductInViewRequest(req);
			return resp;
		}

		//Doc items Services
		if(info->command == "AddDocItemProductInService")
		{
			resp = AddDocItemServiceRequest(req);
			return resp;
		}
		if(info->command == "SaveDocItemProductInService")
		{
			resp = SaveDocItemServiceRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemProductInService")
		{
			resp = RemoveDocItemServiceRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemProductInService")
		{
			resp = GetDocItemServiceRequest(req);
			return resp;
		}

		// Return In Docs
		if(info->command == "AddReturnInDocument")
		{
			resp = AddReturnInDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveReturnInDocument")
		{
			resp = SaveReturnInDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveReturnInDocument")
		{
			resp = RemoveReturnInDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetReturnInDocument")
		{
			resp = GetReturnInDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetReturnInDocumentView")
		{
			resp = GetReturnInDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetReturnInDocuments")
		{
			resp = GetReturnInDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetReturnInDocumentsView")
		{
			resp = GetReturnInDocumentsViewRequest(req);
			return resp;
		}

		if(info->command == "PostReturnInDocument")
		{
			resp = PostReturnInDocumentRequest(req);
			return resp;
		}
		if(info->command == "UnpostReturnInDocument")
		{
			resp = UnpostReturnInDocumentRequest(req);
			return resp;
		}

		// Return In DocItems
		if(info->command == "SaveDocItemReturnIn")
		{
			resp = SaveDocItemReturnInRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemReturnIn")
		{
			resp = RemoveDocItemReturnInRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemReturnIn")
		{
			resp = GetDocItemReturnInRequest(req);
			return resp;
		}
		if(info->command == "GetDocItemsReturnInView")
		{
			resp = GetDocItemsReturnInViewRequest(req);
			return resp;
		}

		// Return Out Docs
		if(info->command == "AddReturnOutDocument")
		{
			resp = AddReturnOutDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveReturnOutDocument")
		{
			resp = SaveReturnOutDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveReturnOutDocument")
		{
			resp = RemoveReturnOutDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetReturnOutDocument")
		{
			resp = GetReturnOutDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetReturnOutDocumentView")
		{
			resp = GetReturnOutDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetReturnOutDocuments")
		{
			resp = GetReturnOutDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetReturnOutDocumentsView")
		{
			resp = GetReturnOutDocumentsViewRequest(req);
			return resp;
		}

		if(info->command == "PostReturnOutDocument")
		{
			resp = PostReturnOutDocumentRequest(req);
			return resp;
		}
		if(info->command == "UnpostReturnOutDocument")
		{
			resp = UnpostReturnOutDocumentRequest(req);
			return resp;
		}

		//Doc items Return Out

		if(info->command == "SaveDocItemReturnOut")
		{
			resp = SaveDocItemReturnOutRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemReturnOut")
		{
			resp = RemoveDocItemReturnOutRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemReturnOut")
		{
			resp = GetDocItemReturnOutRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemsReturnOutView")
		{
			resp = GetDocItemsReturnOutViewRequest(req);
			return resp;
		}

		if(info->command == "ReserveDocItemReturnOut")
		{
			resp = ReserveDocItemReturnOutRequest(req);
			return resp;
		}
		if(info->command == "UnReserveDocItemReturnOut")
		{
			resp = UnReserveDocItemReturnOutRequest(req);
			return resp;
		}


		// Product Out Docs
		if(info->command == "AddProductOutDocument")
		{
			resp = AddProductOutDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveProductOutDocument")
		{
			resp = SaveProductOutDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveProductOutDocument")
		{
			resp = RemoveProductOutDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetProductOutDocument")
		{
			resp = GetProductOutDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetProductOutDocumentView")
		{
			resp = GetProductOutDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetProductOutDocuments")
		{
			resp = GetProductOutDocumentsRequest(req);
			return resp;
		}
		if(info->command == "GetProductOutDocumentsView")
		{
			resp = GetProductOutDocumentsViewRequest(req);
			return resp;
		}
		if(info->command == "PostProductOutDocument")
		{
			resp = PostProductOutDocumentRequest(req);
			return resp;
		}
		if(info->command == "UnpostProductOutDocument")
		{
			resp = UnpostProductOutDocumentRequest(req);
			return resp;
		}

		//Doc items Product Out

		if(info->command == "SaveDocItemProductOut")
		{
			resp = SaveDocItemProductOutRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemProductOut")
		{
			resp = RemoveDocItemProductOutRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemProductOut")
		{
			resp = GetDocItemProductOutRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemsProductOutView")
		{
			resp = GetDocItemsProductOutViewRequest(req);
			return resp;
		}

		if(info->command == "ReserveDocItemProductOut")
		{
			resp = ReserveDocItemProductOutRequest(req);
			return resp;
		}
		if(info->command == "UnReserveDocItemProductOut")
		{
			resp = UnReserveDocItemProductOutRequest(req);
			return resp;
		}

		// Invoice Docs
		if(info->command == "AddInvoiceDocument")
		{
			resp = AddInvoiceDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveInvoiceDocument")
		{
			resp = SaveInvoiceDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveInvoiceDocument")
		{
			resp = RemoveInvoiceDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetInvoiceDocument")
		{
			resp = GetInvoiceDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetInvoiceDocumentView")
		{
			resp = GetInvoiceDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetInvoiceDocumentsView")
		{
			resp = GetInvoiceDocumentsViewRequest(req);
			return resp;
		}
		if(info->command == "PostInvoiceDocument")
		{
			resp = PostInvoiceDocumentRequest(req);
			return resp;
		}
		if(info->command == "UnpostInvoiceDocument")
		{
			resp = UnpostInvoiceDocumentRequest(req);
			return resp;
		}

		//Doc items Invoice

		if(info->command == "SaveDocItemInvoice")
		{
			resp = SaveDocItemInvoiceRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemInvoice")
		{
			resp = RemoveDocItemInvoiceRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemInvoice")
		{
			resp = GetDocItemInvoiceRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemsInvoiceView")
		{
			resp = GetDocItemsInvoiceViewRequest(req);
			return resp;
		}

		if(info->command == "ReserveDocItemInvoice")
		{
			resp = ReserveDocItemInvoiceRequest(req);
			return resp;
		}
		if(info->command == "UnReserveDocItemInvoice")
		{
			resp = UnReserveDocItemInvoiceRequest(req);
			return resp;
		}

		// ORDER IN DOCS

		if(info->command == "AddOrderInDocument")
		{
			resp = AddOrderInDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveOrderInDocument")
		{
			resp = SaveOrderInDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveOrderInDocument")
		{
			resp = RemoveOrderInDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetOrderInDocument")
		{
			resp = GetOrderInDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetOrderInDocumentView")
		{
			resp = GetOrderInDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetOrderInDocumentsView")
		{
			resp = GetOrderInDocumentsViewRequest(req);
			return resp;
		}
		if(info->command == "PostOrderInDocument")
		{
			resp = PostOrderInDocumentRequest(req);
			return resp;
		}
		if(info->command == "UnpostOrderInDocument")
		{
			resp = UnpostOrderInDocumentRequest(req);
			return resp;
		}

		//Doc items OrderIn

		if(info->command == "SaveDocItemOrderIn")
		{
			resp = SaveDocItemOrderInRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemOrderIn")
		{
			resp = RemoveDocItemOrderInRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemOrderIn")
		{
			resp = GetDocItemOrderInRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemsOrderInView")
		{
			resp = GetDocItemsOrderInViewRequest(req);
			return resp;
		}

		// ORDER OUT DOCS

		if(info->command == "AddOrderOutDocument")
		{
			resp = AddOrderOutDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveOrderOutDocument")
		{
			resp = SaveOrderOutDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveOrderOutDocument")
		{
			resp = RemoveOrderOutDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetOrderOutDocument")
		{
			resp = GetOrderOutDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetOrderOutDocumentView")
		{
			resp = GetOrderOutDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetOrderOutDocumentsView")
		{
			resp = GetOrderOutDocumentsViewRequest(req);
			return resp;
		}
		if(info->command == "PostOrderOutDocument")
		{
			resp = PostOrderOutDocumentRequest(req);
			return resp;
		}
		if(info->command == "UnpostOrderOutDocument")
		{
			resp = UnpostOrderOutDocumentRequest(req);
			return resp;
		}

		//Doc items OrderOut

		if(info->command == "SaveDocItemOrderOut")
		{
			resp = SaveDocItemOrderOutRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemOrderOut")
		{
			resp = RemoveDocItemOrderOutRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemOrderOut")
		{
			resp = GetDocItemOrderOutRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemsOrderOutView")
		{
			resp = GetDocItemsOrderOutViewRequest(req);
			return resp;
		}

		// Invoice Factura Docs
		if(info->command == "AddInvoiceFacturaDocument")
		{
			resp = AddInvoiceFacturaDocumentRequest(req);
			return resp;
		}
		if(info->command == "SaveInvoiceFacturaDocument")
		{
			resp = SaveInvoiceFacturaDocumentRequest(req);
			return resp;
		}

		if(info->command == "RemoveInvoiceFacturaDocument")
		{
			resp = RemoveInvoiceFacturaDocumentRequest(req);
			return resp;
		}

		if(info->command == "GetInvoiceFacturaDocument")
		{
			resp = GetInvoiceFacturaDocumentRequest(req);
			return resp;
		}
		if(info->command == "GetInvoiceFacturaDocumentView")
		{
			resp = GetInvoiceFacturaDocumentViewRequest(req);
			return resp;
		}
		if(info->command == "GetInvoiceFacturaDocumentsView")
		{
			resp = GetInvoiceFacturaDocumentsViewRequest(req);
			return resp;
		}

		//Doc items Invoice

		if(info->command == "SaveDocItemInvoiceFactura")
		{
			resp = SaveDocItemInvoiceFacturaRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemInvoiceFactura")
		{
			resp = RemoveDocItemInvoiceFacturaRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemInvoiceFactura")
		{
			resp = GetDocItemInvoiceFacturaRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemsInvoiceFacturaView")
		{
			resp = GetDocItemsInvoiceFacturaViewRequest(req);
			return resp;
		}

		//Doc items  Product Out Services

		if(info->command == "SaveDocItemProductOutService")
		{
			resp = SaveDocItemProductOutServiceRequest(req);
			return resp;
		}

		if(info->command == "RemoveDocItemProductOutService")
		{
			resp = RemoveDocItemProductOutServiceRequest(req);
			return resp;
		}

		if(info->command == "GetDocItemProductOutService")
		{
			resp = GetDocItemProductOutServiceRequest(req);
			return resp;
		}



		if(info->command == "GetPriceAnalytic")
		{
			resp = GetPriceAnalyticRequest(req);
			return resp;
		}

		if(info->command == "CalculateDocItemPrice")
		{
			resp = CalculateDocItemPriceRequest(req);
			return resp;
		}

		if(info->command == "CalculatePaymentSum")
		{
			resp = CalculatePaymentSumRequest(req);
			return resp;
		}

		//PRICE CATEGORIEA=S

		if(info->command == "AddPriceCategory")
		{
			resp = AddPriceCategoryRequest(req);
			return resp;
		}

		if(info->command == "SavePriceCategory")
		{
			resp = SavePriceCategoryRequest(req);
			return resp;
		}

		if(info->command == "RemovePriceCategory")
		{
			resp = RemovePriceCategoryRequest(req);
			return resp;
		}

		if(info->command == "GetPriceCategory")
		{
			resp = GetPriceCategoryRequest(req);
			return resp;
		}
		if(info->command == "GetPriceCategories")
		{
			resp = GetPriceCategoriesRequest(req);
			return resp;
		}

		if(info->command == "AddCustomPriceCategories")
		{
			resp = AddCustomPriceCategoriesRequest(req);
			return resp;
		}
		if(info->command == "GetPriceCategoriesView")
		{
			resp = GetPriceCategoriesViewRequest(req);
			return resp;
		}
		if(info->command == "CalculatePriceCategoriesView")
		{
			resp = CalculatePriceCategoriesViewRequest(req);
			return resp;
		}


		if(info->command == "GetProductSummariesView")
		{
			resp = GetProductSummariesViewRequest(req);
			return resp;
		}
		if(info->command == "GetProductSummariesSubsView")
		{
			resp = GetProductSummariesSubsViewRequest(req);
			return resp;
		}

		if(info->command == "GetObjectID")
		{
			resp = GetObjectIdRequest(req);
			return resp;
		}

		if(info->command == "DuplicateProduct")
		{
			resp = DuplicateProductRequest(req);
			return resp;
		}
		if(info->command == "DuplicatePartner")
		{
			resp = DuplicatePartnerRequest(req);
			return resp;
		}
		if(info->command == "DuplicateDocument")
		{
			resp = DuplicateDocumentRequest(req);
			return resp;
		}


		if(info->command == "GetReportShortIncome")
		{
			resp = GetReportShortIncomeRequest(req);
			return resp;
		}

		if(info->command == "GetReportDetailedIncome")
		{
			resp = GetReportDetailedIncomeRequest(req);
			return resp;
		}

		if(info->command == "GetReportShortSales")
		{
			resp = GetReportShortSalesRequest(req);
			return resp;
		}

		if(info->command == "GetReportBalancesAtWarehouse")
		{
			resp = GetReportBalancesAtWarehouseRequest(req);
			return resp;
		}

		if(info->command == "GetReportProductMove")
		{
			resp = GetReportProductMoveRequest(req);
			return resp;
		}

		if(info->command == "GetReportTurnover")
		{
			resp = GetReportTurnoverRequest(req);
			return resp;
		}

		if(info->command == "GetReportMinProductBalance")
		{
			resp = GetReportMinProductBalanceRequest(req);
			return resp;
		}

		if(info->command == "GetReportSalesWithProfit")
		{
			resp = GetReportSalesWithProfitRequest(req);
			return resp;
		}

		if(info->command == "AddReportTemplate")
		{
			resp = AddReportTemplateRequest(req);
			return resp;
		}

		if(info->command == "SaveReportTemplate")
		{
			resp = SaveReportTemplateRequest(req);
			return resp;
		}

		if(info->command == "RemoveReportTemplate")
		{
			resp = RemoveReportTemplateRequest(req);
			return resp;
		}

		if(info->command == "GetReportTemplate")
		{
			resp = GetReportTemplateRequest(req);
			return resp;
		}

		if(info->command == "GetReportTemplates")
		{
			resp = GetReportTemplatesRequest(req);
			return resp;
		}


		if(info->command == "GetReports")
		{
			resp = GetReportsRequest(req);
			return resp;
		}
		if(info->command == "GetReportClassProperties")
		{
			resp = GetReportClassPropertiesRequest(req);
			return resp;
		}
		if(info->command == "GetReportDataSources")
		{
			resp = GetReportDataSourcesRequest(req);
			return resp;
		}

		if(info->command == "GetReportData")
		{
			resp = GetReportDataRequest(req);
			return resp;
		}


		if(info->command == "AddHelpTopic")
		{
			resp = AddHelpTopicRequest(req);
			return resp;
		}
		if(info->command == "SaveHelpTopic")
		{
			resp = SaveHelpTopicRequest(req);
			return resp;
		}

		if(info->command == "RemoveHelpTopic")
		{
			resp = RemoveHelpTopicRequest(req);
			return resp;
		}

		if(info->command == "GetHelpTopic")
		{
			resp = GetHelpTopicRequest(req);
			return resp;
		}

		if(info->command == "GetHelpTopics")
		{
			resp = GetHelpTopicsRequest(req);
			return resp;
		}

		if(info->command == "ChangeHelpTopicPosition")
		{
			resp = ChangeHelpTopicPositionRequest(req);
			return resp;
		}

	

		if(info->command == "GetDocProductInPrint")
		{
			resp = GetDocProductInPrintRequest(req);
			return resp;
		}

		if(info->command == "GetDocProductOutPrint")
		{
			resp = GetDocProductOutPrintRequest(req);
			return resp;
		}

		// 	if(info->command == "GetPriceAnalytics")
		// 	{
		// 		resp = GetPriceAnalyticsRequest(req);
		// 		return resp;
		// 	}


		if(info->command == "GetCalculateLots")
		{
			resp = GetCalculateLotsRequest(req);
			return resp;
		}

		if(info->command == "RecalculateProductsPrice")
		{
			resp = RecalculateProductsPriceRequest(req);
			return resp;
		}


		if (info->command == "ImportProductsCSV")
		{
			resp = ImportProductsCSVRequest(req);
			return resp;
		}



		if (info->command == "AddRelation")
		{
			resp = AddRelationRequest(req);
			return resp;
		}

		if (info->command == "GetRelation")
		{
			resp = GetRelationRequest(req);
			return resp;
		}

		if (info->command == "GetManagersView")
		{
			resp = GetManagersViewRequest(req);
			return resp;
		}
	

		if(info->command == "AddCustomPermissions")
		{
			resp = AddCustomPermissionsRequest(req);
			return resp;
		}
		if(info->command == "GetPermissionsView")
		{
			resp = GetPermissionsViewRequest(req);
			return resp;
		}
		if(info->command == "GetPermissionView")
		{
			resp = GetPermissionViewRequest(req);
			return resp;
		}
		if(info->command == "GetPermission")
		{
			resp = GetPermissionRequest(req);
			return resp;
		}
		if(info->command == "CheckPermission")
		{
			resp = CheckPermissionRequest(req);
			return resp;
		}


		if(info->command == "GetUpdateUserStatus")
		{
			resp = UpdateUserStatusRequest(req);
			return resp;
		}

		if(info->command == "CheckServerStatus")
		{
			resp = CheckServerStatusRequest(req);
			return resp;
		}

		if(info->command == "StopServer")
		{
			resp = StopServerRequest(req);
			return resp;
		}

		if(info->command == "GetServerInfo")
		{
			resp = GetServerInfoRequest(req);
			return resp;
		}


		if(info->command == "ExportGridToExcel")
		{
			resp = ExportGridToExcelRequest(req);
			return resp;
		}

		if(info->command == "GetDateTime")
		{
			resp = GetDateTimeRequest(req);
			return resp;
		}


		if(info->command == "GetFile")
		{
			resp = GetFileRequest(req);
			return resp;
		}

		if(info->command == "GetReportPDF")
		{
			resp = GetReportPDFRequest(req);
			return resp;
		}

		if(info->command == "AddMessage")
		{
			resp = AddMessageRequest(req);
			return resp;
		}

		if(info->command == "SaveMessage")
		{
			resp = SaveMessageRequest(req);
			return resp;
		}

		if(info->command == "RemoveMessage")
		{
			resp = RemoveMessageRequest(req);
			return resp;
		}

		if(info->command == "GetMessage")
		{
			resp = GetMessageRequest(req);
			return resp;
		}

		if(info->command == "GetMessages")
		{
			resp = GetMessagesRequest(req);
			return resp;
		}

		if(info->command == "GetMessageView")
		{
			resp = GetMessageViewRequest(req);
			return resp;
		}

		if(info->command == "GetMessagesView")
		{
			resp = GetMessagesViewRequest(req);
			return resp;
		}

		if(info->command == "AddWebstore")
		{
			resp = AddWebstoreRequest(req);
			return resp;
		}

		if(info->command == "SaveWebstore")
		{
			resp = SaveWebstoreRequest(req);
			return resp;
		}

		if(info->command == "RemoveWebstore")
		{
			resp = RemoveWebstoreRequest(req);
			return resp;
		}

		if(info->command == "GetWebstore")
		{
			resp = GetWebstoreRequest(req);
			return resp;
		}

		if(info->command == "GetWebstores")
		{
			resp = GetWebstoresRequest(req);
			return resp;
		}


        if(info->command == "GetPHP")
		{
			resp = GetPHP(req);
			return resp;
		}

		if(info->command == "MakeDocumentInvoice")
		{
			resp = MakeDocumentInvoiceRequest(req);
			return resp;
		}


		if(info->command == "GetFinProfitSummaries")
		{
			resp = GetFinProfitSummariesRequest(req);
			return resp;
		}

		if(info->command == "GetFinProfitPaymentsSummaries")
		{
			resp = GetFinProfitPaymentsSummariesRequest(req);
			return resp;
		}

		if(info->command == "GetFinSummariesView")
		{
			resp = GetFinSummariesViewRequest(req);
			return resp;
		}

		if(info->command == "GetAssets")
		{
			resp = GetAssetsRequest(req);
			return resp;
		}

		if(info->command == "GetCronTasksView")
		{
			resp = GetCronTasksViewRequest(req);
			return resp;
		}

		if(resp == NULL)
		{
			resp = new TResponseInfo();
			resp->succesfull = 0;
			resp->message = "Wrong command";
			resp->message_code = "P_WRONG_COMMAND";
		}
	}

	catch (...)
	{
		cout << "... ";
	}

*/
	return resp;
}


/************************************************************************/
/* LANGUAGES                                                            */
/************************************************************************/

#ifdef blah

TResponseInfo * PandoraTest::SaveLanguageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TLanguage * language = NULL;
	bool result = false;

	try
	{

		language = new TLanguage();
		language->FromJson(req->info->data);
		result = db->SaveLanguage(language);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveLanguage succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveLanguage  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveLanguage  error" , "");

	}

	SAFEDELETE(language)
		return resp;
}
TResponseInfo * PandoraTest::AddLanguageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TLanguage * language = NULL;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3550, "Add");

		language = new TLanguage();
		language->FromJson(req->info->data);
		language->id = db->GetObjectID();

		result = db->AddLanguage(language);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddLanguage succesfull.";
			resp->data = TUtility::ToJson(language->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddLanguage  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddLanguage  error" , "");

	}

	SAFEDELETE(language)
		return resp;
}
TResponseInfo * PandoraTest::RemoveLanguageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3550, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveLanguage(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveLanguage succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveLanguage  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveLanguage  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetLanguageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TLanguage * language = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3550, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		language = db->GetLanguage(id);

		if(language != NULL)
		{
			resp->data = language->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetLanguage succesfull.";
		}

		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetLanguage  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetLanguage  error" , "");

	}

	SAFEDELETE(language)
		return resp;
}
#endif

TResponseInfo * PandoraTest::GetLanguagesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TLanguage> * languages = NULL;
	bool inactive = false;
	try
	{
		//Sleep(60000);
		//		db->CheckPermission(req->user_id, 3550, "View");
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		inactive = TUtility::wstoi(params[L"inactive"]);

//		languages = db->GetLanguages(inactive);
		languages = new TCollection<TLanguage>("TLanguage");
		if(languages != NULL)
		{
			resp->data = languages->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetLanguages operation succesfull!";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetLanguages  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetLanguages  error" , "");

	}

	SAFEDELETE(languages)
		return resp;
}

#ifdef sm
TResponseInfo * PandoraTest::ActivateLanguagesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	vector<int> ids;
	try
	{
		logger->Add("ParseVectorIds");
		ids = ParseVectorIds(req->info->parameters);
		logger->Add(TUtility::itoa(ids.size()));
		logger->Add("ActivateLanguages");
		result = db->ActivateLanguages(ids);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ActivateLanguages succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "ActivateLanguages  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "ActivateLanguages  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::SaveDefaultLanguageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int language_id = 0;
	try
	{
		logger->Add("Lang WASSAP!");
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		language_id = TUtility::wstoi(params[L"language_id"]);
		logger->Add("lang: ------------");
		logger->Add(TUtility::itoa(language_id));
		TUtility::CheckZeroValue(1, true, language_id);

		result = db->SaveDefaultLanguage(language_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDefaultLanguage succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveDefaultLanguage  error" , "");

	}


	return resp;
}

/************************************************************************/
/* STATUSES                                                           */
/************************************************************************/

TResponseInfo * PandoraTest::SaveStatusRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TStatus * status = NULL;
	bool result = false;

	try
	{

		status = new TStatus();
		status->FromJson(req->info->data);

		result = db->SaveStatus(status);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveStatus succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveStatus  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveStatus  error" , "");

	}

	SAFEDELETE(status)
		return resp;
}

TResponseInfo * PandoraTest::AddStatusRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TStatus * status = NULL;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3554, "Add");

		status = new TStatus();
		status->FromJson(req->info->data);

		if(status->id == 0)
		{
			status->id = db->GetObjectID();
		}

		result = db->AddStatus(status);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddStatus succesfull.";
			resp->data = TUtility::ToJson(status->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddStatus  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddStatus  error" , "");

	}

	SAFEDELETE(status)
		return resp;
}


TResponseInfo * PandoraTest::RemoveStatusRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3554, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveStatus(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveStatus succesfull.";
		}

		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveStatus  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveStatus  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetStatusRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TStatus * status = NULL;
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3554, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		status = db->GetStatus(id, req->info->language);

		if(status != NULL)
		{
			resp->data = status->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetStatus succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetStatus  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetStatus  error" , "");

	}

	SAFEDELETE(status)
		return resp;
}
TResponseInfo * PandoraTest::GetStatusesRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TStatus> * statuses = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3554, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		statuses = db->GetStatuses(params);

		if(statuses != NULL)
		{
			resp->data = statuses->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetStatuses operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetStatuses  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetStatuses  error" , "");

	}

	SAFEDELETE(statuses)
		return resp;
}


/************************************************************************/
/* MEASUREMENTS                                                         */
/************************************************************************/

TResponseInfo * PandoraTest::SaveMeasurementRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMeasurement * measure = NULL;
	bool result = false;

	try
	{
		measure = new TMeasurement();
		measure->FromJson(req->info->data);

		result = db->SaveMeasurement(measure);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveMeasurement succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveMeasurement  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveMeasurement  error" , "");

	}

	SAFEDELETE(measure)
		return resp;
}

TResponseInfo * PandoraTest::AddMeasurementRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMeasurement * measure = NULL;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3551, "Add");

		measure = new TMeasurement();
		measure->FromJson(req->info->data);
		measure->id = db->GetObjectID();

		result = db->AddMeasurement(measure);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddMeasurement succesfull.";
			resp->data = TUtility::ToJson(measure->id, "id");
		}

		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddMeasurement  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddMeasurement  error" , "");

	}

	SAFEDELETE(measure)
		return resp;
}


TResponseInfo * PandoraTest::RemoveMeasurementRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3551, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveMeasurement(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveMeasurement succesfull.";
			return resp;
		}


		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveMeasurement  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveMeasurement  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetMeasurementRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMeasurement * measure = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3551, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		measure = db->GetMeasurement(id, req->info->language);

		if(measure != NULL)
		{
			resp->data = measure->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMeasurement succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetMeasurement  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetMeasurement  error" , "");

	}

	SAFEDELETE(measure)
		return resp;
}
TResponseInfo * PandoraTest::GetMeasurementsRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TMeasurement> * measurements = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3551, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		measurements = db->GetMeasurements(params);

		if(measurements != NULL)
		{
			resp->data = measurements->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMeasurements operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetMeasurements  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetMeasurements  error" , "");

	}

	SAFEDELETE(measurements)
		return resp;
}
TResponseInfo * PandoraTest::SaveDefaultCurrencyRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int currency_id = 0;
	try
	{
		logger->Add("WATAFAC");
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		currency_id = TUtility::wstoi(params[L"currency_id"]);
		logger->Add("CUrrency:");
		logger->Add(currency_id);
		TUtility::CheckZeroValue(1, true, currency_id);

		result = db->SaveDefaultMeasurementId(currency_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDefaultMeasurementId succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveDefaultCurrency  error" , "");

	}


	return resp;
}

/************************************************************************/
/* RATES		                                                        */
/************************************************************************/

TResponseInfo * PandoraTest::SaveRateRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMeasureRate * rate = NULL;
	bool result = false;

	try
	{
		rate = new TMeasureRate();
		rate->FromJson(req->info->data);
		if (rate->id == 0)
		{
			TError error;
			error.message = L"No ID";
		}

		result = db->SaveRate(rate);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveRate succesfull.";
			resp->data = TUtility::ToJson(rate->id, "id");
			SAFEDELETE(rate)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveRate  error" , "");

	}

	SAFEDELETE(rate)
		return resp;
}

TResponseInfo * PandoraTest::AddRateRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMeasureRate * rate = NULL;
	bool result = false;

	try
	{
		rate = new TMeasureRate();
		rate->FromJson(req->info->data);
		rate->id = db->GetObjectID();
		if (rate->from_id  <= 0 || rate->to_id <= 0)
		{
			TError error;
			error.message = L"rate's from_id, to_id cannot be 0 or less";
			throw error;
		}

		result = db->AddRate(rate);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddMeasureRate succesfull.";
			resp->data = TUtility::ToJson(rate->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddRate  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddRate  error" , "");

	}


	SAFEDELETE(rate)
		return resp;
}

TResponseInfo * PandoraTest::RemoveRateRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveRate(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveMeasureRate succesfull.";
			return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveRate  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetRateByIdRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMeasureRate * rate = NULL;
	int id = 0, object_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		object_id = TUtility::wstoi(params[L"object_id"]);
		TUtility::CheckZeroValue(1, true, id);

		rate = db->GetRateById(id, object_id);

		if(rate != NULL)
		{
			resp->data = rate->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMeasureRate succesfull.";
			delete rate;
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetMeasureRate  error" , "");

	}

	SAFEDELETE(rate)
		return resp;
}
TResponseInfo * PandoraTest::GetRateRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMeasureRate * rate = NULL;
	int from_id = 0, to_id = 0, date = 0, owner_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		owner_id = TUtility::wstoi(params[L"owner_id"]);
		from_id =  TUtility::wstoi(params[L"from_id"]);
		to_id = TUtility::wstoi(params[L"to_id"]);
		date = TUtility::wstoi(params[L"date"]);

		rate = db->GetRate(owner_id, from_id, to_id, date);

		if(rate != NULL)
		{
			resp->data = rate->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMeasureRate succesfull.";
			delete rate;
			return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetMeasureRate  error" , "");

	}

	SAFEDELETE(rate)
		return resp;
}
TResponseInfo * PandoraTest::GetRateViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMeasureRateView * rate = NULL;
	int from_id = 0, to_id = 0, date = 0, owner_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		owner_id = TUtility::wstoi(params[L"owner_id"]);
		from_id =  TUtility::wstoi(params[L"from_id"]);
		to_id = TUtility::wstoi(params[L"to_id"]);
		date = TUtility::wstoi(params[L"date"]);

		rate = db->GetRateView(req->info->language, owner_id, from_id, to_id, date);

		if(rate != NULL)
		{
			resp->data = rate->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetRateView succesfull.";
			delete rate;
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPartnersSearchView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}


	SAFEDELETE(rate)
		return resp;
}
TResponseInfo * PandoraTest::GetRatesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TMeasureRate> * rates = NULL;
	int owner_id = 0, from_date = 0, to_date = 0, num = 0, from = 0, lang_id = 0, from_id = 0, to_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		owner_id = TUtility::wstoi(params[L"owner_id"]);
		from_date = TUtility::wstoi(params[L"from_date"]);
		to_date = TUtility::wstoi(params[L"to_date"]);
		num = TUtility::wstoi(params[L"num"]);
		from = TUtility::wstoi(params[L"from"]);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		from_id = TUtility::wstoi(params[L"from_id"]);
		to_id = TUtility::wstoi(params[L"to_id"]);

		rates = db->GetRates(owner_id, from_date, to_date, num, from, lang_id, from_id, to_id);

		if(rates != NULL)
		{
			resp->data = rates->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMeasureRates operation succesfull!";
			delete rates;
			return resp;
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPartnersSearchView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(rates)
		return resp;
}

TResponseInfo * PandoraTest::GetRatesViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TMeasureRateView> * rates = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		rates = db->GetRatesView(params);

		if(rates != NULL)
		{
			resp->data = rates->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMeasureRatesView operation succesfull!";
			delete rates;
			return resp;
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPartnersSearchView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(rates)
		return resp;
}

/************************************************************************/
/* CATEGORIES                                                           */
/************************************************************************/
TResponseInfo * PandoraTest::GetCategoriesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCategory * cats = NULL;
	int parent_id = 0, type_id = 0;

	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		type_id = TUtility::wstoi(params[L"type_id"]);
		parent_id = TUtility::wstoi(params[L"parent_id"]);

		cats = db->GetCategories(parent_id, type_id, req->info->language);

		if(cats != NULL)
		{
			resp->data = cats->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetCategories operation succesfull!";
			delete cats;
			return resp;
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetCategories  error" , "");

	}

	SAFEDELETE(cats)
		return resp;
}
TResponseInfo * PandoraTest::SaveCategoryRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCategory * cat = NULL;
	bool result = false;

	try
	{

		cat = new TCategory();
		cat->FromJson(req->info->data);

		result = db->SaveCategory(cat);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveCategory succesfull.";
			SAFEDELETE(cat)
				return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveCategory  error" , "");

	}

	SAFEDELETE(cat)
		return resp;
}

TResponseInfo * PandoraTest::AddCategoryRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCategory * cat = NULL;
	bool result = false;
	try
	{
		cat = new TCategory();
		cat->FromJson(req->info->data);
		if (cat->id == 0)
			cat->id = db->GetObjectID();

		result = db->AddCategory(cat);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddCategory succesfull.";
			resp->data = TUtility::ToJson(cat->id, "id");
			SAFEDELETE(cat)
				return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddMCategory  error" , "");

	}

	SAFEDELETE(cat)
		return resp;
}


TResponseInfo * PandoraTest::RemoveCategoryRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0, lang_id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveCategory(id, lang_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveCategory succesfull.";
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveCategory  error" , "");

	}

	return resp;
}


TResponseInfo * PandoraTest::GetCategoryRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCategory * cat = NULL;
	int id = 0, type_id = 0;
	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		id = TUtility::wstoi(params[L"id"]);
		type_id = TUtility::wstoi(params[L"type_id"]);
		TUtility::CheckZeroValue(1, true, id);

		cat = db->GetCategory(id, type_id, req->info->language);

		if(cat != NULL)
		{
			resp->data = cat->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetCategories operation succesfull!";
			delete cat;
			return resp;
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetCategories  error" , "");

	}

	SAFEDELETE(cat)
		return resp;
}

/************************************************************************/
/* ADDRESSES                                                            */
/************************************************************************/
TResponseInfo * PandoraTest::SaveAddressRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TAddress * address = NULL;
	bool result = false;

	try
	{
		address = new TAddress();
		address->FromJson(req->info->data);

		result = db->SaveAddress(address);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveAddress succesfull.";
			SAFEDELETE(address)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveAddress  error" , "");

	}

	SAFEDELETE(address)
		return resp;
}

TResponseInfo * PandoraTest::AddAddressRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TAddress * address = NULL;
	bool result = false;
	try
	{
		address = new TAddress();
		address->FromJson(req->info->data);

		address->id = db->GetObjectID();
		result = db->AddAddress(address);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddAddress succesfull.";
			resp->data = TUtility::ToJson(address->id, "id");
			SAFEDELETE(address)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddAddress  error" , "");

	}

	SAFEDELETE(address)
		return resp;
}


TResponseInfo * PandoraTest::RemoveAddressRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveAddress(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveCategory succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveAddress  error" , "");

	}

	return resp;
}


TResponseInfo * PandoraTest::GetAddressRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TAddress * address = NULL;
	int id = 0;
	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		address = db->GetAddress(id);

		if(address != NULL)
		{
			resp->data = address->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetAddress  error" , "");

	}

	SAFEDELETE(address)
		return resp;
}


/************************************************************************/
/* PERSONS                                                              */
/************************************************************************/
TResponseInfo * PandoraTest::AddPersonRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPerson * person = NULL;
	bool result = false;
	int cat_id = 0;
	try
	{
		person = new TPerson();
		person->FromJson(req->info->data);

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);
		if (person->id == 0)
		{
			person->id = db->GetObjectID();
		}

		result = db->AddPerson(person, cat_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddPerson succesfull.";
			resp->data = TUtility::ToJson(person->id, "id");
			SAFEDELETE(person)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddPerson  error" , "");

	}

	SAFEDELETE(person)
		return resp;
}
TResponseInfo * PandoraTest::SavePersonRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPerson * person = NULL;
	bool result = false;

	try
	{
		person = new TPerson();
		person->FromJson(req->info->data);

		result = db->SavePerson(person);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SavePerson succesfull.";
			SAFEDELETE(person)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SavePerson  error" , "");

	}

	SAFEDELETE(person)
		return resp;
}
TResponseInfo * PandoraTest::RemovePersonRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemovePerson(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemovePerson succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemovePerson  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetPersonRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPerson * person = NULL;
	int id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		person = db->GetPerson(id);

		if(person != NULL)
		{
			resp->data = person->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPerson succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetPerson  error" , "");

	}

	SAFEDELETE(person)
		return resp;
}

/************************************************************************/
/* COMPANIES                                                            */
/************************************************************************/
TResponseInfo * PandoraTest::AddCompanyRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCompany * company = NULL;
	bool result = false;
	int cat_id = 0;
	try
	{
		company = new TCompany();
		company->FromJson(req->info->data);

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);
		if (company->id == 0)
			company->id = db->GetObjectID();

		result = db->AddCompany(company, cat_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddCompany succesfull.";
			resp->data = TUtility::ToJson(company->id, "id");
			SAFEDELETE(company)
				return resp;
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "AddCompany  error" , "");

	}

	SAFEDELETE(company)
		return resp;
}
TResponseInfo * PandoraTest::SaveCompanyRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCompany * company = NULL;
	bool result = false;

	try
	{
		company = new TCompany();
		company->FromJson(req->info->data);

		result = db->SaveCompany(company);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveCompany succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveCompany  error" , "");

	}

	SAFEDELETE(company)
		return resp;
}
TResponseInfo * PandoraTest::RemoveCompanyRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->MayDelete(id);
		if (!result)
		{
			resp->succesfull = 0;
			resp->message = "Error. There are some reference to this object! ";
			return resp;
		}

		result = db->RemoveCompany(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveCompany succesfull.";
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveCompany  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveCompany  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetCompanyRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCompany * company = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		company = db->GetCompany(id);

		if(company != NULL)
		{
			resp->data = company->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetCompany succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetCompany  error" , "");

	}

	SAFEDELETE(company)
		return resp;
}

/************************************************************************/
/* EMPLOYEES                                                            */
/************************************************************************/
TResponseInfo * PandoraTest::AddEmployeeRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TEmployee * employee = NULL;
	bool result = false;
	int cat_id = 0;
	try
	{
		employee = new TEmployee();

		employee->FromJson(req->info->data);
		employee->id = db->GetObjectID();

		result = db->AddEmployee(employee);


		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddEmployee succesfull.";
			resp->data = TUtility::ToJson(employee->id, "id");
			SAFEDELETE(employee)
				return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddEmployee  error" , "");

	}

	SAFEDELETE(employee)
		return resp;
}


TResponseInfo * PandoraTest::SaveEmployeeRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TEmployee * employee = NULL;
	bool result = false;

	try
	{
		employee = new TEmployee();
		employee->FromJson(req->info->data);

		result = db->SaveEmployee(employee);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveEmployee succesfull.";
			SAFEDELETE(employee)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveEmployee  error" , "");

	}

	SAFEDELETE(employee)
		return resp;
}

TResponseInfo * PandoraTest::RemoveEmployeeRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveEmployee(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveEmployee succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveEmployee  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetEmployeeRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TEmployee * employee = NULL;
	int id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		employee = db->GetEmployee(id, req->info->language);

		if(employee != NULL)
		{
			resp->data = employee->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetEmployee succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetEmployee  error" , "");

	}

	SAFEDELETE(employee)
		return resp;
}

TResponseInfo * PandoraTest::GetEmployeesRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TEmployee> * employees = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		employees = db->GetEmployees(params);


		if(employees != NULL)
		{
			resp->data = employees->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetEmployees operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetEmployees  error" , "");

	}

	SAFEDELETE(employees)
		return resp;
}
TResponseInfo * PandoraTest::GetEmployeesViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TEmployeeView> * employees = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"user_id"] = TUtility::itows(req->user_id);

		employees = db->GetEmployeesView(params);

		if(employees != NULL)
		{
			resp->data = employees->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetEmployeesView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetEmployeesView  error" , "");

	}

	SAFEDELETE(employees)
		return resp;
}
/************************************************************************/
/* MANAGERS                                                             */
/************************************************************************/
TResponseInfo * PandoraTest::GetManagersViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TManagerView> * managers = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"user_id"] = TUtility::itows(req->user_id);

		managers = db->GetManagersView(params);

		if(managers != NULL)
		{
			resp->data = managers->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetManagersView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPartnersSearchView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(managers)
		return resp;
}


/************************************************************************/
/* CONTACT PERSONS                                                            */
/************************************************************************/
TResponseInfo * PandoraTest::AddContactPersonRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TContactPerson * cp = NULL;
	bool result = false;

	try
	{

		cp = new TContactPerson();
		cp->FromJson(req->info->data);

		result = db->AddContactPerson(cp);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddContactPerson succesfull.";
			resp->data = TUtility::ToJson(cp->id, "id");
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddContactPerson  error" , "");

	}

	SAFEDELETE(cp)
		return resp;
}


TResponseInfo * PandoraTest::SaveContactPersonRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TContactPerson * cp = NULL;
	bool result = false;

	try
	{
		cp = new TContactPerson();
		cp->FromJson(req->info->data);

		result = db->SaveContactPerson(cp);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveContactPerson succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveContactPerson  error" , "");

	}

	SAFEDELETE(cp)
		return resp;
}

TResponseInfo * PandoraTest::RemoveContactPersonRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0, partner_id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		partner_id = TUtility::wstoi(params[L"owner_id"]);
		TUtility::CheckZeroValue(1, true, id, partner_id);

		result = db->RemoveContactPerson(id, partner_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveContactPerson succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveContactPerson  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetContactPersonRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TContactPerson * cp = NULL;
	int id = 0, partner_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		partner_id =  TUtility::wstoi(params[L"owner_id"]);
		TUtility::CheckZeroValue(1, true, id, partner_id);

		cp = db->GetContactPerson(id, partner_id, req->info->language);

		if(cp != NULL)
		{
			resp->data = cp->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetContactPerson succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetContactPerson  error" , "");

	}

	SAFEDELETE(cp)
		return resp;
}

TResponseInfo * PandoraTest::GetContactPersonsViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TContactPersonView> * cp = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		cp = db->GetContactPersonsView(params);

		if(cp != NULL)
		{
			resp->data = cp->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetContactPersonsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetContactPersonsView  error" , "");

	}

	SAFEDELETE(cp)
		return resp;
}



/************************************************************************/
/* PARTNERS                                                             */
/************************************************************************/
TResponseInfo * PandoraTest::SavePartnerRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPartner * partner = NULL;
	bool result = false;

	try
	{
		partner = new TPartner();
		partner->FromJson(req->info->data);
		TUtility::CheckZeroValue(1, true, partner->id);

		result = db->SavePartner(partner);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SavePartner succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SavePartner  error" , "");

	}

	SAFEDELETE(partner)
		return resp;
}

TResponseInfo * PandoraTest::AddPartnerRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPartner * partner = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		partner = new TPartner();
		partner->FromJson(req->info->data);

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		if (partner->id == 0)
			partner->id = db->GetObjectID();

		result = db->AddPartner(partner, cat_id);


		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddPartner succesfull.";
			resp->data = TUtility::ToJson(partner->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddPartner  error" , "");

	}

	SAFEDELETE(partner)
		return resp;
}

TResponseInfo * PandoraTest::RemovePartnerRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->MayDelete(id);

		if (!result)
		{
			resp->succesfull = 0;
			resp->message = "Error. There are some reference to this object! ";
			return resp;
		}

		result = db->RemovePartner(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemovePartner succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemovePartner  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetPartnerRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPartner * partner = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		partner = db->GetPartner(id);

		if(partner != NULL)
		{
			resp->data = partner->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMeasurement succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetPartner  error" , "");

	}

	SAFEDELETE(partner)
		return resp;
}
TResponseInfo * PandoraTest::GetPartnersRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPartner> * partners = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		partners = db->GetPartners(params);

		if(partners != NULL)
		{
			resp->data = partners->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPartners operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetPartners  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetPartnerViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPartnerView * view = NULL;
	int id = 0;
	bool with_discounts = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		with_discounts = TUtility::wstoi(params[L"with_discounts"]);
		TUtility::CheckZeroValue(1, true, id);

		view = db->GetPartnerView(id, req->info->language, with_discounts);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPartnerView succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (...)
	{
		ProcessError(resp, "", "GetPartnerView  error" , "");

	}

	SAFEDELETE(view)
		return resp;
}
TResponseInfo * PandoraTest::GetPartnersViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPartnerView> * views = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		views = db->GetPartnersView(params);

		if(views != NULL)
		{
			resp->data = views->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPartnersView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetPartnersView  error" , "");

	}

	SAFEDELETE(views)
		return resp;
}

TResponseInfo * PandoraTest::GetPartnersSearchViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPartnerView> * products = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		products = db->GetPartnersSearchView(params);

		if(products != NULL)
		{
			resp->data = products->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPartnersSearchView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPartnersSearchView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(products)
		return resp;
}

TResponseInfo * PandoraTest::SavePartnerNameRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int partner_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		partner_id = TUtility::wstoi(params[L"partner_id"]);
		TUtility::CheckZeroValue(1, true, partner_id);

		result = db->SavePartnerName(partner_id, params[L"partner_name"]);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SavePartnerName succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "ExecCommand()  error" , "");

	}

	return resp;
}


/************************************************************************/
/* TPARTNER BALANCES                                                    */
/************************************************************************/
TResponseInfo * PandoraTest::GetPartnerBalanceViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPartnerBalanceView * view = NULL;
	int partner_id = 0, lang_id = 0, date = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		partner_id = TUtility::wstoi(params[L"partner_id"]);
		lang_id = req->info->language;
		date = TUtility::wstoi(params[L"date"]);
		TUtility::CheckZeroValue(1, true, partner_id);

		view = db->GetPartnerBalanceView(partner_id, date, lang_id);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPartnerBalanceView succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPartnerBalanceView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(view)
		return resp;
}

TResponseInfo * PandoraTest::GetPartnerBalancesViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPartnerBalanceView> * views = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		views = db->GetPartnerBalancesView(params);

		if(views != NULL)
		{
			resp->data = views->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPartnerBalancesViewRequest operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveProduct  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(views)
		return resp;
}



/************************************************************************/
/* PRODUCT                                                     */
/************************************************************************/
TResponseInfo * PandoraTest::AddProductRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProduct * product = NULL;
	bool result = false;
	int cat_id = 0;
	try
	{
		product = new TProduct();
		product->FromJson(req->info->data);
		if (product->id == 0)
			product->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddProduct(product, cat_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddProduct succesfull.";
			resp->data = TUtility::ToJson(product->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddProduct  error" , "");

	}

	SAFEDELETE(product)
		return resp;
}


TResponseInfo * PandoraTest::GetProductRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProduct * product = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		product = db->GetProduct(id);

		if(product != NULL)
		{
			resp->data = product->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProduct succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetProduct  error" , "");

	}

	SAFEDELETE(product)
		return resp;
}


TResponseInfo * PandoraTest::RemoveProductRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->MayDelete(id);
		if (!result)
		{
			resp->succesfull = 0;
			resp->message = "Error. There are some reference to this object! ";
			return resp;
		}

		result = db->RemoveProduct(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveProduct succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveProduct  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

/************************************************************************/
/* TANGIBLE PRODUCT                                                     */
/************************************************************************/
TResponseInfo * PandoraTest::AddTangibleProductRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTangibleProduct * product = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		product = new TTangibleProduct();
		product->FromJson(req->info->data);
		if (product->id == 0)
			product->id = db->GetObjectID();


		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddTangibleProduct(product, cat_id);


		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddTangibleProduct succesfull.";
			resp->data = TUtility::ToJson(product->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddTangibleProduct  error" , "");

	}

	SAFEDELETE(product)
		return resp;
}


TResponseInfo * PandoraTest::SaveTangibleProductRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTangibleProduct * product = NULL;
	bool result = false;

	try
	{
		product = new TTangibleProduct();
		product->FromJson(req->info->data);

		result = db->SaveTangibleProduct(product);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveTangibleProduct succesfull.";
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveTangibleProduct  error" , "");

	}

	SAFEDELETE(product)
		return resp;
}

TResponseInfo * PandoraTest::RemoveTangibleProductRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->MayDelete(id);
		if (!result)
		{
			resp->succesfull = 0;
			resp->message = "Error. There are some reference to this object! ";
			return resp;
		}

		result = db->RemoveTangibleProduct(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveTangibleProduct succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveTangibleProduct  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetTangibleProductRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTangibleProduct * product = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		product = db->GetTangibleProduct(id);

		if(product != NULL)
		{
			resp->data = product->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetTangibleProduct succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetTangibleProduct  error" , "");

	}

	SAFEDELETE(product)
		return resp;
}


TResponseInfo * PandoraTest::GetTangibleProductViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTangibleProductView * view = NULL;
	int id = 0, lang_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		view = db->GetTangibleProductView(id, lang_id);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetTangibleProductView succesfull.";

		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetTangibleProductView  error" , "");

	}


	SAFEDELETE(view)
		return resp;
}
TResponseInfo * PandoraTest::GetProductViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductView * view = NULL;
	int id = 0, warehouse_id = 0, lang_id = 0, date = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		lang_id = req->info->language;
		date = TUtility::wstoi(params[L"date"]);
		TUtility::CheckZeroValue(1, true, id);

		view = db->GetProductView(id,  lang_id, warehouse_id, date);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductView succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductView  error" , "");

	}

	SAFEDELETE(view)
		return resp;
}

/************************************************************************/
/* PRODUCTS                                                             */
/************************************************************************/
TResponseInfo * PandoraTest::GetTangibleProductsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TTangibleProduct> * products = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		products = db->GetTangibleProducts(params);

		if(products != NULL)
		{
			resp->data = products->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetTangibleProducts operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetTangibleProducts  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetProductsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductView> * products = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		products = db->GetProductsView(params);

		if(products != NULL)
		{
			resp->data = products->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetTangibleProductsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(products)
		return resp;
}

TResponseInfo * PandoraTest::GetProductsSearchViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductView> * products = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		products = db->GetProductsSearchView(params);

		if(products != NULL)
		{
			resp->data = products->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductsSearchView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(products)
		return resp;
}

/************************************************************************/
/*   MINIMAL STOCK LEVEL                                                */
/************************************************************************/
TResponseInfo * PandoraTest::SaveProductMinStockLevelsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TProductMinStockLevel> * stocks = NULL;
	bool result = false;
	int prod_id = 0;

	try
	{
		stocks = new TCollection<TProductMinStockLevel>("TProductMinStockLevel");
		stocks->FromJson(req->info->data);
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		prod_id = TUtility::wstoi(params[L"prod_id"]);

		result = db->SaveProductMinStockLevels(stocks, prod_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveProductMinStockLevel succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveProductMinStockLevel  error" , "");

	}

	SAFEDELETE(stocks)
		return resp;
}
TResponseInfo * PandoraTest::GetProductMinStockLevelsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductMinStockLevelView> * stocks = NULL;
	bool result = false;
	int prod_id = 0;
	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		prod_id = TUtility::wstoi(params[L"prod_id"]);

		stocks = db->GetProductMinStockLevelsView(prod_id, req->info->language);

		if(stocks != NULL)
		{
			resp->data = stocks->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductMinStockLevelsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductMinStockLevelsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductMinStockLevelsView  error" , "");

	}

	SAFEDELETE(stocks)
		return resp;
}

/************************************************************************/
/* PRODUCT BUNDLE                                                       */
/************************************************************************/
TResponseInfo * PandoraTest::AddBundleRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTangibleProduct * product = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		product = new TTangibleProduct();
		product->FromJson(req->info->data);
		if (product->id == 0)
			product->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		result = db->AddTangibleProduct(product, cat_id);


		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddProductBundle succesfull.";
			resp->data = TUtility::ToJson(product->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddProductBundle  error" , "");

	}

	SAFEDELETE(product)
		return resp;
}


TResponseInfo * PandoraTest::SaveBundleRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTangibleProduct * product = NULL;
	bool result = false;

	try
	{
		product = new TTangibleProduct();
		product->FromJson(req->info->data);

		result = db->SaveTangibleProduct(product);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveProductBundle succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveProductBundle  error" , "");

	}

	SAFEDELETE(product)
		return resp;
}

TResponseInfo * PandoraTest::RemoveBundleRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveProductBundle(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveProduct succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveBundle  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetBundleRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTangibleProduct * product = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		product = db->GetTangibleProduct(id);

		if(product != NULL)
		{
			resp->data = product->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProduct succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductBundle  error" , "");

	}

	SAFEDELETE(product)
		return resp;
}


TResponseInfo * PandoraTest::GetBundleViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTangibleProductView * view = NULL;
	int id = 0, lang_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		view = db->GetTangibleProductView(id, lang_id);

		if(view !=NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetBundleView succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetBundleView  error" , "");

	}

	SAFEDELETE(view)
		return resp;
}

/************************************************************************/
/*         SERVICE                                                      */
/************************************************************************/
TResponseInfo * PandoraTest::AddServiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProduct * service = NULL;
	bool result = false;
	int cat_id = 0;
	try
	{
		service = new TProduct();
		service->FromJson(req->info->data);
		if (service->id == 0)
			service->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		result = db->AddProduct(service, cat_id);


		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddService succesfull.";
			resp->data = TUtility::ToJson(service->id, "id");
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddService  error" , "");

	}

	SAFEDELETE(service)
		return resp;
}


TResponseInfo * PandoraTest::SaveServiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProduct * service = NULL;
	bool result = false;

	try
	{
		service = new TProduct();
		service->FromJson(req->info->data);
		result = db->SaveProduct(service);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveService succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveService  error" , "");

	}

	SAFEDELETE(service)
		return resp;
}

TResponseInfo * PandoraTest::RemoveServiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveProduct(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveService succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveService  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetServiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProduct * service = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		service = db->GetProduct(id);

		if(service != NULL)
		{
			resp->data = service->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetService succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetService  error" , "");

	}

	SAFEDELETE(service)
		return resp;
}


TResponseInfo * PandoraTest::GetServiceViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductView * view = NULL;
	int id = 0, lang_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id =  req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		view = db->GetProductView(id, lang_id, 0, 0);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetServiceView succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetServiceView  error" , "");

	}

	SAFEDELETE(view)
		return resp;
}

/************************************************************************/
/*         SOFTWARE                                                     */
/************************************************************************/
TResponseInfo * PandoraTest::AddSoftwareRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProduct * service = NULL;
	bool result = false;
	int cat_id = 0;
	try
	{
		service = new TProduct();
		service->FromJson(req->info->data);
		if (service->id == 0)
			service->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		result = db->AddProduct(service, cat_id);


		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddSoftware succesfull.";
			resp->data = TUtility::ToJson(service->id, "id");
			SAFEDELETE(service)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddSoftware  error" , "");

	}

	SAFEDELETE(service)
		return resp;
}


TResponseInfo * PandoraTest::SaveSoftwareRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProduct * service = NULL;
	bool result = false;

	try
	{
		service = new TProduct();
		service->FromJson(req->info->data);
		result = db->SaveProduct(service);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveSoftware succesfull.";
			SAFEDELETE(service)
				return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveSoftware  error" , "");

	}

	SAFEDELETE(service)
		return resp;
}

TResponseInfo * PandoraTest::RemoveSoftwareRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveProduct(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveSoftware succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveSoftware  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetSoftwareRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProduct * service = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		service = db->GetProduct(id);

		if(service != NULL)
		{
			resp->data = service->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetSoftware succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetSoftware  error" , "");

	}

	SAFEDELETE(service)
		return resp;
}



TResponseInfo * PandoraTest::GetSoftwareViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductView * view = NULL;
	int id = 0, lang_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id =  req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		view = db->GetProductView(id, lang_id, 0, 0);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetSoftwareView succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetSoftwareView  error" , "");

	}

	SAFEDELETE(view)
		return resp;
}

/************************************************************************/
/* PRODUCTS  SUBS                                                       */
/************************************************************************/

TResponseInfo * PandoraTest::AddProductSubRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductSub * sub = NULL;
	bool result = false;

	try
	{
		sub = new TProductSub();
		sub->FromJson(req->info->data);
		sub->id = db->GetObjectID();

		result = db->AddProductSub(sub);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddProductSub succesfull.";
			resp->data = TUtility::ToJson(sub->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddProductSub  error" , "");

	}

	SAFEDELETE(sub)
		return resp;
}


TResponseInfo * PandoraTest::SaveProductSubRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductSub * sub = 0;
	bool result = false;

	try
	{

		sub = new TProductSub();

		sub->FromJson(req->info->data);

		result = db->SaveProductSub(sub);



		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveProductSub succesfull.";
			SAFEDELETE(sub)
				return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveProductSub  error" , "");

	}

	SAFEDELETE(sub)
		return resp;
}

TResponseInfo * PandoraTest::RemoveProductSubRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveProductSub(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveProductSub succesfull.";
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveProductSub  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetProductSubRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductSub * sub = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		sub = db->GetProductSub(id);

		if(sub != NULL)
		{
			resp->data = sub->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductSub succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductSub  error" , "");

	}

	SAFEDELETE(sub)
		return resp;
}

TResponseInfo * PandoraTest::GetProductSubsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductSub> * subs = NULL;
	stringstream ss;
	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		subs = db->GetProductSubs(params);


		if(subs != NULL)
		{
			resp->data = subs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductSubs operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductSubs  error" , "");

	}

	SAFEDELETE(subs)
		return resp;
}

TResponseInfo * PandoraTest::GetProductSubViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductSubView * view = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		view = db->GetProductSubView(id, req->info->language);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductSubView succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductSubView  error" , "");

	}

	SAFEDELETE(view)
		return resp;
}

TResponseInfo * PandoraTest::GetProductSubsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductSubView> * views = NULL;

	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		views = db->GetProductSubsView(params);

		if(views != NULL)
		{
			resp->data = views->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductSubsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductSubsView  error" , "");

	}

	SAFEDELETE(views)
		return resp;
}




/************************************************************************/
/* IMAGES                                                               */
/************************************************************************/
TResponseInfo * PandoraTest::AddImageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	map<wstring, wstring> params;
	TImage * image = NULL;
	bool result = false;
	int cat_id = 0;
	try
	{
		image = new TImage();

		if (image->id == 0)
			image->id = db->GetObjectID();

		image->file_name = TUtility::Utf8ToUnicode(req->file.filename);
		image->file_type = TUtility::Utf8ToUnicode(req->file.file_type);
		image->file_raw = req->file.file_raw;
		logger->Add(image->file_raw);
		params = ParseParameters(req->info->parameters);
		image->owner_id = TUtility::wstoi(params[L"owner_id"]);
		if (image->owner_id == 0)
		{
			TError error;
			error.message = L"Owner id cannot be 0";
			throw error;
		}

		image->thumb->width = TUtility::wstoi(TUtility::Utf8ToUnicode(config->content["thumbnail_width"]));
		image->thumb->height = TUtility::wstoi(TUtility::Utf8ToUnicode(config->content["thumbnail_height"]));

		result = db->AddImage(image);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddImage succesfull.";
			resp->data = TUtility::ToJson(image->id, "id");
			SAFEDELETE(image)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddImage  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}

	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(image)
		return resp;
}


TResponseInfo * PandoraTest::SaveImageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TImage * image = NULL;
	bool result = false;

	try
	{

		image = new TImage();

		image->FromJson(req->info->data);
		if (image->owner_id == 0)
		{
			TError error;
			error.message = L"Owner id cannot be 0";
			throw error;
		}

		result = db->SaveImage(image);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveImage succesfull.";
			SAFEDELETE(image)
				return resp;
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "SaveImage  error" , "");

	}

	SAFEDELETE(image)
		return resp;
}

TResponseInfo * PandoraTest::RemoveImageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveImage(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveImage succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveImage  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetImageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TImage * image = NULL;
	int id = 0;
	bool select_raw = true;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		select_raw = TUtility::wstoi(params[L"select_raw"]);
		TUtility::CheckZeroValue(1, true, id);

		image = db->GetImage(id, select_raw);

		if(image != NULL)
		{
			resp->data = image->Serialize(1, "", req->info->response_type, false);
			resp->succesfull = 1;
			resp->message = "GetImage succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetImage  error" , "");

	}

	SAFEDELETE(image)
		return resp;
}

TResponseInfo * PandoraTest::GetImagesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TImage> * images = NULL;

	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		images = db->GetImages(params);


		if(images != NULL)
		{
			resp->data = images->Serialize(1, "", req->info->response_type, false);
			resp->succesfull = 1;
			resp->message = "GetImages operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetImages  error" , "");

	}

	SAFEDELETE(images)
		return resp;
}

/************************************************************************/
/* PRODUCT KIT                                                          */
/************************************************************************/
TResponseInfo * PandoraTest::AddProductKitRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductKit * kit = NULL;
	bool result = false;
	int cat_id = 0;
	try
	{


		kit = new TProductKit();
		kit->FromJson(req->info->data);
		kit->id = db->GetObjectID();

		result = db->AddProductKit(kit);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddProductKit succesfull.";
			resp->data = TUtility::ToJson(kit->id, "id");
		}

		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddProductKit  error" , "");

	}

	SAFEDELETE(kit)
		return resp;
}


TResponseInfo * PandoraTest::SaveProductKitRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductKit * kit = NULL;
	bool result = false;

	try
	{

		kit = new TProductKit();
		kit->FromJson(req->info->data);
		result = db->SaveProductKit(kit);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveProductKit succesfull.";
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveProductKit  error" , "");

	}

	SAFEDELETE(kit)
		return resp;
}

TResponseInfo * PandoraTest::RemoveProductKitRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveProductKit(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveProductKit succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveProductKit  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetProductKitRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductKit * kit = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		kit = db->GetProductKit(id);

		if(kit != NULL)
		{
			resp->data = kit->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductKit succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductKit  error" , "");

	}

	SAFEDELETE(kit)
		return resp;
}

TResponseInfo * PandoraTest::GetProductKitsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductKit> * kits = NULL;

	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		kits = db->GetProductKits(params);

		if(kits != NULL)
		{
			resp->data = kits->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductKits operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductKits  error" , "");

	}

	SAFEDELETE(kits)
		return resp;
}

TResponseInfo * PandoraTest::GetProductKitsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductKitView> * views = NULL;
	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		views = db->GetProductKitsView(params);


		if(views != NULL)
		{
			resp->data = views->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductKitsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductKitsView  error" , "");

	}

	SAFEDELETE(views)
		return resp;
}

TResponseInfo * PandoraTest::GetProductKitViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductKitView * view = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		view = db->GetProductKitView(id);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductKit succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductKitView  error" , "");

	}

	SAFEDELETE(view)
		return resp;
}

/************************************************************************/
/* DOCUMENTS                                                           */
/************************************************************************/

TResponseInfo * PandoraTest::SaveWarehouseRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TWarehouse * warehouse = NULL;
	bool result = false;

	try
	{
		warehouse = new TWarehouse();
		warehouse->FromJson(req->info->data);
		result = db->SaveWarehouse(warehouse);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveWarehouse succesfull.";
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveWarehouse  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::AddWarehouseRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TWarehouse * warehouse = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		warehouse = new TWarehouse();
		warehouse->FromJson(req->info->data);
		warehouse->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		result = db->AddWarehouse(warehouse, cat_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddWarehouse succesfull.";
			resp->data = TUtility::ToJson(warehouse->id, "id");
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddWarehouse  error" , "");

	}

	return resp;
}


TResponseInfo * PandoraTest::RemoveWarehouseRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->MayDelete(id);
		if (!result)
		{
			resp->succesfull = 0;
			resp->message = "Error. There are some reference to this object! ";
			return resp;
		}

		result = db->RemoveWarehouse(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveWarehouse succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveWarehouse  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetWarehouseRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TWarehouse * warehouse = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		warehouse = db->GetWarehouse(id, req->info->language);

		if(warehouse != NULL)
		{
			resp->data = warehouse->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetWarehouse succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetWarehouse  error" , "");

	}

	SAFEDELETE(warehouse)
		return resp;
}
TResponseInfo * PandoraTest::GetWarehousesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TWarehouse> * warehouses = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		warehouses = db->GetWarehouses(params);

		if(warehouses != NULL)

		{
			resp->data = warehouses->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetWarehouses operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetWarehouses  error" , "");

	}

	SAFEDELETE(warehouses)

		return resp;
}


/************************************************************************/
/* USERS                                                                */
/************************************************************************/

TResponseInfo * PandoraTest::SaveUserRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TUser * user = NULL;
	bool result = false;

	try
	{
		user = new TUser();
		user->FromJson(req->info->data);

		boost::regex check_pass("");
		result = boost::regex_match(TUtility::UnicodeToUtf8(user->password), check_pass);

		if (!result)
		{
			TError error;
			error.message = L"Wrong pass";
		}

		result = db->SaveUser(user);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveUser succesfull.";
		}


		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveUser  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(user)
		return resp;
}
TResponseInfo * PandoraTest::SaveUserPasswordRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	wstring passwd;
	int user_id = 0;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		passwd = params[L"password"];
		user_id = TUtility::wstoi(params[L"user_id"]);

		boost::regex check_pass("");
		result = boost::regex_match(TUtility::UnicodeToUtf8(passwd), check_pass);

		if (!result)
		{
			TError error;
			error.message = L"Wrong pass";
		}

		result = db->SaveUserPassword(user_id, passwd);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveUserPassword succesfull.";
		}


		else
		{
			throw 0;

		}

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveUserPassword  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::AddUserRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TUser * user = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{

		user = new TUser();
		user->FromJson(req->info->data);
		user->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		result = db->AddUser(user);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddUser succesfull.";
			resp->data = TUtility::ToJson(user->id, "id");
			SAFEDELETE(user)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddUser  error" , "");

	}

	SAFEDELETE(user)
		return resp;
}


TResponseInfo * PandoraTest::RemoveUserRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveUser(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveUser succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveUser  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetUserRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TUser * user = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		user = db->GetUser(id);

		if(user != NULL)
		{
			resp->data = user->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetUser succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetUser  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetUser()  error" , "");

	}

	SAFEDELETE(user)
		return resp;
}
TResponseInfo * PandoraTest::GetUserViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TUserView * user = NULL;
	int id = 0;
	string select_by = "";

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		select_by = TUtility::UnicodeToUtf8(params[L"select_by"]);
		TUtility::CheckZeroValue(1, true, id);

		user = db->GetUserView(id, req->info->language, select_by);

		if(user != NULL)
		{
			resp->data = user->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetUserView succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetUserView  error" , "");

	}

	SAFEDELETE(user)
		return resp;
}
TResponseInfo * PandoraTest::GetUsersRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TUser> * users = NULL;
	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		users = db->GetUsers(params);

		if(users != NULL)
		{
			resp->data = users->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetUsers operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetUsers  error" , "");

	}

	SAFEDELETE(users)
		return resp;
}

TResponseInfo * PandoraTest::GetUsersViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TUserView> * users = NULL;
	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		users = db->GetUsersView(params);

		if(users != NULL)
		{
			resp->data = users->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetUsersView operation succesfull!";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetUsersView  error" , "");

	}

	SAFEDELETE(users)
		return resp;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/
TResponseInfo * PandoraTest::UpdateUserStatusRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = true;

	try
	{

		is_client_window_open = true;

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UpdateUserStatus succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "UpdateUserStatus  error" , "");

	}

	return resp;
}

/************************************************************************/
/* WAREHOUSES                                                           */
/************************************************************************/


TResponseInfo * PandoraTest::AddPutInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPutInDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3502, "Add");

		doc = new TPutInDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		result = db->AddDocPutIn(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddPutInDocument succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddPutInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddPutInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::SavePutInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPutInDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TPutInDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocPutIn(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SavePutInDocument succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "SavePutInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SavePutInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemovePutInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = 0;

	try
	{
		db->CheckPermission(req->user_id, 3502, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocPutIn(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemovePutInDocument succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemovePutInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemovePutInDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetPutInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPutInDoc * doc = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3502, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocPutIn(id);

		if(doc != 0)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPutInDocument succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPutInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPutInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetPutInDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPutInDocView * view = NULL;
	int id = 0, lang_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3502, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		view = db->GetDocPutInView(id, lang_id);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPutInDocView succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPutInDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPutInDocumentView  error" , "");

	}

	SAFEDELETE(view)
		return resp;
}

TResponseInfo * PandoraTest::GetPutInDocumentsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPutInDoc> * docs = NULL;
	int type_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3502, "Edit");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_GOODS_ENTRY);

		docs = db->GetDocsPutIn(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPutInDocuments operation succesfull!";
		}
		else
		{
			throw 0;

		}


	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPutInDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPutInDocuments  error" , "");

	}
	SAFEDELETE(docs)
		return resp;
}
TResponseInfo * PandoraTest::GetPutInDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPutInDocView> * docs = NULL;
	int type_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3502, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_GOODS_ENTRY);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsPutInView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPutInDocumentsView operation succesfull!";
		}
		else
		{
			throw 0;

		}


	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPutInDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPutInDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}

TResponseInfo * PandoraTest::PostPutInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3502, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);
		
		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocPutInCron(doc_id, req->user_id);
		else
			result = db->PostDocPutIn(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostPutInDoc succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostPutInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostPutInDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostPutInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3502, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnpostDocPutInCron(doc_id, req->user_id);
		else
			result = db->UnpostDocPutIn(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnpostPutInDoc succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostPutInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostPutInDocument  error" , "");

	}

	return resp;
}

/************************************************************************/
/* DOC PUT IN ITEMS                                                     */
/************************************************************************/

TResponseInfo * PandoraTest::AddDocItemRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItem * doc_item = NULL;
	bool result = false;
	int cat_id = 0;
	int user_id = 0;
	try
	{

		doc_item = new TDocItem();
		doc_item->FromJson(req->info->data);
		doc_item->id = db->GetObjectID();
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		user_id = req->user_id;
		TUtility::CheckZeroValue(3, true, doc_item->id, doc_item->price->total->measure_id,
			doc_item->price->value->measure_id, doc_item->owner_id);

		result = db->AddDocItem(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocItem succesfull.";
			resp->data = TUtility::ToJson(doc_item->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddDocItem  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::AddPutInDocItemRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemPutIn * doc_item = NULL;
	bool result = false;
	int cat_id = 0;
	int user_id = 0;
	try
	{

		doc_item = new TDocItemPutIn();
		doc_item->FromJson(req->info->data);
		doc_item->id = db->GetObjectID();

		if (doc_item->price->total->measure_id == 0 || doc_item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		user_id = req->user_id;
		result = db->AddDocItemPutIn(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddPutInDocItem succesfull.";
			resp->data = TUtility::ToJson(doc_item->id, "id");
			SAFEDELETE(doc_item)
				return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddPutInDocItem  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::SavePutInDocItemRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemPutIn * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemPutIn();
		doc_item->FromJson(req->info->data);

		if (doc_item->price->total->measure_id == 0 || doc_item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}
		user_id = req->user_id;
		result = db->SaveDocItemPutIn(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SavePutInDocItem succesfull.";
			SAFEDELETE(doc_item)
				return resp;
		}


		else
		{
			throw 0;

		}

	}


	catch (...)
	{
		ProcessError(resp, "", "SavePutInDocItem  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::SavePutInDocItemsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TDocItemPutIn> * doc_items = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_items = new TCollection<TDocItemPutIn>("TDocItemPutIn");
		doc_items->FromJson(req->info->data);

		for (int i = 0; i < (int) doc_items->items.size(); ++i)
		{
			TDocItemPutIn * item = doc_items->items.at(i);
			if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
			{
				resp->succesfull = 0;
				resp->message = "no currency id";
				return resp;
			}
		}
		user_id = req->user_id;
		result = db->SaveDocItemsPutIn(doc_items, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SavePutInDocItems succesfull.";
			SAFEDELETE(doc_items)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SavePutInDocItems  error" , "");

	}

	SAFEDELETE(doc_items)
		return resp;
}

TResponseInfo * PandoraTest::RemovePutInDocItemRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemPutIn(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemovePutInDocItem succesfull.";
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemovePutInDocItem  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetPutInDocItemRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemPutIn * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemPutIn(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPutInDocItem succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetPutInDocItem  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::GetPutInDocItemsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemPutInView> * doc_items_view = NULL;
	int lang_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		doc_items_view = db->GetDocItemsPutInView(params);

		if(doc_items_view != NULL)
		{
			resp->data = doc_items_view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsPutInView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPutInDocItemsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_items_view)
		return resp;
}



/************************************************************************/
/* DOCUMENTS                                                            */
/************************************************************************/
TResponseInfo * PandoraTest::GetDocumentTypeIdRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int id = 0, type_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, id);

		type_id = db->GetDocumentTypeId(id);

		if(type_id)
		{
			resp->data = TUtility::ToJson(type_id, "type_id");
			resp->succesfull = 1;
			resp->message = "GetDocumentTypeId operation succesfull!";

		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocumentTypeId  error" , "");

	}

	return resp;
}


TResponseInfo * PandoraTest::GetDocumentsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocument> * docs = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocuments(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocuments operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::GetDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocumentView> * docs = NULL;
	bool result = false;

	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocumentsView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocumentsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::GetDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocumentView * doc = NULL;
	bool result = false;
	int id = 0, lang_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = req->info->language;
		id = TUtility::wstoi(params[L"lang_id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocumentView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(NULL, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocumentView operation succesfull!";

		}
		else
		{
			throw 0;

		}


	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


/************************************************************************/
/* TAXES                                                               */
/************************************************************************/

TResponseInfo * PandoraTest::SaveTaxRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTax * tax = NULL;
	bool result = false;

	try
	{
		tax = new TTax();
		tax->FromJson(req->info->data);
		result = db->SaveTax(tax);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveTax succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveTax  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveTax  error" , "");

	}

	SAFEDELETE(tax)
		return resp;
}

TResponseInfo * PandoraTest::AddTaxRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTax * tax = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3552, "Add");

		tax = new TTax();
		tax->FromJson(req->info->data);
		tax->id = db->GetObjectID();

		result = db->AddTax(tax);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddTax succesfull.";
			resp->data = TUtility::ToJson(tax->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddTax  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddTax  error" , "");

	}

	SAFEDELETE(tax)
		return resp;
}


TResponseInfo * PandoraTest::RemoveTaxRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3552, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		if (id == 1000)
		{
			resp->succesfull = 0;
			resp->message = "Cannot delete VAT";
			return resp;
		}

		result = db->RemoveTax(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveTax succesfull.";
		}


		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveTax  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveTax  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetTaxRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TTax * tax = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3552, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		tax = db->GetTax(id, req->info->language);

		if(tax != NULL)
		{
			resp->data = tax->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetTax succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetTax  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetTax  error" , "");

	}

	SAFEDELETE(tax)
		return resp;
}
TResponseInfo * PandoraTest::GetTaxesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TTax> * taxes = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3552, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		taxes = db->GetTaxes(params);

		if(taxes != NULL)
		{
			resp->data = taxes->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetTaxes operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetTaxes  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetTaxes  error" , "");

	}

	SAFEDELETE(taxes)
		return resp;
}

TResponseInfo * PandoraTest::GetTaxesViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TTaxView> * taxes = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3552, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		taxes = db->GetTaxesView(params);

		if(taxes != NULL)
		{
			resp->data = taxes->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetTaxesView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetTaxesView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetTaxesView  error" , "");

	}

	SAFEDELETE(taxes)
		return resp;
}



TResponseInfo * PandoraTest::AddCustomTaxesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TTax> * taxes = NULL;
	bool result = false;
	int object_id = 0;

	try
	{

		taxes = new TCollection<TTax>("TTax");
		taxes->FromJson(req->info->data);
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		object_id =  TUtility::wstoi(params[L"object_id"]);

		result = db->AddCustomTaxes(taxes, object_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddCustomTaxes succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddCustomTaxes  error" , "");

	}

	SAFEDELETE(taxes)
		return resp;
}


/************************************************************************/
/* DISCOUNTS                                                            */
/************************************************************************/
TResponseInfo * PandoraTest::GetDiscountsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDiscountView> * discounts = NULL;
	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		discounts = db->GetDiscountsView(params);

		if(discounts != NULL)
		{
			resp->data = discounts->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDiscountsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetDiscountsView  error" , "");

	}

	SAFEDELETE(discounts)
		return resp;
}



TResponseInfo * PandoraTest::AddCustomDiscountsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TDiscount> * discounts = NULL;
	bool result = false;
	int object_id = 0;

	try
	{

		discounts = new TCollection<TDiscount>("TDiscount");

		discounts->FromJson(req->info->data);

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		object_id =  TUtility::wstoi(params[L"object_id"]);

		result = db->AddCustomDiscounts(discounts, object_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddCustomDiscounts succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddCustomDiscounts  error" , "");

	}

	SAFEDELETE(discounts)
		return resp;
}


/************************************************************************/
/* WRITEOFF DOCUMENTS                                                   */
/************************************************************************/
TResponseInfo * PandoraTest::AddWriteOffDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TWriteOffDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3501, "Add");

		doc = new TWriteOffDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		result = db->AddDocWriteOff(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddWriteOffDocument succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}


		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddWriteOffDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddWriteOffDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::SaveWriteOffDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TWriteOffDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TWriteOffDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocWriteOff(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveWriteOffDocument succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveWriteOffDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveWriteOffDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveWriteOffDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3501, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocWriteOff(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveWriteOffDocument succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveWriteOffDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveWriteOffDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetWriteOffDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TWriteOffDoc * doc = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3501, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocWriteOff(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetWriteOffDocument succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetWriteOffDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetWriteOffDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::GetWriteOffDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TWriteOffDocView * doc = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3501, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocWriteOffView(id, req->info->language);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetWriteOffDocumentView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetWriteOffDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetWriteOffDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetWriteOffDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TWriteOffDocView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3501, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_GOODS_WRITEOFF);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsWriteOffView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsWriteOffView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetWriteOffDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetWriteOffDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::GetWriteOffDocumentsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TWriteOffDoc> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3501, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_GOODS_WRITEOFF);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsWriteOff(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetWriteOffDocuments operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetWriteOffDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetWriteOffDocuments  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::PostWriteOffDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3501, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id = TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocWriteOffCron(doc_id, req->user_id);
		else
			result = db->PostDocWriteOff(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostWriteOffDoc succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostWriteOffDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostWriteOffDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostWriteOffDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3501, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id = TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnpostDocWriteOffCron(doc_id, req->user_id);
		else
			result = db->UnpostDocWriteOff(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnpostWriteOffDoc succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnPostWriteOffDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnPostWriteOffDocument  error" , "");

	}

	return resp;
}

/************************************************************************/
/* DOC ITEMS WRITEOFF                                                   */
/************************************************************************/
TResponseInfo * PandoraTest::AddDocItemWriteOffRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemWriteOff * item = NULL;
	bool result = false;
	int cat_id = 0;
	int user_id = 0;

	try
	{
		item = new TDocItemWriteOff();
		item->FromJson(req->info->data);
		item->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		// 		if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
		// 		{
		// 			resp->succesfull = 0;
		// 			resp->message = "no currency id";
		// 			return resp;
		// 		}

		user_id = req->user_id;
		result = db->AddDocItemWriteOff(item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocItemWriteOff succesfull.";
			resp->data = TUtility::ToJson(item->id, "id");
			SAFEDELETE(item)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddDocItemWriteOff  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}
TResponseInfo * PandoraTest::SaveDocItemWriteOffRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemWriteOff * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemWriteOff();
		doc_item->FromJson(req->info->data);
		user_id = req->user_id;

		result = db->SaveDocItemWriteOff(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemWriteOff succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveDocItemWriteOff  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemWriteOffRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemWriteOff(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemWriteOff succesfull.";
			return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemWriteOff  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemWriteOffRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemWriteOff * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemWriteOff(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemWriteOff succesfull.";
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemWriteOff  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::GetDocItemsWriteOffViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemWriteOffView> * doc_items = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_items = db->GetDocItemsWriteOffView(params);

		if(doc_items != NULL)
		{
			resp->data = doc_items->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsWriteOffView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (string error)
	{
		cout << "Caught exception:  " << error << endl;
		logger->Add("PandoraTest::RequestProcessor() line: ExecCommand(), GetDocItemsWriteOffViewRequest()");
		resp->succesfull = 0;
		resp->message = error;
		resp->data = ParseErrorCode(error);
	}

	SAFEDELETE(doc_items)
		return resp;
}



TResponseInfo * PandoraTest::ReserveDocItemWriteOffRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int item_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		item_id = TUtility::wstoi(params[L"item_id"]);
		TUtility::CheckZeroValue(1, true, item_id);

		result = db->ReserveDocItemWriteOff(item_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ReserveDocItemWriteOff succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "ReserveWriteOffDocItem  error" , "");

	}


	return resp;
}

TResponseInfo * PandoraTest::UnReserveDocItemWriteOffRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int item_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		item_id = TUtility::wstoi(params[L"item_id"]);
		TUtility::CheckZeroValue(1, true, item_id);

		result = db->UnReserveDocItemWriteOff(item_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ReserveDocItemWriteOff succesfull.";
			return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "ReserveWriteOffDocItem  error" , "");

	}


	return resp;
}

/************************************************************************/
/* BANK ACCOUNTS                                                        */
/************************************************************************/
TResponseInfo * PandoraTest::AddBankAccountRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TBankAccount * account = NULL;
	bool result = false;
	int cat_id = 0;
	try
	{
		account = new TBankAccount();
		account->FromJson(req->info->data);
		account->id = db->GetObjectID();

		result = db->AddBankAccount(account);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddBankAccount succesfull.";
			resp->data = TUtility::ToJson(account->id, "id");
			SAFEDELETE(account)
				return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddBankAccount  error" , "");

	}

	SAFEDELETE(account)
		return resp;
}


TResponseInfo * PandoraTest::SaveBankAccountRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TBankAccount * account = NULL;
	bool result = false;

	try
	{

		account = new TBankAccount();
		account->FromJson(req->info->data);
		result = db->SaveBankAccount(account);



		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveBankAccount succesfull.";
			SAFEDELETE(account)
				return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveBankAccount  error" , "");

	}

	SAFEDELETE(account)
		return resp;
}

TResponseInfo * PandoraTest::RemoveBankAccountRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveBankAccount(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveBankAccount succesfull.";
			return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveBankAccount  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetBankAccountRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TBankAccount * account = NULL;
	int id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		account = db->GetBankAccount(id);

		if(account != NULL)
		{
			resp->data = account->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetBankAccount succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetBankAccount  error" , "");

	}

	SAFEDELETE(account)
		return resp;
}

TResponseInfo * PandoraTest::GetBankAccountViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TBankAccountView * account = NULL;
	int id = 0, date = 0;
	bool with_balance = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		with_balance =  TUtility::wstoi(params[L"with_balance"]);
		date =  TUtility::wstoi(params[L"date"]);
		TUtility::CheckZeroValue(1, true, id);

		account = db->GetBankAccountView(id, req->info->language, with_balance, date);

		if(account != NULL)
		{
			resp->data = account->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetBankAccountView succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetBankAccountView  error" , "");

	}

	SAFEDELETE(account)
		return resp;
}

TResponseInfo * PandoraTest::GetBankAccountsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TBankAccount> * accounts = NULL;

	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"user_id"] = TUtility::itows(req->user_id);

		accounts = db->GetBankAccounts(params);

		if(accounts != NULL)
		{
			resp->data = accounts->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetBankAccounts operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetBankAccounts  error" , "");

	}
    SAFEDELETE(accounts)
	return resp;
}

TResponseInfo * PandoraTest::GetBankAccountsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TBankAccountView> * accounts = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		accounts = db->GetBankAccountsView(params);

		if(accounts != NULL)
		{
			resp->data = accounts->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetBankAccountsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetBankAccountsView  error" , "");

	}
    SAFEDELETE(accounts)
	return resp;
}

/************************************************************************/
/* ACCOUNT SUMMARY REQUEST                                              */
/************************************************************************/
TResponseInfo * PandoraTest::GetAccountSummaryRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TFinSummary> * summary = NULL;
	int account_id = 0, measure_id = 0, date = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		account_id = TUtility::wstoi(params[L"account_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		date = TUtility::wstoi(params[L"date"]);
		TUtility::CheckZeroValue(1, true, account_id);

		summary = db->GetAccountSummaries(account_id, measure_id, date);

		if(summary != NULL)
		{
			resp->data = summary->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetAccountSummary succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostWriteOffDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(summary)
		return resp;
}

TResponseInfo * PandoraTest::GetAccountSummariesViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TFinSummaryView> * views = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		views = db->GetAccountSummariesView(params);

		if(views != NULL)
		{
			resp->data = views->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetAccountSummariesView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostWriteOffDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(views)
		return resp;
}

TResponseInfo * PandoraTest::GetTaxesSummaryViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TFinSummaryView> * views = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		views = db->GetTaxesFinSummaries(params);

		if(views != NULL)
		{
			resp->data = views->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetTaxesFinSummaries operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostWriteOffDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(views)
		return resp;
}



/************************************************************************/
/*                                                                      */
/************************************************************************/
TResponseInfo * PandoraTest::GetDocPaymentSumRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TSum * sum = NULL;
	int doc_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id = TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		sum = db->GetDocPaymentSum(doc_id);

		if(sum != NULL)
		{
			resp->data = sum->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocPaymentSum succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocPaymentSum  error" , "");

	}

	SAFEDELETE(sum)
		return resp;
}

/************************************************************************/
/* IN PAYMENT DOCUMENT                                                  */
/************************************************************************/

TResponseInfo * PandoraTest::AddInPaymentDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInPaymentDoc * doc = NULL;
	int cat_id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3506, "Add");

		doc = new TInPaymentDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		result = db->AddDocInPayment(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddInPaymentDoc succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddInPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddInPaymentDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::SaveInPaymentDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInPaymentDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TInPaymentDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocInPayment(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveWriteOffDocument succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveInPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveInPaymentDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveInPaymentDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3506, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocInPayment(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveInPaymentDoc succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveInPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveInPaymentDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetInPaymentDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInPaymentDoc * doc = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3506, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocInPayment(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetInPaymentDoc succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInPaymentDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetInPaymentDocumentViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInPaymentDocView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3506, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocInPaymentView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetInPaymentDocView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInPaymentDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInPaymentDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::GetInPaymentDocumentsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TInPaymentDoc> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3506, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_INCOMING_PAYMENT);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsInPayment(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetInPaymentDocs operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInPaymentDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInPaymentDocuments  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::GetInPaymentDocumentsViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TInPaymentDocView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3506, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_INCOMING_PAYMENT);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsInPaymentView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetInPaymentDocsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInPaymentDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInPaymentDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::PostInPaymentDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3506, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocInPaymentCron(doc_id, req->user_id);
		else
			result = db->PostDocInPayment(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocInPayment succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostInPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostInPaymentDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostInPaymentDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3506, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnPostDocInPaymentCron(doc_id, req->user_id);
		else
			result = db->UnPostDocInPayment(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnPostDocInPayment succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostInPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostInPaymentDocument  error" , "");

	}

	return resp;
}

/************************************************************************/
/* OUT PAYMENT DOCUMENT                                                  */
/************************************************************************/
TResponseInfo * PandoraTest::AddOutPaymentDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TOutPaymentDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3507, "Add");

		doc = new TOutPaymentDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		result = db->AddDocOutPayment(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddOutPaymentDoc succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddOutPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddOutPaymentDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::SaveOutPaymentDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TOutPaymentDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TOutPaymentDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocOutPayment(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocOutPayment succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveOutPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveOutPaymentDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::RemoveOutPaymentDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3507, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocOutPayment(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveOutPaymentDoc succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveOutPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveOutPaymentDocument  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetOutPaymentDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TOutPaymentDoc * doc = NULL;
	int id = 0;

	try
	{

		db->CheckPermission(req->user_id, 3507, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocOutPayment(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetOutPaymentDoc succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOutPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOutPaymentDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::GetOutPaymentDocumentViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TOutPaymentDocView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3507, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocOutPaymentView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetOutPaymentDocView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOutPaymentDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOutPaymentDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::GetOutPaymentDocumentsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TOutPaymentDoc> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3507, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_OUTGOING_PAYMENT);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsOutPayment(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetOutPaymentDocs operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOutPaymentDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOutPaymentDocuments  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}
TResponseInfo * PandoraTest::GetOutPaymentDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TOutPaymentDocView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3507, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_OUTGOING_PAYMENT);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsOutPaymentView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetOutPaymentDocsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOutPaymentDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOutPaymentDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}
TResponseInfo * PandoraTest::PostOutPaymentDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3507, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocOutPaymentCron(doc_id, req->user_id);
		else
			result = db->PostDocOutPayment(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocOutPayment succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostOutPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostOutPaymentDocument  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::UnpostOutPaymentDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3507, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnPostDocOutPaymentCron(doc_id, req->user_id);
		else
			result = db->UnPostDocOutPayment(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnPostDocOutPayment succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostOutPaymentDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostOutPaymentDocument  error" , "");

	}

	return resp;
}
/************************************************************************/
/* OUT ADD COSTS                                                        */
/************************************************************************/
TResponseInfo * PandoraTest::AddAddCostsDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TAddCostsDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3507, "Add");

		doc = new TAddCostsDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		result = db->AddDocAddCosts(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddAddCostsDoc succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddAddCostsDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddAddCostsDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::SaveAddCostsDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TAddCostsDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TAddCostsDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocAddCosts(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocAddCosts succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveAddCostsDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveAddCostsDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::RemoveAddCostsDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3507, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocAddCosts(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveAddCostsDoc succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveAddCostsDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveAddCostsDocument  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetAddCostsDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TAddCostsDoc * doc = NULL;
	int id = 0;

	try
	{

		db->CheckPermission(req->user_id, 3507, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocAddCosts(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetAddCostsDoc succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetAddCostsDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetAddCostsDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::GetAddCostsDocumentViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TAddCostsDocView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3507, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocAddCostsView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetAddCostsDocView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetAddCostsDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetAddCostsDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::GetAddCostsDocumentsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TAddCostsDoc> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3507, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_ADDCOSTS_PAYMENT);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsAddCosts(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetAddCostsDocs operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetAddCostsDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetAddCostsDocuments  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}
TResponseInfo * PandoraTest::GetAddCostsDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TAddCostsDocView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3507, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_ADDCOSTS_PAYMENT);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsAddCostsView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetAddCostsDocsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetAddCostsDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetAddCostsDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}
TResponseInfo * PandoraTest::PostAddCostsDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3507, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocAddCostsCron(doc_id, req->user_id);
		else
			result = db->PostDocAddCosts(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocAddCosts succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostAddCostsDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostAddCostsDocument  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::UnpostAddCostsDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3507, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnPostDocAddCostsCron(doc_id, req->user_id);
		else
			result = db->UnPostDocAddCosts(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnPostDocAddCosts succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostAddCostsDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostAddCostsDocument  error" , "");

	}

	return resp;
}
/************************************************************************/
/* DOC ITEM SERVICES                                                    */
/************************************************************************/
TResponseInfo * PandoraTest::AddDocItemServiceRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemService * item = NULL;
	bool result = false;

	try
	{

		item = new TDocItemService();
		item->FromJson(req->info->data);
		item->id = db->GetObjectID();

		if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		result = db->AddDocItemService(item, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocItemService succesfull.";
			resp->data = TUtility::ToJson(item->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddDocItemService  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}


TResponseInfo * PandoraTest::SaveDocItemServiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemService * item = NULL;
	bool result;

	try
	{

		item = new TDocItemService();
		item->FromJson(req->info->data);

		if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		result = db->SaveDocItemService(item, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemService succesfull.";
			SAFEDELETE(item)
				return resp;
		}

		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveDocItemService  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}

	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemServiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemService(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemService succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemService  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemServiceRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemService * item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		item = db->GetDocItemService(id);

		if(item != NULL)
		{
			resp->data = item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemService succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetWriteOffDocument  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}

/************************************************************************/
/* DOC ITEM PRODUCT IN                                                  */
/************************************************************************/
TResponseInfo * PandoraTest::SaveDocItemProductInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemPutIn * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemPutIn();
		doc_item->FromJson(req->info->data);

		if (doc_item->price->total->measure_id == 0 || doc_item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		user_id = req->user_id;
		result = db->SaveDocItemPutIn(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemProductIn succesfull.";
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveDocItemProductIn  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::SaveDocItemsProductInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TDocItemPutIn> * doc_items = NULL;
	bool result = false;
	int user_id = 0;

	try
	{


		doc_items = new TCollection<TDocItemPutIn>("TDocItemPutIn");
		doc_items->FromJson(req->info->data);

		for (int i = 0; i < (int) doc_items->items.size(); ++i)
		{
			TDocItemPutIn * item = doc_items->items.at(i);
			if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
			{
				resp->succesfull = 0;
				resp->message = "no currency id";
				throw 0;
			}
		}
		user_id = req->user_id;
		result = db->SaveDocItemsPutIn(doc_items, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemsProductIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveDocItemsProductIn  error" , "");

	}

	SAFEDELETE(doc_items)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemProductInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemPutIn(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemProductIn succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemProductIn  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemProductInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemPutIn * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemPutIn(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemProductIn succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemProductIn  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::GetDocItemsProductInViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemPutInView> * doc_items_view = NULL;
	int lang_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_items_view = db->GetDocItemsPutInView(params);

		if(doc_items_view != NULL)
		{
			resp->data = doc_items_view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsPutInView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetDocItemsProductInView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_items_view)
		return resp;
}



/************************************************************************/
/* RELATION DOCUMENTS                                                   */
/************************************************************************/
TResponseInfo * PandoraTest::GetRelationDocumentsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocumentView> * docs = NULL;
	int doc_id = 0, lang_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id = TUtility::wstoi(params[L"doc_id"]);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		docs = db->GetRelationDocuments(doc_id, lang_id);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetRelationDocuments operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetRelationDocuments  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}

/************************************************************************/
/* LOTS                                                                 */
/************************************************************************/
TResponseInfo * PandoraTest::GetLotViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TLotView * lot = NULL;
	int lot_id = 0, date = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		lot_id = TUtility::wstoi(params[L"id"]);
		date = TUtility::wstoi(params[L"date"]);
		TUtility::CheckZeroValue(1, true, lot_id);

		lot = db->GetLotView(lot_id, req->info->language, date);

		if(lot != NULL)
		{
			resp->data = lot->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetLotView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetLotView  error" , "");

	}

	SAFEDELETE(lot)
		return resp;
}


TResponseInfo * PandoraTest::GetLotsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TLotView> * lots = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		lots = db->GetLotsView(params);

		if(lots != NULL)
		{
			resp->data = lots->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetLotsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetLotsView  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemLotsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TLotView> * lots = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		lots = db->GetDocItemLotsView(params);

		if(lots != NULL)
		{
			resp->data = lots->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemLotsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemLotsView  error" , "");

	}

	SAFEDELETE(lots)
		return resp;
}

TResponseInfo * PandoraTest::GetDocItemLotViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TLotView * lot = NULL;
	int lot_id = 0, doc_item_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		lot_id = TUtility::wstoi(params[L"lot_id"]);
		doc_item_id = TUtility::wstoi(params[L"owner_id"]);
		TUtility::CheckZeroValue(1, true, lot_id);

		lot = db->GetDocItemLotView(lot_id, doc_item_id, req->info->language);

		if(lot != NULL)
		{
			resp->data = lot->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemLotView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemLotView  error" , "");

	}

	SAFEDELETE(lot)
		return resp;
}

/************************************************************************/
/* LOGS(HISTORY)                                                        */
/************************************************************************/

TResponseInfo * PandoraTest::GetLogsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <THistoryView> * histories = NULL;
	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		histories = db->GetHistoriesView(params);

		if(histories != NULL)
		{
			resp->data = histories->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetLogsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetLogsView  error" , "");

	}

	return resp;
}

/************************************************************************/
/* CALCULATE PRODUCT SUMMARY                                            */
/************************************************************************/
TResponseInfo * PandoraTest::CalculateProductSummaryRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int prod_id = 0, warehouse_id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		prod_id =  TUtility::wstoi(params[L"id"]);
		warehouse_id =  TUtility::wstoi(params[L"warehouse_id"]);
		TUtility::CheckZeroValue(1, true, prod_id);

		result = db->CalculateProductBalance(prod_id, warehouse_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveBankAccount succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "CalculateProductSummary  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}


/************************************************************************/
/* PRICE ANALYSIS                                                       */
/************************************************************************/
TResponseInfo * PandoraTest::CalculatePaymentSumRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TSum * sum = NULL;
	TSumView * view = NULL;
	bool calculate_by_tax_value = false, calculate_by_total = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		calculate_by_tax_value = TUtility::wstoi(params[L"calculate_by_tax_value"]);
		calculate_by_total = true;
		sum = new TSum();
		sum->FromJson(req->info->data);

		view = db->CalculatePaymentSum(sum, req->info->language, calculate_by_tax_value, calculate_by_total);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "CalculatePaymentSum operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "CalculatePaymentSum  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "CalculatePaymentSum  error" , "");

	}

	SAFEDELETE(view)
		SAFEDELETE(sum)
		return resp;
}
TResponseInfo * PandoraTest::CalculateDocItemPriceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPriceView * view = NULL;
	TDocItem * item = NULL;
	bool calculate_by_tax_value = false, calculate_by_total = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		calculate_by_tax_value = TUtility::wstoi(params[L"calculate_by_tax_value"]);
		calculate_by_total = TUtility::wstoi(params[L"calculate_by_total"]);
		item = new TDocItem();
		item->FromJson(req->info->data);

		view = db->CalculateDocItemPrice(item, req->info->language, calculate_by_tax_value, calculate_by_total);

		if(view != NULL)
		{
			resp->data = view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "CalculateDocItemPrice operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "CalculatePriceCategoriesView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "CalculatePriceCategoriesView  error" , "");

	}

	SAFEDELETE(view)
		SAFEDELETE(item)
		return resp;
}


TResponseInfo * PandoraTest::GetPriceAnalyticRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPriceAnalytics * anal = NULL;
	int prod_id = 0, partner_id = 0, cat_id = 0, price_type_id = 0, date = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		prod_id = TUtility::wstoi(params[L"prod_id"]);
		partner_id = TUtility::wstoi(params[L"partner_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		price_type_id = TUtility::wstoi(params[L"price_type_id"]);
		date = TUtility::wstoi(params[L"date"]);

		anal = db->GetPriceAnalytic(prod_id, partner_id, cat_id, price_type_id, date, req->user_id);

		if(anal != NULL)
		{
			resp->data = anal->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPriceAnalytic succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPriceAnalytic  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPriceAnalytic  error" , "");

	}

	SAFEDELETE(anal)
		return resp;
}

/************************************************************************/
/* DOC PRODUCT IN                                                            */
/************************************************************************/

TResponseInfo * PandoraTest::AddProductInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductInDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3522, "Add");

		doc = new TProductInDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, doc->id);

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddDocProductIn(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocProductIn succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddProductInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddProductInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::SaveProductInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductInDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TProductInDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;
		//		TUtility::CheckZeroValue(1, true, doc);

		result = db->SaveDocProductIn(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocProductIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveProductInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveProductInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveProductInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{

		db->CheckPermission(req->user_id, 3522, "Remove");
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocProductIn(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveProductInDocument succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveProductInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveProductInDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetProductInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductInDoc * doc = NULL;
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3522, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocProductIn(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocProductIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetProductInDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductInDocView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3522, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocProductInView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocProductInView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductInDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductInDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetProductInDocumentsRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductInDoc> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3522, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_GOODS_RECEIPT);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsProductIn(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsProductIn operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductInDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductInDocuments  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::GetProductInDocumentsViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductInDocView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3522, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_GOODS_RECEIPT);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsProductInView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsProductInView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductInDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductInDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::PostProductInDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3522, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		
		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocProductInCron(doc_id, req->user_id);
		else
			result = db->PostDocProductIn(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocProductIn succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostProductInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostProductInDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostProductInDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3522, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnpostDocProductInCron(doc_id, req->user_id);
		else
			result = db->UnpostDocProductIn(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnpostDocProductIn succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostProductInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostProductInDocument  error" , "");

	}

	return resp;
}
/************************************************************************/
/* DOC RETURN IN                                                            */
/************************************************************************/

TResponseInfo * PandoraTest::AddReturnInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReturnInDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		//		db->CheckPermission(req->user_id, 3522, "Add");

		doc = new TReturnInDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;
		TUtility::CheckZeroValue(2, true, req->user_id, doc->id);

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddDocReturnIn(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocReturnIn succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddReturnInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddReturnInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::SaveReturnInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReturnInDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TReturnInDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocReturnIn(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocReturnIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveReturnInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveReturnInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveReturnInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		//		db->CheckPermission(req->user_id, 3522, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocReturnIn(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveReturnInDocument succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveReturnInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveReturnInDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetReturnInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReturnInDoc * doc = NULL;
	int id = 0;
	bool result = false;

	try
	{
		//		db->CheckPermission(req->user_id, 3522, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocReturnIn(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocReturnIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetReturnInDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReturnInDocView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{
		//		db->CheckPermission(req->user_id, 3522, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocReturnInView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocReturnInView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReturnInDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReturnInDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetReturnInDocumentsRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TReturnInDoc> * docs = NULL;

	try
	{
		//		db->CheckPermission(req->user_id, 3522, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_CUSTOMER_RETURNS);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsReturnIn(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsReturnIn operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReturnInDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReturnInDocuments  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::GetReturnInDocumentsViewRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TReturnInDocView> * docs = NULL;

	try
	{
		//		db->CheckPermission(req->user_id, 3522, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_CUSTOMER_RETURNS);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsReturnInView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsReturnInView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReturnInDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReturnInDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::PostReturnInDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3522, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocReturnInCron(doc_id, req->user_id);
		else
			result = db->PostDocReturnIn(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocReturnIn succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostReturnInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostReturnInDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostReturnInDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3522, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnpostDocReturnInCron(doc_id, req->user_id);
		else
			result = db->UnpostDocReturnIn(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnpostDocReturnIn succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostReturnInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostReturnInDocument  error" , "");

	}

	return resp;
}
/************************************************************************/
/* DOC ITEM RETURN  IN                                                  */
/************************************************************************/
TResponseInfo * PandoraTest::SaveDocItemReturnInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemReturnIn * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemReturnIn();
		doc_item->FromJson(req->info->data);

		if (doc_item->price->total->measure_id == 0 || doc_item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		user_id = req->user_id;
		result = db->SaveDocItemReturnIn(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemReturnIn succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveDocItemReturnIn  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemReturnInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemReturnIn(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemReturnIn succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemReturnIn  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemReturnInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemReturnIn * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemReturnIn(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemReturnIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemReturnIn  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::GetDocItemsReturnInViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemReturnInView> * doc_items_view = NULL;
	int lang_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		doc_items_view = db->GetDocItemsReturnInView(params);

		if(doc_items_view != NULL)
		{
			resp->data = doc_items_view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsReturnInView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetDocItemsReturnInView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetDocItemsReturnInView  error" , "");

	}

	SAFEDELETE(doc_items_view)
		return resp;
}

/************************************************************************/
/* DOC PRODUCT OUT                                                      */
/************************************************************************/

TResponseInfo * PandoraTest::AddProductOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductOutDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3521, "Add");

		doc = new TProductOutDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, doc->id);

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddDocProductOut(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocProductOut succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddProductOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddProductOutDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::SaveProductOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductOutDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TProductOutDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocProductOut(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocProductOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveProductOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveProductOutDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveProductOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3521, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocProductOut(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveProductOutDocument succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveProductOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveProductOutDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetProductOutDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductOutDoc * doc = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3521, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocProductOut(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocProductOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductOutDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetProductOutDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TProductOutDocView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3521, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocProductOutView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocProductOutView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductOutDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductOutDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetProductOutDocumentsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductOutDoc> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3521, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_SALES);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsProductOut(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsProductOut operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductOutDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductOutDocuments  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::GetProductOutDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductOutDocView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3521, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_SALES);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsProductOutView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsProductOutView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetProductOutDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetProductOutDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::PostProductOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3521, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocProductOutCron(doc_id, req->user_id);
		else
			result = db->PostDocProductOut(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocProductOut succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostProductOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostProductOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3521, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);
		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnpostDocProductOutCron(doc_id, req->user_id);
		else
			result = db->UnpostDocProductOut(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnpostDocProductOut succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostProductOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}
/************************************************************************/
/* DOC ITEMS WRITEOFF                                                   */
/************************************************************************/
TResponseInfo * PandoraTest::AddDocItemProductOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemProductOut * item = NULL;
	bool result = false;
	int cat_id = 0;
	int user_id = 0;

	try
	{
		item = new TDocItemProductOut();
		item->FromJson(req->info->data);
		item->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}
		user_id = req->user_id;
		TUtility::CheckZeroValue(2, true, req->user_id, item->id);

		result = db->AddDocItemProductOut(item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocItemProductOut succesfull.";
			resp->data = TUtility::ToJson(item->id, "id");
			SAFEDELETE(item)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddDocItemProductOut  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}
TResponseInfo * PandoraTest::SaveDocItemProductOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemProductOut * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemProductOut();
		doc_item->FromJson(req->info->data);
		user_id = req->user_id;

		//		TUtility::CheckZeroValue(5, true,  doc_item->price, doc_item->price->value,
		//			doc_item->price->total, doc_item->price->unit, doc_item->quantity);
		//		TUtility::CheckZeroValue(7, true,  doc_item->price->total->measure_id, doc_item->price->value->measure_id,
		//			doc_item->warehouse_id, doc_item->product_id, doc_item->owner_id, doc_item->id, doc_item->type_id, req->user_id);

		TUtility::CheckZeroValue(1, true, (TCollection<TBaseClass> *) doc_item->lots);

		result = db->SaveDocItemProductOut(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemProductOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveDocItemProductOut  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemProductOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  atoi(TUtility::UnicodeToUtf8(params[L"id"]).c_str());
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemProductOut(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemProductOut succesfull.";
			return resp;
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemProductOut  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemProductOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemProductOut * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemProductOut(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemProductOut succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemProductOut  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::GetDocItemsProductOutViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemProductOutView> * doc_items_view = NULL;
	int lang_id = 0;
	stringstream ss;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_items_view = db->GetDocItemsProductOutView(params);

		if(doc_items_view != NULL)
		{
			resp->data = doc_items_view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsProductOutView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetDocItemsProductOutView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_items_view)
		return resp;
}

TResponseInfo * PandoraTest::ReserveDocItemProductOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int item_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		item_id =  TUtility::wstoi(params[L"item_id"]);
		TUtility::CheckZeroValue(1, true, item_id);

		result = db->ReserveDocItemProductOut(item_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ReserveDocItemProductOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "ReserveDocItemProductOut  error" , "");

	}


	return resp;
}

TResponseInfo * PandoraTest::UnReserveDocItemProductOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int item_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		item_id =  TUtility::wstoi(params[L"item_id"]);
		TUtility::CheckZeroValue(1, true, item_id);

		result = db->UnReserveDocItemProductOut(item_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ReserveDocItemProductOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "UnReserveDocItemProductOut  error" , "");

	}


	return resp;
}

/************************************************************************/
/* DOC ITEM PRODUCT OUT SERVICES                                        */
/************************************************************************/
TResponseInfo * PandoraTest::SaveDocItemProductOutServiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemService * item = NULL;
	bool result;

	try
	{

		item = new TDocItemService();
		item->FromJson(req->info->data);

		result = db->SaveDocItemService(item, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemService succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveDocItemProductOutService  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemProductOutServiceRequest(request * req)

{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemService(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemService succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemProductOutService  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemProductOutServiceRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemService * item = NULL;
	int id;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		item = db->GetDocItemService(id);

		if(item != NULL)
		{
			resp->data = item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemService succesfull.";
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemProductOutService  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}

/************************************************************************/
/* DOC RETURN  OUT                                                      */
/************************************************************************/

TResponseInfo * PandoraTest::AddReturnOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReturnOutDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		//		db->CheckPermission(req->user_id, 3523, "Add");

		doc = new TReturnOutDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddDocReturnOut(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocReturnOut succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddReturnOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddReturnOutDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::SaveReturnOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReturnOutDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TReturnOutDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocReturnOut(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocReturnOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveReturnOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveReturnOutDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveReturnOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		//		db->CheckPermission(req->user_id, 3523, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocReturnOut(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveReturnOutDocument succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveReturnOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveReturnOutDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetReturnOutDocumentRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReturnOutDoc * doc = NULL;
	int id = 0;

	try
	{
		//		db->CheckPermission(req->user_id, 3523, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocReturnOut(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocReturnOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReturnOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReturnOutDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetReturnOutDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReturnOutDocView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{
		//		db->CheckPermission(req->user_id, 3523, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocReturnOutView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocReturnOutView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReturnOutDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReturnOutDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetReturnOutDocumentsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TReturnOutDoc> * docs = NULL;

	try
	{
		//		db->CheckPermission(req->user_id, 3523, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_SUPPLIER_RETURNS);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsReturnOut(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsReturnOut operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReturnOutDocuments  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReturnOutDocuments  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::GetReturnOutDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TReturnOutDocView> * docs = NULL;

	try
	{
		//		db->CheckPermission(req->user_id, 3521, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_SUPPLIER_RETURNS);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsReturnOutView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsReturnOutView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReturnOutDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReturnOutDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::PostReturnOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		//		db->CheckPermission(req->user_id, 3521, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocReturnOutCron(doc_id, req->user_id);
		else
			result = db->PostDocReturnOut(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocReturnOut succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostReturnOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostReturnOutDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostReturnOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		//		db->CheckPermission(req->user_id, 3521, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnpostDocReturnOutCron(doc_id, req->user_id);
		else
			result = db->UnpostDocReturnOut(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnpostDocReturnOut succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostReturnOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostReturnOutDocument  error" , "");

	}

	return resp;
}
/************************************************************************/
/* DOC ITEM RETURN OUT                                                  */
/************************************************************************/
TResponseInfo * PandoraTest::AddDocItemReturnOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemReturnOut * item = NULL;
	bool result = false;
	int cat_id = 0;
	int user_id = 0;

	try
	{
		item = new TDocItemReturnOut();
		item->FromJson(req->info->data);
		item->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		user_id = req->user_id;
		result = db->AddDocItemReturnOut(item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocItemReturnOut succesfull.";
			resp->data = TUtility::ToJson(item->id, "id");
			SAFEDELETE(item)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddDocItemReturnOut  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}
TResponseInfo * PandoraTest::SaveDocItemReturnOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemReturnOut * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemReturnOut();
		doc_item->FromJson(req->info->data);
		user_id = req->user_id;

		TUtility::CheckZeroValue(5, true,  doc_item->price, doc_item->price->value,
			doc_item->price->total, doc_item->price->unit, doc_item->quantity);
		TUtility::CheckZeroValue(7, true,  doc_item->price->total->measure_id, doc_item->price->value->measure_id,
			doc_item->warehouse_id, doc_item->product_id, doc_item->owner_id, doc_item->id, doc_item->type_id, req->user_id);

		result = db->SaveDocItemReturnOut(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemReturnOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveDocItemReturnOut  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemReturnOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemReturnOut(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemReturnOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemReturnOut  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemReturnOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemReturnOut * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemReturnOut(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemReturnOut succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemReturnOut  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::GetDocItemsReturnOutViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemReturnOutView> * doc_items_view = NULL;
	int lang_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_items_view = db->GetDocItemsReturnOutView(params);

		if(doc_items_view != NULL)
		{
			resp->data = doc_items_view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsReturnOutView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetDocItemsReturnOutView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_items_view)
		return resp;
}




TResponseInfo * PandoraTest::ReserveDocItemReturnOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int item_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		item_id = TUtility::wstoi(params[L"item_id"]);
		TUtility::CheckZeroValue(1, true, item_id);

		result = db->ReserveDocItemReturnOut(item_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ReserveDocItemReturnOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "ReserveDocItemReturnOut  error" , "");

	}


	return resp;
}

TResponseInfo * PandoraTest::UnReserveDocItemReturnOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int item_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		item_id = TUtility::wstoi(params[L"item_id"]);
		TUtility::CheckZeroValue(1, true, item_id);

		result = db->UnReserveDocItemReturnOut(item_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ReserveDocItemReturnOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "UnReserveDocItemReturnOut  error" , "");

	}


	return resp;
}

/************************************************************************/
/* DOC INVOICE                                                          */
/************************************************************************/
TResponseInfo * PandoraTest::AddInvoiceDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInvoiceDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3508, "Add");

		doc = new TInvoiceDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddDocInvoice(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocInvoice succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddInvoiceDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddInvoiceDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::SaveInvoiceDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInvoiceDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TInvoiceDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocInvoice(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocInvoice succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveInvoiceDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveInvoiceDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveInvoiceDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3508, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);

		result = db->RemoveDocInvoice(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocInvoice succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveInvoiceDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveInvoiceDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetInvoiceDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInvoiceDoc * doc = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3508, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocInvoice(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocInvoice succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInvoiceDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInvoiceDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetInvoiceDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInvoiceDocView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{

		db->CheckPermission(req->user_id, 3508, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocInvoiceView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocInvoiceView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInvoiceDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInvoiceDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetInvoiceDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TInvoiceDocView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3508, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_INVOICE);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsInvoiceView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsInvoiceView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInvoiceDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInvoiceDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::PostInvoiceDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3508, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocInvoiceCron(doc_id, req->user_id);
		else
			result = db->PostDocInvoice(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocInvoice succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostInvoiceDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostInvoiceDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostInvoiceDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3508, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnpostDocInvoiceCron(doc_id, req->user_id);
		else
			result = db->UnpostDocInvoice(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnpostDocInvoice succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostInvoiceDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostInvoiceDocument  error" , "");

	}

	return resp;
}
/************************************************************************/
/* DOC ITEMS INVOICE                                                   */
/************************************************************************/
TResponseInfo * PandoraTest::AddDocItemInvoiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemInvoice * item = NULL;
	bool result = false;
	int cat_id = 0;
	int user_id = 0;

	try
	{
		item = new TDocItemInvoice();
		item->FromJson(req->info->data);
		item->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  atoi(TUtility::UnicodeToUtf8(params[L"cat_id"]).c_str());

		if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		user_id = req->user_id;
		result = db->AddDocItemInvoice(item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocItemInvoice succesfull.";
			resp->data = TUtility::ToJson(item->id, "id");
			SAFEDELETE(item)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddDocItemInvoice  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}
TResponseInfo * PandoraTest::SaveDocItemInvoiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemInvoice * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemInvoice();
		doc_item->FromJson(req->info->data);
		user_id = req->user_id;

		TUtility::CheckZeroValue(5, true,  doc_item->price, doc_item->price->value,
			doc_item->price->total, doc_item->price->unit, doc_item->quantity);
		TUtility::CheckZeroValue(7, true,  doc_item->price->total->measure_id, doc_item->price->value->measure_id,
			doc_item->warehouse_id, doc_item->product_id, doc_item->owner_id, doc_item->id, doc_item->type_id, req->user_id);

		result = db->SaveDocItemInvoice(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemInvoice succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveDocItemInvoice  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemInvoiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemInvoice(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemInvoice succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemInvoice  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemInvoiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemInvoice * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemInvoice(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemInvoice succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemInvoice  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::GetDocItemsInvoiceViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemInvoiceView> * doc_items_view = NULL;
	int lang_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_items_view = db->GetDocItemsInvoiceView(params);

		if(doc_items_view != NULL)
		{
			resp->data = doc_items_view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsInvoiceView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetDocItemsInvoiceView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_items_view)
		return resp;
}




TResponseInfo * PandoraTest::ReserveDocItemInvoiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int item_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		item_id =  TUtility::wstoi(params[L"item_id"]);
		TUtility::CheckZeroValue(1, true, item_id);

		result = db->ReserveDocItemInvoice(item_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ReserveDocItemInvoice succesfull.";
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "ReserveDocItemInvoice  error" , "");

	}


	return resp;
}

TResponseInfo * PandoraTest::UnReserveDocItemInvoiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int item_id = 0;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		item_id = TUtility::wstoi(params[L"item_id"]);
		TUtility::CheckZeroValue(1, true, item_id);

		result = db->UnReserveDocItemInvoice(item_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ReserveDocItemInvoice succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "UnReserveDocItemInvoice  error" , "");

	}

	return resp;
}

/************************************************************************/
/* DOC ORDER IN                                                          */
/************************************************************************/
TResponseInfo * PandoraTest::AddOrderInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocOrderIn * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3524, "Add");

		doc = new TDocOrderIn();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddDocOrderIn(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocOrderIn succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddOrderInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddOrderInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::SaveOrderInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocOrderIn * doc = NULL;
	bool result = false;

	try
	{
		doc = new TDocOrderIn();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocOrderIn(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocOrderIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveOrderInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveOrderInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveOrderInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3524, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);

		result = db->RemoveDocOrderIn(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocOrderIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveOrderInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveOrderInDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetOrderInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocOrderIn * doc = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3524, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocOrderIn(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocOrderIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOrderInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOrderInDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetOrderInDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocOrderInView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{

		db->CheckPermission(req->user_id, 3524, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocOrderInView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocOrderInView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOrderInDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOrderInDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetOrderInDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocOrderInView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3524, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_ORDER_IN);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsOrderInView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsOrderInView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOrderInDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOrderInDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::PostOrderInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3524, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocOrderInCron(doc_id, req->user_id);
		else
			result = db->PostDocOrderIn(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocOrderIn succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostInvoiceDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostOrderInDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostOrderInDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3524, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnpostDocOrderInCron(doc_id, req->user_id);
		else
			result = db->UnpostDocOrderIn(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnpostDocOrderIn succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostOrderInDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostOrderInDocument  error" , "");

	}

	return resp;
}
/************************************************************************/
/* DOC ITEMS ORDER IN                                                   */
/************************************************************************/
TResponseInfo * PandoraTest::AddDocItemOrderInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemOrderIn * item = NULL;
	bool result = false;
	int cat_id = 0;
	int user_id = 0;

	try
	{
		item = new TDocItemOrderIn();
		item->FromJson(req->info->data);
		item->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		user_id = req->user_id;
		result = db->AddDocItemOrderIn(item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocItemOrderIn succesfull.";
			resp->data = TUtility::ToJson(item->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddDocItemOrderIn  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}
TResponseInfo * PandoraTest::SaveDocItemOrderInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemOrderIn * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemOrderIn();
		doc_item->FromJson(req->info->data);
		user_id = req->user_id;

		TUtility::CheckZeroValue(5, true,  doc_item->price, doc_item->price->value,
			doc_item->price->total, doc_item->price->unit, doc_item->quantity);
		TUtility::CheckZeroValue(7, true,  doc_item->price->total->measure_id, doc_item->price->value->measure_id,
			doc_item->warehouse_id, doc_item->product_id, doc_item->owner_id, doc_item->id, doc_item->type_id, req->user_id);

		result = db->SaveDocItemOrderIn(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemOrderIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveDocItemOrderIn  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemOrderInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemOrderIn(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemOrderIn succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemOrderIn  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemOrderInRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemOrderIn * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemOrderIn(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemOrderIn succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemOrderIn  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::GetDocItemsOrderInViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemOrderInView> * doc_items_view = NULL;
	int lang_id = 0;
	stringstream ss;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_items_view = db->GetDocItemsOrderInView(params);

		if(doc_items_view != NULL)
		{
			resp->data = doc_items_view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsOrderInView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetDocItemsOrderInView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_items_view)
		return resp;
}



/************************************************************************/
/* DOC ORDER OUT                                                          */
/************************************************************************/
TResponseInfo * PandoraTest::AddOrderOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocOrderOut * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3525, "Add");

		doc = new TDocOrderOut();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddDocOrderOut(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocOrderOut succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddOrderOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddOrderOutDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}


TResponseInfo * PandoraTest::SaveOrderOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocOrderOut * doc = NULL;
	bool result = false;

	try
	{
		doc = new TDocOrderOut();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocOrderOut(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocOrderOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveOrderOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveOrderOutDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveOrderOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3525, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);

		result = db->RemoveDocOrderOut(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocOrderOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveOrderOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveOrderOutDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetOrderOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocOrderOut * doc = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3525, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocOrderOut(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocOrderOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOrderOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOrderOutDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetOrderOutDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocOrderOutView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{

		db->CheckPermission(req->user_id, 3525, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocOrderOutView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocOrderOutView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOrderOutDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOrderOutDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetOrderOutDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocOrderOutView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3525, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_ORDER_OUT);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsOrderOutView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsOrderOutView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetOrderOutDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetOrderOutDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


TResponseInfo * PandoraTest::PostOrderOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3525, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->PostDocOrderOutCron(doc_id, req->user_id);
		else
			result = db->PostDocOrderOut(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "PostDocOrderOut succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "PostInvoiceDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "PostOrderOutDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::UnpostOrderOutDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3525, "Post");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id =  TUtility::wstoi(params[L"doc_id"]);
		TUtility::CheckZeroValue(1, true, doc_id);

		if(TUtility::StringToBool(config->content["post_with_cron"].c_str()) && !req->cron)
			result = db->UnpostDocOrderOutCron(doc_id, req->user_id);
		else
			result = db->UnpostDocOrderOut(doc_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "UnpostDocOrderOut succesfull.";
			CronResume();
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "UnpostOrderOutDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "UnpostOrderOutDocument  error" , "");

	}

	return resp;
}
/************************************************************************/
/* DOC ITEMS ORDER OUT                                                  */
/************************************************************************/
TResponseInfo * PandoraTest::AddDocItemOrderOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemOrderOut * item = NULL;
	bool result = false;
	int cat_id = 0;
	int user_id = 0;

	try
	{
		item = new TDocItemOrderOut();
		item->FromJson(req->info->data);
		item->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		user_id = req->user_id;
		result = db->AddDocItemOrderOut(item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocItemOrderOut succesfull.";
			resp->data = TUtility::ToJson(item->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddDocItemOrderOut  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}
TResponseInfo * PandoraTest::SaveDocItemOrderOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemOrderOut * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemOrderOut();
		doc_item->FromJson(req->info->data);
		user_id = req->user_id;

		TUtility::CheckZeroValue(5, true,  doc_item->price, doc_item->price->value,
			doc_item->price->total, doc_item->price->unit, doc_item->quantity);
		TUtility::CheckZeroValue(7, true,  doc_item->price->total->measure_id, doc_item->price->value->measure_id,
			doc_item->warehouse_id, doc_item->product_id, doc_item->owner_id, doc_item->id, doc_item->type_id, req->user_id);

		result = db->SaveDocItemOrderOut(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemOrderOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveDocItemOrderOut  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::RemoveDocItemOrderOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemOrderOut(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemOrderOut succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemOrderOut  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocItemOrderOutRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemOrderOut * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemOrderOut(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemOrderOut succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemOrderOut  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}

TResponseInfo * PandoraTest::GetDocItemsOrderOutViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemOrderOutView> * doc_items_view = NULL;
	int lang_id = 0;
	stringstream ss;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_items_view = db->GetDocItemsOrderOutView(params);

		if(doc_items_view != NULL)
		{
			resp->data = doc_items_view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsOrderOutView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetDocItemsOrderOutView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_items_view)
		return resp;
}



/************************************************************************/
/* DOC INVOICE  FACTURA                                                 */
/************************************************************************/
TResponseInfo * PandoraTest::AddInvoiceFacturaDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInvoiceFacturaDoc * doc = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3509, "Add");

		doc = new TInvoiceFacturaDoc();
		doc->FromJson(req->info->data);
		doc->id = db->GetObjectID();
		doc->user_id = req->user_id;

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id =  TUtility::wstoi(params[L"cat_id"]);

		result = db->AddDocInvoiceFactura(doc);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocInvoiceFactura succesfull.";
			resp->data = TUtility::ToJson(doc->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddInvoiceFacturaDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddInvoiceFacturaDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}
TResponseInfo * PandoraTest::SaveInvoiceFacturaDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInvoiceFacturaDoc * doc = NULL;
	bool result = false;

	try
	{
		doc = new TInvoiceFacturaDoc();
		doc->FromJson(req->info->data);
		doc->user_id = req->user_id;

		result = db->SaveDocInvoiceFactura(doc, doc->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocInvoiceFactura succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveInvoiceFacturaDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveInvoiceFacturaDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::RemoveInvoiceFacturaDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3509, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocInvoiceFactura(id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocInvoiceFactura succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveInvoiceFacturaDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveInvoiceFacturaDocument  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetInvoiceFacturaDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInvoiceFacturaDoc * doc = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3509, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocInvoiceFactura(id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocInvoice succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInvoiceFacturaDocument  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInvoiceFacturaDocument  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetInvoiceFacturaDocumentViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TInvoiceFacturaDocView * doc = NULL;
	int id = 0, lang_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3509, "View");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		lang_id = req->info->language;
		TUtility::CheckZeroValue(1, true, id);

		doc = db->GetDocInvoiceFacturaView(id, lang_id);

		if(doc != NULL)
		{
			resp->data = doc->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocInvoiceFacturaView succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInvoiceFacturaDocumentView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInvoiceFacturaDocumentView  error" , "");

	}

	SAFEDELETE(doc)
		return resp;
}

TResponseInfo * PandoraTest::GetInvoiceFacturaDocumentsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TInvoiceFacturaDocView> * docs = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3509, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"type_id"] = TUtility::itows(DOC_WAY_BILL);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		docs = db->GetDocsInvoiceFacturaView(params);

		if(docs != NULL)
		{
			resp->data = docs->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocsInvoiceFacturaView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetInvoiceFacturaDocumentsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetInvoiceFacturaDocumentsView  error" , "");

	}

	SAFEDELETE(docs)
		return resp;
}


/************************************************************************/
/* DOC ITEMS INVOICE  FACTURA                                           */
/************************************************************************/
TResponseInfo * PandoraTest::AddDocItemInvoiceFacturaRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemInvoiceFactura * item = NULL;
	bool result = false;
	int cat_id = 0;
	int user_id = 0;

	try
	{
		item = new TDocItemInvoiceFactura();
		item->FromJson(req->info->data);
		item->id = db->GetObjectID();

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		if (item->price->total->measure_id == 0 || item->price->value->measure_id == 0)
		{
			resp->succesfull = 0;
			resp->message = "no currency id";
			return resp;
		}

		user_id = req->user_id;
		result = db->AddDocItemInvoiceFactura(item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddDocItemInvoiceFactura succesfull.";
			resp->data = TUtility::ToJson(item->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "AddDocItemInvoiceFactura  error" , "");

	}

	SAFEDELETE(item)
		return resp;
}
TResponseInfo * PandoraTest::SaveDocItemInvoiceFacturaRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemInvoiceFactura * doc_item = NULL;
	bool result = false;
	int user_id = 0;

	try
	{

		doc_item = new TDocItemInvoiceFactura();
		doc_item->FromJson(req->info->data);
		user_id = req->user_id;

		TUtility::CheckZeroValue(5, true,  doc_item->price, doc_item->price->value,
			doc_item->price->total, doc_item->price->unit, doc_item->quantity);
		TUtility::CheckZeroValue(7, true,  doc_item->price->total->measure_id, doc_item->price->value->measure_id,
			doc_item->warehouse_id, doc_item->product_id, doc_item->owner_id, doc_item->id, doc_item->type_id, req->user_id);

		result = db->SaveDocItemInvoiceFactura(doc_item, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveDocItemInvoiceFactura succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveDocItemInvoiceFactura  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}
TResponseInfo * PandoraTest::RemoveDocItemInvoiceFacturaRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;
	int user_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		user_id = req->user_id;
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveDocItemInvoiceFactura(id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveDocItemInvoiceFactura succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveDocItemInvoiceFactura  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetDocItemInvoiceFacturaRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemInvoiceFactura * doc_item = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		doc_item = db->GetDocItemInvoiceFactura(id);

		if(doc_item != NULL)
		{
			resp->data = doc_item->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemInvoiceFactura succesfull.";
		}

		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocItemInvoiceFactura  error" , "");

	}

	SAFEDELETE(doc_item)
		return resp;
}
TResponseInfo * PandoraTest::GetDocItemsInvoiceFacturaViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TDocItemInvoiceFacturaView> * doc_items_view = NULL;
	int lang_id = 0;
	stringstream ss;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_items_view = db->GetDocItemsInvoiceFacturaView(params);

		if(doc_items_view != NULL)
		{
			resp->data = doc_items_view->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetDocItemsInvoiceFacturaView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetDocItemsInvoiceFacturaView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_items_view)
		return resp;
}

/************************************************************************/
/* PRICE CATEGORY                                                       */
/************************************************************************/
TResponseInfo * PandoraTest::SavePriceCategoryRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPriceCategory * category = NULL;
	bool result = false;

	try
	{
		category = new TPriceCategory();
		category->FromJson(req->info->data);
		result = db->SavePriceCategory(category, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SavePriceCategory succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "SavePriceCategory  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SavePriceCategory  error" , "");

	}

	SAFEDELETE(category)
		return resp;
}
TResponseInfo * PandoraTest::AddPriceCategoryRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPriceCategory * category = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3553, "Add");

		category = new TPriceCategory();
		category->FromJson(req->info->data);
		category->id = db->GetObjectID();

		result = db->AddPriceCategory(category, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddPriceCategory succesfull.";
			resp->data = TUtility::ToJson(category->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddPriceCategory  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddPriceCategory  error" , "");

	}

	SAFEDELETE(category)
		return resp;
}
TResponseInfo * PandoraTest::RemovePriceCategoryRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3553, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemovePriceCategory(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemovePriceCategory succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemovePriceCategory  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemovePriceCategory  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetPriceCategoryRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPriceCategory * category = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3553, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		category = db->GetPriceCategory(id, req->user_id);

		if(category != NULL)
		{
			resp->data = category->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPriceCategory succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPriceCategory  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPriceCategory  error" , "");

	}

	SAFEDELETE(category)
		return resp;
}
TResponseInfo * PandoraTest::GetPriceCategoriesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPriceCategory> * categories = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3553, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"user_id"] = TUtility::itows(req->user_id);

		categories = db->GetPriceCategories(params);

		if(categories != NULL)
		{
			resp->data = categories->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPriceCategories operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPriceCategories  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPriceCategories  error" , "");

	}

	SAFEDELETE(categories)
		return resp;
}
TResponseInfo * PandoraTest::AddCustomPriceCategoriesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TPriceCategory> * values = NULL;
	bool result = false;
	int object_id = 0;

	try
	{

		values = new TCollection<TPriceCategory>("TPriceCategory");
		values->FromJson(req->info->data);

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		object_id =  TUtility::wstoi(params[L"object_id"]);
		result = db->AddCustomPriceCategories(values, object_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddCustomPriceCategories succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPriceCategoriesView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPriceCategoriesView  error" , "");

	}

	SAFEDELETE(values)
		return resp;
}
TResponseInfo * PandoraTest::GetPriceCategoriesViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPriceCategoryView> * categories = NULL;

	try
	{
		db->CheckPermission(req->user_id, 3553, "View");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		params[L"user_id"] = TUtility::itows(req->user_id);

		categories = db->GetPriceCategoriesView(params);

		if(categories != NULL)
		{
			resp->data = categories->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPriceCategoriesView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPriceCategoriesView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPriceCategoriesView  error" , "");

	}

	SAFEDELETE(categories)
		return resp;
}
TResponseInfo * PandoraTest::CalculatePriceCategoriesViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPriceCategoryView> * categories = NULL;
	TCollection<TLot> * lots = NULL;
	int prod_id = 0, partner_id = 0, currency_id  = 0, measure_id = 0, date = 0, owner_id = 0;


	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		lots = new TCollection<TLot>("TLot");

		lots->FromJson(req->info->data);

		owner_id = TUtility::wstoi(params[L"owner_id"]);
		prod_id =  TUtility::wstoi(params[L"prod_id"]);
		partner_id =  TUtility::wstoi(params[L"bill_to"]);
		currency_id =  TUtility::wstoi(params[L"currency_id"]);
		measure_id =  TUtility::wstoi(params[L"measure_id"]);
		date =  TUtility::wstoi(params[L"date"]);

		categories = db->CalculatePriceCategoriesView(owner_id, lots, prod_id, partner_id, currency_id, measure_id, date);

		if(categories != NULL)
		{
			resp->data = categories->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPriceCategories operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "CalculatePriceCategoriesView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "CalculatePriceCategoriesView  error" , "");

	}

	SAFEDELETE(lots)
		SAFEDELETE(categories)
		return resp;
}


/************************************************************************/
/* PRODUCT SUMMARIES                                                    */
/************************************************************************/
TResponseInfo * PandoraTest::GetProductSummariesViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductSummaryView> * views = NULL;
	int prod_id = 0, date = 0, lang_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		prod_id = TUtility::wstoi(params[L"prod_id"]);
		lang_id = req->info->language;
		date = TUtility::wstoi(params[L"date"]);
		if (date == 0)
			date = time(0);

		TUtility::CheckZeroValue(1, true, prod_id);
		views = db->GetProductSummariesView(prod_id, date, lang_id);

		if(views != NULL)
		{
			resp->data = views->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductSummariesView operation succesfull!";
		}
		else

		{
			throw 0;

		}


	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductSummariesView  error" , "");

	}

	SAFEDELETE(views)
		return resp;
}
TResponseInfo * PandoraTest::GetProductSummariesSubsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TProductSummaryView> * views = NULL;
	int prod_id = 0, date = 0, lang_id = 0, warehouse_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		prod_id = TUtility::wstoi(params[L"prod_id"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		lang_id = req->info->language;
		date = TUtility::wstoi(params[L"date"]);
		if (date == 0)
			date = time(0);

		TUtility::CheckZeroValue(1, true, prod_id);

		views = db->GetProductSummariesSubsView(prod_id, warehouse_id, date, lang_id);

		if(views != NULL)
		{
			resp->data = views->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetProductSummariesSubsView operation succesfull!";
		}
		else
		{
			throw 0;

		}


	}

	catch (...)
	{
		ProcessError(resp, "", "GetProductSummariesSubsView  error" , "");

	}

	SAFEDELETE(views)
		return resp;
}

/************************************************************************/
/* SETTINGS                                                             */
/************************************************************************/

TResponseInfo * PandoraTest::GetSettingsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	wstring param_name;
	wstring settings;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		param_name = params[L"param_name"];
		settings = db->GetSettings(req->user_id, param_name, 0);

		if(settings != L"")
		{
			resp->data = TUtility::UnicodeToUtf8(settings);
			resp->succesfull = 1;
			resp->message = "GetSettings succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetSettings  error" , "", false);

	}

	return resp;
}

TResponseInfo * PandoraTest::SetSettingsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	wstring param_name, value;
	bool result;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		param_name =  params[L"param_name"];
		value =  params[L"value"];
		result = db->SetSettings(req->user_id, param_name, value);

		if(result != false)
		{
			resp->succesfull = 1;
			resp->message = "GetSettings succesfull.";
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SetSettings  error" , "", false);

	}


	return resp;
}
/************************************************************************/
/* CATS_OBJECTS                                                         */
/************************************************************************/

TResponseInfo * PandoraTest::GetCategoriesByObjectRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	vector<int> ids;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		ids = db->GetCategoriesByObject(id);

		resp->data = TUtility::ToJson(ids, "cats");
		resp->succesfull = 1;
		resp->message = "GetCategoriesByObject succesfull.";


	}

	catch (...)
	{
		ProcessError(resp, "", "GetCategoriesByObject  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::SaveCategoriesByObjectRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	vector<int> ids;
	bool result = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		ids = ParseVectorIds(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->SaveCategoriesByObject(ids, id);

		if(result != false)
		{
			resp->succesfull = 1;
			resp->message = "SaveCategoriesByObject succesfull.";
		}


		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "SaveCategoriesByObject  error" , "");

	}


	return resp;
}
/************************************************************************/
/* GET OBJECT ID                                                        */
/************************************************************************/
TResponseInfo * PandoraTest::GetObjectIdRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;

	try
	{
		id = db->GetObjectID();

		if(id != 0)
		{
			resp->data = TUtility::ToJson(id, "id");
			resp->succesfull = 1;
			resp->message = "GetObjectIdRequest succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetObjectId  error" , "");

	}

	return resp;
}

/************************************************************************/
/* GET CALCULATE LOTS                                                   */
/************************************************************************/
TResponseInfo * PandoraTest::GetCalculateLotsRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TDocItemWriteOff * doc_item = NULL;
	TPriceView * price = NULL;
	int doc_date = 0;
	try
	{

		doc_item = new TDocItemWriteOff();
		doc_item->FromJson(req->info->data);
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		doc_date = TUtility::wstoi(params[L"doc_date"]);

		price = db->CalculateLotsTotal(doc_item->product_id, doc_item->warehouse_id, doc_item->lots, doc_item->quantity, doc_date);

		if(price != NULL)
		{
			resp->data = price->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "CalculateLotsTotal operation succesfull!";
		}
		else
		{
			throw 0;

		}


	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetCalculateLots  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(doc_item)
		SAFEDELETE(price)
		return resp;
}


TResponseInfo * PandoraTest::RecalculateProductsPriceRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int price_cat_id = 0;
	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		price_cat_id = TUtility::wstoi(params[L"price_cat_id"]);
		TUtility::CheckZeroValue(1, true, price_cat_id);

		result = db->RecalculateProductsPrice(price_cat_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RecalculateProductsPriceRequest operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "RecalculateProductsPrice  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}


/************************************************************************/
/*  DUPLICATES                                                          */
/************************************************************************/
TResponseInfo * PandoraTest::DuplicateProductRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int prod_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		prod_id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, prod_id);

		result = db->DuplicateProduct(prod_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "DuplicateProduct succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "DuplicateProduct  error" , "");

	}

	return resp;
}



TResponseInfo * PandoraTest::DuplicatePartnerRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int partner_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		partner_id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, partner_id);

		result = db->DuplicatePartner(partner_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "DuplicatePartner succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "DuplicatePartner  error" , "");

	}

	return resp;
}



TResponseInfo * PandoraTest::DuplicateDocumentRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	int document_id = 0, user_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		document_id = TUtility::wstoi(params[L"id"]);
		user_id = req->info->language;
		TUtility::CheckZeroValue(1, true, document_id);

		result = db->DuplicateDocument(document_id, user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "DuplicateDocument succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "DuplicateDocument  error" , "");

	}

	return resp;
}



/************************************************************************/
/* WAREHOUSE REPORTS                                                   */
/************************************************************************/
TResponseInfo * PandoraTest::GetReportShortIncomeRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int lang_id = 0, from_date = 0, to_date = 0, warehouse_id = 0, cat_id = 0, partner_id = 0, measure_id = 0, currency_id = 0;
	vector<int> product_types;
	string sid = "";

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = req->info->language;
		from_date = TUtility::wstoi(params[L"from_date"]);
		to_date = TUtility::wstoi(params[L"to_date"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		partner_id = TUtility::wstoi(params[L"partner_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		currency_id = TUtility::wstoi(params[L"currency_id"]);

		sid = db->GetReportShortIncome(lang_id, from_date, to_date, warehouse_id, cat_id, partner_id, measure_id, currency_id, product_types, params);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetReportShortIncome succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportShortIncome  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReportShortIncome()  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetReportDetailedIncomeRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int lang_id = 0, from_date = 0, to_date = 0, warehouse_id = 0, cat_id = 0, partner_id = 0, measure_id = 0, currency_id = 0;
	int price_cat_id = 0;
	vector<int> product_types;
	string sid = "";

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		from_date = TUtility::wstoi(params[L"from_date"]);
		to_date = TUtility::wstoi(params[L"to_date"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		partner_id = TUtility::wstoi(params[L"partner_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		currency_id = TUtility::wstoi(params[L"currency_id"]);
		price_cat_id = TUtility::wstoi(params[L"price_cat_id"]);

		sid = db->GetReportDetailedIncome(lang_id, from_date, to_date, warehouse_id, cat_id, price_cat_id, partner_id, measure_id, currency_id, product_types, params);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetReportDetailedIncome succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportDetailedIncome  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReportDetailedIncome()  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetReportShortSalesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int lang_id = 0, from_date = 0, to_date = 0, warehouse_id = 0, cat_id = 0, partner_id = 0, measure_id = 0, currency_id = 0;
	vector<int> product_types;
	string sid = "";

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		from_date = TUtility::wstoi(params[L"from_date"]);
		to_date = TUtility::wstoi(params[L"to_date"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		partner_id = TUtility::wstoi(params[L"partner_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		currency_id = TUtility::wstoi(params[L"currency_id"]);
		partner_id = TUtility::wstoi(params[L"partner_id"]);

		sid = db->GetReportShortSales(lang_id, from_date, to_date, warehouse_id, cat_id, partner_id, measure_id, currency_id, product_types, params);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetReportShortSales succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportShortSales  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReportShortSales()  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetReportBalancesAtWarehouseRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int lang_id = 0, date = 0, warehouse_id = 0, cat_id = 0, measure_id = 0, currency_id = 0;
	vector<int> product_types;
	string sid = "";

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		date = TUtility::wstoi(params[L"date"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		currency_id = TUtility::wstoi(params[L"currency_id"]);

		sid = db->GetReportBalancesAtWarehouse(lang_id, date, warehouse_id, cat_id, measure_id, currency_id, product_types, params);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetReportBalancesAtWarehouse succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportBalancesAtWarehouse  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReportBalancesAtWarehouse()  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetReportProductMoveRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int lang_id = 0, from_date = 0, to_date = 0, warehouse_id = 0, cat_id = 0, measure_id = 0, currency_id = 0, prod_id = 0;
	vector<int> product_types;
	string sid = "";

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = req->info->language;
		from_date = TUtility::wstoi(params[L"from_date"]);
		to_date = TUtility::wstoi(params[L"to_date"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		currency_id = TUtility::wstoi(params[L"currency_id"]);
		prod_id = TUtility::wstoi(params[L"prod_id"]);

		sid = db->GetReportProductMove(lang_id, from_date, to_date, warehouse_id, prod_id, cat_id, measure_id, currency_id, product_types);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetReportProductMove succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportProductMove  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReportProductMove  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetReportTurnoverRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int lang_id = 0, from_date = 0, to_date = 0, warehouse_id = 0, cat_id = 0, measure_id = 0, currency_id = 0;
	vector<int> product_types;
	string sid = "";

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = req->info->language;
		from_date = TUtility::wstoi(params[L"from_date"]);
		to_date = TUtility::wstoi(params[L"to_date"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		currency_id = TUtility::wstoi(params[L"currency_id"]);

		sid = db->GetReportTurnover(lang_id, from_date, to_date, warehouse_id, cat_id, measure_id, currency_id, product_types, params);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetReportTurnover succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportTurnover  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReportTurnover  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetReportProductTurnoverRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int lang_id = 0, from_date = 0, to_date = 0, warehouse_id = 0, cat_id = 0, measure_id = 0, currency_id = 0;
	vector<int> product_types;
	string sid = "";

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = req->info->language;
		from_date = TUtility::wstoi(params[L"from_date"]);
		to_date = TUtility::wstoi(params[L"to_date"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		currency_id = TUtility::wstoi(params[L"currency_id"]);

		sid = db->GetReportProductTurnover(lang_id, from_date, to_date, warehouse_id, cat_id, measure_id, currency_id);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetReportProductTurnover succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportProductTurnover  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReportProductTurnover  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetReportMinProductBalanceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int lang_id = 0, from_date = 0, date = 0, warehouse_id = 0, cat_id = 0, measure_id = 0, currency_id = 0;
	vector<int> product_types;
	string sid = "";

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = req->info->language;
		date = TUtility::wstoi(params[L"date"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		currency_id = TUtility::wstoi(params[L"currency_id"]);

		sid = db->GetReportMinProductBalance(lang_id, date, warehouse_id, cat_id, measure_id, currency_id, product_types, params);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetReportMinProductBalance succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportMinProductBalance  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReportMinProductBalance  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetReportSalesWithProfitRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int lang_id = 0, from_date = 0, to_date = 0, warehouse_id = 0, cat_id = 0, measure_id = 0, currency_id = 0, manager_id = 0;
	vector<int> product_types;
	string sid = "";

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		lang_id = req->info->language;
		from_date = TUtility::wstoi(params[L"from_date"]);
		to_date = TUtility::wstoi(params[L"to_date"]);
		warehouse_id = TUtility::wstoi(params[L"warehouse_id"]);
		cat_id = TUtility::wstoi(params[L"cat_id"]);
		measure_id = TUtility::wstoi(params[L"measure_id"]);
		currency_id = TUtility::wstoi(params[L"currency_id"]);

		sid = db->GetReportSalesWithProfit(lang_id, from_date, to_date, warehouse_id, cat_id, measure_id, currency_id, product_types, params);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetReportSalesWithProfit succesfull.";
		}
		else
		{
			throw 0;

		}
	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportSalesWithProfit  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetReportSalesWithProfit()  error" , "");

	}

	return resp;
}
/************************************************************************/
/*  REPORTS TEMPLATE                                                    */
/************************************************************************/
TResponseInfo * PandoraTest::AddReportTemplateRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	map<wstring, wstring> params;
	TReportTemplate * rep = NULL;
	bool result = false;

	try
	{
		rep = new TReportTemplate();

		rep->FromJson(req->info->data);
		if (rep->id == 0)
			rep->id = db->GetObjectID();

		result = db->AddRepTemplate(rep);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddRepTemplate succesfull.";
			resp->data = TUtility::ToJson(rep->id, "id");
			SAFEDELETE(rep)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddReportTemplate  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}
	SAFEDELETE(rep)
		return resp;
}


TResponseInfo * PandoraTest::SaveReportTemplateRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReportTemplate * rep = NULL;
	bool result = false;

	try
	{

		rep = new TReportTemplate();

		rep->FromJson(req->info->data);

		result = db->SaveRepTemplate(rep);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveRepTemplate succesfull.";
			SAFEDELETE(rep)
				return resp;
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "SaveReportTemplate  error" , "");

	}

	SAFEDELETE(rep)
		return resp;
}

TResponseInfo * PandoraTest::RemoveReportTemplateRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id = TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveRepTemplate(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveRepTemplate succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveReportTemplate  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetReportTemplateRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TReportTemplate * rep = NULL;
	int id = 0;
	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		rep = db->GetRepTemplate(id, req->info->language);

		if(rep != NULL)
		{
			resp->data = rep->Serialize(1, "", req->info->response_type, false);
			resp->succesfull = 1;
			resp->message = "GetRepTemplate succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetReportTemplate  error" , "");

	}

	SAFEDELETE(rep)
		return resp;
}

TResponseInfo * PandoraTest::GetReportTemplatesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TReportTemplate> * templs = NULL;
	int lang_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		templs = db->GetRepTemplates(params);

		if(templs)
		{
			resp->data = templs->Serialize(1, "", req->info->response_type, false );
			resp->succesfull = 1;
			resp->message = "GetRepTemplates operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportTemplates  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}
	SAFEDELETE(templs)
		return resp;
}




/************************************************************************/
/* REPORTS                                                              */
/************************************************************************/
TResponseInfo * PandoraTest::GetReportsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TReport> * reps = NULL;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		reps = db->GetReports(params);

		if(reps != NULL)
		{
			resp->data = reps->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetReports succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetReports  error" , "");

	}

	SAFEDELETE(reps)
		return resp;
}

TResponseInfo * PandoraTest::GetReportDataSourcesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TReportDataSource> * reps = NULL;
	int cat_id = 0;
	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		cat_id = TUtility::wstoi(params[L"cat_id"]);

		reps = db->GetReportDataSources(cat_id);

		if(reps != NULL)
		{
			resp->data = reps->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetReportDataSources succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetReportDataSources  error" , "");

	}

	SAFEDELETE(reps)
		return resp;
}

TResponseInfo * PandoraTest::GetReportClassPropertiesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TReportClassProperty> * reps = NULL;
	int id = 0;
	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);

		reps = db->GetReportClassProperties(params);

		if(reps != NULL)
		{
			resp->data = reps->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetReportClassProperties succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetReportClassProperties  error" , "");

	}

	SAFEDELETE(reps)
		return resp;
}

/************************************************************************/
/*  HELP TOPICS 		                                                */
/************************************************************************/
TResponseInfo * PandoraTest::AddHelpTopicRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	map<wstring, wstring> params;
	THelpTopic * topic = NULL;
	bool result = false;

	try
	{

		topic = new THelpTopic();

		topic->FromJson(req->info->data);
		if (topic->id == 0)
			topic->id = db->GetObjectID();

		result = db->AddHelpTopic(topic);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddHelpTopic succesfull.";
			resp->data = TUtility::ToJson(topic->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddHelpTopic  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	SAFEDELETE(topic)
		return resp;
}


TResponseInfo * PandoraTest::SaveHelpTopicRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	THelpTopic * topic = NULL;
	bool result = false;

	try
	{

		topic = new THelpTopic();

		topic->FromJson(req->info->data);

		result = db->SaveHelpTopic(topic);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveHelpTopic succesfull.";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "SaveHelpTopic  error" , "");

	}

	SAFEDELETE(topic)
		return resp;
}

TResponseInfo * PandoraTest::RemoveHelpTopicRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveHelpTopic(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveHelpTopic succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "RemoveHelpTopic  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetHelpTopicRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	THelpTopic * topic = NULL;
	int id = 0;
	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		topic = db->GetHelpTopic(id, req->info->language);

		if(topic != NULL)
		{
			resp->data = topic->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetHelpTopic succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetHelpTopic  error" , "");

	}

	SAFEDELETE(topic)
		return resp;
}

TResponseInfo * PandoraTest::GetHelpTopicsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	THelpTopic * tops = NULL;
	int parent_id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		tops = db->GetHelpTopics(parent_id, req->info->language);

		if(tops != NULL)
		{
			resp->data = tops->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetHelpTopics operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		ProcessError(resp, "", "GetHelpTopics  error" , "");

	}

	SAFEDELETE(tops)
		return resp;
}
TResponseInfo * PandoraTest::ChangeHelpTopicPositionRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int first_id = 0, second_id = 0;
	bool result = false;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		first_id =  TUtility::wstoi(params[L"first_id"]);
		second_id =  TUtility::wstoi(params[L"second_id"]);

		result = db->ChangeHelpTopicPosition(first_id, second_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "GetHelpTopic succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "ChangeHelpTopicPosition  error" , "");

	}
	return resp;
}

/************************************************************************/
/* REPORTS DATA															*/
/************************************************************************/
TResponseInfo * PandoraTest::GetReportDataRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	string sid, rep;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		sid =  TUtility::UnicodeToUtf8(params[L"sid"]);

		rep = db->GetRepData(sid);

		if(rep != "")
		{
			resp->data = rep;
			resp->succesfull = 1;
			resp->message = "GetRepData succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetReportData  error" , "");

	}

	return resp;
}

/************************************************************************/
/* PRINT                                                     */
/************************************************************************/
TResponseInfo * PandoraTest::GetDocProductInPrintRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0, lang_id = 0;
	string sid;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"doc_id"]);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		TUtility::CheckZeroValue(1, true, id);

		sid = db->GetDocProductInPrintJson(id, lang_id);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetDocProductInPrint succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocProductInPrint  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocProductOutPrintRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0, lang_id = 0;
	string sid;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"doc_id"]);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		TUtility::CheckZeroValue(1, true, id);

		sid = db->GetDocProductOutPrintJson(id, lang_id);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetDocProductOutPrintJson succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocProductOutPrint  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocInvoicePrintRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0, lang_id = 0;
	string sid;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"doc_id"]);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		TUtility::CheckZeroValue(1, true, id);

		sid = db->GetDocInvoicePrintJson(id, lang_id);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetDocInvoicePrintJson succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocInvoicePrint  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocOrderInPrintRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0, lang_id = 0;
	string sid;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"doc_id"]);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		TUtility::CheckZeroValue(1, true, id);

		sid = db->GetDocOrderInPrintJson(id, lang_id);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetDocOrderInPrint succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocOrderInPrint  error" , "");

	}

	return resp;
}

TResponseInfo * PandoraTest::GetDocOrderOutPrintRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0, lang_id = 0;
	string sid;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"doc_id"]);
		lang_id = TUtility::wstoi(params[L"lang_id"]);
		TUtility::CheckZeroValue(1, true, id);

		sid = db->GetDocOrderOutPrintJson(id, lang_id);

		if(sid != "")
		{
			resp->data = TUtility::ToJson(sid, "sid");
			resp->succesfull = 1;
			resp->message = "GetDocOrderOutPrintJson succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDocOrderOutPrint  error" , "");

	}

	return resp;
}

/************************************************************************/
/* PDF                                                                  */
/************************************************************************/
TResponseInfo * PandoraTest::GetReportPDFRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	map<wstring, wstring> params;
	TImage * image = NULL;
	int file_id = 0;

	try
	{
		image = new TImage();
		image->FromJson(req->info->data);

		if (image->id == 0)
			image->id = db->GetObjectID();

		//		image->file_name = TUtility::Utf8ToUnicode(req->file.filename);
		//		image->file_type = TUtility::Utf8ToUnicode(req->file.file_type);
		//		image->file_raw = req->file.file_raw;
		params = ParseParameters(req->info->parameters);

		file_id = db->GetReportPDF(image, req->user_id);

		if(file_id)
		{
			resp->succesfull = 1;
			resp->message = "GetReportPDF succesfull.";
			resp->data = TUtility::ToJson(file_id, "pdf_file");
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetReportPDF  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}

	catch (...)
	{
		ProcessError(resp, "", "GetReportPDF  error" , "");

	}

	SAFEDELETE(image)
		return resp;
}

/************************************************************************/
/* PRODUCTS EXEL                                                        */
/************************************************************************/

TResponseInfo * PandoraTest::ImportProductsCSVRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	map<wstring, wstring> params;
	vector<int> cat_ids;
	bool result = false;

	try
	{
		params = ParseParameters(req->info->parameters);
		params[L"user_id"] = TUtility::itows(req->user_id);

		cat_ids = ParseVectorIds(req->info->data);

		result = db->ImportProductsCSV(req->file.file_raw, params, cat_ids);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "ImportProductsCSV succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "ImportProductsCSV  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}

	catch (...)
	{
		ProcessError(resp, "", "RecalculateProductsPrice  error" , "");

	}

	return resp;
}

/************************************************************************/
/* RELATION                                                             */
/************************************************************************/
TResponseInfo * PandoraTest::GetRelationRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TRelation * rel = NULL;
	int object_id = 0, parent_id = 0, type_id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		object_id = TUtility::wstoi(params[L"object_id"]);
		parent_id = TUtility::wstoi(params[L"parent_id"]);
		type_id = TUtility::wstoi(params[L"type_id"]);

		rel = db->GetRelation(object_id, parent_id, type_id);

		if(rel)
		{
			resp->data = rel->ToJson(NULL, 1, "");
			resp->succesfull = 1;
			resp->message = "GetRelation succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetRelation  error" , "");

	}

	SAFEDELETE(rel)
		return resp;
}
TResponseInfo * PandoraTest::AddRelationRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	map<wstring, wstring> params;
	TRelation * rel = NULL;
	bool result = false;

	try
	{

		rel = new TRelation();

		rel->FromJson(req->info->data);
		if (rel->id == 0)
			rel->id = db->GetObjectID();

		result = db->AddRelation(rel);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddRelation succesfull.";
			resp->data = TUtility::ToJson(rel->id, "id");
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddRelation  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddRelation  error" , "");

	}

	SAFEDELETE(rel)
		return resp;
}


/************************************************************************/
/* PERMISSIONS                                                          */
/************************************************************************/
TResponseInfo * PandoraTest::CheckPermissionRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	bool result = false;
	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"user_id"] = TUtility::itows(req->user_id);

		result = db->CheckPermission(params);

		if(result)
		{
			resp->data = TUtility::ToJson(result, "allow");
			resp->succesfull = 1;
			resp->message = "CheckPermission succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "CheckPermission  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "CheckPermission  error" , "");

	}
	return resp;
}
TResponseInfo * PandoraTest::AddCustomPermissionsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection<TPermission> * values = NULL;
	bool result = false;
	int object_id = 0;

	try
	{

		values = new TCollection<TPermission>("TPermission");
		values->FromJson(req->info->data);

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		object_id =  TUtility::wstoi(params[L"object_id"]);
		result = db->AddCustomPermissions(values, object_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddCustomPermissions succesfull.";
			SAFEDELETE(values)
				return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddCustomPermissions  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddCustomPermissions  error" , "");

	}

	SAFEDELETE(values)
		return resp;
}


TResponseInfo * PandoraTest::GetPermissionsViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TPermissionView> * permissions = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		permissions = db->GetPermissionsView(params);

		if(permissions != NULL)
		{
			resp->data = permissions->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPermissionsView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPermissionsView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPermissionsView  error" , "");

	}

	SAFEDELETE(permissions)
		return resp;
}


TResponseInfo * PandoraTest::GetPermissionViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPermissionView * permission = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		permission = db->GetPermissionView(params);

		if(permission != NULL)
		{
			resp->data = permission->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPermissionView operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPermissionView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPermissionView  error" , "");

	}

	SAFEDELETE(permission)
		return resp;
}

TResponseInfo * PandoraTest::GetPermissionRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TPermission * permission = NULL;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		permission = db->GetPermission(params);

		if(permission != NULL)
		{
			resp->data = permission->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetPermission operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetPermission  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetPermission  error" , "");

	}

	SAFEDELETE(permission)
		return resp;
}


/************************************************************************/
/* CHECK SERVER STATUS                                                  */
/************************************************************************/
TResponseInfo * PandoraTest::CheckServerStatusRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();
	bool result = true;
	try
	{

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "CheckServerStatus succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "CheckServerStatus  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::StopServerRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();
	bool result = true;

	try
	{
		pmutex.lock();

		if (!stoping_server)
		{
			logger->Add("stoping server");
			boost::thread test_thread(boost::bind(&PandoraTest::CloseServer, this));
		}

		pmutex.unlock();

		if(result)
		{
			logger->Add("stoping succsess");
			resp->succesfull = 1;
			resp->message = "CloseServer succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "CloseServer  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetServerInfoRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TServerInfo * info = NULL;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		info = db->GetServerInfo(params);

		if(info != NULL)
		{
			info->server_host = pandora_host;
			info->server_port = pandora_port;
			resp->data = info->Serialize(1, "", req->info->response_type, false);
			resp->succesfull = 1;
			resp->message = "GetServerInfo succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetServerInfo  error" , "");

	}

	SAFEDELETE(info)
		return resp;
}

/************************************************************************/
/* GRID TO EXCEL                                                        */
/************************************************************************/
TResponseInfo * PandoraTest::ExportGridToExcelRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"user_id"] = TUtility::itows(req->user_id);

		id = db->ExportGridToExcel(TUtility::UriDecode(req->info->data), params);

		if(id)
		{
			resp->succesfull = 1;
			resp->message = "ExportGridToExcel operation succesfull!";
			resp->data = TUtility::ToJson(id, "id");
		}
		else
		{
			throw 0;

		}
	}

	catch (TError& error)
	{
		ProcessError(resp, error.code, "ExportGridToExcel  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "ExportGridToExcel  error" , "");

	}

	return resp;
}
/************************************************************************/
/* Files                                                                */
/************************************************************************/
TResponseInfo * PandoraTest::GetFileRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TFile * file = NULL;
	int id = 0;
	bool select_raw = true;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		select_raw = TUtility::wstoi(params[L"select_raw"]);
		TUtility::CheckZeroValue(1, true, id);

		file = db->GetFile(id, select_raw);

		if(file != NULL)
		{
			resp->data = file->Serialize(1, "", req->info->response_type, true);
			resp->succesfull = 1;
			resp->message = "GetFile succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetFile  error" , "");

	}

	SAFEDELETE(file)
		return resp;
}


/************************************************************************/
/* SERVER TIME(DATE)                                                    */
/************************************************************************/
TResponseInfo * PandoraTest::GetDateTimeRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int date = 0;

	try
	{

		date = TUtility::GetServerTime();

		if(date)
		{
			resp->data = TUtility::ToJson(date, "date");
			resp->succesfull = 1;
			resp->message = "GetServerTime succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		ProcessError(resp, "", "GetDateTime  error" , "");

	}

	return resp;
}

/************************************************************************/
/* MESSAGES                                                             */
/************************************************************************/
TResponseInfo * PandoraTest::SaveMessageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMessage * message = NULL;
	bool result = false;

	try
	{

		message = new TMessage();
		message->FromJson(req->info->data);
		message->user_id = req->user_id;
		result = db->SaveMessage(message);


		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveMessage succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "SaveMessage  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "SaveMessage  error" , "");

	}

	SAFEDELETE(message)
		return resp;
}
TResponseInfo * PandoraTest::AddMessageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMessage * message = NULL;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3550, "Add");

		message = new TMessage();
		message->FromJson(req->info->data);
		message->id = db->GetObjectID();
		message->user_id = req->user_id;

		result = db->AddMessage(message);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddMessage succesfull.";
			resp->data = TUtility::ToJson(message->id, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "AddMessage  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "AddMessage  error" , "");

	}

	SAFEDELETE(message)
		return resp;
}
TResponseInfo * PandoraTest::RemoveMessageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int id = 0;
	bool result = false;

	try
	{
		db->CheckPermission(req->user_id, 3550, "Remove");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->RemoveMessage(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveMessage succesfull.";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "RemoveMessage  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "RemoveMessage  error" , "");

	}

	return resp;
}
TResponseInfo * PandoraTest::GetMessageRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMessage * message = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3550, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		message = db->GetMessage(id);

		if(message != NULL)
		{
			resp->data = message->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMessage succesfull.";
		}

		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetMessage  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetMessage  error" , "");

	}

	SAFEDELETE(message)
		return resp;
}
TResponseInfo * PandoraTest::GetMessagesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TMessage> * messages = NULL;
	bool inactive = false;
	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);

		messages = db->GetMessages(params);

		if(messages != NULL)
		{
			resp->data = messages->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMessages operation succesfull!";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetMessages  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetMessages  error" , "");

	}

	SAFEDELETE(messages)
		return resp;
}
TResponseInfo * PandoraTest::GetMessageViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TMessageView * message = NULL;
	int id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3550, "Edit");

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		message = db->GetMessageView(id, req->info->language);

		if(message != NULL)
		{
			resp->data = message->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMessageView succesfull.";
		}

		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetMessageView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetMessageView  error" , "");

	}

	SAFEDELETE(message)
		return resp;
}
TResponseInfo * PandoraTest::GetMessagesViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TMessageView> * messages = NULL;
	bool inactive = false;
	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		messages = db->GetMessagesView(params);

		if(messages != NULL)
		{
			resp->data = messages->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetMessagesView operation succesfull!";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetMessagesView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetMessagesView  error" , "");

	}

	SAFEDELETE(messages)
		return resp;
}

/************************************************************************/
/* PROFIT DIAGRAM                                                       */
/************************************************************************/
TResponseInfo * PandoraTest::GetFinProfitSummariesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TFinDocSummary> * sums = NULL;
	int from_date = 0, to_date = 0, type_id = 0;
	wstring interval = L"";

	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		from_date =  TUtility::wstoi(params[L"from_date"]);
		to_date =  TUtility::wstoi(params[L"to_date"]);
		type_id =  TUtility::wstoi(params[L"type_id"]);
		interval = params[L"period"];

		sums = db->GetFinProfitSummaries(from_date, to_date, interval, type_id);

		if(sums != NULL)
		{
			resp->data = sums->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetFinProfitSummaries operation succesfull!";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetFinProfitSummaries  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetFinProfitSummaries  error" , "");

	}

	SAFEDELETE(sums)
		return resp;
}
TResponseInfo * PandoraTest::GetFinProfitPaymentsSummariesRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TFinSummary> * sums = NULL;
	int from_date = 0, to_date = 0, type_id = 0;
	wstring interval = L"";

	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		from_date =  TUtility::wstoi(params[L"from_date"]);
		to_date =  TUtility::wstoi(params[L"to_date"]);
		type_id =  TUtility::wstoi(params[L"type_id"]);
		interval = params[L"period"];

		sums = db->GetFinProfitPaymentsSummaries(from_date, to_date, interval, type_id);

		if(sums != NULL)
		{
			resp->data = sums->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetFinProfitPaymentsSummaries operation succesfull!";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetFinProfitPaymentsSummaries  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetFinProfitPaymentsSummaries  error" , "");

	}

	SAFEDELETE(sums)
		return resp;
}
TResponseInfo * PandoraTest::GetFinSummariesViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TFinSummaryView> * sums = NULL;
	int date = 0, money_type_id = 0;


	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		date =  TUtility::wstoi(params[L"date"]);
		money_type_id =  TUtility::wstoi(params[L"money_type_id"]);


		sums = db->GetFinSummariesView(req->info->language, money_type_id, date);

		if(sums != NULL)
		{
			resp->data = sums->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetFinSummariesViewRequest operation succesfull!";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetFinSummariesView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetFinSummariesView  error" , "");

	}

	SAFEDELETE(sums)
		return resp;
}
TResponseInfo * PandoraTest::GetAssetsRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TAssets * assets = NULL;
	int from_date = 0, to_date = 0;
	wstring interval = L"";


	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		from_date =  TUtility::wstoi(params[L"from_date"]);
		to_date =  TUtility::wstoi(params[L"to_date"]);
		interval = params[L"period"];

		assets = db->GetAssetsSummaries(from_date, to_date, interval);

		if(assets != NULL)
		{
			resp->data = assets->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetAssetsRequest operation succesfull!";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetAssets  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetAssets  error" , "");

	}

	SAFEDELETE(assets)
		return resp;
}
/************************************************************************/
/* MAKE DOCUMENT INVOICE                                                */
/************************************************************************/
TResponseInfo * PandoraTest::MakeDocumentInvoiceRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	int result = 0, doc_id = 0;

	try
	{
		db->CheckPermission(req->user_id, 3550, "Add");

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		doc_id = TUtility::wstoi(params[L"doc_id"]);

		result = db->MakeDocumentInvoice(doc_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "MakeDocumentInvoice succesfull.";
			resp->data = TUtility::ToJson(result, "id");
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "MakeDocumentInvoice  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "MakeDocumentInvoice  error" , "");

	}

	return resp;
}
/************************************************************************/
/* WEBSTORES                                                            */
/************************************************************************/

TResponseInfo * PandoraTest::AddWebstoreRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();
	TWebstore * ws = NULL;
	bool result = false;
	int cat_id = 0;

	try
	{
		ws = new TWebstore();
		ws->FromJson(req->info->data);
		ws->id = db->GetObjectID();

		result = db->AddWebstore(ws, cat_id, req->user_id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "AddWebstore succesfull.";
			resp->data = TUtility::ToJson(ws->id, "id");
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		cout << "Caught exception ";
		logger->Add("PandoraTest::RequestProcessor() line: ExecCommand(), AddWebstoreRequest()");
		resp->succesfull = 0;
		resp->message = "Caught exception at line: ExecCommand(), AddWebstoreRequest()";
	}

	SAFEDELETE(ws)

		return resp;
}
TResponseInfo * PandoraTest::RemoveWebstoreRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();
	int id = 0;
	bool result = false;

	try
	{
		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		result = db->MayDelete(id);
		if (!result)
		{
			resp->succesfull = 0;
			resp->message = "Error. There are some reference to this object! ";
			return resp;
		}

		result = db->RemoveWebstore(id);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "RemoveWebstore succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		cout << "Caught exception ";
		logger->Add("PandoraTest::RequestProcessor() line: ExecCommand(), RemoveWebstoreRequest()");
		resp->succesfull = 0;
		resp->message = "Caught exception at line: ExecCommand(), RemoveWebstoreRequest";
	}

	return resp;
}
TResponseInfo * PandoraTest::GetWebstoreRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();
	TWebstore * webstore = NULL;
	int id = 0;

	try
	{

		map <wstring, wstring> params = ParseParameters(req->info->parameters);
		id =  TUtility::wstoi(params[L"id"]);
		TUtility::CheckZeroValue(1, true, id);

		webstore = db->GetWebstore(id, req->info->language);

		if(webstore != NULL)
		{
			resp->data = webstore->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetWebstore succesfull.";
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		cout << "Caught exception ";
		logger->Add("PandoraTest::RequestProcessor() line: ExecCommand(), GetWebstoreRequest()");
		resp->succesfull = 0;
		resp->message = "Caught exception at line: ExecCommand(), GetWebstoreRequest()";
	}

	SAFEDELETE(webstore)
		return resp;
}
TResponseInfo * PandoraTest::GetWebstoresRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();
	TCollection <TWebstore> * webstores = NULL;
	bool result = false;

	try
	{
		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);

		webstores = db->GetWebstores(params);

		if(webstores != NULL)

		{
			resp->data = webstores->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetWebstores operation succesfull!";
		}
		else
		{
			throw 0;

		}
	}

	catch (...)
	{
		cout << "Caught exception ";
		logger->Add("PandoraTest::RequestProcessor() line: ExecCommand(), GetWebstoresRequest()");
		resp->succesfull = 0;
		resp->message = "Caught exception at line: ExecCommand(), GetWebstoresRequest()";
	}

	SAFEDELETE(webstores)

		return resp;
}
TResponseInfo * PandoraTest::SaveWebstoreRequest(request * req)
{
	TResponseInfo * resp = new TResponseInfo();
	TWebstore * webstore = NULL;
	bool result = false;

	try
	{
		webstore = new TWebstore();
		webstore->FromJson(req->info->data);
		result = db->SaveWebstore(webstore);

		if(result)
		{
			resp->succesfull = 1;
			resp->message = "SaveWebstore succesfull.";
			return resp;
		}
		else
		{
			throw 0;

		}

	}

	catch (...)
	{
		cout << "Caught exception ";
		logger->Add("PandoraTest::RequestProcessor() line: ExecCommand(), SaveWebstore()");
		resp->succesfull = 0;
		resp->message = "Caught exception at line: ExecCommand(), SaveWebstore()";
	}

	SAFEDELETE(webstore)

		return resp;
}

/************************************************************************/
/* CRONTASKS                                                            */
/************************************************************************/
TResponseInfo * PandoraTest::GetCronTasksViewRequest(request * req)
{

	TResponseInfo * resp = new TResponseInfo();	// ������� TResponseInfo
	TCollection <TCronTaskView> * views = NULL;

	try
	{

		map<wstring, wstring> params = ParseParameters(req->info->parameters);
		params[L"lang_id"] = TUtility::itows(req->info->language);
		views = db->GetCronTasksView(params);

		if(views != NULL)
		{
			resp->data = views->Serialize(1, "", req->info->response_type);
			resp->succesfull = 1;
			resp->message = "GetCronTasksView operation succesfull!";
		}
		else
		{
			throw 0;

		}

	}
	catch (TError& error)
	{
		ProcessError(resp, error.code, "GetCronTasksView  error" , TUtility::UriEncode(error.Serialize(1, "", req->info->response_type)));

	}
	catch (...)
	{
		ProcessError(resp, "", "GetCronTasksView  error" , "");

	}

	SAFEDELETE(views)
		return resp;
}

/************************************************************************/
/*   GET PHP                                                            */
/************************************************************************/
TResponseInfo * PandoraTest::GetPHP(request * req)
{
	TResponseInfo * resp = new TResponseInfo();
	FILE * output;
	char buff[1024];
	string json = req->post_parms["json"];
	string command = config->content["scripts_path"] + "php\\php.exe " + config->content["scripts_path"] + "index.php ";
	string s = "post_params=" + TUtility::base64_encode(json.c_str(), json.size());
	command.append(s);

	try
	{
		output = POPEN(command.c_str(), "r");
		s = "";
		if(output != NULL)
		{
			while ( fgets( buff, sizeof buff, output ) != NULL )
			{
				s += buff;
			}
			try
			{
				resp->FromJson(s);
			}
			catch(...)
			{
				TError error;
				error.code = "php error";
				error.message = TUtility::Utf8ToUnicode("php error : " + s);
				throw error;
			}
		}
	}
	catch (TError& error)
	{
		cout << "Caught exception " << error.code << endl;
		logger->Add("PandoraTest::RequestProcessor() line: ExecCommand(), GetPHP()");
		resp->succesfull = 0;
		resp->message = "GetPHP error";
		resp->error = TUtility::UriEncode(error.Serialize(1, "", req->info->response_type));
	}
	catch(...)
	{

	}
	PCLOSE(output);
	return resp;
}
/************************************************************************/
/* REPORTS DATA                                                         */
/************************************************************************/
/************************************************************************/
/* PrepareHTTPResponce - ��������� ����� ���� ��� ���                   */
/************************************************************************/
#endif
string PandoraTest::PrepareHTTPResponce(string str)
{

	stringstream ss;
	string s;
	auto result = str.find("HTTP/");
	if(result == str.npos)
	{
		string str_header ="HTTP/1.1 200 OK\r\n"
			"Content-type: text/html\r\n"
			"Content-Length: ";

		ss << str_header << str.length() << "\r\n\r\n" << str;
	}
	else
		ss << str;

	s = ss.str();
	ss.str("");
	ss.clear();
	return s;
}
string PandoraTest::GenCookie(int id)
{
	time_t cookie_time = time(0);
	string cookie;
	stringstream ss;
	srand(TUtility::GetTimeMilliseconds());

	for(int i = 0; i < 3; ++i)
	{
		int n = rand() % 26;
		char c = (char)(n+65);
		cookie.append(1, c);

	}


	ss<< cookie << id << cookie_time << TUtility::GetTimeMilliseconds();

	ss >> cookie;

	return cookie;

}

string PandoraTest::ParseErrorCode(string error)
{
	size_t end = 0;
	size_t begin = 0;
	size_t diff = 0;
	stringstream ss;
	string xml = "";
	string param = "";
	string val = "";

	begin = error.find(": ", begin);
	begin++;

	while (begin < error.length())
	{
		begin += 1;
		end = error.find("=", begin);
		diff = end - begin;

		param.append(error, begin, diff);
		begin = end;
		begin += 1;


		end = error.find(";", begin);
		diff = end - begin;

		val.append(error, begin, diff);
		begin = end;

		ss << "<" << param << ">";
		ss << val;
		ss << "</" << param << ">";

		param.clear();
		val.clear();
		begin += 1;

	}  // while

	xml = ss.str();
	ss.str();
	ss.clear();

	return xml;

}

void PandoraTest::ProcessError(TResponseInfo * resp, string error_code, string error_message, string terror, bool send_error)
{
	cout << "Caught exception " << error_code;
	logger->Add(error_message);
	if(resp)
	{
		resp->succesfull = 0;
		resp->message = error_message;
		resp->error = terror;
	}

	if(send_error && manager)
	{
		stringstream message;
		message << "error="<< TUtility::str_replace(" ", "%20", error_message) << "&key=" << config->content["key"];
		manager->SendHttpRequest("www.monely.com", "80", PrepareHTTPRequest(true, "", "/error.php", "www.monely.com", message.str()), 3, true);
	}

}
