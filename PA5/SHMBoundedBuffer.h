#ifndef SHMBoundedBuffer_h
#define SHMBoundedBuffer_h

#include "common.h"
#include<sys/mman.h>
#include "KernelSemaphore.h"

using namespace std;

class SHMBoundedBuffer
{
private:

	char* data;
	int shm_id;
	string shm_name;
	KernelSemaphore *empty, *full;

public:
	SHMBoundedBuffer(string name){
		string empty_name = name + "_empty";
		string full_name = name + "_full";
		
		shm_name = "/shm_" + name;

		empty = new KernelSemaphore(empty_name.c_str(), 1);
		full = new KernelSemaphore(full_name.c_str(), 0);

		shm_id = shm_open(shm_name.c_str(), O_RDWR | O_CREAT, 0666);
		ftruncate(shm_id, MAX_MESSAGE);

		data = (char*) mmap(NULL, MAX_MESSAGE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_id, 0);
	}

	~SHMBoundedBuffer(){
		delete empty;
		delete full;

		close(shm_id);
		munmap(data, MAX_MESSAGE);
		
		if (shm_unlink(shm_name.c_str()) < 0)
			EXITONERROR(shm_name.c_str());
	}

	void push(char* msg, int len){
		empty->P();
		
		memcpy(data, msg, len);

		full->V();
	}

	char* pop(){
		full->P();

		char* buf = new char[MAX_MESSAGE];

		memcpy(buf, data, MAX_MESSAGE);

		empty->V();
		return buf;
	}
};

#endif /* BoundedBuffer_ */
