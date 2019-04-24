#include "../../include/http/TQueueItem.h"


TQueueItem::TQueueItem()
{
	id = 0;
	status = 0;
	response = "";
	request_ = NULL;
}
TQueueItem::~TQueueItem()
{
    SAFEDELETE(request_)
}

TQueueItem * TQueueItem::Clone()
{
	TQueueItem * item = new TQueueItem();
	item->id = id;
	item->request_ = request_->Clone();
	item->response = response;
	item->status = 0;
	item->thread_index = thread_index;
	return item;
}

void TQueueItem::WaitForReply(int time_out)
{
	time_t begintime = time(0);
	time_t timer = 0;
	time_t endtime = 0;

	while (status != 2)
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(200));
		endtime	= time(0);
		timer = endtime - begintime;

		if (timer > time_out)
		{
			std::cout<<"timeout \n";
			stringstream ss;
			string message = "<TResponseInfo type=\"xml\" version=\"1.0\" request_id=\"\" client_id=\"0\" time=\"0\" exec_time=\"0\">"
				"<command></command><parameters></parameters><succesfull>0</succesfull><message_code>00</message_code>"
				"<message>Request Timeout!</message><lang_id></lang_id><data></data></TResponseInfo>";

			ss << "HTTP/1.1 200 OK\r\n" << "Content-type: text/html\r\n"
				<< "Content-Length: " << message.length() << "\r\n\r\n"
				<< message;
			status = 3;

			response = ss.str();
			return;
		}
	}
}
