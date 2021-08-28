// ---------------------------------------------------------------------------
//	Serial I/O Unit
//	Copyright (C) 1998, 2000 cisc.
// ---------------------------------------------------------------------------
//	$Id: sio.cpp,v 1.3 2000/01/06 01:35:55 cisc Exp $

#define INVALID_HANDLE_VALUE 0
#define DWORD uint32 

#include "headers.h"
#include "sio.h"
#include <iostream>
#include <sstream>

extern bool verbose;

// ---------------------------------------------------------------------------
//	Construct/Destruct
//
SIO::SIO()
//: hfile(INVALID_HANDLE_VALUE)
{
	hSerialPort = INVALID_HANDLE_VALUE;
}

DCB::DCB()
{}

SIO::~SIO()
{
	if (hSerialPort != INVALID_HANDLE_VALUE)
		Close();
}

DCB::~DCB()
{}

// ---------------------------------------------------------------------------
//	�f�[�^�𑗂�
//	src		�f�[�^�̃|�C���^
//	len		�f�[�^�̒���
//
SIO::Result SIO::Write(const void* src, int len)
{
	DWORD writtensize;
	//printf("trying write...\n");
	writtensize = write(hSerialPort, src, len);

	if(verbose) {
	//	std::cout << "Wrote " << writtensize << " byte(s) of " << len << std::endl;
	}

	if(int(writtensize) == len) {
		//printf("write ok\n");	
		return OK;
	}

	//if (WriteFile(hfile, src, len, &writtensize, 0))
	//{
	//	if (int(writtensize) == len)
	//		return OK;
	//}
	if (verbose)
		printf("sio::write error\n");
	return ERR;
}

// ---------------------------------------------------------------------------
//	�f�[�^���󂯎��
//	dest	�f�[�^�̎󂯎���
//	len		�ǂݍ��ރf�[�^�̒���
//
SIO::Result SIO::Read(void* dest, int len)
{
	//DWORD readsize;
	size_t readsize;
	for(int f = 0; f < 3; f++)
	{
		readsize = read(hSerialPort, dest, len);// fread(dest, 1, len, hfile);
		if(readsize == len) f = 4;
	}
	//if (ReadFile(hfile, dest, len, &readsize, 0))
	//{
	//	if (int(readsize) == len)
	//		return OK;
	//}
	if(int(readsize) == len) 
	{
		return OK;
	}
	if (verbose)
		printf("sio::read error\n");
	return ERR;
}

// ---------------------------------------------------------------------------
//	�ʐM�f�o�C�X���J��
//
SIO::Result SIO::Open(const std::string& serialDevice, int baud)
{
	//if (hfile != INVALID_HANDLE_VALUE)
	//	Close();
	//if (port < 1)
	//	return ERRINVALID;
	//char buf[16];
	//printf(buf, "COM%d", port);

	timeouts = 2000;

	hSerialPort = open(serialDevice.c_str(), O_RDWR | O_NDELAY);
	ASSERT_NO_ERROR(hSerialPort);
	/*
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
	*/
	//memset(&dcb, 0, sizeof(DCB));
	//dcb.DCBlength = sizeof(DCB);
	//GetCommState(hfile, &dcb); < tcgetattr
	// Get the current serial port status, try to set baud, and reset attr
	int result = tcgetattr(hSerialPort, &dcb.config);
	ASSERT_NO_ERROR(result);
	if (!SetMode(baud))
	{
		Close();
		return ERRCONFIG;
	}
	result = tcflush(hSerialPort, TCIFLUSH) >= 0;
	ASSERT_NO_ERROR(result);
	//PurgeComm(hfile, 
	//	PURGE_TXABORT| // terminate write 
	//	PURGE_TXCLEAR| // clear output buffer
	//	PURGE_RXABORT| // terminate read
	//	PURGE_RXCLEAR); // clear input buffer
	return OK;
}

// ---------------------------------------------------------------------------
//	�ʐM�f�o�C�X�����
//
SIO::Result SIO::Close()
{
	if (hSerialPort != INVALID_HANDLE_VALUE)
	{
		//PurgeComm(hfile, 
		//	PURGE_TXABORT|PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
		//CloseHandle(hfile);
		close(hSerialPort);
		hSerialPort = INVALID_HANDLE_VALUE;
	}
	return OK;
}

