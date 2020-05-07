# makefile

all: shell

interpreter.o: interpreter.h interpreter.cpp
	g++ -g -W -Wall -Wextra -pedantic -std=c++11 -c interpreter.cpp

shell: shell.cpp interpreter.o
	g++ -g -W -Wall -Wextra -pedantic -std=c++11 -o shell shell.cpp interpreter.o

clean:
	rm -rf *.o
