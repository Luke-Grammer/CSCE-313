
#include "interpreter.h"
#include <typeinfo>
	
Interpreter::Interpreter() 
{ // Default constructor
	cwd = (string)getenv("HOME"); // Set working directory to linux environment variable $HOME
	chdir(cwd.c_str());
	pwd = "";
}

void Interpreter::parse(string input)
{ // Parses input text and calls execution function
	bool collect_single_string = false, collect_double_string = false;
	vector<string> curr_command;
	vector<vector<string>> commands; 
	string curr_token = "";

	while(input.size() > 0) // While there is still input
	{
		char ch = input.front();

		if(ch == '"' && !collect_single_string)
			collect_double_string = !collect_double_string;

		else if(ch == '\'' && !collect_double_string)
			collect_single_string = !collect_single_string;

		else if(collect_single_string || collect_double_string)
			curr_token += ch;

		else
		{ // If the current character is not in a string literal
			if (ch == '|') // Piping
			{
				if(curr_command.size() > 0)
					commands.push_back(curr_command); // Push back collected command so far

				curr_command.clear();
			}
			else if (ch == ' ') // End of token
			{
				if(curr_token.size() > 0)
					curr_command.push_back(curr_token); // Add to command collected so far

				curr_token = ""; // Reset token
			}
			else if (ch == '<' || ch == '>' || ch == '&')
			{
				if(curr_token.size() > 0)
					curr_command.push_back(curr_token);

				curr_command.push_back(string(1, ch));
				curr_token = "";
			}
			else // No pipe and not end of token
			{
				curr_token += ch;
			}
		}

		input.erase(input.begin());
	}

	if (collect_single_string)
	{ // If there were an odd number of single quotation marks ('')
		printf("ERROR: Expected closing \'.\n");
		return;		
	}

	if (collect_double_string)
	{ // If there were an odd number of double quotation marks ("")
		printf("ERROR: Expected closing \".\n");
		return;			
	}

	if (curr_token.size() > 0) // If a token is unfinished but we reached EOF, add it to command
		curr_command.push_back(curr_token);

	if (curr_command.size() > 0) // If a command is unfinished but we reached EOF, add it to commands
		commands.push_back(curr_command);

	if (commands.size() > 1) // If there's more than one command, handle piping
		pipe_commands(commands);

	else if (commands.size() == 1) // Otherwise, execute individual command
		parse_command(commands.at(0));
}

void Interpreter::pipe_commands(vector<vector<string>> commands)
{ // Handles piped commands and calls parse_command for each 

	int std_in = dup(STDIN_FILENO);
	int std_out = dup(STDOUT_FILENO);

	int fd_in = dup(std_in);
	int fd_out;

	for (int i = 0; i < (int) commands.size(); i++)
	{
		dup2(fd_in, STDIN_FILENO);
		close(fd_in);

		if (i == ((int) commands.size()) - 1)
			fd_out = dup(std_out);
		else
		{
			int fdpipe[2];
			pipe(fdpipe);
			fd_in = fdpipe[0];
			fd_out = fdpipe[1];
		}

		dup2(fd_out, STDOUT_FILENO);
		close(fd_out);

		parse_command(commands.at(i));
	}

	dup2(std_in, STDIN_FILENO);
	dup2(std_out, STDOUT_FILENO);
	close(std_in);
	close(std_out);
}

void Interpreter::parse_command(vector<string> command)
{ // Parses a command for redirection/background and calls execution function
	bool run_in_bg = false;
	int fd_out = 0;
	int fd_in = 0;
	string filename = "";

	
	if (command.at(command.size() - 1) == "&") // If the last argument of the command is '&'
	{
		run_in_bg = true;
		command.erase(command.end());
	}

	for (int i = 0; i < (int) (command.size()) - 1; ++i) // Loop through the command and search for input redirection operator
	{
		if (command.at(i) == "<") 
		{
			filename = command.at(i + 1);
			command.erase(command.begin() + i, command.begin() + i + 2); // Erase redirection operator and filename from command
			
			fd_in = open(filename.c_str(), O_RDONLY); // Open file for input 
			if (fd_in < 0)
			{
				printf("No such file or directory\n");
				return;		
			}
			break;
		}
	}

	for (int i = 0; i < (int) (command.size()) - 1; ++i) // Loop through the command and search for output redirection operator
	{
		if (command.at(i) == ">")
		{
			filename = command.at(i + 1);
			command.erase(command.begin() + i, command.begin() + i + 2);
			
			fd_out = open(filename.c_str(), O_CREAT | O_WRONLY | O_TRUNC, S_IWUSR | S_IRUSR); // Open or create file for output
			if (fd_out < 0)
			{
				printf("Could not create file\n");
				return;		
			}
			break;
		}	
	}

	if (command.at(0) == "cd" || command.at(0) == "jobs") // Handle special commands
	{
		execute_special_command(command);
		return;
	}

	if (command.size() > 0)
	{
		pid_t pid = fork();
		if(pid == 0) // Fork and let child execute command
		{
			if (fd_in)
    	    	dup2(fd_in, STDIN_FILENO); // Redirect input

			if (fd_out)
    	    	dup2(fd_out, STDOUT_FILENO); // Redirect output

			execute(command);
		}
		else
		{
			if (!run_in_bg) // Wait or add to background processes
				wait(NULL);

			else
			{
				bg_processes.push_back(Process(command, pid));
				printf("[%d] %d\n", (int) bg_processes.size(), pid);
			}

			if(fd_in)
				close(fd_in);
			
			if(fd_out)
				close(fd_out);
		}
	}
}

