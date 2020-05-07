#include "common.h"
#include "BoundedBuffer.h"
#include "Histogram.h"
#include "common.h"
#include "HistogramCollection.h"
#include "FIFOreqchannel.h"
#include <sys/wait.h>

using namespace std;

HistogramCollection HIST_COLLECTION; // global variables needed for bonus signal handler
__uint64_t FILE_SIZE = 0;
__uint64_t TRANSFERRED_SIZE = 0;

struct patient_thread_args 
{
    int n; // number of datapoints [0-15000]
    int patient; // which patient [1-15]
	BoundedBuffer* request_buffer;
};

struct worker_thread_args
{
	vector<Histogram*>* hists; // one histogram per patient
	BoundedBuffer* request_buffer;
	FIFORequestChannel* request_channel; // every worker has its own channel
	pthread_mutex_t* mtx; // avoid race conditions updating histograms
};

struct filereq_thread_args
{
	string f; // name of input file
    int m; // max buffer size (BREAKS IF > 256)
	__int64_t file_size; // size of input file
	BoundedBuffer* request_buffer; 
};

struct fileworker_thread_args
{
	string f; // name of input file (for message size)
	int fd; // descriptor of output file
	FIFORequestChannel* request_channel; // every worker has its own channel
	BoundedBuffer* request_buffer;
	pthread_mutex_t* mtx; // avoid race conditions writing to output file
};

void* patient_thread_function(void* arg)
{ // sends server requests for patient ECG information to a bounded buffer
	struct patient_thread_args* arguments;
	arguments = (struct patient_thread_args*) arg; // collect args
	char* msg = new char[sizeof(datamsg)];

	for(int i = 0; i < arguments->n; i++) {
		*(datamsg*) msg = datamsg(arguments->patient, (double) i * (60.0 / 15000.0), 1); // format new message and push to buffer
        arguments->request_buffer->push(msg, sizeof(datamsg));      
	}

	delete msg;
    pthread_exit(NULL);
}

void* worker_thread_function(void* arg)
{ 
	struct worker_thread_args* arguments;
	arguments = (struct worker_thread_args*) arg; // collect args

	while(true)
	{
		vector<char> ret_msg = arguments->request_buffer->pop(); // pop message from buffer
		if (ret_msg.size() == 0) // if worker pops quit message, exit
			break;
		char* msg = ret_msg.data();
		
		arguments->request_channel->cwrite( msg, sizeof(datamsg)); // write message to req channel 

		double result = *(double*) arguments->request_channel->cread(); // read result

		pthread_mutex_lock(arguments->mtx);
		(arguments->hists->at(((datamsg*) msg)->person - 1))->update(result); // update patient's histogram (avoids race conditions)
		pthread_mutex_unlock(arguments->mtx);
	}
	pthread_exit(NULL);
}

void* filereq_thread_function(void* arg)
{ // sends server requests for file data to a bounded buffer
	struct filereq_thread_args* arguments;
	arguments = (struct filereq_thread_args*) arg; // collect args
	char* msg = new char[sizeof(filemsg) + arguments->f.length()];

	for(__int64_t offset = 0; offset < arguments->file_size; offset += arguments->m)
	{ // Request file in increments of m bytes
		int buf_size = (offset + arguments->m < arguments->file_size) ? arguments->m : arguments->file_size - offset; // Determine how much can be requested
		*(filemsg*) msg = filemsg(offset, buf_size); // format request

		for(int i = 0; i < arguments->f.length(); i++)
		{ // append filename to request
			*(msg + sizeof(filemsg) + i) = arguments->f[i];
		}

		arguments->request_buffer->push(msg, sizeof(filemsg) + sizeof(arguments->f)); // send request to buffer
	}

	delete[] msg;
    pthread_exit(NULL);
}

void* fileworker_thread_function(void* arg)
{
	struct fileworker_thread_args* arguments;
	arguments = (struct fileworker_thread_args*) arg; // collect args

	int fd = open(arguments->f.c_str(), O_CREAT | O_WRONLY | O_NDELAY, S_IWUSR | S_IRUSR);

	while(true)
	{
		vector<char> ret_msg = arguments->request_buffer->pop(); // pop message from buffer
		if (ret_msg.size() == 0) // if worker pops quit message, exit
			break;
		char* msg = ret_msg.data();

		arguments->request_channel->cwrite( msg, sizeof(filemsg) + sizeof(arguments->f)); // write message to req channel 

		char* result = arguments->request_channel->cread(); // read result

		__int64_t offset = ((filemsg*) msg)->offset; // determine offset and buffer size from message
		int bufsize = ((filemsg*) msg)->length;
		
		pwrite(fd, result, bufsize, offset);

		pthread_mutex_lock(arguments->mtx); // avoid race conditions updating shared resources
		TRANSFERRED_SIZE += bufsize; // update amout transferred for the console updater  
		pthread_mutex_unlock(arguments->mtx);
	}

	close(fd);
	pthread_exit(NULL);
}

