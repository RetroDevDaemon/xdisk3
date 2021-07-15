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
#include <stdio.h>
#include <string.h>
#include <unistd.h>     // unix
#include <fcntl.h>      // file control
#include <errno.h>      // error number definitions
#include <termios.h>    // posix terminal stuff
#include <sys/ioctl.h>
// ---------------------------------------------------------------------------

class DCB 
{
public: 
	uint32 DCBlength;
	uint32 BaudRate;	//	= baud;
	uint32 fBinary;		//= TRUE;
	uint32 fParity;		//= FALSE;
	uint32 fOutxCtsFlow;	//= TRUE;
	uint32 fOutxDsrFlow;	//= TRUE;
	uint32 fDtrControl;	//	= DTR_CONTROL_ENABLE;
	uint32 fDsrSensitivity;	//= FALSE;
	uint32 fTXContinueOnXoff; //	= FALSE;
	uint32 fOutX;		//= TRUE;
	uint32 fInX;		//= FALSE;
	uint32 fErrorChar;
	uint32 fNull;		//= FALSE;
	uint32 fRtsControl;	//	= RTS_CONTROL_HANDSHAKE;
	uint32 fAbortOnError;	//= FALSE;
	uint32 fDummy2;
	uint16 wReserved;
	uint16 XonLim;
	uint8 ByteSize;	//	= 8;
	uint8 Parity;		//= NOPARITY;
	uint8 StopBits;	//	= ONESTOPBIT;
	char XonChar;
	char XoffChar;
	char ErrorChar;
	char EofChar;
	char EvtChar;		//= 10;
	uint16 wReserved1;
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
	char fpa[13] = "/dev/ttyUSB "; // 12 +1null
	

public:
	SIO();
	~SIO();
	Result Open(int port, int baud);
	Result Close();
	Result Read(void*, int);
	Result Write(const void*, int);
	bool SetMode(int baud);
	void SetTimeouts(int to);
	void DCBToNix();
private:
	//HANDLE hfile;
	//FILE* hfile; // file handler
	int serialport;
	//PORTINFO dcb;
	DCB dcb;
	int timeouts;
};

#endif
