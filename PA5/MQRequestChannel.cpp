#include "MQRequestChannel.h"
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

MQRequestChannel::MQRequestChannel(const string _name, const Side _side) : RequestChannel(_name, _side)
{
	queue_name1 = "/mq_" + my_name + "1";
	queue_name2 = "/mq_" + my_name + "2";

	attr = new mq_attr();
	memset(attr, 0, sizeof(mq_attr));
	attr->mq_maxmsg = 10;
	attr->mq_flags = NULL;
	attr->mq_curmsgs = 0;
	attr->mq_msgsize = MAX_MESSAGE;

	if (_side == SERVER_SIDE){
		wfd = mq_open(queue_name1.c_str(), O_WRONLY | O_CREAT, 0666, attr);
		rfd = mq_open(queue_name2.c_str(), O_RDONLY | O_CREAT, 0666, attr);
	}
	else{
		rfd = mq_open(queue_name1.c_str(), O_RDONLY | O_CREAT, 0666, attr);
		wfd = mq_open(queue_name2.c_str(), O_WRONLY | O_CREAT, 0666, attr);
	}
}

MQRequestChannel::~MQRequestChannel()
{
	mq_close(rfd);
	mq_close(wfd);
	
	if (my_side == CLIENT_SIDE)
	{
		if (mq_unlink(queue_name1.c_str()) < 0)
			EXITONERROR(queue_name1.c_str());
			
		if (mq_unlink(queue_name2.c_str()) < 0)
			EXITONERROR(queue_name2.c_str());
	}

	delete attr;
}

char* MQRequestChannel::cread(int *len)
{
	char* buf = new char[MAX_MESSAGE];
	int length = -1; 
	uint* priority = NULL;
	length = mq_receive(rfd, buf, MAX_MESSAGE, priority);
	if (length < 0)
		EXITONERROR(buf);

	if (len) // the caller wants to know the length
		*len = length;

	return buf;
}

int MQRequestChannel::cwrite(char* msg, int len)
{
	if (len > MAX_MESSAGE)
		EXITONERROR("cwrite");

	if (mq_send(wfd, msg, len, NULL) < 0)
		EXITONERROR("cwrite");

	return len;
}

