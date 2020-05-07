#include "KernelSemaphore.h"

KernelSemaphore::KernelSemaphore(string _name, int _val)
{
	value = new int();

	name = "/" + _name;
	sem = sem_open(name.c_str(), O_CREAT, 0666, _val);
	if (sem < 0)
	{
		EXITONERROR(name);
	}
} 

KernelSemaphore::~KernelSemaphore()
{
	sem_unlink(name.c_str());
	sem_close(sem);
}

void KernelSemaphore::P()
{
	sem_wait(sem);
}

void KernelSemaphore::V()
{
	sem_post(sem);
}