/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/19
 */
#include <fstream>
#include <sys/wait.h>
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;

// Input/Output filenames
const char* const IN_FILE_2 = "1.csv"; 
const char* const IN_FILE_3 = "500MB";
const char* const OUT_FILE_1 = "received/x1.csv";
const char* const OUT_FILE_2 = "received/y1.csv";
const char* const OUT_FILE_3 = "received/500MB";

void compare_files(const char* const server_file, const char* const received_file) 
{ // Compares two files using the 'diff' linux command
	char* cmd = new char[MAX_MESSAGE];
	cmd = strcpy(cmd, "diff -s -q BIMDC/");
	cmd = strcat(cmd, server_file);
	cmd = strcat(cmd, " ");
	cmd = strcat(cmd, received_file);
	system(cmd);
	delete[] cmd;
}

int main(int argc, char *argv[])
{
	int child_status;

	pid_t pid = fork(); // Creates child process to become dataserver

	if (pid < 0) 
	{ // If there is an error forking the process
		printf("ERROR: Could not create child process!\n");
		exit(EXIT_FAILURE); 
	}
	else if (!pid)
	{ // If the forked process is the child, start the dataserver
		execvp("./dataserver", argv);
	}
	else
	{ // Otherwise, run the client
		char* msg = new char[MAX_MESSAGE]; // For sending messages to the dataserver
		void* result; // For receiving information from the dataserver
		__int64_t* file_size; // For holding the file sizes
		int buf_size; // For holding the max buffer size for each file request
		ofstream out; // File output stream
		double elapsed_time; // For timer implementation
		timeval t1, t2; 

		// Connect to dataserver
	    FIFORequestChannel* chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
		printf("Client connected to dataserver.\n");


		printf("\nTest 1 - Requesting ECG data for person 1\n\n");

		// Open output file
		out.open(OUT_FILE_1);
		if(!out)
		{
			printf("Could not open output file %s", OUT_FILE_1);
			exit(EXIT_FAILURE);
		}

		gettimeofday(&t1, NULL); // Start timer
		for(int i = 0; i < 150; i++)
		{ // Request data points for person 1 (ECG1 and ECG2)
			out << i * (60.0 / 15000.0) << ",";
			for (int j = 1; j <= 2; j++)
			{
				*(datamsg*) msg = datamsg(1, i * (60.0 / 15000.0), j); // Format request

				chan->cwrite(msg, sizeof(datamsg)); // Send request
				result = chan->cread(); // Receive response
				out << *(double*) result << ((j == 1) ? "," : "\n"); // Output results to file
				delete result;
			}
		}
		gettimeofday(&t2, NULL); // End timer
		out.close(); // Close output file

		// Print information
		printf("Data received, information saved in %s\n", OUT_FILE_1);
		elapsed_time = ((t2.tv_usec - t1.tv_usec) / 1000000.0 + (t2.tv_sec - t1.tv_sec));
		printf("Elapsed time: %f seconds\n", elapsed_time);
		printf("Comparing files. . .\n");
  		compare_files("1.csv", OUT_FILE_1 );
		
		
		printf("\nTest 2 - Requesting text file from dataserver\n\n");

		// Open output file
		out.open(OUT_FILE_2, ios::binary | ios::out);
		if(!out)
		{
			printf("Could not open output file %s", OUT_FILE_2);
			exit(EXIT_FAILURE);
		}

		gettimeofday(&t1, NULL); // Start timer

		*(filemsg*) msg = filemsg(0, 0); // Format file size request
		for(int i = 0; i < sizeof(IN_FILE_2); i++)
		{ // Append filename to request
			*(msg + sizeof(filemsg) + i) = IN_FILE_2[i];
		}
		chan->cwrite(msg, MAX_MESSAGE); // Send request to dataserver
		file_size = (__int64_t*) chan->cread(); // Read response
		
		for(__int64_t offset = 0; offset < *file_size; offset += MAX_MESSAGE)
		{ // Request file in increments of MAX_MESSAGE bytes
			buf_size = (offset + MAX_MESSAGE < *file_size) ? MAX_MESSAGE : *file_size - offset; // Determine how much can be requested
			*(filemsg*) msg = filemsg(offset, buf_size); // Format request
			chan->cwrite(msg, MAX_MESSAGE); // Send request
			result = chan->cread(); // Read response
			out.write((char*) result, buf_size); // Write data to output file
			delete[] result;
		}

		delete file_size;

		gettimeofday(&t2, NULL); // End timer
		out.close(); // Close file

		// Print information
		printf("Data received, file copied to %s\n", OUT_FILE_2);
		elapsed_time = ((t2.tv_usec - t1.tv_usec) / 1000000.0 + (t2.tv_sec - t1.tv_sec));
		printf("Elapsed time: %f seconds\n", elapsed_time);
		printf("Comparing files. . .\n");
  		compare_files(IN_FILE_2, OUT_FILE_2);

		
		printf("\nTest 3 - Requesting binary file from dataserver\n\n");

		// Open output file
		out.open(OUT_FILE_3, ios::binary | ios::out);
		if(!out)
		{
			printf("Could not open output file %s", OUT_FILE_3);
			exit(EXIT_FAILURE);
		}

		gettimeofday(&t1, NULL); // Start timer

		*(filemsg*) msg = filemsg(0, 0); // Format file size request
		for(int i = 0; i < sizeof(IN_FILE_3); i++)
		{ // Append filename to request
			*(msg + sizeof(filemsg) + i) = IN_FILE_3[i];
		}
		chan->cwrite(msg, MAX_MESSAGE); // Send request to dataserver
		file_size = (__int64_t*) chan->cread(); // Read response

		for(__int64_t offset = 0; offset < *file_size; offset += MAX_MESSAGE)
		{ // Request file in increments of MAX_MESSAGE bytes
			buf_size = (offset + MAX_MESSAGE < *file_size) ? MAX_MESSAGE : *file_size - offset; // Determine how much can be requested
			*(filemsg*) msg = filemsg(offset, buf_size); // Format request
			chan->cwrite(msg, MAX_MESSAGE); // Send request
			result = chan->cread(); // Read response
			out.write((char*) result, buf_size); // Write data to output file
			delete[] result;
		}

		delete file_size;

		gettimeofday(&t2, NULL); // End timer
		out.close(); // Close file

		// Print information
		printf("Data received, file copied to %s\n", OUT_FILE_3);
		elapsed_time = ((t2.tv_usec - t1.tv_usec) / 1000000.0 + (t2.tv_sec - t1.tv_sec));
		printf("Elapsed time: %f seconds\n", elapsed_time);
		printf("Comparing files. . .\n");
  		compare_files(IN_FILE_3, OUT_FILE_3);


		printf("\nTest 4 - Requesting additional channels\n\n");

		*(newchannelmsg*) msg = newchannelmsg(); // Format new channel request
		chan->cwrite(msg, sizeof(newchannelmsg)); // Send request to server
		result = chan->cread(); // Read response
	
		FIFORequestChannel* new_chan = new FIFORequestChannel((char*) result, FIFORequestChannel::CLIENT_SIDE); // Connect to new channel
		delete[] result;

		printf("Client connected to new channel\n"); 
		printf("Requesting sample ECG1 data for person 1 . . .\n");

		for(int i = 0; i < 10; i++)
		{ // Request sample datapoints (10 ECG1 datapoints for person 1)
			usleep(250000); // Sleep for a quarter second so data can be read in the terminal

			*(datamsg*) msg = datamsg(1, i * (60.0 / 15000.0), 1); // Format request

			new_chan->cwrite(msg, sizeof(datamsg)); // Write request to channel
			result = new_chan->cread(); // Read response
			cout << i * (60.0 / 15000.0) << "," << *(double*) result << "\n";  // Output results
			delete result;
		}

		*(quitmsg*) msg = quitmsg(); 
		new_chan->cwrite(msg, sizeof(quitmsg)); // Close new channel

		delete new_chan;
		printf("Client disconnected from new channel.\n");

		printf("\nClient shutting down server.\n");
		chan->cwrite(msg, sizeof(quitmsg)); // Close original channel
		wait(&child_status); // Wait for child process to terminate

		delete[] msg;
		delete chan;
	}
	
	exit(EXIT_SUCCESS);
}
