// ---------------------------------------------------------------------------
//	Serial I/O Unit
//	Copyright (C) cisc 1998.
// ---------------------------------------------------------------------------
//	$Id: sio.h,v 1.3 2000/01/06 01:35:55 cisc Exp $

#ifndef SIO_H
#define SIO_H

#define TRUE 1
#define FALSE 0

#include "types.h"
#include "headers.h"
#include <string>
// ---------------------------------------------------------------------------

class DCB 
{
public: 
	uint32 BaudRate;	//	= baud;
	struct termios config;
	
public:
	DCB();
	~DCB();
};

class SIO
{
public:
	enum Result
	{
		OK = 0,
		ERR,
		ERRINVALID,
		ERROPEN,
		ERRCONFIG,
	};
public:
	SIO();
	~SIO();
	Result Open(const std::string& serialDevice, int baud);
	Result Close();
	Result Read(void*, int);
	Result Write(const void*, int);
	bool SetMode(int baud);
	void SetTimeouts(int to);
	void DCBToNix();
private:
	//HANDLE hfile;
	int hSerialPort;
	DCB dcb;
	int timeouts;
};

#endif