void parseArgs(int& argc, char* argv[], string& f, int& n, int& p, int& w, int& b, int& m)
{
	int opt = 0;
	while ((opt = getopt(argc, argv, "n:p:w:b:f:m:")) != -1)
	{ // while options were received from getopt
		int arg = atoi(optarg);
		switch (opt)
		{	
			case 'f': // if filename is specified for file transfers
				f = optarg;
				break;
			case 'n': // if number of datapoints is specified
				if (arg < 1 || arg > 15000)
				{
					printf("ERROR: Number of datapoints out of acceptable range! [1-15000]\n");
					exit(EXIT_FAILURE);
				}
				n = arg;
				break;
			case 'p': // if number of patients is specified
				if (arg < 1 || arg > 15)
				{
					printf("ERROR: Number of patients out of acceptable range! [1-15]\n");
					exit(EXIT_FAILURE);
				}
				p = arg;
				break;
			case 'w': // if number of worker threads is specified
				if (arg < 50 || arg > 2000)
				{
					printf("ERROR: Number of worker threads out of acceptable range! [50-2000]\n");
					printf("Cannot have w > 2000 without increasing ulimit\n");
					exit(EXIT_FAILURE);
				}
				w = arg;
				break;
			case 'b': // if total size is specified
				if (arg < 1 || arg > 1000)
				{
					printf("ERROR: Buffer size out of acceptable range! [1-1000]\n");
					exit(EXIT_FAILURE);
				}
				b = arg;
				break;
			case 'm': // if maximum message size file file transfers is specified
				if (arg < 1 || arg > 1000)
				{
					printf("ERROR: Max message size out of acceptable range! [1-1000]\n");
					exit(EXIT_FAILURE);
				}
				m = arg;
				break;
			case '?': // if unknown, end the program (getopt produces its own error message)
				exit(EXIT_FAILURE);
		}
	}	
}

void update_console(int sig)
{ // updates console (called every 2s by signal handler + timer)
	system("clear");
	if (FILE_SIZE == 0) // file transfer or data request?
	{
		HIST_COLLECTION.print();
	}
	else
	{
		cout << "Transfer " << (TRANSFERRED_SIZE / (double) FILE_SIZE) * 100 << "% complete." << endl; 
	}
}

void handle_data_request(int n, int p, int w, FIFORequestChannel* chan, BoundedBuffer& request_buffer) 
{ // creates p patient threads and w worker threads to collect patient ECG data from server using a buffer
	pthread_t patient_threads[p];
	pthread_t worker_threads[w];
	struct patient_thread_args patient_args[p];
	struct worker_thread_args worker_args[w];
	vector<Histogram*> hists; 
	pthread_mutex_t mtx;
	pthread_mutex_init(&mtx, NULL);

	for(int i = 0; i < p; i++)
	{ // create p patient threads
		patient_args[i].n = n;
		patient_args[i].patient = i + 1;
		patient_args[i].request_buffer = &request_buffer;
		pthread_create(&patient_threads[i], NULL, patient_thread_function, (void*) &patient_args[i]);
		hists.push_back(new Histogram(36, -8, 7.5));
	}
	
	for(int i = 0; i < hists.size(); i++)
	{ // add new histograms to collection
		HIST_COLLECTION.add(hists[i]);
	}

	for(int i = 0; i < w; i++)
	{ // create w worker threads
		char* msg = (char*) new newchannelmsg(); // format new channel request
		chan->cwrite(msg, sizeof(newchannelmsg)); // send request to server
		worker_args[i].request_channel = new FIFORequestChannel(chan->cread(), FIFORequestChannel::CLIENT_SIDE);
		worker_args[i].hists = &hists;
		worker_args[i].request_buffer = &request_buffer;
		worker_args[i].mtx = &mtx;
		pthread_create(&worker_threads[i], NULL, worker_thread_function, (void*) &worker_args[i]);
		delete msg;
	}

	for(int i = 0; i < p; i++)
	{ // make sure patient threads have finished
		pthread_join(patient_threads[i], NULL);
	}

	for(int i = 0; i < w; i++)
	{ // push quit messages to worker threads
		request_buffer.push(NULL, 0);
	}

	for(int i = 0; i < w; i++)
	{ // make sure worker threads have finished
		pthread_join(worker_threads[i], NULL);
		delete worker_args[i].request_channel;
	}

	pthread_mutex_destroy(&mtx);
}

