#ifndef _KernelSemaphore_H_
#define _KernelSemaphore_H_

#include <semaphore.h>
#include "common.h"
using namespace std;

class KernelSemaphore {
	sem_t* sem;
	int* value;
	string name;

public:
	KernelSemaphore(string name, int _val); 
	~KernelSemaphore();

	void P();
	void V();
};

#endif