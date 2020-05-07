# makefile

all: dataserver client

common.o: common.h common.cpp
	g++ -g -w -std=c++11 -c common.cpp

Histogram.o: Histogram.h Histogram.cpp
	g++ -g -w -std=c++11 -c Histogram.cpp

FIFORequestChannel.o: RequestChannel.h FIFORequestChannel.h FIFORequestChannel.cpp
	g++ -g -w -std=c++11 -c FIFORequestChannel.cpp

MQRequestChannel.o: RequestChannel.h MQRequestChannel.h MQRequestChannel.cpp
	g++ -g -w -std=c++11 -c MQRequestChannel.cpp

KernelSemaphore.o: KernelSemaphore.h KernelSemaphore.cpp
	g++ -g -w -std=c++11 -c KernelSemaphore.cpp

SHMRequestChannel.o: RequestChannel.h SHMBoundedBuffer.h SHMRequestChannel.h SHMRequestChannel.cpp
	g++ -g -w -std=c++11 -c SHMRequestChannel.cpp

client: client.cpp Histogram.o FIFORequestChannel.o MQRequestChannel.o SHMRequestChannel.o KernelSemaphore.o common.o
	g++ -g -w -std=c++11 -o client client.cpp Histogram.o FIFORequestChannel.o MQRequestChannel.o SHMRequestChannel.o KernelSemaphore.o common.o -lpthread -lrt

dataserver: dataserver.cpp FIFORequestChannel.o MQRequestChannel.o SHMRequestChannel.o common.o KernelSemaphore.o
	g++ -g -w -std=c++11 -o dataserver dataserver.cpp FIFORequestChannel.o MQRequestChannel.o SHMRequestChannel.o KernelSemaphore.o common.o -lpthread -lrt

clean:
	rm -rf *.o