void handle_file_request(string f, int m, int w, FIFORequestChannel* chan, BoundedBuffer& request_buffer) 
{ // creates a file request thread and w worker threads to transfer a file via a server using a buffer
	pthread_t filereq_thread;
	pthread_t worker_threads[w];
	struct filereq_thread_args filereq_args;
	struct fileworker_thread_args worker_args[w];
	pthread_mutex_t mtx;
	pthread_mutex_init(&mtx, NULL);

	char* msg = new char[sizeof(filemsg) + f.length()];
	*(filemsg*) msg = filemsg(0, 0); // format file size request
	for(int i = 0; i < f.length(); i++)
	{ // append filename to request
		*(msg + sizeof(filemsg) + i) = f[i];
	}
	
	chan->cwrite(msg, sizeof(filemsg) + sizeof(f)); // send request to dataserver
	FILE_SIZE = *(__int64_t*) chan->cread(); // read response
	
	// create a file request thread
	filereq_args.f = f;
	filereq_args.m = m;
	filereq_args.file_size = FILE_SIZE;
	filereq_args.request_buffer = &request_buffer;

	pthread_create(&filereq_thread, NULL, filereq_thread_function, (void*) &filereq_args);
	
	f = "received/" + f;
	int fd = open(f.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IWUSR | S_IRUSR); // create file for output
	close(fd);

	for(int i = 0; i < w; i++)
	{ // create w worker threads
		char* msg = (char*) new newchannelmsg(); // format new channel request
		chan->cwrite(msg, sizeof(newchannelmsg)); // send request to server
		worker_args[i].request_channel = new FIFORequestChannel(chan->cread(), FIFORequestChannel::CLIENT_SIDE);	
		worker_args[i].request_buffer = &request_buffer;
		worker_args[i].mtx = &mtx;
		worker_args[i].fd = fd;
		worker_args[i].f = f;
		pthread_create(&worker_threads[i], NULL, fileworker_thread_function, (void*) &worker_args[i]);
		delete msg;
	}
	
	// make sure the file request thread finished
	pthread_join(filereq_thread, NULL);

	for(int i = 0; i < w; i++)
	{ // push quit messages to worker threads
		request_buffer.push(NULL, 0);
	}

	for(int i = 0; i < w; i++)
	{ // make sure worker threads have finished
		pthread_join(worker_threads[i], NULL);
		delete worker_args[i].request_channel;
	}

	pthread_mutex_destroy(&mtx);
	delete msg;

	printf("File successfully copied!\n");
	printf("Results written to %s\n", f.c_str());
}

int main(int argc, char *argv[])
{
	string f = "";  // filename for file access
    int n = 100;    // default number of requests per "patient"
    int p = 10;     // number of patients [1,15]
    int w = 100;    // default number of worker threads
    int b = 1;   	// default capacity of the request buffer, you should change this default
	int m = 256; 	// default capacity of the file buffer
    srand(time_t(NULL));
    
	parseArgs(argc, argv, f, n, p, w, b, m);
    
    int pid = fork();
    if (pid == 0)
	{
		char str[sizeof(m) * 8]; // create a string large enough to hold m
		snprintf(str, sizeof(str), "%d", m); // copy m's data to the new string
        execl ("dataserver", "dataserver", str);   
    }

	FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
    BoundedBuffer request_buffer(b);
	
	struct sigaction action;
	struct sigevent clock_sig_event; 
	struct itimerspec t_val;
	timer_t t_id; 
	
	action.sa_handler = update_console;
	assert(sigaction(SIGALRM, &action, NULL) == 0);

	clock_sig_event.sigev_notify = SIGEV_SIGNAL;
	clock_sig_event.sigev_signo = SIGALRM;
	clock_sig_event.sigev_notify_attributes = NULL;

	assert(timer_create(CLOCK_MONOTONIC, &clock_sig_event, &t_id) == 0);

	t_val.it_value.tv_sec = 2; // starting timer
	t_val.it_value.tv_nsec = 0;
	t_val.it_interval.tv_sec = 2; // interval time after start
	t_val.it_interval.tv_nsec = 0;

	assert(timer_settime(t_id, NULL, &t_val, NULL) == 0);

    struct timeval start, end;
    gettimeofday (&start, 0);

	if (f == "") // if file string is empty, process data requests
	{
		handle_data_request(n, p, w, chan, request_buffer);
	}
	else
	{
		handle_file_request(f, m, w, chan, request_buffer);
	}

    gettimeofday (&end, 0);

	timer_delete(t_id);

	if(f == "")
		HIST_COLLECTION.print();

    int secs = (end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)/(int) 1e6;
    int usecs = (int)(end.tv_sec * 1e6 + end.tv_usec - start.tv_sec * 1e6 - start.tv_usec)%((int) 1e6);
    cout << "Took " << secs << " seconds and " << usecs << " microseconds" << endl;

    char* q = (char*) new quitmsg();
    chan->cwrite ( q, sizeof (quitmsg));
	wait(NULL);

    cout << "All Done!!!" << endl;
	delete q;
    delete chan;
}