void SIO::DCBToNix()
{
	// dcb is the local, unapplied settings
	// dcb.config is the *NIX format settings
	dcb.config.c_iflag &= ~(IGNBRK | BRKINT | ICRNL |\
                    INLCR | PARMRK | INPCK | ISTRIP | IXON);
	dcb.config.c_oflag = 0;
	dcb.config.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN | ISIG);
	switch(dcb.BaudRate)
	{
		case(9600):
			cfsetispeed(&dcb.config, (speed_t)B9600);
			cfsetospeed(&dcb.config, (speed_t)B9600);
			break;
		case(19200):
			cfsetispeed(&dcb.config, (speed_t)B19200);
			cfsetospeed(&dcb.config, (speed_t)B19200);
			break;
		default:
			break;
	}
	//fBinary is not used.
	dcb.config.c_iflag &= ~(INPCK|IXOFF); // disable input parity checking
	// Linux only supports CTS.
	// Linux does not support DSR.
	// -
	// -
	// xoff/lim windows only.
	dcb.config.c_iflag |= IXON; // foutx, finx
	// rts not on linux...
	// abort on error ??
	dcb.config.c_cflag &= ~(CSIZE | PARENB);
	dcb.config.c_cflag |= CS8; // set byte size to 8, no parity
	dcb.config.c_cflag &= ~(CSTOPB); // 1 stop bit
	dcb.config.c_cc[VINTR] = 10;
	dcb.config.c_lflag &= ~(ICANON);
	dcb.config.c_cc[VMIN] = 1;
	dcb.config.c_cc[VTIME] = 2; 
}

bool SIO::SetMode(int baud)
{
	//PurgeComm(hfile, PURGE_TXCLEAR|PURGE_RXABORT|PURGE_RXCLEAR);
	assert(tcflush(hSerialPort, TCIFLUSH) >= 0);
	/*
	dcb.BaudRate		= baud;
	dcb.fBinary		= TRUE;
	dcb.fParity		= FALSE;
	dcb.fOutxCtsFlow	= TRUE;
	dcb.fOutxDsrFlow	= TRUE;
	dcb.fDtrControl		= DTR_CONTROL_ENABLE;
	dcb.fDsrSensitivity	= FALSE;
	dcb.fTXContinueOnXoff 	= FALSE;
	dcb.fOutX		= TRUE;		//IXON
	dcb.fInX		= FALSE;  	//IXOFF
	dcb.fNull		= FALSE;
	dcb.fRtsControl		= RTS_CONTROL_HANDSHAKE;
	dcb.fAbortOnError	= FALSE;
	dcb.ByteSize		= 8;
	dcb.Parity		= NOPARITY;
	dcb.StopBits		= ONESTOPBIT;
	dcb.EvtChar		= 10;

	*/
	//bool r = SetCommState(hfile, &dcb) != 0;
	//SetTimeouts(timeouts);
	dcb.BaudRate = baud;
	DCBToNix();
	int r = tcsetattr(hSerialPort, TCSANOW, &dcb.config);
	assert(fcntl(hSerialPort, F_SETFL, 0) >= 0);
	assert(r >= 0);
	// 0 is a pass..

	if(r == 0) r = 1;
	else r = 0; // flip r
	return r; // fail = 0, pass = nonzero
}

void SIO::SetTimeouts(int rto)
{
	assert(tcflush(hSerialPort, TCIFLUSH) >= 0);
	if(dcb.BaudRate == 9600)
		dcb.config.c_cc[VTIME] = 3;
	else 
		dcb.config.c_cc[VTIME] = 2;
	assert(tcsetattr(hSerialPort, TCSANOW, &dcb.config) >= 0);

	/*
	COMMTIMEOUTS cto;
	cto.ReadIntervalTimeout = 0; // disable timeout
	cto.ReadTotalTimeoutConstant = timeouts = rto;
	cto.ReadTotalTimeoutMultiplier = 20000 / dcb.BaudRate + 1; 3??
	cto.WriteTotalTimeoutConstant = 2000;
	cto.WriteTotalTimeoutMultiplier = 20000 / dcb.BaudRate + 1;
	SetCommTimeouts(hfile, &cto);
	*/
}
