#ifndef REQUEST_CHANNEL_H
#define REQUEST_CHANNEL_H

#include "common.h"

class RequestChannel {
public:
	typedef enum {SERVER_SIDE , CLIENT_SIDE} Side;
	typedef enum {READ_MODE, WRITE_MODE} Mode;

protected: // ADDED
	string my_name;
	Side my_side;

public:
	/* CONSTRUCTOR/DESTRUCTOR */
	RequestChannel (const string name, const Side side) : my_name{name}, my_side{side} {};
	RequestChannel(); // ADDED
	
	virtual ~RequestChannel() {};

	virtual char* cread(int* len = NULL) = 0;
	/* Blocking read of data from the channel . Returns a string of
	characters read from the channel . Returns NULL if read failed. */

	virtual int cwrite ( char* msg, int msglen ) = 0;
	/* Write the data to the channel. The function returns
	the number of characters written to the channel. */
};

#endif