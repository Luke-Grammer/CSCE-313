
#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <time.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <fcntl.h>
#include <sys/wait.h>

using namespace std;

const int PIPE_READ = 0;
const int PIPE_WRITE = 1;

struct Process
{ // Process object for managing background processes
	Process(vector<string> name, pid_t pid) 
	: name{name}, pid{pid} {};
	vector<string> name;
	pid_t pid; 
};

class Interpreter
{
	string cwd, pwd; // Current & previous working directories
	vector<Process> bg_processes; // Processes in background

public:
	Interpreter();
	void parse(string in); // Parses input text and calls execution function
	void pipe_commands(vector<vector<string>> commands); // Handles piped commands and calls parse_command for each 
	void parse_command(vector<string> command); // Parses a command for redirection/background and calls execution function
	void execute(vector<string> command); // Executes a single well-formed command
	void execute_special_command(vector<string> command); // Executes cd/jobs
	void manage_processes(); // Keeps track of background processes to avoid making zombies
	void display_prompt(); // Displays prompt for user
	void shutdown(); // Wait for background processes to finish
};

#endif