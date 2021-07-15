// ---------------------------------------------------------------------------
//	Serial I/O Unit
//	Copyright (C) cisc 1998.
// ---------------------------------------------------------------------------
//	$Id: sio.h,v 1.3 2000/01/06 01:35:55 cisc Exp $

#ifndef SIO_H
#define SIO_H

#include "types.h"

// ---------------------------------------------------------------------------

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
	Result Open(int port, int baud);
	Result Close();
	Result Read(void*, int);
	Result Write(const void*, int);
	bool SetMode(int baud);
	void SetTimeouts(int to);

private:
	HANDLE hfile;
	DCB dcb;
	int timeouts;
};

#endif
