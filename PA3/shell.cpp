
#include <iostream>
#include "interpreter.h"

using namespace std;

int main()
{
	Interpreter interpreter = Interpreter();
	vector<Process> bg_processes;
	string input = ""; 

    while(input != "exit")
	{
    	interpreter.display_prompt();

        getline(cin, input); // Collect user input

        if(input == "exit")
			break;

		interpreter.manage_processes(); // Manage background processes
		interpreter.parse(input); // Parse & execute input
    }

	interpreter.shutdown(); // Wait for background processes to terminate

    printf("Exiting...\n");
	exit(EXIT_SUCCESS);
}
