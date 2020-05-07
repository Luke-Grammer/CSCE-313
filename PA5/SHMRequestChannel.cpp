#include "SHMRequestChannel.h"
using namespace std;

/*--------------------------------------------------------------------------*/
/* CONSTRUCTOR/DESTRUCTOR FOR CLASS   R e q u e s t C h a n n e l  */
/*--------------------------------------------------------------------------*/

SHMRequestChannel::SHMRequestChannel(const string _name, const Side _side) : RequestChannel(_name, _side)
{
	string name1 = my_name + "_client";
	string name2 = my_name + "_server";

	shm_name = "/shm_" + ((my_side == SERVER_SIDE) ? name2 : name1);
	client_buffer = new SHMBoundedBuffer(name1);
	server_buffer = new SHMBoundedBuffer(name2);
}

SHMRequestChannel::~SHMRequestChannel()
{ 
	delete client_buffer;
	delete server_buffer;
}

char* SHMRequestChannel::cread(int *len)
{
	char* buf = (my_side == CLIENT_SIDE) ? server_buffer->pop() : client_buffer->pop();

	if(len != nullptr)
		*len = sizeof(buf);

	return buf;
}

int SHMRequestChannel::cwrite(char* msg, int len)
{
	(my_side == CLIENT_SIDE) ? client_buffer->push(msg, len) : server_buffer->push(msg, len);
}