void Interpreter::execute(vector<string> command)
{ // Executes a single well-formed command
	vector<char*> args;

	for(int i = 0; i < (int) command.size(); ++i) // Push back arguments
	{
		args.push_back(const_cast<char*>(command[i].c_str()));
	}

    args.push_back(nullptr);

    execvp(*(char**) &command.at(0), args.data()); // Execute command
	printf("%s: Command not found\n", command.at(0).c_str()); 
	exit(EXIT_FAILURE);
}

void Interpreter::execute_special_command(vector<string> command)
{ // Executes cd/jobs
	if (command.at(0) == "cd") // Handle 'cd'
	{
		if (command.size() > 2) // cd can have at most 1 additional argument
		{
			printf("cd: Too many arguments\n");
			return;
		}
		else if(command.size() == 1) // If command is typed by itself, return to home directory
		{
			pwd = cwd; // Update previous directory
			cwd = (string)getenv("HOME");
			return;
		}

		if (command.at(1).front() == '~') // Substitute '~' at the beginning of the argument with home directory
		{
			command.at(1) = (string)getenv("HOME") + command.at(1).substr(1);
		}
		else if (command.at(1).front() == '.') // If first character is '.' (curr dir or up a level)
		{
			if (command.at(1).size() > 1)
			{
				command.at(1) = command.at(1).substr(1);
				if (command.at(1).front() == '.')
				{
					command.at(1) = command.at(1).substr(1);
					command.at(1) = cwd.substr(0, cwd.rfind('/')) + command.at(1);
				}
				else
				{
					command.at(1) = command.at(1).substr(1);	
				}
			}	
			else
				return;
		}

		if (command.at(1) == "-") // Go to pwd (previous working directory)
		{
			if (pwd != "")
			{
				string temp = pwd;
				pwd = cwd;
				cwd = temp;
				chdir(cwd.c_str());
				printf("%s\n",cwd.c_str());
			}
			else 
				printf("Previous directory not set.\n");
		}

		else // If the command is not '-'
		{
			if(chdir((command.at(1)).c_str()) == 0) // Attempt to change directory
			{
				pwd = cwd;
				if (command.at(1).front() == '/') // Check if absolute or relative and update cwd
					cwd = command.at(1);
				else
					cwd = cwd + "/" + command.at(1);	
			}
			else
				printf("%s: No such file or directory\n", command.at(1).c_str());
		}

		while (cwd.back() == '/') // If command ends with '/+', ignore them
			cwd = cwd.substr(0, cwd.size() - 1);
	}
	else if (command.at(0) == "jobs") // Handle 'jobs'
	{ 
		int index = 1;
		for(auto process : bg_processes) // Loop through all running bg processes and print them
		{
			printf("[%d] Running\t", index++);

			for (auto token : process.name)
				printf("%s ", token.c_str());

			printf("\n");
		}
	}
}

void Interpreter::manage_processes()
{ // Keeps track of background processes to avoid making zombies
	int index = 1;
	for(auto process : bg_processes) // Loop through processes
	{
       	if(waitpid(process.pid, NULL, WNOHANG) > 0) // Check if process is done. if it is, erase it from bg_processes
		{
			printf("[%d] Done \t", index);

			for (auto token : process.name)
				printf("%s ", token.c_str());

			printf("\n");
       	    bg_processes.erase(bg_processes.begin() + index++ - 1);
        }
    }
}

void Interpreter::display_prompt()
{ // Displays prompt for user
	string user = (string)getenv("USER"); // Get linux environment variable $USER
	
	char time_buf[FILENAME_MAX]; 
	time_t curr_time = time(NULL); // Get current time
	struct tm ts;
	ts = *localtime(&curr_time);
	strftime(time_buf, sizeof(time_buf), "%m-%d-%Y %X", &ts);
	
	printf("[%s][%s][%s] > ", ((string) time_buf).c_str(), user.c_str(), cwd.c_str());
}

void Interpreter::shutdown()
{ // Wait for all background processes to finish
	if (bg_processes.size() > 0)
		printf("Waiting for child processes to terminate...\n");

	for(auto i : bg_processes)
		wait(NULL);
}