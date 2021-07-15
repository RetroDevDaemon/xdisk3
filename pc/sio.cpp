// ---------------------------------------------------------------------------
//	Serial I/O Unit
//	Copyright (C) 1998, 2000 cisc.
// ---------------------------------------------------------------------------
//	$Id: sio.cpp,v 1.3 2000/01/06 01:35:55 cisc Exp $

#include "headers.h"
#include "sio.h"

extern bool verbose;

// ---------------------------------------------------------------------------
//	Construct/Destruct
//
SIO::SIO()
: hfile(INVALID_HANDLE_VALUE)
{
}

SIO::~SIO()
{
	if (hfile)
		Close();
}

// ---------------------------------------------------------------------------
//	データを送る
//	src		データのポインタ
//	len		データの長さ
//
SIO::Result SIO::Write(const void* src, int len)
{
	DWORD writtensize;
	if (WriteFile(hfile, src, len, &writtensize, 0))
	{
		if (int(writtensize) == len)
			return OK;
	}
	if (verbose)
		printf("sio::write error\n");
	return ERR;
}

// ---------------------------------------------------------------------------
//	データを受け取る
//	dest	データの受け取り先
//	len		読み込むデータの長さ
//
SIO::Result SIO::Read(void* dest, int len)
{
	DWORD readsize;
	if (ReadFile(hfile, dest, len, &readsize, 0))
	{
		if (int(readsize) == len)
			return OK;
	}
	if (verbose)
		printf("sio::read error\n");
	return ERR;
}

// ---------------------------------------------------------------------------
//	通信デバイスを開く
//
SIO::Result SIO::Open(int port, int baud)
{
	if (hfile != INVALID_HANDLE_VALUE)
		Close();

	if (port < 1)
		return ERRINVALID;
	char buf[16];
	wsprintf(buf, "COM%d", port);

	timeouts = 2000;
	hfile = CreateFile(	buf,
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						0,
						NULL );

	if (hfile == INVALID_HANDLE_VALUE)
	{
		return ERROPEN;
	}
	
	memset(&dcb, 0, sizeof(DCB));
	dcb.DCBlength		= sizeof(DCB);
	GetCommState(hfile, &dcb);
	
	if (!SetMode(baud))
	{
		Close();
		return ERRCONFIG;
	}
	PurgeComm(hfile, 
		PURGE_TXABORT|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
	return OK;
}

// ---------------------------------------------------------------------------
//	通信デバイスを閉じる
//
SIO::Result SIO::Close()
{
	if (hfile != INVALID_HANDLE_VALUE)
	{
		PurgeComm(hfile, 
			PURGE_TXABORT|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
		CloseHandle(hfile);
		hfile = INVALID_HANDLE_VALUE;
	}
	return OK;
}

bool SIO::SetMode(int baud)
{
	PurgeComm(hfile, PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
	
	dcb.BaudRate		= baud;
	dcb.fBinary			= TRUE;
	dcb.fParity			= FALSE;
	dcb.fOutxCtsFlow	= TRUE;
	dcb.fOutxDsrFlow	= TRUE;
	dcb.fDtrControl		= DTR_CONTROL_ENABLE;
	dcb.fDsrSensitivity	= FALSE;
	dcb.fTXContinueOnXoff = FALSE;
	dcb.fOutX			= TRUE;
	dcb.fInX			= FALSE;
	dcb.fNull			= FALSE;
	dcb.fRtsControl		= RTS_CONTROL_HANDSHAKE;
	dcb.fAbortOnError	= FALSE;
	dcb.ByteSize		= 8;
	dcb.Parity			= NOPARITY;
	dcb.StopBits		= ONESTOPBIT;
	dcb.EvtChar			= 10;

	bool r = SetCommState(hfile, &dcb) != 0;

	SetTimeouts(timeouts);

	return r;
}

void SIO::SetTimeouts(int rto)
{
	COMMTIMEOUTS cto;
	cto.ReadIntervalTimeout = 0;
	cto.ReadTotalTimeoutConstant = timeouts = rto;
	cto.ReadTotalTimeoutMultiplier = 20000 / dcb.BaudRate + 1;
	cto.WriteTotalTimeoutConstant = 2000;
	cto.WriteTotalTimeoutMultiplier = 20000 / dcb.BaudRate + 1;
	SetCommTimeouts(hfile, &cto);
}
