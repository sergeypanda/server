#ifndef TQUEUE_H
#define TQUEUE_H

#include "TQueueItem.h"

#include <boost/thread/mutex.hpp>
#include <vector>


struct reply;

class TQueue
{
public:

	int count;
	std::vector<TQueueItem*> items;
	boost::mutex mutex;
	bool stop_queue;

	TQueue();
	~TQueue();

	TQueueItem * AddRequest(request * request);
	void AddResponse(int id, std::string response);
	TQueueItem * GetRequest();
	void DeleteQueueItem(int index);
	std::string GetReply(request * req, reply& rep);

	void StopQueue();
	int GetQueueSize();

	void RefuseRequest(TQueueItem * q_i);
	TQueueItem * GetNextRequest(TQueueItem * q_i);

};
#endif
