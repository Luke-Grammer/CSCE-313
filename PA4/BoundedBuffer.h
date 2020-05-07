#ifndef BoundedBuffer_h
#define BoundedBuffer_h

#include <stdio.h>
#include <queue>
#include <string>
#include <pthread.h>

using namespace std;

class BoundedBuffer
{
private:
  	int cap;
  	int size;
	queue<vector<char>> q;
	pthread_mutex_t mtx;
	pthread_cond_t cond1, cond2;

public:
	BoundedBuffer(int _cap){
		cap = _cap;
		size = 0;
		pthread_mutex_init(&mtx, NULL);
		pthread_cond_init(&cond1, NULL);
		pthread_cond_init(&cond2, NULL);
	}

	~BoundedBuffer(){
		pthread_mutex_destroy(&mtx);
		pthread_cond_destroy(&cond1);
		pthread_cond_destroy(&cond2);
	}

	void push(char* data, int len){
		pthread_mutex_lock(&mtx);
		while(size == cap)
		{
			pthread_cond_wait(&cond2, &mtx);
		}
		
		if (len > 0)
		{
			vector<char> values(data, data + len);
			q.push(values);
		}
		else
		{
			q.push(vector<char>());
		}
		
		size++;

		pthread_cond_signal(&cond1);
		pthread_mutex_unlock(&mtx);
	}

	vector<char> pop(){
		pthread_mutex_lock(&mtx);
		
		while(size == 0)
		{
			pthread_cond_wait(&cond1, &mtx);
		}

		vector<char> result = q.front();
		q.pop();
		size--;
		
		pthread_cond_signal(&cond2);
		pthread_mutex_unlock(&mtx);
		return result;
	}

	int getSize()
	{
		return q.size();
	}
};

#endif /* BoundedBuffer_ */
