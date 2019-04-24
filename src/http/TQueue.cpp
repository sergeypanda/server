#include "../../include/http/TQueue.h"
#include "../../include/http/reply.hpp"

TQueue::TQueue()
{
	count = 0;
	stop_queue = false;
}
TQueue::~TQueue()
{
    for(int i = 0; i < (int)items.size(); ++i )
    {
        SAFEDELETE(items.at(i))
    }
}

void TQueue::StopQueue()
{
	stop_queue = true;
}

int TQueue::GetQueueSize()
{
	return items.size();
}

TQueueItem * TQueue::AddRequest(request * req)
{

	TQueueItem * item = new TQueueItem();
	count++;
	item->status = 0;
	item->id = count;
	item->request_ = req->Clone();
	req->enter_time = TUtility::GetTimeMilliseconds();

	if (!stop_queue)
		items.push_back(item);

	std::cout << (int)items.size()<< std::endl;

	return item;
}


void TQueue::AddResponse(int id, std::string response)
 {
	 mutex.lock();
	 try
	 {
		for(int i=0; i < (int)items.size(); ++i)
		{
			if(items.at(i)->id == id)
			{
				items.at(i)->response = response;
				items.at(i)->status = 2;

				break;
			}
		}
	 }

	catch (int error)
	{
		cout << "Caught exception number:  " << error << endl;
		logger->Add("TGuardian::RequestProcessor() line: AddResponse(Qit->id, r_xml)");

	}
	
	mutex.unlock();
 }

void TQueue::RefuseRequest(TQueueItem * q_i)
{
	mutex.lock();
	q_i->status = 0;
	mutex.unlock();
}

TQueueItem * TQueue::GetNextRequest(TQueueItem * q_i)
{
	RefuseRequest(q_i);

	TQueueItem * tmp = NULL;
	int k = -1;
	mutex.lock();
	for(int i = 0; i < (int)items.size(); ++i)
	{
		if(items.at(i) == q_i)
		{
			k = i;
			break;
		}

	}
	mutex.unlock();
	//???

	if(k == -1)
		return tmp;

	mutex.lock();
	for(int i = k + 1; i < (int)items.size(); ++i)
	{
		if(items.at(i)->status == 0)
		{
			items.at(i)->status = 1;
			tmp = items.at(i);
			break;
		}
	}
	mutex.unlock();
	return tmp;
}


 TQueueItem * TQueue::GetRequest()
{
	TQueueItem * tmp = NULL;

	mutex.lock();
	try
	{
		for(int i = 0; i < (int)items.size(); ++i)
		{
			TQueueItem * it = items.at(i);
			if(it)
			{
				if(it->status == 0)
				{
					it->status = 1;
					tmp = items.at(i);
					break;
				}
			}

		}


	}

	catch (int error)
	{
		cout << "Caught exception number:  " << error << endl;
		logger->Add("TGuardian::RequestProcessor() line: GetRequest()");
	}

	mutex.unlock();

	return tmp;
}

void TQueue::DeleteQueueItem(int id)
 {
	mutex.lock();
	 for(int i=0; i < (int)items.size(); ++i)
	 {
		 TQueueItem * item = items.at(i);

		 if(item->id == id)
		 {
			 if (item->status != 3)
				 SAFEDELETE(item);

			 items.erase(remove(items.begin(), items.end(), items.at(i)), items.end());
		 }
	 }
	mutex.unlock();
 }


std::string TQueue::GetReply(request * req, reply & rep)
{
	std::string str= "";

	mutex.lock();

	boost::thread::id my_thread = boost::this_thread::get_id();
	TQueueItem * qi = AddRequest(req);
	rep.queue_item_id = qi->id;

	mutex.unlock();

	time_t begintime = time(0);
	time_t timer = 0;
	time_t endtime = 0;

	while (qi->status != 2)
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(200));
		endtime	= time(0);
		timer = endtime - begintime;

		if (timer > 60)
		{
			std::cout<<"timeout \n";
			stringstream ss;
			string message = "{\"class_name\":\"TRequestInfo\", \"request_id\":\"\", \"client_id\":0, \"time\":0, \"exec_time\":0, \"command\": \"\", \"parameters\":\"\", \"succesfull\":0, \"message_code\":\"00\",\"message\":\"Request Timeout!\",\"lang_id\":0, \"data\":\"\" }";
			ss << "HTTP/1.1 200 OK\r\n" << "Content-type: text/html\r\n"
				<< "Content-Length: " << message.length() << "\r\n\r\n"
				<< message;

			qi->status = 3;

			return ss.str();
		}
	}

	str = qi->response;

	return str;
}

