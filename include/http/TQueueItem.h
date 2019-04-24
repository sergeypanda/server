#ifndef TQUEUEITEM_H
#define TQUEUEITEM_H

#include "request.hpp"


class TQueueItem
{
public:

	int id;
	request * request_;
	std::string response;
	std::atomic<int> status; // 0- new, 1 - in progress, 2 - ready, 3 - timeout; 
    int thread_index;

	TQueueItem();
	~TQueueItem();
	void WaitForReply(int time_out);
	TQueueItem * Clone();
};
#endif